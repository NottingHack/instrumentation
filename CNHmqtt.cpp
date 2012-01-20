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
 * 3. Neither the name of nh-irc nor the names of its
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

#include "CNHmqtt.h"

#include <iostream>
#include <cstdlib>
#include <unistd.h>

CNHmqtt::CNHmqtt(int argc, char *argv[]) 
{
  log = NULL;
  string config_file = "";
  config_file_parsed = false;
  int c;
  std::stringstream out;
  reader = NULL;
  mosq_connected = false;
  log = new CLogging();
  string logfile;
  uid = 0;
  no_staus_debug = false;
  
  mosquitto_lib_init();
  mosq = NULL;
    
  debug_mode = false;
    
  /* Read in command line parameters:
   * -c <config file>       specify config file
   * -d                     Do not daemonize & ouput debug info to stdout  s
   * -l <log file>          Specify location to write logfile to TODO
   */
  
  opterr = 0;
  optopt = 1;
  optind = 1;
  while ((c = getopt (argc, argv, "c:dl:")) != -1)
    switch (c)
      {
        case 'c':
          config_file = optarg;
          break;
          
        case 'd':
          log->dbg("Starting in debug mode");
          debug_mode = true;
          break;
        
        case 'l':
          log->dbg("Uncoded -l flag given");
          break;
        
        case '?':
          if (optopt == 'c')
            log->dbg ("Option -c requires an argument.");
          else  
            log->dbg("Unknown option given");
          return;
        
        default:
          log->dbg("CNHmqtt::CNHmqtt ?!?");
      }
      
      
  CNHmqtt::mqtt_topic = "test"; //man_channel;
  CNHmqtt::status = "Running";
  CNHmqtt::mosq_server = "127.0.0.1";
  CNHmqtt::mosq_port = 1883;      
  
  pthread_mutex_init (&mosq_mutex, NULL);
      
  if (config_file != "")
  {
    reader = new INIReader(config_file);
    
    if (reader->ParseError() < 0) 
    {
        log->dbg("Failed to open/parse config file [" + config_file + "]");
    } else
    {
      config_file_parsed = true;
      mosq_server = reader->Get("mqtt", "host", "localhost");
      mosq_port   = reader->GetInteger("mqtt", "port", 1883);
      mqtt_topic  = reader->Get("mqtt", "topic", "test"); 
      logfile     = reader->Get("mqtt", "logfile", ""); 
      uid         = reader->GetInteger("mqtt", "uid", 0);
      
      if (reader->Get("mqtt", "no_status_debug", "false") == "true")
        no_staus_debug = true;
      else no_staus_debug = false; 
    }    
  }
  
  // Switch to less privileged user if set
  if (uid)
    if (setuid(uid))
    {
      log->dbg("Failed to switch user!");
      exit(1);
    }
  
  if (!debug_mode)
    if(!log->open_logfile(logfile))
      exit(1);
  
  mqtt_rx = mqtt_topic + "/rx";
  mqtt_tx = mqtt_topic + "/tx";
  
  log->dbg("mosq_server = " + mosq_server);
  out << "" << mosq_port;
  log->dbg("mosq_port = "   + out.str());
  log->dbg("mqtt_tx = "     + mqtt_tx);
  log->dbg("mqtt_rx = "     + mqtt_rx);
  
  terminate = false;
  reset = false;  
}

CNHmqtt::~CNHmqtt()
{
  if (mosq_connected && (mosq != NULL))
    mosquitto_disconnect(mosq);         
  
  if (mosq != NULL)
    mosquitto_destroy(mosq); 
  
  mosquitto_lib_cleanup();
  
  if (reader!=NULL)
  {
    delete reader;
    reader=NULL;
  }
  
  if (log!=NULL)
  {
    delete log;
    log=NULL;
  }
}
  
int CNHmqtt::daemonize()
{
  if (daemonized)
    return 0;
  
  if (debug_mode)
    return 0;
  else
  {
    daemonized = true;
    return  daemon(1, 0); // don't change dir, but do redirect stdio to /dev/null
  }
}

string CNHmqtt::get_str_option(string section, string option, string def_value)
{
  if (config_file_parsed && reader != NULL)
    return reader->Get(section, option, def_value); 
  else
    return def_value;
}

int CNHmqtt::get_int_option(string section, string option, int def_value)
{
  if (config_file_parsed && reader != NULL)
    return reader->GetInteger(section, option, def_value); 
  else
    return def_value;
}

