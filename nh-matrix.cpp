/* 
 * Copyright (c) 2011, Daniel Swann <hs@dswann.co.uk>, Matt Lloyd 
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
 
#include "CNHmqtt_irc.h"
#include "nh-matrix.h"
#include <string.h>

class nh_matrix : public CNHmqtt_irc
{
public:

  string mb_in;
  string mb_out;
  string display_string;
  string msg;
  string clrscreen_string;
  string nick_string;
  
  nh_matrix(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
  {
    mb_in = get_str_option("matrix", "mb_in", "nh/mb/rx");
    mb_out = get_str_option("matrix", "mb_out", "nh/mb/tx");
    display_string = get_str_option("matrix", "display_string", "D:");
    clrscreen_string = get_str_option("matrix", "clrscreen_string", "CLEAR:");
    nick_string = get_str_option("matrix", "nick_string", "N:");
  }
  
  void process_message(string topic, string message)
  {
  
      if (topic==mb_in)
      {
        // handel mesage sent from mattrix to say that message was actually displayed and pass back to irc
        // TODO ***LWK***
      }
 
  
  
    CNHmqtt_irc::process_message(topic, message);
  }
  
  void process_irc_message(irc_msg msg)
  {  
    string message;
    string dsp_msg;
    
    if (msg == "!help")
      msg.reply("!display \"msg\"- Display msg on the hackspace boards, use blank msg to clear");
    
    message = (string)msg;
    
    // just check at start
    if (message.substr(0, strlen("!display")) == "!display")
    {
      // chrunch message
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