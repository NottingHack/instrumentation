#pragma once
#include "CNHmqtt.h"


class CNHmqtt_irc : CNHmqtt
{
  public:
    CNHmqtt_irc(int argc, char *argv[]);
    ~CNHmqtt_irc();
    
    class irc_msg {
      public:
        std::string nick;
        std::string channel;
        std::string message;
        
        irc_msg(std::string msg, std::string chan, std::string nk, CNHmqtt_irc *irc_con)
        { 
          message = msg;
          channel = chan;
          nick = nk;
          con = irc_con;  
        }
        
        int reply(std::string rep)
        {
          if (channel=="")
            return con->irc_send_nick (rep, nick);
          else
            return con->irc_send_channel (rep, channel);        
        }
        
        int reply_pm(std::string rep)
        {
          return con->irc_send_nick (rep, nick);        
        }        
        
        bool is_pm()
        {
          return (channel=="");
        }

        // Allow comparision to a std::string to work (and only consider message not nick/chan)
        bool operator==(const std::string &str_message) const 
        {
          return (message==str_message);
        }
        
        // Allow cast to std::string
        operator std::string() 
        { 
          return message; 
        }        
        
      private:
        CNHmqtt_irc *con;
    };    
    
    std::string irc_out;
    std::string irc_in;    
    
    static bool decode_irc_topic(std::string irc_in, std::string topic, std::string &nick, std::string &channel);
    int irc_reply(std::string message, irc_msg dst);
    int irc_send_nick (std::string message, std::string nick);
    int irc_send_channel (std::string message, std::string channel);
    bool is_irc_msg(std::string topic); 
    void process_message(std::string topic, std::string message);
    virtual void process_irc_message(irc_msg msg) = 0;
    bool init();

    using CNHmqtt::get_str_option;
    using CNHmqtt::get_int_option;
    using CNHmqtt::message_send;
    using CNHmqtt::log;
    using CNHmqtt::subscribe;
    using CNHmqtt::message_loop;

        
//  private:

};