int CNHmqtt::mosq_connect()
{
  std::stringstream out;
  out << mqtt_topic << "-" << getpid();
  
  log->dbg("Connecting to Mosquitto as [" + out.str() + "]");;
  
  mosq = mosquitto_new(out.str().c_str(), this);  
  if(!mosq)
  {
    cout << "mosquitto_new() failed!";
    return -1;
  }  
  
  mosquitto_connect_callback_set(mosq, CNHmqtt::connect_callback);
  mosquitto_message_callback_set(mosq, CNHmqtt::message_callback);  
  
    // int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive, bool clean_session);
  if(mosquitto_connect(mosq, mosq_server.c_str(), mosq_port, 300, true)) 
  {
    log->dbg("mosq_connnect failed!");
    mosquitto_destroy(mosq);
    mosq = NULL;
    return -1;
  }
  mosq_connected = true;
    
  return 0;       
}

void CNHmqtt::connect_callback(void *obj, int result)
{
  CNHmqtt *m = (CNHmqtt*)obj;

  if(!result)
  {  
    m->log->dbg("Connected to mosquitto.");  
    mosquitto_subscribe(m->mosq, NULL, m->get_topic().c_str(), 0);
  } else
  {
    m->log->dbg("Failed to connect to mosquitto!");
  }
}

void CNHmqtt::message_callback(void *obj, const struct mosquitto_message *message)
{
  CNHmqtt *m = (CNHmqtt*)obj;
  string payload;
  string topic;
 
  
  if(message->payloadlen)
  { 
    payload = ((char *)message->payload);
    topic = ((char *)message->topic);    
    
    if (!m->no_staus_debug)
      m->log->dbg("Got mqtt message, topic=[" + topic + "], message=[" + payload + "]");
    else // no_staus_debug is set - so only print out message to log if it's /not/ a status request
    {
      if (!((topic == m->mqtt_rx) && (payload == "STATUS")))
        m->log->dbg("Got mqtt message, topic=[" + topic + "], message=[" + payload + "]");
    }
      
    m->process_message(topic, payload);
  }
}

int CNHmqtt::subscribe(string topic)
{
  log->dbg("Subscribing to topic [" + topic + "]");
  if (topic=="")
  {
    log->dbg("Cannot subscribe to empty topic!");
    return -1;
  }
    
  if (!mosq_connected)
  {
    log->dbg("Subscribe failed - Not connected!");
    return -1;
  }    
    
  if(mosquitto_subscribe(mosq, NULL, topic.c_str(), 0))
  {
    log->dbg("Subscribe failed!");
    return -1;
  }
  
  return 0;
}

void CNHmqtt::process_message(string topic, string message)
{
  // Process any messages sent to our management topic (terminate or status)
  if (topic == mqtt_rx)
  {
    if (message == "TERMINATE")
    {
      log->dbg("Terminate message received...");  
      terminate = true;
    }
    
    if (message == "STATUS")
      message_send(mqtt_tx, status, no_staus_debug);
    
    if (message == "RESET")
    {
      log->dbg("RESET message received...");
      reset = true;
    }
  }
}

int CNHmqtt::message_send(string topic, string message, bool no_debug)
{
  int ret;
  
  if (!no_debug)
    log->dbg("Sending message,  topic=[" + topic + "], message=[" + message + "]");
  pthread_mutex_lock(&mosq_mutex);
  ret = mosquitto_publish(mosq, NULL, topic.c_str(), message.length(), (uint8_t*)message.c_str(), 0, false);
  pthread_mutex_unlock(&mosq_mutex);
  return ret;
}

int CNHmqtt::message_send(string topic, string message)
{
  return message_send(topic, message, false);
}

string CNHmqtt::get_topic()
{
  return mqtt_rx;
}

int CNHmqtt::message_loop(void)
{
  string dbgmsg="";
  
  if (mosq==NULL)
    return -1;
  
  while(!mosquitto_loop(mosq, 50)  && !terminate && !reset);
  
  if (terminate)
    log->dbg("terminate=true");
  else
    log->dbg("terminate=false");
  
  if (reset)
    log->dbg("reset=true");
  else
    log->dbg("reset=false");  
  
  log->dbg("Exit.");
  mosq_connected = false;
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosq = NULL;  
  
  if (terminate)
    return EXIT_TERMINATE;
  else if (reset)
    return EXIT_RESET;
  else
    return 0;
}


// Other stuff not mqtt / instrumentation specifc
// Integer to String
string CNHmqtt::itos(int n)
{
  string s;
  stringstream out;
  out << n;
  return out.str();
}

