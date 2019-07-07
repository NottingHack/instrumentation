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
 * See description of this module in nh-tools-bookings.h
 */


#include "nh-tools-bookings.h"

using namespace std;

nh_tools_bookings::nh_tools_bookings(CLogging *log, string db_server, string db_username, string db_password, string db_name,
                                     string bookings_topic, InstCBI *cb)
{
  _log            = log;
  _cb             = cb;
  _tool_id        = -1;
  _bookings_topic = bookings_topic;
  _setup_done     = false;
  _got_valid_booking_data = false;

  _db = new CNHDBAccess(db_server, db_username, db_password, db_name, log);

  pthread_mutex_init(&_cal_mutex, NULL);
  pthread_cond_init(&_cal_condition_var, NULL);
  _cal_thread_msg = CAL_MSG_NOTHING;
  _exit_notification_thread = false;
}

nh_tools_bookings::~nh_tools_bookings()
{
  delete _db;

  if (_setup_done)
  {
    // Signal calThread to stop
    pthread_mutex_lock(&_cal_mutex);
    _cal_thread_msg = CAL_MSG_EXIT;
    pthread_cond_signal(&_cal_condition_var);
    pthread_mutex_unlock(&_cal_mutex);

    // Join thread
    dbg("Join calThread");
    pthread_join(calThread, NULL);
  }
  dbg("Done - Bookings exit...");
}

void nh_tools_bookings::setup(int tool_id)
/* Connect to database, start the threads that will poll for new bookings and maintain
 * the push notification channel (renewing when nessesary), then return */
{
  if (_setup_done)
    return;

  if (_db->dbConnect())
    return;

  _tool_id = tool_id;
  _cal_thread_msg = CAL_MSG_POLL; // Want the calendar thread to poll for an update as soon as it starts

 /* Start thead that will poll for new bookings, and publish to MQTT (if enabled for tool)  *
  * Doing this in a seperate thread so http delays, etc, won't impact on tool sign on times */
  pthread_create(&calThread, NULL, &nh_tools_bookings::s_cal_thread, this);
  _setup_done = true;
}

void nh_tools_bookings::poll()
/* Signal the calendar thread to re-download/process/publish calendar data.
 * This function should be called when a push notification for the tool is 
 * recieved, and the display first starting up */
{
  if (!_setup_done)
  {
    dbg("Poll() called before setup done!");
    return;
  }

  if (_cal_thread_msg == CAL_MSG_EXIT)
  {
    dbg("Poll() called with pending exit. Ignoring.");
    return;
  }

  // Signal cal_thread to download & process the ical booking data from google
  pthread_mutex_lock(&_cal_mutex);
  _cal_thread_msg = CAL_MSG_POLL;
  pthread_cond_signal(&_cal_condition_var);
  pthread_mutex_unlock(&_cal_mutex);
}

void *nh_tools_bookings::s_cal_thread(void *arg)
/* Static function passed to pthread_create that just launches the cal_thread */
{
  nh_tools_bookings *tools;
  tools = (nh_tools_bookings*) arg;
  mysql_thread_init();
  tools->cal_thread();
  mysql_thread_end();
  return NULL;
}


void nh_tools_bookings::cal_thread()
/* Main cal_thread entry point. This thread polls google for updated calendar data
 * either every 15minutes, or when signalled to by the poll() function.
 * It also publishes the now/next bookings using mqtt either after a poll, or when
 * it changes (i.e. current booking finishes, etc). */
{
  int timeout = (15*60); 
  struct timespec to;
  int err;
  time_t last_poll = 0;
  time_t wait_until;
  char buf[30] = "";
  cal_msg msg;

  dbg("Entered cal_thread");

  while (1)
  {
    pthread_mutex_lock(&_cal_mutex); 

    wait_until = _next_event;
    if (wait_until > time(NULL) + timeout)
      wait_until = time(NULL) + timeout + 1;

    to.tv_sec = wait_until+2; 
    to.tv_nsec = 0; 

    if (_cal_thread_msg == CAL_MSG_NOTHING)
    {
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&wait_until));
      dbg("cal_thread> Waiting until: " + (string)buf);
    }

    while (_cal_thread_msg == CAL_MSG_NOTHING) 
    {  
      err = pthread_cond_timedwait(&_cal_condition_var, &_cal_mutex, &to); 
      if (err == ETIMEDOUT) 
      {  
        dbg("cal_thread> Timeout reached");
        break;
      }
    }
    msg = _cal_thread_msg;
    if (_cal_thread_msg != CAL_MSG_EXIT)
      _cal_thread_msg = CAL_MSG_NOTHING;

    pthread_mutex_unlock(&_cal_mutex);

    if (msg == CAL_MSG_EXIT)
    {
      dbg("exiting cal_thread");
      return;
    }

    if ((msg == CAL_MSG_POLL) || (time(NULL) - last_poll) > timeout)
    {
      get_cal_data();           // Get the ICAL file from google & process it
      last_poll = time(NULL);
    }

    _next_event = time(NULL) + timeout; // publish_now_next_bookings should lower this, if there's change before the timeout 
    publish_now_next_bookings(); // using the downloaded & processed ICAL data, publish now & next data over MQTT
  }
}

