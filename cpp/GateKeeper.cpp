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
#include "CNHDBAccess.h"
#include "CGatekeeper_door.h"
#include "nh-cbi.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>



using namespace std;

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

class GateKeeper : public CNHmqtt_irc, public InstCBI
{
public:
    string lastman;
    string lastman_open;
    string lastman_close;
    string twitter_out;
    
    string entry_announce;
    string base_topic;
    CNHDBAccess *db;
    
    int read_timeout;
    
    std::map<int,CGatekeeper_door> _doors;

    GateKeeper(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      base_topic = get_str_option("gatekeeper", "base_topic", "nh/gk");
      lastman = get_str_option("gatekeeper", "lastman", "nh/gk/LastManState");
      lastman_open = get_str_option("gatekeeper", "lastman_open", "Hackspace now Open!");
      lastman_close = get_str_option("gatekeeper", "lastman_close", "Hackspace is closed");
      twitter_out = get_str_option("gatekeeper", "twitter_out", "nh/twitter/tx/status");
      entry_announce = get_str_option("gatekeeper", "entry_announce", "nh/gk/entry_announce");
      read_timeout = get_int_option("gatekeeper", "read_timeout", 4);
      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   
    }

    ~GateKeeper()
    {
      delete db;
    }
 
    void process_message(string topic, string message)
    {
      int door_id;
      string command;

      if (!get_door_id_cmd(topic, door_id, command))
      {
        // Message does relate to a specific door
        if (_doors.count(door_id))
        {
          _doors[door_id].process_door_event(command, message);
        }
        else
        {
          log->dbg("Received message from unknown door! ([" + itos(door_id) + "])");
        }
      }

      else if (topic==lastman)
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
          message_send(slack_out, lastman_close);
          db->sp_log_event("LAST_OUT", "");
        } else if (message=="First In") 
        {
          tweet = lastman_open + tweet_time;
          message_send(twitter_out, tweet);
          message_send(irc_out, lastman_open);
          message_send(slack_out, lastman_open);
          db->sp_log_event("FIRST_IN", "");
        }
      }

      CNHmqtt_irc::process_message(topic, message);
  }

  void process_irc_message(irc_msg msg)
  {
    log->dbg("Got IRC message: " + (string)msg);
  }
  
  int get_door_id_cmd(string topic, int &door_id, string &command)
  // If the topic relates to a door-specific messages, returns 0, with door_id and command set.
  // E.g for "nh/gk/1/RFID", returns 0 with door_id=1, command="RFID"
  {
    if (topic.length() > base_topic.length() + 2)
    {
      // If the message relates to a specific door, the part of the topic just after the base_topic 
      // should be the door number; e.g. the topic for an RFID read from door 1 would be:
      //   "nh/gk/1/RFID"
      // We're assuming that if that value is numeric, it relates to a specific door

      unsigned int id_start = base_topic.length() + 1;
      unsigned int id_end   = topic.find_first_of("/", base_topic.length()+1);
      string id_str = topic.substr(id_start, id_end-id_start);

      // Check there's a number where we're looking for door_id
      const char *id_char = id_str.c_str();
      while (*(id_char))
        if (!isdigit(*id_char++))
          return -1; 

      door_id = atoi(id_str.c_str());

      // Sanity check remaining string length, to make sure there is actually a command there
      if (topic.length() < id_end+3)
        return -1;

      command = topic.substr(id_end+1, string::npos);

      return 0;
    }
    return -1;
  }


  int db_connect()
  {
    db->dbConnect();
    return 0;
  }
  
  void setup()
  {
    // Get a list of all doors
    dbrows doors;
    db->sp_gatekeeper_get_doors(-1, &doors);

    for (dbrows::const_iterator iterator = doors.begin(), end = doors.end(); iterator != end; ++iterator) 
    {
      dbrow row = *iterator;
      int door_id = row["door_id"].asInt();

      // log->dbg(row["door_id"].asStr() + "\t" + row["door_short_name"].asStr());
      _doors[door_id].door_short_name = row["door_short_name"].asStr();
      _doors[door_id].set_opts(door_id, base_topic, log, db, this, entry_announce, read_timeout);
    }

    // Subscribe to wildcard MQTT topics for door events
    string subscribe_base = base_topic + "/+/";
    subscribe(subscribe_base + "DoorState");
    subscribe(subscribe_base + "DoorButton");
    subscribe(subscribe_base + "RFID");
    subscribe(subscribe_base + "Keypad");
    subscribe(lastman);
  }
  
  int cbiSendMessage(string topic, string message)
  {
    message_send(topic, message);

    return 0;
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