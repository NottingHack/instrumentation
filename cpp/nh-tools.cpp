/* 
 * Copyright (c) 2015, Daniel Swann <hs@dswann.co.uk>
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

/*
 * For Tools access control (e.g. laser cutter) at Nottingham Hackspace
 */


#include "nh-tools.h"

using namespace std;

nh_tools::nh_tools(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
{
  _tool_topic     = get_str_option("tools", "tool_topic"   , "nh/tools/"); // tool name is appended to this, e.g. laser's topic is nh/tools/laser/
  _client_id      = get_str_option("tools", "client_id"    , "<NOT SET>");
  _client_secret  = get_str_option("tools", "client_secret", "<NOT SET>");
  _bookings_topic = get_str_option("tools", "bookings_topic", "nh/bookings/");
  _push_url       = get_str_option("tools", "push_url"     , "https://lspace.nottinghack.org.uk/temp/google.php");

  _db_server   = get_str_option("mysql", "server"  , "localhost");
  _db_username = get_str_option("mysql", "username", "gatekeeper");
  _db_password = get_str_option("mysql", "password", "gk");
  _db_name     = get_str_option("mysql", "database", "gk");

  _db = new CNHDBAccess(_db_server, _db_username, _db_password, _db_name, log);

  _setup_done = false;
   _bookings_log = NULL;
}

nh_tools::~nh_tools()
{
  delete _db;

  for (std::map<string,nh_tools_bookings*>::iterator it = _bookings.begin(); it != _bookings.end(); ++it)
    delete it->second;
  _bookings.clear();
  
  if (_bookings_log)
    delete _bookings_log;
}

void nh_tools::process_message(string topic, string message)
{
  std::vector<string> split_topic;
  int member_id = 0;
  string disp_msg;

  // E.g. "nh/tools/laser/RFID"

  // Look for a message to the _tool_topic
  if (topic.substr(0, _tool_topic.length()) == _tool_topic)
  {
    string tool_name, tool_message, msg;
    int access_result = 0;
    split(split_topic, topic.substr(_tool_topic.length(), string::npos), '/');
    if (split_topic.size() != 2)
    {
      log->dbg("invalid topic");
      return;
    }

    tool_name    = split_topic[0];
    tool_message = split_topic[1];

    if (tool_message == "AUTH")
    {
      if (_db->sp_tool_sign_on(tool_name, message, access_result, msg, member_id))
      {
        message_send(_tool_topic + tool_name + "/DENY", "Failure.");
      } else
      {
        if (access_result)
        {
          // Access granted
          _db->sp_tool_pledged_remain(tool_name, member_id, disp_msg);
          message_send(_tool_topic + tool_name + "/GRANT", msg + disp_msg);
        }
        else
        {
          message_send(_tool_topic + tool_name + "/DENY", msg);
        }
      }
    } else if (tool_message == "COMPLETE")
    {
      if (_db->sp_tool_sign_off(tool_name, atoi(tool_message.c_str()), msg))
      {
        log->dbg("sp_tool_sign_off failed...");
      } else if (msg.length() > 0)
      {
        log->dbg("sp_tool_sign_off: " + msg);
      }
    } else if (tool_message == "RESET")
    {
      // Device has either just been powered up, or has reconnected and isn't in use - so make sure it's signed off
      if ((message == "BOOT") || (message == "IDLE"))
      {
        if (_db->sp_tool_sign_off(tool_name, atoi(tool_message.c_str()), msg))
        {
          log->dbg("sp_tool_sign_off failed...");
        } else if (msg.length() > 0)
        {
          log->dbg("sp_tool_sign_off: " + msg);
        }
      }
    }
    else if (tool_message == "INDUCT")
    {
      // Induct button has been pushed, and a new card presented
      string card_inductor;
      string card_inductee;
      string err;
      size_t pos;
      int ret=1;
      
      pos = message.find_first_of(":");
      if (pos == string::npos)
      {
        log->dbg("Invalid induct message");
      } else
      {
        card_inductor = message.substr(0, pos);
        card_inductee = message.substr(pos+1, string::npos);

        log->dbg("card_inductor=" + card_inductor + ", card_inductee=" + card_inductee);
        if (_db->sp_tool_induct(tool_name, card_inductor, card_inductee, ret, err))
        {
          log->dbg("sp_tool_induct failed...");
          ret = 1;
        } else if (msg.length() > 0)
        {
          log->dbg("sp_tool_induct: " + msg);
        }

        if (ret)
          message_send(_tool_topic + tool_name + "/IFAL", err); // Induct FAiLed
        else
          message_send(_tool_topic + tool_name + "/ISUC", err); // Induct SUCcess
      }

    }
    else if (tool_message == "BOOKINGS")
    {
      if (message == "POLL")
        if (_bookings.count(tool_name) == 1)
          _bookings[tool_name]->poll();
    }
  } // End of _tools_topic processing
  
  else if (topic == _bookings_topic + "poll")
  {
    // Message to the tools booking/poll topic. Get the tool name from the message
    // payload, then send update now/next booking data for that tool
    if (message.length() > 0)
    {
      if (_bookings.count(message) == 1)
        _bookings[message]->poll();
      else
        log->dbg("Unknown tool: " + message);
    }
  }

  CNHmqtt_irc::process_message(topic, message);
}


void nh_tools::process_irc_message(irc_msg msg)
{
  log->dbg("Got IRC message: " + (string)msg);
}

int nh_tools::db_connect()
{
  _db->dbConnect();
  return 0;
}

int nh_tools::cbiSendMessage(string topic, string message)
{
  message_send(topic, message, false, true); // send retained messages from tools bookings logic

  return 0;
}

void nh_tools::setup()
{
  if (_setup_done)
    return;
  
  string bookings_logfile = get_str_option("mqtt", "bookings_logfile", "");

  subscribe(_tool_topic + "#");
  subscribe(_bookings_topic + "poll");
  
  // The bookings logic writes a fair bit to the log file. Put this in a different 
  // file to seperate it out from the normal tools signon/signoff stuff.
  if (!debug_mode && (bookings_logfile != ""))
  {
    _bookings_log = new CLogging();
    if (!_bookings_log->open_logfile(bookings_logfile))
    {
      log->dbg("failed to open bookings logfile. Using the normal logfile for bookings logic");
      delete _bookings_log;
      _bookings_log = NULL;
    }
  }

  // For each tool that has booking notifications enabled, create an nh_tools_bookings object
  // to manage the sending of booking info periodically over MQTT.
  dbrows tool_list;
  _db->sp_tool_get_calendars(-1, &tool_list); 
  for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator)
  {
    dbrow row = *iterator;
    log->dbg("Creating booking object for tool [" + row["tool_name"].asStr() + "], id = [" +  row["tool_id"].asStr() + "]");
    _bookings[row["tool_name"].asStr()] = new nh_tools_bookings((_bookings_log ? _bookings_log : log), _db_server, _db_username, _db_password, _db_name,_client_id, _client_secret, _bookings_topic, _push_url, this);
    _bookings[row["tool_name"].asStr()]->setup(row["tool_id"].asInt());
  }

  _setup_done = true;
}

/* split - taken from Alec Thomas's answer to http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
void nh_tools::split(vector<string> &tokens, const string &text, char sep) 
{
  int start = 0;
  size_t end = 0;
  while ((end = text.find(sep, start)) != string::npos) 
  {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
}



int main(int argc, char *argv[])
{
  curl_global_init(CURL_GLOBAL_DEFAULT);

  nh_tools nh = nh_tools(argc, argv);

  nh.db_connect();

  nh.init();
  nh.setup();
  nh.message_loop();

  return 0;  
}
