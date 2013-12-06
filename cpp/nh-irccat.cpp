#include "CNHmqtt_irc.h"
#include "nh-irccat.h"
#include <string.h>


class nh_irccat: public CNHmqtt_irc
{
  public:
    string motd;
    string command_runner;
    string commands;
    
    nh_irccat(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      command_runner = get_str_option("irccat", "command_runner", "./command_runner.py"); 
      commands = get_str_option("irccat", "commands", "irccat/commands.sh");
    }    
    
   void process_irc_message(irc_msg msg)
   {
     FILE *fp;  
     char cmd_ret[4000];     
     string cmd;
     string irc_chan;
     string str_ret;

     if (msg=="!help")
     {
       msg.reply("?commands - List commands ripped off from London Hackspace");
     }
     
     // Special case for ?commands, as it needs to PM always PM the results
     // (nb commands.sh needs tweaking to remove netcat call)
     if (msg=="?commands")
     {
       cmd = commands + " \"" + msg.nick + "\"";       
       log->dbg("cmd = [" + cmd + "]");
       fp = popen(cmd.c_str(), "r");
       memset(cmd_ret, 0, sizeof(cmd_ret));
       while (fgets (cmd_ret , sizeof(cmd_ret) , fp))
         msg.reply_pm(cmd_ret); 
       pclose(fp);   
       
       if (!msg.is_pm())
         irc_send_channel(msg.nick + ", I have pm'd you the list of available commands.", msg.channel);       
     } else
     
     if (msg.message.substr(0, 1)=="?")
     {
       if (msg.is_pm())
         irc_chan="null"; 
       else 
         irc_chan = msg.channel;

       cmd = command_runner + " \"" + msg.nick + " " + irc_chan + " " + msg.nick + " " + msg.message.substr(1, string::npos) + "\"";

       log->dbg("cmd = [" + cmd + "]");
       fp = popen(cmd.c_str(), "r");
       memset(cmd_ret, 0, sizeof(cmd_ret));
       while (fgets (cmd_ret , sizeof(cmd_ret) , fp))
       {
         str_ret = cmd_ret;
         
         if (str_ret.find_first_not_of(" \r\n") != string::npos)
           msg.reply(str_ret); 
       }
       pclose(fp);
     }
   }
   
   bool setup()
   {
     if (!init()) // connect to mosquitto, daemonize, etc
      return -1;

     return true;
   }
};

int main(int argc, char *argv[])
{ 
  nh_irccat nh = nh_irccat(argc, argv);
  nh.setup();
  
  nh.message_loop();
  return 0;
  
}