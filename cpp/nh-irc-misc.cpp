/* 
 * Copyright (c) 2014, Daniel Swann <hs@dswann.co.uk>
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
#include "CNHDBAccess.h"
#include "nh-irc-misc.h"

using namespace std;

class nh_irc_misc : public CNHmqtt_irc
{
  public:
    CNHDBAccess *db;
    string entry_announce;
    string door_button;
    string slack_mqtt_tx;
    string slack_mqtt_rx;
    string ircmsg_mqtt_tx;
    string slack_channel;
    
    nh_irc_misc(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   ;   
      entry_announce = get_str_option("gatekeeper", "entry_announce", "nh/gk/entry_announce");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/DoorButton");
      ircmsg_mqtt_tx = get_str_option("irc", "msg_mqtt_tx", "nh/ircmsg/tx");
      slack_mqtt_tx = get_str_option("slack", "mqtt_tx", "nh/slack/tx");
      slack_mqtt_rx = get_str_option("slack", "mqtt_rx", "nh/slack/rx");
      slack_channel = get_str_option("slack", "channel", "irc");
    }

    void process_message(string topic, string message)
    {
      
      // Send any messages from the slack channel to IRC
      if 
      (
        (topic.length()+2 > slack_mqtt_rx.length()) &&
        (topic.substr(0, slack_mqtt_rx.length()) == slack_mqtt_rx)
      )
      {
        string username;
        string channel;
        decode_irc_topic(slack_mqtt_rx, topic, username, channel); // function originally written for IRC, but works well enough for slack
        if (channel == slack_channel) // the slack bot can be in multiple channels, so only forward messages from the nominated channel to avoid confusion...
          message_send(ircmsg_mqtt_tx, "<" + username + "> " + message);
      }
      
      // Entry annouce is Door opened / Door opened by: etc
      if ((topic.substr(0, entry_announce.length() ) == entry_announce))
      {
        message_send(irc_out, message);
      }
      
      if (topic == door_button)
      {
        string tmp;
        tmp = message;
        for (int c = 0; message[c]; c++)
          tmp[c] = tolower(message[c]);
        message_send(irc_out, "Door Bell (" + tmp +")");
      }
      
      CNHmqtt_irc::process_message(topic, message);
    }
    
   void process_irc_message(irc_msg msg)
   {
     string activity;
     string temperature;
     string sMsg = msg;
     string tts_msg;

     // Send all chat message to the slack channel
     if (!msg.is_pm())
       message_send(slack_mqtt_tx, "<" + msg.nick + "> " + msg.message);

     if (msg=="!help")
     {
       // .reply will reply either in the channel, or via PM, depending
       // on how the message was received.
//     msg.reply("!alert - Flash light next to matrix display");  Comming one day. maybe.
       msg.reply("!status - Best guess if the space is open");   
       msg.reply("!temp - Latest Temperature readings"); 
       msg.reply("!tools - Show current status of all access controlled tools");
       msg.reply("Temperature graphs: http://cacti.nottinghack.org.uk/graph_view.php"); 

     }

     if (msg=="!status")
     {
        db->sp_space_net_activity(activity);
        msg.reply(activity);
     }
    
     if (msg=="!temp")
     {
        db->sp_temperature_check(temperature);
        msg.reply(temperature);      
     }

     if (msg=="!tools")
     {
       dbrows tool_list;
       string tools_msg;

       db->sp_tool_get_status(-1, &tool_list);
       for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator)
       {
         dbrow row = *iterator;
         tools_msg = row["tool_name"].asStr() + " - ";

         if (row["tool_status"].asStr() == "FREE")
         {
           tools_msg += "Free; last used: " + row["usage_end"].asStr();
         }
         else if (row["tool_status"].asStr() == "IN_USE")
         {
           tools_msg += "In use since: " + row["usage_start"].asStr();
         }
         else if (row["tool_status"].asStr() == "DISABLED")
         {
           tools_msg += "Out of sevice";
           if (!(row["tool_status_text"].isNull()))
             tools_msg += ": " + row["tool_status_text"].asStr();
         }
         else
         {
           tools_msg += "Status unknown!";
         }
         msg.reply(tools_msg);
       }
     } //end if (msg=="!tools")

   }

   bool setup()
   {
     if (!init()) // connect to mosquitto, daemonize, etc
      return false;
     
     if (db->dbConnect())
       return false;
     
     subscribe(entry_announce + "/#");
     subscribe(door_button);
     subscribe(slack_mqtt_rx + "/#");
     
     return true;
   }
  
};


int main(int argc, char *argv[])
{
 // run with "-d" flag to avoid daemonizing
 
  nh_irc_misc nh = nh_irc_misc(argc, argv);
  
  if (nh.setup())
  {
    nh.message_loop();
    return 0;
  } else
    return 1;
  
}
