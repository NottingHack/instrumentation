#include "CNHmqtt_irc.h"
#include "nh-test-irc.h"

/* 
 * Test app using the IRC bot over MQTT.
 * Execute as "./nh-test-irc -d -c nh-test-irc.conf" to run without daemonizing
 */

class nh_test_irc : public CNHmqtt_irc
{
  public:
    string motd;
    
    nh_test_irc(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      // read motd from the config file
      motd = get_str_option("mqtt-irc-test", "motd", "(motd default)");
    }
   
    /* Optional function which, if present, can be used to process any 
    * MQTT message receiced.
    * nb. must call CNHmqtt_irc::process_message so process_irc_message will 
    * be called or IRC messages. 

    void process_message(string topic, string message)
    {
    
      CNHmqtt_irc::process_message(topic, message);
    }
    */
    
    
   void process_irc_message(irc_msg msg)
   {
     if (msg=="!help")
     {
       // .reply will reply either in the channel, or via PM, depending
       // on how the message was received.
       msg.reply("!echo <msg> - echo <msg>");   
       msg.reply("!hello - say hello");
       msg.reply("!motd - send message of the day via PM");
     }
     
     if (msg=="!hello")
     {
       msg.reply("Hello " + msg.nick + "!");
     }
     
     if (msg=="!motd")
     {
       msg.reply_pm(motd);  // Force the reply to be via PM
       
       // If the !motd wasn't PM'd to the bot, say "motd sent" in the channel
       if (!msg.is_pm())
       {
        irc_send_channel("motd sent", msg.channel);       
       }
     }
     
     if (msg.message.substr(0, 6)=="!echo ")
     {
       msg.reply(msg.message.substr(6));
     }
   }
   
   bool setup()
   {
     if (!init()) // connect to mosquitto, daemonize, etc
      return -1;
     
     // Do any setup, subscribe to other mqtt topics, etc, here
     //subscribe("nh/whatever");
     
     return true;
   }
  
};



int main(int argc, char *argv[])
{
 // run with "-d" flag to avoid daemonizing
 
  nh_test_irc nh = nh_test_irc(argc, argv);
  nh.setup();
  
  nh.message_loop();
  return 0;
  
}