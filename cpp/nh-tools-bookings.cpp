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
                                     string client_id, string client_secret, string bookings_topic, string push_url, InstCBI *cb)
{
  _log            = log;
  _client_id      = client_id;
  _client_secret  = client_secret;
  _cb             = cb;
  _tool_id        = -1;
  _bookings_topic = bookings_topic;
  _push_url       = push_url;
  _setup_done     = false;
  _got_valid_booking_data = false;

  _db = new CNHDBAccess(db_server, db_username, db_password, db_name, log);

  pthread_mutex_init(&_cal_mutex, NULL);
  pthread_mutex_init(&_chanel_mutex, NULL);
  pthread_cond_init(&_cal_condition_var, NULL);
  pthread_cond_init(&_channel_condition_var, NULL);
  _cal_thread_msg = CAL_MSG_NOTHING;
  _exit_notification_thread = false;
}

nh_tools_bookings::~nh_tools_bookings()
{
  delete _db;

  if (_setup_done)
  {
    // Signal chanThread to stop
    pthread_mutex_lock(&_chanel_mutex);
    _exit_notification_thread = true;
    pthread_cond_signal(&_channel_condition_var);
    pthread_mutex_unlock(&_chanel_mutex);

    // Signal calThread to stop
    pthread_mutex_lock(&_cal_mutex);
    _cal_thread_msg = CAL_MSG_EXIT;
    pthread_cond_signal(&_cal_condition_var);
    pthread_mutex_unlock(&_cal_mutex);

    // Join both threads
    dbg("Join chanThread");
    pthread_join(chanThread, NULL);

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

  /* Start thead that will setup the push notification channel, and renew it when/as nessesary */
  pthread_create(&chanThread, NULL, &nh_tools_bookings::s_notification_channel_thread, this); 


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

void *nh_tools_bookings::s_notification_channel_thread(void *arg)
/* Static function passed to pthread_create that just launches the notification_channel_thread */
{
  nh_tools_bookings *tools;
  tools = (nh_tools_bookings*) arg;
  tools->notification_channels_thread();
  return NULL;
}


void nh_tools_bookings::notification_channels_thread()
/* Thread which is responsible for setting the push notification channel, then 
 * renewing it as approaches its expiry */
{
  time_t expiration_time = 0;
  time_t wait_until = 0;
  struct timespec to;
  char buf[50];
  int err;

  dbg("Entered notification_channels_thread");

  expiration_time = time(NULL)+65; // register push notification channel 5 seconds after first starting

  while (1)
  {
    pthread_mutex_lock(&_chanel_mutex);

    wait_until = expiration_time-60; // renew 60 seconds before expiry.
    to.tv_sec =  wait_until;
    to.tv_nsec = 0; 

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&wait_until));
    dbg("notification_channels_thread> Waiting until: " + (string)buf);


    while (_exit_notification_thread == false) 
    {
      err = pthread_cond_timedwait(&_channel_condition_var, &_chanel_mutex, &to);
      if (err == ETIMEDOUT) 
      {
        dbg("notification_channels_thread> Timeout reached");
        break;
      }
    }

    pthread_mutex_unlock(&_chanel_mutex);
    
    if (_exit_notification_thread)
    {
      dbg("exiting chanThread");
      return;
    }
    else
    {
      if (google_renew_channel(expiration_time)) // Actually do the work to renew the channel...
      {
        strftime(buf, sizeof(buf), "Channel expiration time: %Y-%m-%d %H:%M:%S", localtime(&expiration_time));
        dbg("notification_channels_thread> " + (string)buf);
      } else
      {
        // failed to renew channel - try again in 15 minutes
        expiration_time = time(NULL) + (60*15);
      }
    }
  }

  return;
}

