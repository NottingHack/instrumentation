/*
 * Copyright (c) 2018, Matt Lloyd <dps.lwk@gmail.com>
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

#include <stdio.h>
#include <signal.h>

#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

using namespace std;

class nh_trustee : public CNHmqtt
{
  public:
    string entry_announce;
    string door_button;
    string api_url;
    string slack_channel;

    nh_trustee(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      entry_announce = get_str_option("gatekeeper", "entry_announce", "nh/gk/entry_announce/");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/DoorButton");
      api_url = get_str_option("slack", "webhook", "https://hooks.slack.com/services/");
      slack_channel = get_str_option("slack", "channel", "#door-log");
    }

    ~nh_trustee()
    {

    }

    void slack_post(string message)
    {
      string payload;
      int res;
      CURL *curl;
      struct curl_slist *headers = NULL;

      payload = json_encode_slack_message(slack_channel, message);

      // $ch = curl_init($url);
      // $jsonData = array('text' => $user.": ".$msg);
      // $jsonDataEncoded = json_encode($jsonData);
      // curl_setopt($ch, CURLOPT_POST, 1);
      // curl_setopt($ch, CURLOPT_POSTFIELDS, $jsonDataEncoded);
      // curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json')); 
      // $result = curl_exec($ch);

      // Init cURL
      curl = curl_easy_init();
      // curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
      // curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
      // curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
      // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s_curl_write);
      // curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &payload);
      // curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
      
      headers = curl_slist_append(headers, "Accept: application/json");
      headers = curl_slist_append(headers, "Content-Type: application/json");

      curl_easy_setopt(curl, CURLOPT_URL, api_url);
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

      /* Perform the request, res will get the return code */ 
      res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
        log->dbg("cURL perform failed: " + (string)_errorBuffer);
        curl_easy_cleanup(curl);
        return -1;
      } else
      {
        log->dbg("Got data");
      }

    // dbg(LOG_DEBUG, "Got: " + payload);
      curl_easy_cleanup(curl);

      return 0;
    }

    string json_encode_slack_message(string channel, string text)
    {

      json_object *j_obj_root   = json_object_new_object();
      json_object *jstr_channel = json_object_new_string(channel.c_str());
      json_object *jstr_text    = json_object_new_string(text.c_str());

      json_object_object_add(j_obj_root, "channel", jstr_channel);
      json_object_object_add(j_obj_root, "text"   , jstr_text);

      string json_encoded = json_object_to_json_string(j_obj_root);

      json_object_put(j_obj_root);

      return json_encoded;
    }

    void process_message(string topic, string message)
    {
      // Entry annouce is Door opened / Door opened by: etc
      if ((topic.substr(0, entry_announce.length() ) == entry_announce))
      {
        slack_post(message);
      }

      if (topic == door_button)
      {
        string tmp;
        tmp = message;
        for (int c = 0; message[c]; c++)
          tmp[c] = tolower(message[c]);
        
        string msg_to_send = "Door Bell (" + tmp +")";
        slack_post(msg_to_send);
      }

      CNHmqtt::process_message(topic, message);
    }

    void mqtt_disconnect()
    {
      mosquitto_disconnect(_mosq);
    }

    bool setup()
    {
     subscribe(entry_announce + "/#");
     subscribe(door_button);
      
      return true;
    }
};

nh_trustee *nh;

int main(int argc, char *argv[])
{
  nh = new nh_trustee(argc, argv);

 
  nh_trustee::daemonize();

  while (nh->mosq_connect())
    sleep(10);
  
  nh->setup();

  nh->message_loop();

  delete nh;

  return 0;
}