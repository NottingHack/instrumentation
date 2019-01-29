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
 * This module deals with getting booking information from the database
 * calendars linked to each tool, and publishing over MQTT for display on 
 * a now/next LED display(s) (only one so far).
 *
 * There should be one instance of this nh_tools_bookings class per tool.
 *
 * This is a little convoluted as it originaly downloaded an ICAL file from
 * google (and received push notifications), but the calendar is now in DB.
 * 
 * There is one additional thread created when setup() is first called:
 *
 *  calThread   - This gets the calendar/booking data for the tool
 *                from the database, determines the current and next booking,
 *                then publishes this information (JSON encoded) over
 *                MQTT.
 *                It will download the calendar:
 *                  - when first started
 *                  - when poll() is called, which it should be when a push
 *                    notfication of calendar change is received from hms2.
 *                    The nh_tools class deals with calling poll() at the
 *                    appropriate time.
 *                  - If it's been more than tl_tools.tool_cal_poll_ival seconds
 *                    since the last download (this should be >= 15 minutes)
 *                It will also send an MQTT message with the now/next data when 
 *                the display needs to be updated (e.g. current booking ends, new
 *                one starts, whatever. The display has no concept of wall time).
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

class nh_tools_bookings
{
  public:
    nh_tools_bookings(CLogging *log, std::string db_server, std::string db_username, std::string db_password, std::string db_name,
                      std::string bookings_topic, InstCBI *cb);
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

    CLogging *_log;

    pthread_t calThread;
    pthread_mutex_t _cal_mutex;
    pthread_cond_t  _cal_condition_var, _channel_condition_var;

    cal_msg _cal_thread_msg;
    bool _exit_notification_thread;
    time_t _next_event;
    int _tool_id;
    std::string _bookings_topic;
    bool _setup_done;
    std::string _tool_name;
    bool _got_valid_booking_data;

    void dbg(std::string msg);

    static void *s_cal_thread(void *arg);

    CNHDBAccess *_db;
    void cal_thread();
    InstCBI *_cb;
    std::vector<evtdata> _bookings;
    int get_now_next_bookings(std::vector<evtdata> const& tool_bookings, evtdata &evt_now, evtdata &evt_next);
    std::string json_encode_booking_data(evtdata event_now, evtdata event_next);
    std::string get_json_encoded_booking_data(evtdata event_now, evtdata event_next);
    std::string json_encode_booking_data(std::string now_time, std::string now_description, std::string next_time, std::string next_description);
    void get_cal_data();
    int publish_now_next_bookings();
    static bool event_by_start_time_sorter(evtdata const& i, evtdata const& j);
    
    int process_bookings_data(dbrows rows);
    
};
