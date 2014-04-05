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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>

using namespace std;

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

class nh_tools : public CNHmqtt_irc
{
  public:
    string _tool_topic;
    CNHDBAccess *_db;

    nh_tools(int argc, char *argv[]) : CNHmqtt_irc(argc, argv)
    {
      _tool_topic = get_str_option("tools", "_tool_topic", "nh/tools/"); // tool name is appended to this, e.g. laser's topic is nh/tools/laser/
      _db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   
    }
    
    ~nh_tools()
    {
      delete _db;
    }
    
    void process_message(string topic, string message)
    {
      std::vector<string> split_topic;
      
      
      
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

        if (tool_message == "RFID")
        {
          if (_db->sp_tool_sign_on(tool_name, message, access_result, msg))
          {
            message_send(_tool_topic + tool_name + "/DENY", "Failure.");
          } else
          {
            if (access_result)
              message_send(_tool_topic + tool_name + "/GRANT", msg);
            else
              message_send(_tool_topic + tool_name + "/DENY", msg);
          }
        }
      }
        
      CNHmqtt_irc::process_message(topic, message);
    }
 

    void process_irc_message(irc_msg msg)
    {
      log->dbg("Got IRC message: " + (string)msg);
    }

    int db_connect()
    {
      _db->dbConnect();
      return 0;
    }
    
    void setup()
    {
      subscribe(_tool_topic + "#");
    }
    
    /* split - taken from Alec Thomas's answer to http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
    void split(vector<string> &tokens, const string &text, char sep) 
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
};



int main(int argc, char *argv[])
{
  nh_tools nh = nh_tools(argc, argv);
   
  nh.db_connect();  

  nh.init();
  nh.setup();
  nh.message_loop(); 
  return 0;  
}