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
#include <libgen.h>
#include <string.h>

CNHmqtt::CNHmqtt(int argc, char *argv[]) 
{
  log = NULL;
  string config_file = "";
  _config_file_parsed = false;
  _config_file_default_parsed = false;
  int c;
  std::stringstream out;
  _reader = NULL;
  _mosq_connected = false;
  log = new CLogging();
  string logfile;
  _uid = 0;
  _no_staus_debug = false;
  string def_config="";
  char buf[256]="";
  
  mosquitto_lib_init();
  _mosq = NULL;
    
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
      
      
  CNHmqtt::_mqtt_topic = "test"; //man_channel;
  CNHmqtt::_mosq_server = "127.0.0.1";
  CNHmqtt::_mosq_port = 1883;      
  
  pthread_mutex_init (&_mosq_mutex, NULL);
      
  if (config_file != "")
  {
    // Open process specific config file
    log->dbg("Opening config file: [" + config_file + "]");
    _reader = new INIReader(config_file);
    if (_reader->ParseError() < 0) 
      log->dbg("Failed to open/parse config file [" + config_file + "]");
    else
      _config_file_parsed = true;
        
    // Open general config file
    strcpy(buf, config_file.c_str());
    def_config = (string)dirname(buf) + "/default.conf" ;
    log->dbg("Opening config file: [" + def_config + "]");
    _reader_default = new INIReader(def_config);
    if (_reader_default->ParseError() < 0) 
      log->dbg("Failed to open/parse config file [" + def_config + "]");
    else
      _config_file_default_parsed = true;
            
    _mosq_server  = get_str_option("mqtt", "host", "localhost");
    _mosq_port    = get_int_option("mqtt", "port", 1883);
    _mqtt_topic   = get_str_option("mqtt", "topic", "test"); 
    logfile       = get_str_option("mqtt", "logfile", ""); 
    _uid          = get_int_option("mqtt", "uid", 0);
    
    _status_req_topic = get_str_option("mqtt", "status_request", "nh/status/req");
    _status_res_topic = get_str_option("mqtt", "status_response", "nh/status/res");
    _status_name  = get_str_option("mqtt", "status_name", "");
    
    // No status/proces name set in config file, default to process id
    if (_status_name == "")
      _status_name = itos(getpid());
      
    if (get_str_option("mqtt", "no_status_debug", "false") == "true")
      _no_staus_debug = true;
    else 
      _no_staus_debug = false;    
  }
  
  // Switch to less privileged user if set
  if (_uid)
    if (setuid(_uid))
    {
      log->dbg("Failed to switch user!");
      exit(1);
    }
  
  if (!debug_mode)
    if(!log->open_logfile(logfile))
      exit(1);
  
  _mqtt_rx = _mqtt_topic + "/rx";
  _mqtt_tx = _mqtt_topic + "/tx";
  
  log->dbg("mosq_server = " + _mosq_server);
  out << "" << _mosq_port;
  log->dbg("mosq_port = "   + out.str());
  log->dbg("mqtt_tx = "     + _mqtt_tx);
  log->dbg("mqtt_rx = "     + _mqtt_rx);
}

CNHmqtt::~CNHmqtt()
{
  if (_mosq_connected && (_mosq != NULL))
    mosquitto_disconnect(_mosq);         
  
  if (_mosq != NULL)
    mosquitto_destroy(_mosq); 
  
  mosquitto_lib_cleanup();
  
  if (_reader!=NULL)
  {
    delete _reader;
    _reader=NULL;
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
/* Get string value from config file. First try process specific config file, then 
 * default config file, then finally just return def_value */
{
  if (_config_file_parsed && _reader != NULL && _reader->KeyExists(section, option))    
    return _reader->Get(section, option, def_value); 
  else if (_config_file_default_parsed && _reader_default != NULL && _reader_default->KeyExists(section, option))
    return _reader_default->Get(section, option, def_value); 
  else
    return def_value; 
}

int CNHmqtt::get_int_option(string section, string option, int def_value)
{
  if (_config_file_parsed && _reader != NULL && _reader->KeyExists(section, option))    
    return _reader->GetInteger(section, option, def_value); 
  else if (_config_file_default_parsed && _reader_default != NULL && _reader_default->KeyExists(section, option))
    return _reader_default->GetInteger(section, option, def_value); 
  else
    return def_value;  
}

int CNHmqtt::mosq_connect()
{
  std::stringstream out;
  out << _mqtt_topic << "-" << getpid();
  
  log->dbg("Connecting to Mosquitto as [" + out.str() + "]");;
  
  _mosq = mosquitto_new(out.str().c_str(), true, this);  
  if(!_mosq)
  {
    cout << "mosquitto_new() failed!";
    return -1;
  }  
  
  mosquitto_message_callback_set(_mosq, CNHmqtt::message_callback);  
  
  if(mosquitto_connect(_mosq, _mosq_server.c_str(), _mosq_port, 300)) 
  {
    log->dbg("mosq_connnect failed!");
    mosquitto_destroy(_mosq);
    _mosq = NULL;
    return -1;
  }
  _mosq_connected = true;
  subscribe(_mqtt_rx);
  subscribe(_status_req_topic);
  
  message_send(_status_res_topic, "Restart: " + _status_name);

  return 0;
}

void CNHmqtt::message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
  CNHmqtt *m = (CNHmqtt*)obj;
  string payload;
  string topic;
 
  
  if(message->payloadlen)
  { 
    payload = ((char *)message->payload);
    topic = ((char *)message->topic);
    
    if (!m->_no_staus_debug)
      m->log->dbg("Got mqtt message, topic=[" + topic + "], message=[" + payload + "]");
    else // no_staus_debug is set - so only print out message to log if it's /not/ a status request
    {
      if (topic != m->_status_req_topic)
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
    
  if (!_mosq_connected)
  {
    log->dbg("Subscribe failed - Not connected!");
    return -1;
  }    
    
  if(mosquitto_subscribe(_mosq, NULL, topic.c_str(), 0))
  {
    log->dbg("Subscribe failed!");
    return -1;
  }
  
  return 0;
}

void CNHmqtt::process_message(string topic, string message)
{
  // Process any messages sent to our management topic (terminate or status)
  if (topic == _mqtt_rx)
  {
    if (message == "TERMINATE")
    {
      log->dbg("Terminate message received...");  
      mosquitto_disconnect(_mosq);
    }
  } else if (topic == _status_req_topic)
  {
    if (message == "STATUS")
      message_send(_status_res_topic, "Running: " + _status_name, _no_staus_debug);
  } 
}

int CNHmqtt::message_send(string topic, string message, bool no_debug)
{
  int ret;
  
  if (!no_debug)
    log->dbg("Sending message,  topic=[" + topic + "], message=[" + message + "]");
  pthread_mutex_lock(&_mosq_mutex);
  ret = mosquitto_publish(_mosq, NULL, topic.c_str(), message.length(), message.c_str(), 0, false);
  pthread_mutex_unlock(&_mosq_mutex);
  return ret;
}

int CNHmqtt::message_send(string topic, string message)
{
  return message_send(topic, message, false);
}

string CNHmqtt::get_topic()
{
  return _mqtt_rx;
}

int CNHmqtt::message_loop(void)
{
  string dbgmsg="";
  int ret;
  
  if (_mosq==NULL)
    return -1;
  
  ret = mosquitto_loop_forever(_mosq, 50, 999);
  
  log->dbg("Exit. mosquitto_loop_forever ret = " + itos(ret));
  _mosq_connected = false;
  mosquitto_disconnect(_mosq);
  mosquitto_destroy(_mosq);
  _mosq = NULL;  
  
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

