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
    string door_bell_msg;
    string handle;
    string lastman;
    string lastman_open;
    string lastman_close;
    string twitter_out;
    string last_seen;
    int bell_duration;
    CNHDBAccess *db;

    GateKeeper(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      door_buzzer = get_str_option("gatekeeper", "door_buzzer", "nh/gk/buzzer");
      bell_duration = get_int_option("gatekeeper", "bell_duration", 100);
      door_contact = get_str_option("gatekeeper", "door_contact", "nh/gk/contact");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/button");
      rfid = get_str_option("gatekeeper", "rfid", "nh/gk/RFID");
      unlock = get_str_option("gatekeeper", "unlock", "nh/gk/Unlock");
      keypad = get_str_option("gatekeeper", "keypad", "nh/gk/Keypad");
      door_bell_msg = get_str_option("gatekeeper", "door_bell_msg", "Door Bell");
      
      lastman = get_str_option("gatekeeper", "lastman", "nh/gk/LastManState");
      lastman_open = get_str_option("gatekeeper", "lastman_open", "Hackspace now Open!");
      lastman_close = get_str_option("gatekeeper", "lastman_close", "Hackspace is closed");
      twitter_out = get_str_option("gatekeeper", "twitter_out", "nh/twitter/tx/status");

      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   
      handle = "";
    }
    
    ~GateKeeper()
    {
      delete db;
    }
    
    void process_message(string topic, string message)
    {
      pthread_attr_t tattr;
      pthread_t bell_thread;
      string unlock_text;
      string err;

      if (topic==door_contact)
      {
        if (message=="LOW")
          message_send(irc_out, "Door opened");
        else if (message=="HIGH")
          message_send(irc_out, "Door closed");
        else if ((message.substr(0, ((string)("Door Opened by:")).length() ) == "Door Opened by:") && (handle != ""))
        {
          if (last_seen.length() > 1)
            message_send(irc_out, message + " " + handle + " (last seen " + last_seen + " ago)");
          else
          {
            log->dbg("No last seen time set");
            message_send(irc_out, message + " " + handle);
          }
          handle = "";
          last_seen = "";
        }
        else if (message=="Door Closed")
        {
          log->dbg("Ignoring door closed message");
        }
        else if (message=="Door Time Out")
        {
          handle = "";
          last_seen = "";
        }
        else message_send(irc_out, message); // Else just pass the message on verbatim (probably "door opened" or "door closed")
      }   
          
      if (topic==door_button)
      {
        // v1 - actaully ring the bell
        if (message=="HIGH")
          message_send(door_buzzer, "HIGH");
        else if (message=="LOW")
          message_send(door_buzzer, "LOW");
        
       // v2 - output to irc
       if (message=="BING")
       {
         message_send(irc_out, door_bell_msg);
         
         // Now ring the bell
         pthread_attr_init(&tattr);
         pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
         pthread_create(&bell_thread, &tattr, &ring_bell, this);         
       }
      }          
      
      if (topic==lastman)
      { 
        if (message=="Last Out") {
          message_send(twitter_out, lastman_close);
          message_send(irc_out, lastman_close);
        } else if (message=="First In") {
          message_send(twitter_out, lastman_open);
          message_send(irc_out, lastman_open);
        }         
      }
      
      if (topic==rfid)
      {
        if(db->sp_check_rfid(message, unlock_text, handle, last_seen, err))
        {
          log->dbg("Call to sp_check_rfid failed");
           message_send(unlock, "Access Denied");        
        } else
        {
          message_send(unlock, unlock_text); 
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
    pthread_attr_t tattr;
    pthread_t bell_thread;

    if (msg=="!help")
    {
      msg.reply("!bell - Ring doorbell in hackspace");
    }
          
    if (msg=="!bell")
    {
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
      pthread_create(&bell_thread, &tattr, &ring_bell, this);
    }         
  }

  int db_connect()
  {
    db->dbConnect();
    return 0;
  }

  static void *ring_bell(void *arg)
  {
    GateKeeper *gk;
    int n;
      
    gk = (GateKeeper*) arg;
    
    // Ring door bell
    gk->message_send(gk->door_buzzer, "HIGH");
          
    for (n=0; n < (gk->bell_duration); n++)
      usleep(1000); // 1ms
          
    gk->message_send(gk->door_buzzer, "LOW");
    
    return NULL;
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