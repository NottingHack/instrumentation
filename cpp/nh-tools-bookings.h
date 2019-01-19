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
 * 
 * This module deals with downloading booking information from the google
 * calendars linked to each tool, and publishing over MQTT for display on 
 * a now/next LED display(s) (only one so far).
 * 
 * There should be one instance of this nh_tools_bookings class per tool that 
 * has now/next publishing switched on (i.e. tl_tools.tool_cal_poll_ival > 0).
 * 
 * There are two additional threads created when setup() is first called:
 * 
 *  calThread   - This downloads the ICAL formatted calendar for the tool
 *                from google, determines the current and next booking, 
 *                then pusblishes this information (JSON encoded) over
 *                MQTT.
 *                It will download the calendar:
 *                  - when first started
 *                  - when poll() is called, which it should be when a push
 *                    notfication of calendar change is received from google.
 *                    The nh_tools class deals with calling poll() at the
 *                    appropriate time.
 *                  - If it's been more than tl_tools.tool_cal_poll_ival seconds
 *                    since the last download (this should be >= 15 minutes)
 *                It will also send an MQTT message with the now/next data when 
 *                the display needs to be updated (e.g. current booking ends, new
 *                one starts, whatever. The display has no concept of wall time).
 * 
 * chanThread -   This thread is responsible for setting up the push notification 
 *                channel so we get a notification when a new booking is made.
 *                More info on this service can be found at:
 *                  https://developers.google.com/google-apps/calendar/v3/push 
 *                Note that this thread has nothing to do with receiving/processing
 *                the push notfications, it only registers us to receive them (and 
 *                keeps us registered).
 * 
 *                Before this will work, a few things need to be in place:
 *                  - The google magic needs to be set up and fully working in HMS
 *                  - client_id & client_secret need to be set in nh-tool.conf
 *                  - The URL that google will call needs to be registered with google:
 *                    https://developers.google.com/google-apps/calendar/v3/push#registering
 *                  - That URL needs to be set as the push_url in nh-tools.conf (TODO)
 *                  (how google calling that URL results in a call to poll() is outside the
 *                  scope of this module).
 * 
 *                Once the push notfication channel has been set up, the thread will wait
 *                until shortly before the channel expiry time, then renew it (i.e. it
 *                could/should be waiting for days).
 *                As there isn't (currently) a way to directly renew a channel, it deletes 
 *                the old one(s), then sets up a new push channel.
 *
 */


#include "CNHmqtt_irc.h"
#include "CNHDBAccess.h"
#include "nh-cbi.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <json-c/json.h>     // libjson-c-dev
#include <curl/curl.h>       // libcurl4-gnutls-dev
#include <libical/ical.h>    // libical-dev 
#include <libical/icalss.h>
#include <uuid/uuid.h>       // uuid-dev


class nh_tools_bookings
{
  public:
    nh_tools_bookings(CLogging *log, std::string db_server, std::string db_username, std::string db_password, std::string db_name,
                      std::string client_id, std::string client_secret, std::string bookings_topic, std::string push_url, InstCBI *cb);
    ~nh_tools_bookings();

    void setup(int tool_id);  // Call once with the tool_id id to publish bookings for
    void poll();              // Trigger a poll for new bookings, then publish the results

  private:
    struct evtdata 
    {
      std::string full_name;
      time_t start_time;
      time_t end_time;
    };

    enum cal_msg 
    {
      CAL_MSG_NOTHING,
      CAL_MSG_POLL,
      CAL_MSG_EXIT
    };

    char _errorBuffer[CURL_ERROR_SIZE];
    CLogging *_log;

    pthread_t calThread;
    pthread_t chanThread;
    pthread_mutex_t _cal_mutex, _chanel_mutex;
    pthread_cond_t  _cal_condition_var, _channel_condition_var;

    cal_msg _cal_thread_msg;
    bool _exit_notification_thread;
    time_t _next_event;
    std::string _client_id;
    std::string _client_secret;
    int _tool_id;
    std::string _bookings_topic;
    bool _setup_done;
    std::string _tool_name;
    std::string _push_url;
    bool _got_valid_booking_data;

    void dbg(std::string msg);

    static void *s_cal_thread(void *arg);
    static void *s_notification_channel_thread(void *arg);
    static size_t s_curl_write(char *data, size_t size, size_t nmemb, void *p);

    int process_ical_data(std::string ical_data);
    CNHDBAccess *_db;
    void cal_thread();
    void notification_channels_thread();
    InstCBI *_cb;
    std::vector<evtdata> _bookings;
    int get_now_next_bookings(std::vector<evtdata> const& tool_bookings, evtdata &evt_now, evtdata &evt_next);
    std::string json_encode_booking_data(evtdata event_now, evtdata event_next);
    std::string get_json_encoded_booking_data(evtdata event_now, evtdata event_next);
    std::string json_encode_booking_data(std::string now_time, std::string now_description, std::string next_time, std::string next_description);
    void get_cal_data();
    int publish_now_next_bookings();
    bool google_get_auth_token(std::string& auth_token);
    std::string http_escape(CURL *curl, std::string parameter);
    std::string extract_value(std::string json_in, std::string param);
    bool google_delete_channels(int tool_id, std::string auth_token);
    bool google_delete_channel(std::string auth_token, std::string channel_id, std::string resource_id);
    bool google_add_channel(int tool_id, std::string auth_token, std::string tool_calendar, time_t& expiration_time);
    bool google_renew_channel(time_t& expiration_time);
    std::string json_encode_id_resourse_id(std::string channel_id, std::string resource_id);
    std::string json_encode_for_add_chan(std::string channel_id, std::string resource_id, std::string url);
    static bool event_by_start_time_sorter(evtdata const& i, evtdata const& j);
};
