#include "CNHmqtt.h"
#include "irc.h"
#include <stdio.h>

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

class nh_irc : public CNHmqtt
{
  
  public:
    irc *irccon;
    string irc_server;
    int irc_port;
    string irc_nick;
    string irc_channel;
    string irc_mqtt_tx;
    string irc_mqtt_rx;
    string irc_nickserv_password;
  
    nh_irc(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      irc_server  = get_str_option("irc", "server", "irc.freenode.net");
      irc_nick    = get_str_option("irc", "nick", "test12341234");
      irc_mqtt_tx = get_str_option("irc", "mqtt_tx", "irc/tx");
      irc_mqtt_rx = get_str_option("irc", "mqtt_rx", "irc/rx");
      irc_channel = "#" + get_str_option("irc", "channel", "test4444");
      irc_port    = get_int_option("irc", "port", 6667);
      irc_nickserv_password =  get_str_option("irc", "nickserv_password", "");
      irccon = NULL;
    }
    
    ~nh_irc()
    {
      if (irccon != NULL)
      {
        delete irccon;
      }
    }
   
    int irc_connect()
    {
      if (mosq == NULL)
      {
        log->dbg ("Must connect to mosquitto before IRC");
        return -1;
      }
      
      log->dbg("Connecting to irc...");
      irccon = new irc(irc_server, irc_port, irc_nick, irc_nickserv_password, log);
      
      if (irccon->ircConnect())
      {
        log->dbg("Failed to connect to IRC server");
        return -1;
      }
  
      irccon->join(irc_channel); 
      irccon->addCallback("", &irc_callback, this);
      log->dbg("Connected to irc.");
      subscribe(irc_mqtt_tx);
      return 0;
    }
    
  static int irc_callback(string user, string channel, string message, void *obj)
  {
    nh_irc *m;
    
    m = (nh_irc*)obj;
    
    if ((user=="") && (channel=="INTERNAL") && (message=="DISCONNECTED"))
    {
      // Disconnected from IRC, so send ourselves a reset message
      m->log->dbg("Disconnected from IRC!");
      m->message_send(m->mqtt_rx, "RESET");      
      return NULL;
    }
      
    if (m->mosq_connected)
      m->message_send(m->irc_mqtt_rx, message);
  
    return NULL;
  }    
    
    
  void process_message(string topic, string message)
  {
    
    if (topic == irc_mqtt_tx)
      irccon->send(message);
    
    CNHmqtt::process_message(topic, message);
  }
  
};

int main(int argc, char *argv[])
{
 
  nh_irc *nh;
  bool reset = true;
  
  nh = NULL;
  
  
  
  // All this is basicly just to auto-reconnect to IRC, with a small delay between retries
  while (reset)
  {
    reset = false;
    if (nh!=NULL)
    {
      delete nh;
      nh = NULL;
    }
    nh = new nh_irc(argc, argv);
    nh_irc::daemonize(); // will only work on first run
    nh->mosq_connect();
    
    if (nh->irc_connect())
    {
      reset = true;
      sleep(2);
      continue;
    }
    
    if (nh->message_loop() == 2)
    {
      reset = true;
      sleep(2);
    }
  }
    
  
  return 0;
  
}