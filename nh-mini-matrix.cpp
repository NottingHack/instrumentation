/*   
 * Copyright (c) 2012, Daniel Swann <hs@dswann.co.uk>
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
 * 3. Neither the name of the owner nor the names of its
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
#include "nh-mini-matrix.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#define BUFLEN 512

enum {MSG_ALERT, MSG_IRC, MSG_MAIL, MSG_TWITTER, MSG_MAX};

class nh_mini_matrix : public CNHmqtt_irc
{
  public:
    string topic_mail;
    string topic_twitter;
    string topic_gatekeeper;
    string topic_doorbell;
    string topic_temperature;
    int msg_display_duration, alert_display_duration;
    
    
    nh_mini_matrix(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      sck_tx = -1;
      sck_rx = -1;
      port = get_int_option("display", "port", 52240);      
      msg_display_duration   = get_int_option("display", "msg_duration", 45);      
      alert_display_duration = get_int_option("display", "alert_duration", 10); 
      ip = get_str_option("display", "address", "192.168.1.99"); 
      topic_twitter = get_str_option("message_source", "twitter", "nh/twitter/rx");
      topic_mail = get_str_option("message_source", "mail", "nh/mail/rx");
      topic_gatekeeper = get_str_option("message_source", "entry_announce", "nh/gk/entry_announce");
      topic_doorbell = get_str_option("gatekeeper", "door_button", "nh/gk/DoorButton");
      topic_temperature = get_str_option("temperature", "topic", "nh/temp");
      current_msg = MSG_IRC;
      active_disp_buf = 0;
      alert_active = 0;
      
      dispbuf[0].top = 0;
      dispbuf[0].bottom = 1;
      dispbuf[1].top = 2;
      dispbuf[1].bottom = 3;
      
      // Init msg_details
      for (int x=0; x < MSG_MAX; x++)
      {
        msg_details[x].top_line = "";
        msg_details[x].bottom_line = "";
        msg_details[x].display_time = msg_display_duration;
        msg_details[x].sent = 1;
      }
    }

    void process_message(string topic, string message)
    {   
      char envlope[4];
      unsigned int pos;
      
      // Door opened - cancel any doorbell alert
      if (topic.substr(0, topic_gatekeeper.length()) == topic_gatekeeper)
        cancel_alert();
      
      // Door opened - show on display (but only interested if a name was reported)
      if (topic == (topic_gatekeeper + "/known"))
      { 
        pos = message.find_first_of('(');
        if (pos != string::npos)
        {
          display_alert(message.substr(0, pos-1), message.substr(pos, string::npos), false); 
        } else
          display_alert(message, "", false); 
        
        
      }

      // Doorbell rang
      if (topic==topic_doorbell)
        display_alert("[     >>> D O O R    B E L L <<<     ]", message, true);

      // Email received
      if (topic.substr(0, topic_mail.length()+1) == topic_mail + "/")
      {
        // Char 128-130 in the matrix f/w is an envolope icon
        envlope[0] = 126;
        envlope[1] = 127;
        envlope[2] = 128;
        envlope[3] = 0;
        set_message_buffer(MSG_MAIL, envlope + topic.substr(topic.find_last_of("/")+1, string::npos), message);  
      }
      
      // tweet received
      if (topic.substr(0, topic_twitter.length()) == topic_twitter)
        set_message_buffer(MSG_TWITTER, "@" + message.substr(0,message.find_first_of(":")-1), message.substr(message.find_first_of(":")+1, string::npos));

      CNHmqtt_irc::process_message(topic, message);
    }
    
    
    void process_irc_message(irc_msg msg)
    {
     string ircmsg;
     
     ircmsg = msg;
     
     if (msg=="!help")
     {
        // N/A nh-matrix outputs the "!display" option
     }     
     
     if (ircmsg.substr(0, strlen("!display")) == "!display")
     {
       if (ircmsg.length() <= strlen("!display "))
       { // clear display
         set_message_buffer(MSG_IRC, "", "");
       } else
       {
         set_message_buffer(MSG_IRC, "<" + msg.nick + ">", ircmsg.substr(strlen("!display "), string::npos));
       }
     }
   }
       
    
   void set_message_buffer(int bufno, string top, string bottom)
   {
     msg_details[bufno].top_line = top;
     msg_details[bufno].bottom_line = bottom;       
     msg_details[bufno].sent = 0;       
     log->dbg("Set buffer [" + itos(bufno) + "] to [" + top + "] / [" + bottom + "]");
   }
   
   void display_alert(string top_line, string bottom_line, bool flash)
   {
     int buf;
     string opts;
     
     set_message_buffer(MSG_ALERT, top_line, bottom_line);
     alert_active = 1;
     msg_details[MSG_ALERT].display_time = 0;
     
     if (active_disp_buf)
       buf = 0;
     else
       buf = 1;
     
     opts = get_option_str(false, false, true, flash); // no scrolling, instant display
     
     // Send new message to display non-active message buffer
     udp_send("B" + itos(dispbuf[buf].top)    + opts + msg_details[MSG_ALERT].top_line   );
     udp_send("B" + itos(dispbuf[buf].bottom) + opts + msg_details[MSG_ALERT].bottom_line);
     
     // Tell display to switch buffers 
     udp_send("S" + itos(dispbuf[buf].top) + itos(dispbuf[buf].bottom));     
   }
   
   void cancel_alert()
   {
     if (alert_active)
     {
       log->dbg("Cancelling alert");    
       alert_active = 0;
     
       // Switch the display back to whatever it was showing before the alert
       udp_send("S" + itos(dispbuf[active_disp_buf].top) + itos(dispbuf[active_disp_buf].bottom));    
     }
   }
   
   string get_option_str(bool scroll_in, bool scroll, bool instant_display, bool flash)
   {
     string str;
     char val;
     val = 0;
     
     if (scroll_in)       val |= 1;
     if (scroll)          val |= 2;
     if (instant_display) val |= 4;
     if (flash)           val |= 8;
     
     if (val < 10)
       str = "0" + itos(val);
     else
       str = itos(val);
     
     return str;
   }

   bool setup()
   { 
     if (!init()) // connect to mosquitto, daemonize, etc
      return -1;
 
     struct sockaddr_in addr;
      
     // Create rx socket
     if ((sck_rx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
     {
       log->dbg("Failed to create rx socket.");
       return -1;        
     }
      
     // Bind socket
     memset((char *) &addr, 0, sizeof(addr));
     addr.sin_family = AF_INET;
     addr.sin_port = htons(port);
     addr.sin_addr.s_addr = htonl(INADDR_ANY);
     if (bind(sck_rx, (struct sockaddr *)&addr, sizeof(addr))==-1)
     {
       log->dbg("Failed to bind to socket (port = " + itos(port) + ")");
       return -1;        
     } else
       log->dbg("Listening on port " + itos(port) + ".");

     // create tx socket
     if ((sck_tx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
     {
       log->dbg("Failed to create tx socket.");
       return -1;        
     }
     memset((char *) &addr_tx, 0, sizeof(addr_tx));
     addr_tx.sin_family = AF_INET;
     addr_tx.sin_port = htons(port);
     
     if (!inet_aton(ip.c_str(), &addr_tx.sin_addr))
     {
       log->dbg("Invalid address: [" + ip + "]");
       return -1;
     } else
     {
       log->dbg("Mini-matrix address: [" + ip + ":" + itos(port) + "]");
     }
     
     
     // Start receive thread
     pthread_create(&rcvThread, NULL, &nh_mini_matrix::s_receive_thread, this);    
     
     // Start msg-display thread
     pthread_create(&msgThread, NULL, &nh_mini_matrix::s_msg_thread, this);          
     
     // Subscribe to mail, twitter & gatekeeper topics
     subscribe(topic_mail + "/#");
     subscribe(topic_twitter + "/#");
     subscribe(topic_gatekeeper + "/#");
     subscribe(topic_doorbell);
     
     
     return true;
   }
   
   void udp_send(string msg)
   {
     char dbgbuf[BUFLEN+256];
     socklen_t addrlen;
     addrlen = sizeof(addr_tx);

     sprintf(dbgbuf, "%s:%d > %s", inet_ntoa(addr_tx.sin_addr), port, msg.c_str());
     log->dbg(dbgbuf);
       
     if (sendto(sck_tx, msg.c_str(), msg.length(), 0, (struct sockaddr *)&addr_tx, addrlen) == -1)
     {
       log->dbg("Send failed.");
     } 
   }
   
    
   static void *s_receive_thread(void *arg)
   {      
     ((nh_mini_matrix*)arg)->receive_thread();
     return NULL;
   }
   
   static void *s_msg_thread(void *arg)
   {      
     ((nh_mini_matrix*)arg)->msg_thread();
     return NULL;
   }   
   
   void msg_thread()
   {
     int buf, nxtmsg;
     string opts;
     
     while(1)
     {
       
       if (alert_active)
       {
         if (msg_details[MSG_ALERT].display_time++ > alert_display_duration)
           cancel_alert();
       } else
       {
 
         // Timeout expired - switch to next message
         if (msg_details[current_msg].display_time++ > msg_display_duration)
         {
           // Get the next non-empty message
           nxtmsg = current_msg;
           do
           {
             nxtmsg++;
             nxtmsg = nxtmsg % MSG_MAX;
             if (nxtmsg==0)
               nxtmsg++;
             
             // If either top or bottom of the next message is set, then display it.
             if ((msg_details[nxtmsg].top_line.length()) || (msg_details[nxtmsg].bottom_line.length()))           
               break;
           }  while (nxtmsg != current_msg);
          
           if ((nxtmsg != current_msg) || (msg_details[current_msg].sent==0))
           {
             // If either top or bottom of the next message is set, then display it.
             msg_details[nxtmsg].display_time = 0; 
           
             if (active_disp_buf)
               buf = 0;
             else
               buf = 1;
              
             opts = get_option_str(true,false,false,false);
             udp_send("B" + itos(dispbuf[buf].top)    + opts + msg_details[nxtmsg].top_line   );
             
             opts = get_option_str(true,true,false,false);
             udp_send("B" + itos(dispbuf[buf].bottom) + opts + msg_details[nxtmsg].bottom_line);             
             
             active_disp_buf = buf;
             current_msg = nxtmsg;
             udp_send("S" + itos(dispbuf[active_disp_buf].top) + itos(dispbuf[active_disp_buf].bottom)); 
             msg_details[current_msg].sent=1;
           }
         }
       }
       
       sleep(1);
     }
     
     
   }
    
   void receive_thread()
   {
     char buf[BUFLEN];
     char response[BUFLEN];
     char dbgbuf[BUFLEN+256];
     struct sockaddr_in addr;
     socklen_t addrlen;
     int len;
     
     memset(buf, 0, sizeof(buf));
     
     log->dbg("entered receive_thread()");      
      
     memset(buf, 0, sizeof(buf));
     addrlen = sizeof(addr);
     while ((len=recvfrom(sck_rx, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr, &addrlen)) != -1)
     {
       if (buf[strlen(buf)-1] == '\n') // If the last character is a newline, remove it.
         buf[strlen(buf)-1] = 0;
            
       sprintf(dbgbuf, "%s:%d < %s", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
       log->dbg(dbgbuf);
       if (strcmp(inet_ntoa(addr.sin_addr), ip.c_str()))
       {
         log->dbg("Message not from mini-matrix display! Ignoring..."); 
       } else
       {
         memset(response, 0, sizeof(response));
         process_message(buf, len, response);
         if (strlen(response) > 0)
         {                
          udp_send(response);
         }
       }
     
       memset(buf, 0, sizeof(buf));
     }

     perror("Done.");
     return;         
    }
    
    void process_message(char *msgbuf, unsigned int len, char *response)
    {
      unsigned char data[9];
      unsigned char addr[8];
      unsigned int raw;
      char cfg;
      float celsius;
      char txt[100];
      
      
      if ((msgbuf[0] == 'T') && (len >18))
      {
        log->dbg("Temperature report"); 
        
        memcpy(addr, msgbuf+1, 8); // read address
        memcpy(data, msgbuf+10, 9); // read data
        
        raw = (data[1] << 8) | data[0];
        cfg = (data[4] & 0x60);
        if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
        // default is 12 bit resolution, 750 ms conversion time
    
        celsius = (float)raw / 16.0;
       
        sprintf(txt, "Address: %X:%X:%X:%X:%X:%X:%X:%X, Data: %X:%X:%X:%X:%X:%X:%X:%X:%X, Temp: %f", addr[0],addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], data[0],data[1], data[2], data[3], data[4], data[5], data[6], data[7],  data[8], celsius);
        log->dbg(txt);
        
        sprintf(txt, "%02X%02X%02X%02X%02X%02X%02X%02X:%4.2f", addr[0],addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], celsius);
        
        if ((celsius < -30) || (celsius > 80))
        {
          log->dbg(txt);
          log->dbg("Out of range temperature - not logging"); 
        }
        else          
          message_send(topic_temperature, txt);
      }
    }
    
    string itos(int n)
    {
      string s;
      stringstream out;
      out << n;
      return out.str();
    }      
   
   
  private:
    int sck_tx;
    int sck_rx;
    int port;
    int current_msg;
    string ip;
    struct sockaddr_in addr_tx;
    pthread_t rcvThread; 
    pthread_t msgThread;
    int alert_active;
    int active_disp_buf;
    
    struct display_buffer
    {
      int top;
      int bottom;
    } dispbuf[2];
    
    
    struct msgd
    {
      string top_line;
      string bottom_line;
      char options;      
      int display_time;
      int sent;
    } msg_details [MSG_MAX];
  
};



int main(int argc, char *argv[])
{
 // run with "-d" flag to avoid daemonizing
 
  nh_mini_matrix nh = nh_mini_matrix(argc, argv);
  nh.setup();
  
  nh.message_loop();
  return 0;
  
}