void nh_tools_bookings::get_cal_data()
/* Get calandar from database, then process */
{
  dbrows tool_list;

  dbg("Entered get_cal_data");
  _got_valid_booking_data = false;

  // Get tool details - only expecting one row
  _db->sp_tool_get_calendars(_tool_id, &tool_list);
  if (tool_list.size() != 1)
  {
    dbg("Unexpected number of tools returned: " + CNHmqtt_irc::itos(tool_list.size()));
    return;
  }

  dbrows::const_iterator iterator = tool_list.begin();
  dbrow row = *iterator;

  dbg("Processing tool: " +
      row["tool_id"].asStr() + "\t" +
      row["tool_name"].asStr());

  _tool_name = row["tool_name"].asStr();

  // Get bookings
  dbrows db_bookings;
  if(_db->sp_tool_get_bookings(_tool_id, &db_bookings))
  {
    dbg("Failed to get bookings from DB");
  } else
  {
    dbg("Got calendar data");

    // Extract the booking information from the returned calendar data
    _bookings.clear();
    if (process_bookings_data(db_bookings) == 0)
      _got_valid_booking_data = true;
  }

  return;
}

int nh_tools_bookings::process_bookings_data(dbrows rows)
{
  for (dbrows::const_iterator iterator = rows.begin(), end = rows.end(); iterator != end; ++iterator) 
  {
    dbrow row = *iterator;
    evtdata current_event;

    current_event.full_name  = row["username"].asStr();
    current_event.start_time = row["start"].asTime();
    current_event.end_time   = row["end"].asTime();

    // Add to buffer
    _bookings.push_back(current_event);
  }

  // Sort events in descening order by start datetime
  sort (_bookings.begin(), _bookings.end(), event_by_start_time_sorter);

  return 0;
}

int nh_tools_bookings::publish_now_next_bookings()
/* Using the sorted (furthest first) list of bookings in _bookings, push the current and 
 * next booking (for just "none" if there isn't one */
{
  dbrows tool_list;
  evtdata event_now;
  evtdata event_next;
  string booking_info;

  // Find the current/next booking for the tool
  get_now_next_bookings(_bookings, event_now, event_next);

  // Save the time of the nearest event that will require the display to be updated.
  if ((event_now.end_time > time(NULL)) && (event_now.end_time < _next_event))
    _next_event = event_now.end_time;

  if ((event_next.start_time > time(NULL)) && (event_next.start_time < _next_event))
    _next_event = event_next.start_time;


  booking_info = get_json_encoded_booking_data(event_now, event_next);


  if (!_got_valid_booking_data)
    booking_info = json_encode_booking_data("Now", "<Unable to get booking information>", "Next", "");

  // Use the callback function passes in when consturcted to send the now/next
  // booking data over MQTT.
  _cb->cbiSendMessage(_bookings_topic + _tool_name + "/nownext", booking_info);

  return 0;
}

