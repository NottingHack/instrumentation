/*   
 * Copyright (c) 2011, Daniel Swann <hs@dswann.co.uk>
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
    struct service_info 
    {
      string sname;
      string config_file;
      string mqtt_topic;
      bool reply_received;
      service_info *next;
    };
    CNHDBAccess *db;    
    service_info *service_list;
    int timeout_period;
    int query_interval;
    bool query_thread_exit;
    pthread_t qThread;
    string config_path;
    
    
    nh_monitor(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      service_list = NULL;
      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   ;
      timeout_period = get_int_option("monitor", "timeout", 5);
      query_interval = get_int_option("monitor", "query_interval", 30);
      config_path = get_str_option("monitor", "config_path", "./conf/");
      qThread = -1;
    }
    
    ~nh_monitor()
    {
      delete db;
      service_info *si1;
      service_info *si2;
      
      if (service_list != NULL)
      {
        si1 = si2 = service_list;
    
        do
        {
          si1 = si1->next;
          delete si2;
          si2 = si1;
        } while (si1 != NULL);    
      }      
    }
  
  
  int add_service(string sname, string config_file, string mqtt_topic)
  {
    service_info *si;
    service_info *next;
    
    si = new service_info;
    si->sname = sname;
    si->config_file = config_file;
    si->mqtt_topic = mqtt_topic;
    si->reply_received = false;
    si->next = NULL;
    
    if (service_list==NULL)
    {
      // first in list
      service_list = si;
    } else
    {
      // add to end of list
      next = service_list;
      while (next->next != NULL)
        next = next->next;
      
      next->next = si;
    }
    
    return 0;
  }
    
  int read_config_file(string path, string filename)
  {
    INIReader *inireader;
    string mqtt_topic;
    string filepath;
    
    filepath = path+filename;    
    mqtt_topic = "";
  
    inireader = new INIReader(path + filename);
    
    if (inireader->ParseError() < 0) 
    {
        log->dbg("Failed to open/parse config file [" + filepath + "]");
    } else
    {
      mqtt_topic  = inireader->Get("mqtt", "topic", ""); 
    }    
    
    if (mqtt_topic != "")
      add_service(filename.substr(0,filename.length()-5), filepath, mqtt_topic);
    
    log->dbg("[" + filepath + "] \t\tmqtt_topic = " + mqtt_topic);  
    
    delete inireader;
    return 0;
  }  
  
  int read_config_files(string path)
  {
    DIR *dp;
    struct dirent *dirp;
    string fname;
    
    if((dp = opendir(path.c_str())) == NULL) 
    {
      log->dbg("Error(" + itos(errno) + ") opening " + path);
      return errno;
    }
    
    while ((dirp = readdir(dp)) != NULL) 
    {
      fname = dirp->d_name;
      if (fname.length() <= 4) continue;
      
      if (fname.substr(fname.length()-4, string::npos)=="conf")
        read_config_file(path, fname);
    }

    closedir(dp);
    return 0;
  }

  // Subscribe to the mqtt topics that the various services will reply on    
  int subscribe_service()
  {
    service_info *si;
    
    if (service_list == NULL)
      return 0;
    
    si = service_list;
    
    do
    {
      subscribe(si->mqtt_topic + "/tx");
      si = si->next;
    } while (si != NULL);    
    
    return 0;
  }
  
  
  string get_service_from_topic(string topic)
  {
    service_info *si;
    
    if (service_list == NULL)
      return "";
    
    si = service_list;
    
    do
    {
      if ((si->mqtt_topic + "/tx") == topic)
        return si->sname;
      
      si = si->next;
    } while (si != NULL);    
    
    return "";
  }

  void process_message(string topic, string message)
  {
    string sname;
    
    sname = get_service_from_topic(topic);
    
    // Record the reply
    db->sp_record_service_status(sname, RUNNING_TRUE, message);
    
    CNHmqtt::process_message(topic, message);    
  }
    
  int query_all()
  {
    db->sp_service_status_update(timeout_period); 
    service_info *si;
    
    if (service_list == NULL)
      return 0;
    
    si = service_list;
    
    do
    {
      db->sp_record_service_status_request(si->sname);
      message_send(si->mqtt_topic + "/rx", "STATUS");
      si = si->next;
    } while (si != NULL);    
    
    return 0;
  }
  
  int init()
  {
    read_config_files(config_path);
   
    if (db->dbConnect())
      return -1; // With access to MySQL, results can't be saved, so exit
    else
    { // MySQL can obviously never be recorded as not running...
      db->sp_record_service_status_request("MySQL");  
      db->sp_record_service_status("MySQL", RUNNING_TRUE, "Connected");
    }
    
    db->sp_record_service_status_request("Mosquitto");  
    if (mosq_connect())
    {
      db->sp_record_service_status("Mosquitto", RUNNING_FALSE, "Connection error");
      return -1;
    } else
    {
      db->sp_record_service_status("Mosquitto", RUNNING_TRUE, "Connected");
    }
    
    db->sp_service_status_update(timeout_period);
    return 0; 
  }
  
  static void *query_thread(void *arg)
  {
    nh_monitor *nh;
    nh = (nh_monitor*) arg;
    
    while (!nh->query_thread_exit)
    {
      nh->query_all();
      sleep(nh->query_interval);
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
    query_all();
    
    signal(SIGUSR1,sig_func); 
    
    // Start thread that will regularly query services
    query_thread_exit = false;
    pthread_create(&qThread, NULL, &nh_monitor::query_thread, this);
    
    // Enter mosquitto message loop
    retval = message_loop();
    query_thread_exit = true;
   
    // Interupt & join query thread    
    if (qThread > 0)
    {
      pthread_kill(qThread, SIGUSR1);
      pthread_join(qThread, NULL);
      qThread = -1;      
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