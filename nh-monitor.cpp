/*   
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "CNHmqtt_irc.h"
#include "db/lib/CNHDBAccess.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>  

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

#define RUNNING_FALSE 0
#define RUNNING_TRUE 1


class nh_monitor : public CNHmqtt
{
  
  public:
    CNHDBAccess *_db;
    int _timeout_period;
    int _query_interval;
    bool _query_thread_exit;
    pthread_t _qThread;

    nh_monitor(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      _db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);
      _timeout_period = get_int_option("monitor", "timeout", 5);
      _query_interval = get_int_option("monitor", "query_interval", 30);
      _qThread = -1;
    }
    
    ~nh_monitor()
    {
      delete _db;
    }

  // Subscribe to the status MQTT topc 
  int subscribe_service()
  {
    subscribe(_status_res_topic);
    return 0;
  }
  

  void process_message(string topic, string message)
  /* Status message is expected to be in the form "<state>: <service>", e.g: *
   *    Running: Gatekeeper                                                  *
   *    Terminated: WorkshopMQTT                                             */
  {
    string sname, state;
    unsigned int pos;
    
    if ((topic != _status_res_topic) || (message == "STATUS"))
      return;
    
    if ((pos = message.find_first_of(":"))==string::npos)
      return;
    
    if (message.length() < pos + 3)
      return;
    
    state = message.substr(0, pos);               // E.g. "Running" or "Terminated"
    sname = message.substr(pos+2, string::npos);  // E.g. "Gatekeeper"
    
    if (state != "Running" && state != "Terminated" && state != "Restart")
      return;
    
    if (state == "Restart")
    {
      _db->sp_record_service_restart(sname);
      _db->sp_log_event("PROCESS_RESTART", sname);
    }
    
    // Record the reply
    _db->sp_record_service_status(sname, (state=="Running" ? RUNNING_TRUE : RUNNING_FALSE), state);
    
    CNHmqtt::process_message(topic, message);
  }
    
  int query_all()
  {
    _db->sp_service_status_update(_timeout_period);
    _db->sp_record_service_status_request("N/A");

    switch(message_send(_status_req_topic, "STATUS"))
    {
      case MOSQ_ERR_SUCCESS:
        _db->sp_record_service_status("Mosquitto", RUNNING_TRUE, "Connected");
        break;
        
      case MOSQ_ERR_NO_CONN:
        _db->sp_record_service_status("Mosquitto", RUNNING_FALSE, "Not connected!");
        break;
        
      default:
        _db->sp_record_service_status("Mosquitto", RUNNING_FALSE, "ERROR");
    }
    return 0;
  }
  
  int init()
  {
    if (_db->dbConnect())
      return -1; // With access to MySQL, results can't be saved, so exit

    _db->sp_record_service_status_request("Mosquitto");
    if (mosq_connect())
    {
      _db->sp_record_service_status("Mosquitto", RUNNING_FALSE, "Connection error");
      return -1;
    } else
    {
      _db->sp_record_service_status("Mosquitto", RUNNING_TRUE, "Connected");
    }

    _db->sp_service_status_update(_timeout_period);
    return 0; 
  }

  static void *query_thread(void *arg)
  {
    nh_monitor *nh;
    nh = (nh_monitor*) arg;
    
    while (!nh->_query_thread_exit)
    {
      nh->query_all();
      sleep(nh->_query_interval);
    }
    
    nh->dbglog("Exiting query thread");
    return 0;
  }
  
  static void sig_func(int sig)
  {

  }
  
  
  int go()
  {
    int retval;
    
    if (init())
      return -1;
    
    subscribe_service();
    
    signal(SIGUSR1,sig_func); 
    
    // Start thread that will regularly query services
    _query_thread_exit = false;
    pthread_create(&_qThread, NULL, &nh_monitor::query_thread, this);
    
    // Enter mosquitto message loop
    retval = message_loop();
    _query_thread_exit = true;
   
    // Interupt & join query thread    
    if (_qThread > 0)
    {
      pthread_kill(_qThread, SIGUSR1);
      pthread_join(_qThread, NULL);
      _qThread = -1;      
    }
    
    return retval;
  }
  
  void dbglog(string msg)
  {
    log->dbg(msg);
  }

};

int main(int argc, char *argv[])
{
  bool terminate;
  nh_monitor *nh;
  
  int retry_time = 10;
  
  terminate = false;
  while (!terminate)
  {
    nh = new nh_monitor(argc, argv);
    nh_monitor::daemonize();
    if (nh->go() == EXIT_TERMINATE)  
    {
      terminate = true;
    } else
    {
      nh->dbglog("Connection problems - retrying in " + CNHmqtt::itos(retry_time) + " seconds.");
      sleep(retry_time);
    }
    delete nh;     
  }
  
}