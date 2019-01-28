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

#include <json-c/json.h>     // libjson-c-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 
std::string CNHmqtt::_pid_file = "";

using namespace std;

class nh_trustee : public CNHmqtt
{
  public:
    string entry_announce;
    string door_button;
    string lastman;
    string lastman_open;
    string lastman_close;
    string api_url;
    string slack_channel;
    char _errorBuffer[CURL_ERROR_SIZE];

    nh_trustee(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      entry_announce = get_str_option("gatekeeper", "entry_announce", "nh/gk/entry_announce");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/DoorButton");
      lastman = get_str_option("gatekeeper", "lastman", "nh/gk/LastManState");
      lastman_open = get_str_option("gatekeeper", "lastman_open", "Hackspace now Open!");
      lastman_close = get_str_option("gatekeeper", "lastman_close", "Hackspace is closed");
      api_url = get_str_option("slack", "webhook", "https://hooks.slack.com/services/");
      slack_channel = get_str_option("slack", "trustee_channel", "door-log");

      log->dbg("api_url: " + api_url);
      log->dbg("slack_channel: " + slack_channel);
    }

    ~nh_trustee()
    {

    }

    static size_t s_curl_write(char *data, size_t size, size_t nmemb, void *p)
    /* static callback used by cURL when data is recieved. */
    {
      ((string*)p)->append(data, size * nmemb);
      return size*nmemb;
    }

    int slack_post(string message)
    {
      string body;
      string payload;
      int res;
      CURL *curl;
      struct curl_slist *headers = NULL;

      body = json_encode_slack_message(slack_channel, message);

      // Init cURL
      curl = curl_easy_init();

      headers = curl_slist_append(headers, "Accept: application/json");
      headers = curl_slist_append(headers, "Content-Type: application/json");

      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
      curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
      curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
      curl_easy_setopt(curl, CURLOPT_URL          , api_url.c_str()); // URL for this request
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");  // we are doing a POST
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER   , headers); // set the json headers
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS   , body.c_str()); // body of our post
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, s_curl_write); // callback for response
      curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &payload); // pointer for response storage

      /* Perform the request, res will get the return code */ 
      res = curl_easy_perform(curl);
      if (res != CURLE_OK)
      {
        log->dbg("cURL perform failed: " + (string)_errorBuffer);
        log->dbg("URL: " + api_url);
        log->dbg("body: " + body);
        log->dbg("Slack replied: " + payload);
        curl_easy_cleanup(curl);
        return -1;
      } else
      {
        log->dbg("Sent to slack: " + body);
        log->dbg("Slack replied: " + payload);
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

      } else if (topic == lastman) 
      {
        if (message=="Last Out") 
        {
          slack_post(lastman_close);
        } else if (message=="First In")
        {
          slack_post(lastman_open);
        }

      } else if (topic == door_button)
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
     subscribe(lastman);
      
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