void *nh_tools_bookings::s_cal_thread(void *arg)
/* Static function passed to pthread_create that just launches the cal_thread */
{
  nh_tools_bookings *tools;
  tools = (nh_tools_bookings*) arg;
  tools->cal_thread();
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
/* Get ICAL file from google, then process */
{
  dbrows tool_list;
  CURL *curl;
  string curl_read_buffer;
  int res;

  dbg("Entered get_cal_data");
  _got_valid_booking_data = false;

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools_bookings::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);   // wait a maximum of 20 seconds before giving up

  // Get tool details - only expecting one row
  _db->sp_tool_get_calendars(_tool_id, &tool_list);
  if (tool_list.size() != 1)
  {
    dbg("Unexpected number of tools returned: " + CNHmqtt_irc::itos(tool_list.size()));
    return;
  }

  dbrows::const_iterator iterator = tool_list.begin();
  dbrow row = *iterator;

  string calendar_url = "http://www.google.com/calendar/ical/" + row["tool_calendar"].asStr() + "/public/basic.ics";
  dbg("calendar = " + calendar_url);

  dbg("Processing tool: " + 
      row["tool_id"].asStr()       + "\t" + 
      row["tool_address"].asStr()  + "\t" +
      row["tool_name"].asStr()     + "\t" +
      row["tool_calendar"].asStr() + "\t" +
      row["tool_cal_poll_ival"].asStr());

  _tool_name = row["tool_name"].asStr();
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, calendar_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("cURL perform failed: " + (string)_errorBuffer);
  } else
  {
    dbg("Got calendar data");

    // Extract the booking information from the returned calendar (ical format) data
    _bookings.clear();
    if (process_ical_data(curl_read_buffer) == 0) // Decode ICAL data and store events in _bookings
      _got_valid_booking_data = true;
  }

  curl_easy_cleanup(curl);
  return;
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

size_t nh_tools_bookings::s_curl_write(char *data, size_t size, size_t nmemb, void *p)
/* static callback used by cURL when data is recieved. */
{
  ((string*)p)->append(data, size * nmemb);
  return size*nmemb;
}

int nh_tools_bookings::process_ical_data(string ical_data)
/* Decode the ICAL data passed in, extract the bookings/events and add to _bookings */
{
  evtdata current_event;


  icalcomponent *root = icalparser_parse_string(ical_data.c_str());
  if (!root)
  {
    dbg("Failed to parse ical data!");
    return -1;
  }

  // Search for the next, current and previous events.
  icalcomponent *component = icalcomponent_get_first_component(root, ICAL_VEVENT_COMPONENT);
  while (component)
  {
    // Get summary
    icalproperty *prop_summary = icalcomponent_get_first_property(component, ICAL_SUMMARY_PROPERTY);
    if (prop_summary) 
    {
      icalvalue *value = icalproperty_get_value(prop_summary);
      current_event.full_name = icalvalue_as_ical_string(value);
      //printf("Summary Value: %s\n", icalvalue_as_ical_string(value));
    } else
    {
      dbg("Summary not found!");
      continue;
    }

    // Possibly TODO: extract data from JSON encoded DESCRIPTION: type (normal/induct?), booked (time booking made) and member_id (!)

    // get start time
    icalproperty *prop_start = icalcomponent_get_first_property(component, ICAL_DTSTART_PROPERTY);
    if (prop_start) 
    {
      icalvalue *v = icalproperty_get_value(prop_start);
      struct icaltimetype  dt_start = icalvalue_get_datetime(v); 

      current_event.start_time = icaltime_as_timet(dt_start);

    } else
    {
      dbg("No start time");
      continue;
    }

    // get end time
    icalproperty *prop_end = icalcomponent_get_first_property(component, ICAL_DTEND_PROPERTY);
    if (prop_end) 
    {
      icalvalue *v = icalproperty_get_value(prop_end);
      struct icaltimetype  dt_end = icalvalue_get_datetime(v); 

      current_event.end_time = icaltime_as_timet(dt_end);
    } else
    {
      dbg("No end time");
      continue;
    }

    // To have got this far, all the required data should have been found / extracted, so add to buffer
    _bookings.push_back(current_event);

    component = icalcomponent_get_next_component(root, ICAL_VEVENT_COMPONENT);
  }

  icalcomponent_free(root);

  // Sort events in descening order by start datetime
  sort (_bookings.begin(), _bookings.end(), event_by_start_time_sorter);

  return 0;
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

bool nh_tools_bookings::google_get_auth_token(string& auth_token)
/* Using the refresh token in the database and the client id+secret in the
 * config file, get the auth token.
 * Must only be used from the cal thread (it uses class buffer_errorBuffer) */
{
  string curl_read_buffer;
  string post_fields;
  string refresh_token;
  string identity;
  int res;
  CURL *curl;

  // Get refresh token from database
  if (_db->sp_tool_get_google_info(1, identity, refresh_token))
  {
    dbg("sp_tool_get_google_info failed..");
    return false;
  }

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools_bookings::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up

  post_fields =  "client_id="     + http_escape(curl, _client_id)
              + "&client_secret=" + http_escape(curl, _client_secret)
              + "&refresh_token=" + http_escape(curl, refresh_token)
              + "&grant_type=refresh_token";

  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());
  dbg("get_auth_token> post fields: [" + post_fields + "]");


  string oauth_url = "https://accounts.google.com/o/oauth2/token";

  dbg("get_auth_token> Using URL: [" + oauth_url + "]");
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, oauth_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("get_auth_token> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return false;
  } else
  {
    dbg("get_auth_token> Got data");
  }

  auth_token = extract_value(curl_read_buffer, "access_token");
  curl_easy_cleanup(curl);
 
  if (auth_token.length() > 2)
    return true;
  else
    return false;
}

