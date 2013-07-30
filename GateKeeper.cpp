/* 
 * Copyright (c) 2011, Daniel Swann <hs@dswann.co.uk>
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
#include "db/lib/CNHDBAccess.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

class GateKeeper : public CNHmqtt_irc
{
  public:
    string door_buzzer;
    string door_contact;
    string door_button;
    string rfid;
    string unlock;
    string keypad;
    string handle;
    string lastman;
    string lastman_open;
    string lastman_close;
    string twitter_out;
    string last_seen;
    string entry_announce;
    string tts_topic;
    CNHDBAccess *db;
    time_t last_valid_read;
    int read_timeout;

    GateKeeper(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      door_buzzer = get_str_option("gatekeeper", "door_buzzer", "nh/gk/buzzer");
      door_contact = get_str_option("gatekeeper", "door_contact", "nh/gk/contact");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/button");
      rfid = get_str_option("gatekeeper", "rfid", "nh/gk/RFID");
      unlock = get_str_option("gatekeeper", "unlock", "nh/gk/Unlock");
      keypad = get_str_option("gatekeeper", "keypad", "nh/gk/Keypad");
      
      lastman = get_str_option("gatekeeper", "lastman", "nh/gk/LastManState");
      lastman_open = get_str_option("gatekeeper", "lastman_open", "Hackspace now Open!");
      lastman_close = get_str_option("gatekeeper", "lastman_close", "Hackspace is closed");
      twitter_out = get_str_option("gatekeeper", "twitter_out", "nh/twitter/tx/status");
      entry_announce = get_str_option("gatekeeper", "entry_announce", "nh/gk/entry_announce");
      tts_topic = get_str_option("tts", "topic", "nh/tts/gk");
      read_timeout = get_int_option("gatekeeper", "read_timeout", 4);
      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   
      handle = "";
      memset(&last_valid_read, 0, sizeof(last_valid_read));
    }
    
    ~GateKeeper()
    {
      delete db;
    }
    
    void process_message(string topic, string message)
    {
      string unlock_text;
      string err;
      time_t current_time;

      if (topic==door_contact)
      {
        if ((message.substr(0, ((string)("Door Opened by:")).length() ) == "Door Opened by:") && (handle != ""))
        {
          db->sp_log_event("DOOR_OPENED", "");
          
          if (last_seen.length() > 1)
          {
            message_send(entry_announce + "/known", message + " " + handle + " (last seen " + last_seen + " ago)");
          }
          else
          {
            log->dbg("No last seen time set");
            message_send(entry_announce, message + " " + handle);
          }
          handle = "";
          last_seen = "";
        }
        else if (message=="Door Closed")
        {
          log->dbg("Ignoring door closed message");
          db->sp_log_event("DOOR_CLOSED", "");
        }
        else if (message=="Door Time Out")
        {
          handle = "";
          last_seen = "";
          db->sp_log_event("DOOR_TIMEOUT", "");
        }
        else if (message=="Door Opened")
        {
          message_send(entry_announce + "/unknown", "Door opened");
          db->sp_log_event("DOOR_OPENED", "");
        }
        else message_send(entry_announce, message); // Else just pass the message on verbatim
      }   
          
      if (topic==door_button)
        db->sp_log_event("DOORBELL", message);
         
      
      if (topic==lastman)
      { 
        // LWK adding time stamp to tweets
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        
        string tweet;
        char tweet_time [32];
        strftime(tweet_time, 80, " %d/%m %H:%M", timeinfo);
                
        if (message=="Last Out") 
        {
          tweet = lastman_close + tweet_time;
          message_send(twitter_out, tweet);
          message_send(irc_out, lastman_close);
          db->sp_log_event("LAST_OUT", "");
        } else if (message=="First In") 
        {
          tweet = lastman_open + tweet_time;
          message_send(twitter_out, tweet);
          message_send(irc_out, lastman_open);
          db->sp_log_event("FIRST_IN", "");
        }         
      }

      time(&current_time);
      if (topic==rfid)
      {
        if (difftime(current_time, last_valid_read) > read_timeout) // If there's been an unlock message sent in the
        {                                                           // last few seconds, do nothing (door is already open)
          if(db->sp_check_rfid(message, unlock_text, handle, last_seen, err))
          {
            log->dbg("Call to sp_check_rfid failed");
            message_send(unlock, "Access Denied");
          } else
          {
            message_send(unlock, unlock_text); 

            if (unlock_text.substr(0, 7) == "Unlock:")
            {
              time(&last_valid_read); 
              message_send(tts_topic, unlock_text.substr(7, string::npos));
            }
            else
              message_send(tts_topic, unlock_text);
          }
        } else
        {
          log->dbg("Ignoring message: came too soon after previous valid card");
        }
      }

      if (topic==keypad)
      {
        db->sp_check_pin(message, unlock_text, handle, err);
        log->dbg("err = [" + err + "]");        
        message_send(unlock, unlock_text);
      }
        
      CNHmqtt_irc::process_message(topic, message);
  }

  void process_irc_message(irc_msg msg)
  {
         
  }

  int db_connect()
  {
    db->dbConnect();
    return 0;
  }
  
  void setup()
  {
    subscribe(door_contact);
    subscribe(door_button);
    subscribe(rfid);
    subscribe(keypad);
    subscribe(lastman);
  }
};



int main(int argc, char *argv[])
{
 
  string nick;
  string channel;

  string handle="";
  GateKeeper nh = GateKeeper(argc, argv);
   
  nh.db_connect();  

  nh.init();
  nh.setup();
  nh.message_loop(); 
  return 0;
  
}