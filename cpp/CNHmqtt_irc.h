#pragma once
#include "CNHmqtt.h"

using namespace std;


class CNHmqtt_irc : CNHmqtt
{
  public:
    CNHmqtt_irc(int argc, char *argv[]);
    ~CNHmqtt_irc();
    
    class irc_msg {
      public:
        string nick;
        string channel;
        string message;
        
        irc_msg(string msg, string chan, string nk, CNHmqtt_irc *irc_con)
        { 
          message = msg;
          channel = chan;
          nick = nk;
          con = irc_con;  
        }
        
        int reply(string rep)
        {
          if (channel=="")
            return con->irc_send_nick (rep, nick);
          else
            return con->irc_send_channel (rep, channel);        
        }
        
        int reply_pm(string rep)
        {
          return con->irc_send_nick (rep, nick);        
        }        
        
        bool is_pm()
        {
          return (channel=="");
        }

        // Allow comparision to a string to work (and only consider message not nick/chan)
        bool operator==(const string &str_message) const 
        {
          return (message==str_message);
        }
        
        // Allow cast to string
        operator string() 
        { 
          return message; 
        }        
        
      private:
        CNHmqtt_irc *con;
    };    
    
    string irc_out;
    string irc_in;    
    
    static bool decode_irc_topic(string irc_in, string topic, string &nick, string &channel);
    int irc_reply(string message, irc_msg dst);
    int irc_send_nick (string message, string nick);
    int irc_send_channel (string message, string channel);
    bool is_irc_msg(string topic); 
    void process_message(string topic, string message);
    virtual void process_irc_message(irc_msg msg) {};
    bool init();

    using CNHmqtt::get_str_option;
    using CNHmqtt::get_int_option;
    using CNHmqtt::message_send;
    using CNHmqtt::log;
    using CNHmqtt::subscribe;
    using CNHmqtt::message_loop;

        
//  private:

};