bool nh_tools_bookings::google_renew_channel(time_t& expiration_time)
/* Renew the push notification channel (get auth token, delete any current 
 * channels, set up new ones, and put the new expiry time in expiration_time.
 */
{
  string auth_token;
  dbrows tool_details;

  // get auth_token
  if (!google_get_auth_token(auth_token))
  {
    dbg("google_renew_channel> Failed to get auth token, unable to renew push notification channels");
    return false;
  }

  _db->sp_tool_get_calendars(_tool_id, &tool_details); 
  if (tool_details.size() != 1)
  {
    dbg("google_renew_channel> Unexpected number of tools returned: " + CNHmqtt_irc::itos(tool_details.size()));
    return false;
  }

  dbrows::const_iterator iterator = tool_details.begin();
  dbrow row = *iterator;

  // Delete any push notification channels that may have previously been set up 
  // (Ideally there should be at most one, unless things havn't gone so well).
  google_delete_channels(_tool_id, auth_token);

  // Set up new channel
  google_add_channel(_tool_id, auth_token, row["tool_calendar"].asStr(), expiration_time);

  return true;
}

bool nh_tools_bookings::google_delete_channels(int tool_id, string auth_token)
/* Delete all recorded push notification channels for tool_id */
{
  dbrows chan_list;

  _db->sp_tool_get_google_channels(tool_id, &chan_list); 
  for (dbrows::const_iterator iterator = chan_list.begin(), end = chan_list.end(); iterator != end; ++iterator) 
  {
    string calendar_url;
    dbrow row = *iterator;

    google_delete_channel(auth_token, row["channel_id"].asStr(), row["resource_id"].asStr());

  }

  return true;
}

bool nh_tools_bookings::google_delete_channel(string auth_token, string channel_id, string resource_id)
{
  string curl_read_buffer;
  string post_fields;
  string refresh_token;
  string identity;
  int res;
  CURL *curl;
  struct curl_slist *list = NULL;

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1);     // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools_bookings::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
  
  // Setup post data
  post_fields = json_encode_id_resourse_id(channel_id, resource_id);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());

  dbg("google_delete_channel> post fields: [" + post_fields + "]");

  // setup http headers
  string auth_header = "Authorization: Bearer " + auth_token;
  list = curl_slist_append(list, auth_header.c_str());
  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  
  string stop_url = "https://www.googleapis.com/calendar/v3/channels/stop";
  dbg("google_delete_channel> Using URL: [" + stop_url + "]");
  
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, stop_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("google_delete_channel> cURL perform failed: " + (string)_errorBuffer);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_HTTP_RETURNED_ERROR)
    {
      // Going to assume that this means we've been able to contact google, but 
      // there was an error deleting the channel (e.g. becasue it's already
      // expired). Therefore, just delete it from the DB so we don't try endlessly
      // to delete it.
      dbg("HTTP error from google... delete record of channel from DB");
      _db->sp_tool_delete_google_channel(channel_id);
    }
    
    return false;
  } else
  {
    dbg("google_delete_channel> Got data: [" + curl_read_buffer + "]");
    _db->sp_tool_delete_google_channel(channel_id);
  }

  curl_slist_free_all(list); // free header list
  curl_easy_cleanup(curl);
 
  return true;
}



