#include "CNHmqtt_irc.h"
#include "nh-matrix.h"
#include <string.h>

class nh_matrix : public CNHmqtt_irc
{
public:
  string irc_in;
  string irc_out;
  string door_button;
  string mb_in;
  string mb_out;
  string display_string;
  string msg;
  string clrscreen_string;
  string nick_string;
  
  nh_matrix(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
  {
    irc_in = get_str_option("matrix", "irc_in", "nh/irc/rx");
    irc_out = get_str_option("matrix", "irc_out", "nh/irc/tx");
    door_button = get_str_option("matrix", "door_button", "nh/gk/button");
    mb_in = get_str_option("matrix", "mb_in", "nh/mb/rx");
    mb_out = get_str_option("matrix", "mb_out", "nh/mb/tx");
    display_string = get_str_option("matrix", "display_string", "D:");
    clrscreen_string = get_str_option("matrix", "clrscreen_string", "CLEAR:");
    nick_string = get_str_option("matrix", "nick_string", "N:");
  }
  
  void process_message(string topic, string message)
  {
    CNHmqtt_irc::process_message(topic, message);
  }
  
  void process_irc_message(irc_msg msg)
  {  
    string message;
    string dsp_msg;
    
    if (msg == "!help")
      msg.reply("!display \"msg\"- Display msg on the hackspace boards, use blank msg to clear");
    
    message = (string)msg;
    
    // TODO just check at start
    if (message.substr(0, strlen("!display")) == "!display")
    {
      // TODO chrunch message
      if (message.length() > (strlen("!display")+1)) 
      {
        dsp_msg = message.substr(strlen("!display")+1, string::npos);
        message_send(mb_out, nick_string + msg.nick);
        message_send(mb_out, display_string + dsp_msg);
          
        msg.reply("Displayed on Matrix");
      } else 
      {
        message_send(mb_out, clrscreen_string);
        msg.reply("Matrix wiped");
      }   
    }     
  }
  
  int setup()
  {
    if (!init()) // connect to mosquitto, daemonize, etc
      return 1;
     
    subscribe(irc_in + "/#");
    subscribe(door_button);
    subscribe(mb_in);
    
    return 0;
  }
};

int main(int argc, char *argv[])
{
  // run with "-d" flag to avoid daemonizing
  nh_matrix nh = nh_matrix(argc, argv);
  
  if (nh.setup())
    return -1;
  
  nh.message_loop();
  
  return 0;
}