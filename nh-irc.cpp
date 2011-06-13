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
      subscribe(irc_mqtt_tx + "/#");
      return 0;
    }
    
  static int irc_callback(string user, string channel, string message, void *obj)
  {
    nh_irc *m;
    
    // "channel" is either the channel name (e.g. '#nottinghack'), or an irc nick
    // if the bot was privmsg'd (e.g. 'daniel1111')
    
    m = (nh_irc*)obj;
    cout << "user=" << user << endl;
    
    if ((user=="") && (channel=="INTERNAL") && (message=="DISCONNECTED"))
    {
      // Disconnected from IRC, so send ourselves a reset message
      m->log->dbg("Disconnected from IRC!");
      m->message_send(m->mqtt_rx, "RESET");      
      return NULL;
    }
      
    // If the bot is sent a private message, publish to /<nick>, if 
    // it's a chat message in the channel, send to /#<channel>/<nick>.
    if (m->mosq_connected)
    {
      if (channel.substr(0,1)=="#")
        m->message_send(m->irc_mqtt_rx + "/" + channel.substr(1) + "/" + user, message);
      else
        m->message_send(m->irc_mqtt_rx + "/" + user, message);
    }
        
    return NULL;
  }    
    
  void process_message(string topic, string message)
  {
    // Send to the channel
    if (topic == irc_mqtt_tx)
    {
      irccon->send(message);
      return;
    }
  
    // send to a specific user (e.g. topic="nh/irc/tx/daniel1111")
    if (irc_mqtt_tx == topic.substr(0, irc_mqtt_tx.length()))
    {
      irccon->send(topic.substr(irc_mqtt_tx.length()+1),  message);
      return;
    }
 
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
    
  if (nh!=NULL)
  {
    delete nh;
    nh = NULL;
  }
  
  return 0;
  
}