bool nh_tools_bookings::google_add_channel(int tool_id, string auth_token, string tool_calendar, time_t& expiration_time)
/* set up push notfication channel for the google calendar linked to tool_id */
{
  string curl_read_buffer;
  string post_fields;
  string refresh_token;
  string identity;
  int res;
  CURL *curl;
  struct curl_slist *list = NULL;
  expiration_time = 0;

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Get "*** longjmp causes uninitialized stack frame ***:" without this.
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools_bookings::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
  
  // Generate channel_id
  uuid_t uuid;
  uuid_generate(uuid);
  char uuid_str[37];
  uuid_unparse_lower(uuid, uuid_str);
  
  // Setup post data
  string channel_token = CNHmqtt_irc::itos(tool_id);
  string channel_id = (string)uuid_str;
  post_fields = json_encode_for_add_chan(channel_id, channel_token, _push_url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());
  dbg("google_add_channel> post fields: [" + post_fields + "]");

  // setup http headers
  string auth_header = "Authorization: Bearer " + auth_token;
  list = curl_slist_append(list, auth_header.c_str());
  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

  string watch_url = "https://www.googleapis.com/calendar/v3/calendars/" + tool_calendar + "/events/watch";
  dbg("google_add_channel> Using URL: [" + watch_url + "]");

  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, watch_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    dbg("google_add_channel> cURL perform failed: " + (string)_errorBuffer);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return false;
  }

  dbg("google_add_channel> Got data: [" + curl_read_buffer + "]");
  string resource_id    = extract_value(curl_read_buffer, "resourceId");
  string expiration_s   = extract_value(curl_read_buffer, "expiration");

  long long int expiration_i = atoll(expiration_s.c_str());
  expiration_i /= 1000;  // Expiration time is a Unix timestamp (in ms), convert to seconds
  _db->sp_tool_set_google_channel(tool_id, resource_id, channel_token, channel_id, expiration_i);
  expiration_time = expiration_i;

  curl_slist_free_all(list); // free header list
  curl_easy_cleanup(curl);

  return true;
}

string nh_tools_bookings::json_encode_id_resourse_id(string channel_id, string resource_id)
/* JSON encode the post data used to delete a notification channel. should look something like:
 *  {"id": "521aebbc-3f9c-11e5-9eaf-3c970e884252", "resourceId": "x8j_XzxxxxxxxxxxxxxxxxxxxxM" }
 */
{
  json_object *j_obj_root       = json_object_new_object();
  json_object *jstr_id          = json_object_new_string(channel_id.c_str() );
  json_object *jstr_resource_id = json_object_new_string(resource_id.c_str());

  json_object_object_add(j_obj_root, "id"        , jstr_id);
  json_object_object_add(j_obj_root, "resourceId", jstr_resource_id);

  string json_encoded = json_object_to_json_string(j_obj_root);
  json_object_put(j_obj_root);

  return json_encoded;
}

string nh_tools_bookings::json_encode_for_add_chan(string channel_id, string token, string url)
/* JSON encode the post data used to add a notification channel. should look something like:
 *  { "id": "cf32051e-4135-11e5-97d6-3c970e884252", "type": "web_hook", "address": "https:\/\/lspace.nottinghack.org.uk\/temp\/google.php", "token": "1" }
 */
{
  json_object *j_obj_root = json_object_new_object();
  json_object *jstr_id    = json_object_new_string(channel_id.c_str() );
  json_object *jstr_type  = json_object_new_string("web_hook");
  json_object *jstr_url   = json_object_new_string(url.c_str());
  json_object *jstr_token = json_object_new_string(token.c_str());

  json_object_object_add(j_obj_root, "id"     , jstr_id);
  json_object_object_add(j_obj_root, "type"   , jstr_type);
  json_object_object_add(j_obj_root, "address", jstr_url);
  json_object_object_add(j_obj_root, "token"  , jstr_token);

  string json_encoded = json_object_to_json_string(j_obj_root);

  json_object_put(j_obj_root);  

  return json_encoded;
}


string nh_tools_bookings::extract_value(string json_in, string param)
/* Extract a value/string from a JSON formatting string. e.g. for JSON string:
 *
 * {"access_token" : "yxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxA","token_type" : "Bearer","expires_in" : 3600}
 * 
 * param = "token_type" would return "Bearer"
 */
{
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    dbg("extract_value> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return "";
  }

  json_object *obj_param = json_object_object_get(jobj, param.c_str());
  if (obj_param == NULL)
  {
    dbg("extract_value> json_object_object_get failed. JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return "";
  }

  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    dbg("extract_value> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return "";
  }

  dbg("extract_value> got [" + param + "] = [" + (string)param_val + "]");
  string result = (string)param_val;

  json_object_put(obj_param);
  json_object_put(jobj);

  return result;
}

string nh_tools_bookings::http_escape(CURL *curl, string parameter)
/* Escape string <parameter> in a way suitable to be used in a URL */
{
  string escaped_str;
  char   *escaped_curl;

  escaped_curl = curl_easy_escape(curl, parameter.c_str(), parameter.length());
  if (escaped_curl == NULL)
  {
    dbg("curl_easy_escape failed");
    return "";
  }
  
  escaped_str = (string)escaped_curl;
  curl_free(escaped_curl);
  return escaped_str;
}

void nh_tools_bookings::dbg(std::string msg)
/* Debuging output using _log object passed in when contructed. All log entries 
 * are prefixed with "[bookings-<tool id>".
 */
{
  _log->dbg("bookings-" + CNHmqtt_irc::itos(_tool_id) , msg);  
}
