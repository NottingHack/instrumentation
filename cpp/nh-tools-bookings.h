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


#include "CNHmqtt_irc.h"
#include "CNHDBAccess.h"
#include "nh-tools-cbi.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <json/json.h>       // libjson0-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev
#include <libical/ical.h>    // libical-dev 
#include <libical/icalss.h>
#include <uuid/uuid.h>       // uuid-dev


class nh_tools_bookings
{
  public:
    nh_tools_bookings(CLogging *log, std::string db_server, std::string db_username, std::string db_password, std::string db_name,
                      std::string client_id, std::string client_secret, std::string tool_topic, ToolsCallbackInterface *cb);
    ~nh_tools_bookings();

    void setup(int tool_id);
    void poll();

  private:
    struct evtdata 
    {
      std::string full_name;
      time_t start_time;
      time_t end_time;
    };

    char _errorBuffer[CURL_ERROR_SIZE];
    CLogging *_log;
    pthread_t calThread;
    pthread_mutex_t _cal_mutex;
    pthread_cond_t  _condition_var;
    bool _do_poll;
    time_t _next_event;
    std::string _client_id;
    std::string _client_secret;
    int _tool_id;
    std::string _tool_topic;
    bool _setup_done;
    void dbg(std::string msg);

    static void *s_cal_thread(void *arg);
    static size_t s_curl_write(char *data, size_t size, size_t nmemb, void *p);

    int process_ical_data(std::string ical_data, int tool_id);
    CNHDBAccess *_db;
    void cal_thread();
    ToolsCallbackInterface *_cb;
    std::map <int, std::vector<evtdata> > _bookings; // bookings - [tool_id][booking]
    int get_now_next_bookings(std::vector<evtdata> const& tool_bookings, evtdata &evt_now, evtdata &evt_next);
    std::string json_encode_booking_data(evtdata event_now, evtdata event_next);
    void get_cal_data();
    int publish_now_next_bookings(int tool_id);
    bool google_get_auth_token(std::string& auth_token);
    std::string http_escape(CURL *curl, std::string parameter);
    std::string extract_value(std::string json_in, std::string param);
    bool google_delete_channels(int tool_id, std::string auth_token);
    bool google_delete_channel(std::string auth_token, std::string channel_id, std::string resource_id);
    bool google_add_channel(int tool_id, std::string auth_token, std::string tool_calendar);
    bool google_renew_channels();
    std::string json_encode_id_resourse_id(std::string channel_id, std::string resource_id);
    std::string json_encode_for_add_chan(std::string channel_id, std::string resource_id, std::string url);
    static bool event_by_start_time_sorter(evtdata const& i, evtdata const& j);
};
