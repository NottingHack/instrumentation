#pragma once
#include "CNHmqtt.h"


class CNHmqtt_irc : CNHmqtt
{
  public:
    CNHmqtt_irc(int argc, char *argv[]);
    ~CNHmqtt_irc();
    
    class irc_msg {
      public:
        
        enum msg_type
        {
          MSGTYPE_IRC,
          MSGTYPE_SLACK
        };
        
        std::string nick;
        std::string channel;
        std::string message;
        msg_type msgtype; // IRC or SLACK
        
        irc_msg(std::string msg, std::string chan, std::string nk, CNHmqtt_irc *irc_con, msg_type mtyp)
        { 
          message = msg;
          channel = chan;
          nick = nk;
          con = irc_con;
          msgtype = mtyp;
          
          printf("irc_msg> message = [%s], channel = [%s], nick = [%s]\n",
                 message.c_str(), channel.c_str(), nick.c_str());
        }
        
        int reply(std::string rep)
        {
          if (msgtype == MSGTYPE_IRC)
          {
            if (channel=="")
              return con->irc_send_nick (rep, nick);
            else
              return con->irc_send_channel (rep, channel);
          } 
          else if (msgtype == MSGTYPE_SLACK)
          {
            if (channel=="")
              return con->slack_send_nick (rep, nick);
            else
              return con->slack_send_channel (rep, channel);
          } 
          else
            return -1;
        }

        int reply_pm(std::string rep)
        {
          if (msgtype == MSGTYPE_IRC)
            return con->irc_send_nick (rep, nick);
          else if (msgtype == MSGTYPE_IRC)
            return con->slack_send_nick (rep, nick);
          else
            return -1;
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
    std::string ircmsg_mqtt_tx;
    std::string irc_channel;
    
    std::string slack_out;
    std::string slack_in;
    std::string slack_irc_channel;
    
    static bool decode_irc_topic(std::string irc_in, std::string topic, std::string &nick, std::string &channel);
    int irc_reply(std::string message, irc_msg dst);
    int irc_send_nick (std::string message, std::string nick);
    int irc_send_channel (std::string message, std::string channel);
    int slack_reply(std::string message, irc_msg dst);
    int slack_send_nick (std::string message, std::string nick);
    int slack_send_channel (std::string message, std::string channel);
    bool is_irc_msg(std::string topic);
    bool is_slack_msg(std::string topic);
    void process_message(std::string topic, std::string message);
    virtual void process_irc_message(irc_msg msg) = 0;
    bool init();

    using CNHmqtt::get_str_option;
    using CNHmqtt::get_int_option;
    using CNHmqtt::message_send;
    using CNHmqtt::log;
    using CNHmqtt::subscribe;
    using CNHmqtt::message_loop;
    using CNHmqtt::itos;

        
//  private:

};


