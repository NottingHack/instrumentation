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


#include "CNHmqtt.h"
#include "SlackRTMCallbackInterface.h"
#include "CSlackRTM.h"

#include <stdio.h>
#include <signal.h>

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

using namespace std;

class nh_slack : public CNHmqtt, public SlackRTMCallbackInterface
{
  
  public:
    string slack_server;
    string slack_token;
    string slack_channel;
    string slack_mqtt_tx;
    string slack_mqtt_rx;
    CSlackRTM *_rtm;
  
    nh_slack(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      slack_server  = get_str_option("slack", "apiurl",  "https://slack.com/api/");
      slack_token   = get_str_option("slack", "token",   "xoxb-11832276226-HiO5bl2qSGxdqAhqM5tca90E");
      slack_mqtt_tx = get_str_option("slack", "mqtt_tx", "nh/slack/tx");
      slack_mqtt_rx = get_str_option("slack", "mqtt_rx", "nh/slack/rx");
      slack_channel = get_str_option("slack", "channel", "irc");
      _rtm = new CSlackRTM(slack_token, slack_server, this);
    }
    
    ~nh_slack() 
    {
      delete _rtm;
    }
   
    int slack_connect()
    {
      if (_mosq == NULL)
      {
        log->dbg ("Must connect to mosquitto before slack");
        return -1;
      }
      
      log->dbg("Connecting to slack...");
      
      _rtm->go();

      subscribe(slack_mqtt_tx);
      subscribe(slack_mqtt_tx + "/#");
      return 0;
    }


  void process_message(string topic, string message)
  {
    string chan;
    
    if ((slack_mqtt_tx.length() <= topic.length()) && (slack_mqtt_tx == topic.substr(0, slack_mqtt_tx.length())))
    {
      // Send to the channel
      if (topic == slack_mqtt_tx)
      {
         _rtm->send(slack_channel, message);
        return;
      }
    
      // remove leading nh/irc/tx/
      chan = topic.substr(slack_mqtt_tx.length()+1);
      if (chan.length() < 2) 
      {
        log->dbg("Ignoring <2 char nick/chan");
        return;
      }

      // Msg specific channel
      _rtm->send(chan, message);
    }

    CNHmqtt::process_message(topic, message);
  }

  void mqtt_disconnect()
  {
    mosquitto_disconnect(_mosq);
  }
  
  int cbi_got_slack_message(string channel, string username, string message)
  {
    cbi_debug_message("cbi_got_slack_message> #" + channel + "/<" + username + "> " + message);

    message_send(slack_mqtt_rx + "/" + channel + "/" + username, message);

    return 0;
  }
  
  void cbi_debug_message(string msg)
  {
    log->dbg("[" + msg + "]");
  }

};

nh_slack *nh;



int main(int argc, char *argv[])
{
  nh = new nh_slack(argc, argv);

 
  nh_slack::daemonize();

  while (nh->mosq_connect())
    sleep(10);

  while (nh->slack_connect())
    sleep(10);

  nh->message_loop();

  delete nh;

  return 0;
}