string nh_tools_bookings::get_json_encoded_booking_data(evtdata event_now, evtdata event_next)
/* JSON encode the now/next booking data */
{
  string now_time;
  string now_description;
  string next_time;
  string next_description;

  struct tm *timeinfo;
  char buf[100] = "";
  time_t current_time;  

  time(&current_time);


  // Current booking details
  if (event_now.start_time > 0)
  {
    now_time = "now";
    now_description = event_now.full_name;
  } 
  else
  {
    now_time        = "now";
    now_description = "none";
  }

  // Next booking details
  if 
    (
      (event_next.start_time > 0) &&
      ((event_next.start_time - current_time) < (60*60*20)) // To avoid confusion (as only the time, not date, is displayed), only 
    )                                                       // show the next booking if it starts within the next 20 hrs.
  {
    timeinfo = localtime (&event_next.start_time);
    strftime (buf, sizeof(buf), "%R", timeinfo); // HH:MM
    next_time = buf;
    next_description = event_next.full_name; 
  }
  else
  {
    next_time = "next";
    next_description = "none";
  }

  return json_encode_booking_data(now_time, now_description, next_time, next_description);
}


string nh_tools_bookings::json_encode_booking_data(string now_time, string now_description, string next_time, string next_description)
/* JSON encode the now/next booking data. Generated string should look somthing like:
 * { "now": { "display_time": "now", "display_name": "none" }, "next": { "display_time": "12:00", "display_name": "admin user" } 
 */
{
  json_object *jstr_now_display_time;
  json_object *jstr_now_display_name;
  json_object *jstr_next_display_time;
  json_object *jstr_next_display_name;

  json_object *j_obj_root = json_object_new_object();
  json_object *j_obj_now  = json_object_new_object();
  json_object *j_obj_next = json_object_new_object();

  jstr_now_display_time = json_object_new_string(now_time.c_str());
  jstr_now_display_name = json_object_new_string(now_description.c_str());
  
  json_object_object_add(j_obj_now, "display_time", jstr_now_display_time);
  json_object_object_add(j_obj_now, "display_name", jstr_now_display_name);


  // Next booking details
  jstr_next_display_time = json_object_new_string(next_time.c_str());
  jstr_next_display_name = json_object_new_string(next_description.c_str());

  json_object_object_add(j_obj_next, "display_time", jstr_next_display_time);
  json_object_object_add(j_obj_next, "display_name", jstr_next_display_name);


  json_object_object_add(j_obj_root, "now",  j_obj_now);
  json_object_object_add(j_obj_root, "next", j_obj_next);

  string json_encoded = json_object_to_json_string(j_obj_root);
  json_object_put(j_obj_root);

  return json_encoded;
}


int nh_tools_bookings::get_now_next_bookings(vector<evtdata> const& tool_bookings, evtdata &evt_now, evtdata &evt_next)
/* Get the current and next booking from tool_bookings. Assumes that <tool_bookings> has already
 * been sorted by start time decending */
{
  time_t current_time;
  int event_now_idx = -1;
  int event_next_idx = -1;

  time(&current_time);
  
  for (unsigned int j=0; j < tool_bookings.size(); j++) 
  {
    // Check if this is the current booking
    if (
         (tool_bookings[j].start_time <= current_time) &&
         (current_time   <  tool_bookings[j].end_time)
       )
    {
      event_now_idx = (int)j;
    }

    // Get next booking. Note: event_buffer is sorted decending by start time, so
    // event_next should end up holding the nearest event that's in the future.
    if ((tool_bookings[j].start_time > current_time) && ((int)j != event_now_idx))
      event_next_idx = j;

    // Not interested in past bookings
    if (tool_bookings[j].start_time < current_time)
      break;
  }

  if (event_next_idx >= 0)
    evt_next = tool_bookings[event_next_idx];
  else
    evt_next.start_time = 0;

  if (event_now_idx >= 0)
     evt_now = tool_bookings[event_now_idx];
  else
    evt_now.start_time = 0;

  return 0;
}


bool nh_tools_bookings::event_by_start_time_sorter(evtdata const& i, evtdata const& j)
/* Decending sort by start time */
{
  return i.start_time > j.start_time;
}

void nh_tools_bookings::dbg(std::string msg)
/* Debuging output using _log object passed in when contructed. All log entries 
 * are prefixed with "[bookings-<tool id>".
 */
{
  _log->dbg("bookings-" + CNHmqtt_irc::itos(_tool_id) , msg);  
}
