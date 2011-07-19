#include "CNHmqtt_irc.h"
#include "nh-irccat.h"
#include <string.h>


class nh_irccat: public CNHmqtt_irc
{
  public:
    string motd;
    string flasher;
    int flash_duration;
    
    nh_irccat(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      flasher = get_str_option("alert", "flasher", "nh/gk/relay2");
      flash_duration = get_int_option("alert", "flash_duration", 4000);
    }    
    
   void process_irc_message(irc_msg msg)
   {
     FILE *fp;  
     char cmd_ret[4000];     
     string cmd;
     string irc_chan;
     string str_ret;
     pthread_attr_t tattr;
     pthread_t light_thread;     

     if (msg=="!help")
     {
       msg.reply("?commands - List commands ripped off from London Hackspace");
       msg.reply("!alert - Flash light next to matrix display");
     }
     
     if (msg=="!alert")
     {
         // Now flash the light
         pthread_attr_init(&tattr);
         pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
         pthread_create(&light_thread, &tattr, &flash_light, this);          
     }
     
     // Special case for ?commands, as it needs to PM always PM the results
     // (nb commands.sh needs tweaking to remove netcat call)
     if (msg=="?commands")
     {
       cmd = "./irccat/commands.sh \"" + msg.nick + "\"";       
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
       
       //cmd = "./irccat/" + msg.message.substr(1, msg.message.find_first_of(" ")) + " \"" + msg.nick + "\""+ " \"" + irc_chan + "\"" + " \"" + msg.nick + "\" ";
       
       cmd = "./command_runner.py \"" + msg.nick + " " + irc_chan + " " + msg.nick + " " + msg.message.substr(1, string::npos) + "\"";

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
   
 static void *flash_light(void *arg)
  {
    nh_irccat *ir;
    int n;
      
    ir = (nh_irccat*) arg;
    
    // Ring door bell
    ir->message_send(ir->flasher, "HIGH");
          
    for (n=0; n < (ir->flash_duration); n++)
      usleep(1000); // 1ms
          
    ir->message_send(ir->flasher, "LOW");
    
    return NULL;
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
  nh_irccat nh = nh_irccat(argc, argv);
  nh.setup();
  
  nh.message_loop();
  return 0;
  
}