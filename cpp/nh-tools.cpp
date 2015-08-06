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
  _tool_topic    = get_str_option("tools", "tool_topic"   , "nh/tools/"); // tool name is appended to this, e.g. laser's topic is nh/tools/laser/
  _client_id     = get_str_option("tools", "client_id"    , "<NOT SET>");
  _client_secret = get_str_option("tools", "client_secret", "<NOT SET>");

  _db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);

  pthread_mutex_init(&_cal_mutex, NULL);
  pthread_cond_init(&_condition_var, NULL);
  _do_poll = false;
}

nh_tools::~nh_tools()
{
  delete _db;
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
      {
        // Signal cal_thread to download & process the ical booking data from google
        pthread_mutex_lock(&_cal_mutex);
        _do_poll = true;
        pthread_cond_signal(&_condition_var);
        pthread_mutex_unlock(&_cal_mutex);
      }
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

void nh_tools::setup()
{
  subscribe(_tool_topic + "#");
  
  _do_poll = true;
  
  /* Start thead that will poll for new bookings, and publish to MQTT (if enabled for tool)  *
   * Doing this in a seperate thread so http delays, etc, won't impact on tool sign on times */
  pthread_create(&calThread, NULL, &nh_tools::s_cal_thread, this);
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

void *nh_tools::s_cal_thread(void *arg)
{
    nh_tools *tools;
    tools = (nh_tools*) arg;
    tools->cal_thread();
    return NULL;
}


void nh_tools::cal_thread()
{
  int timeout = (15*60); 
  struct timespec to;
  int err;
  time_t last_poll = 0;
  time_t wait_until;
  char buf[30] = "";
  
  while (1)
  {
    pthread_mutex_lock(&_cal_mutex); 

    wait_until = _next_event;
    if (wait_until > time(NULL) + timeout)
      wait_until = time(NULL) + timeout + 1;

    to.tv_sec = wait_until+2; 
    to.tv_nsec = 0; 

    if (_do_poll == FALSE)
    {
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&wait_until));
      log->dbg("Waiting until: " + (string)buf);
    }

    while (_do_poll == FALSE) 
    {  
      err = pthread_cond_timedwait(&_condition_var, &_cal_mutex, &to); 
      if (err == ETIMEDOUT) 
      { 
        log->dbg("Timeout reached");
        break;
      }
    }

    pthread_mutex_unlock(&_cal_mutex);


    if ((_do_poll) || (time(NULL) - last_poll) > timeout)
    {
      get_cal_data();
      last_poll = time(NULL);
    }
    
    _next_event = time(NULL) + timeout; // publish_now_next_bookings should lower this, if there's change before the timeout 
    publish_now_next_bookings(-1);
    _do_poll = FALSE;
  }
}

void nh_tools::get_cal_data()
{
  dbrows tool_list;
  CURL *curl;
  string curl_read_buffer;
  int res;
  
  
  log->dbg("Entered cal_thread");
  google_renew_channels();
  return; // TODO: Remove me
  
  // TODO: Remove me
  /*
  string auth_token;
  if (google_get_auth_token(auth_token))
  {
    int tool_id = 1;
    google_delete_channels(tool_id, auth_token);
    google_add_channel(tool_id, auth_token);
  }
  */
  
  return; // TODO: Remove me
  
  
  if (_db->dbConnect())
  {
    log->dbg("Error - not connected to DB!");
    return;
  }

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);   // wait a maximum of 20 seconds before giving up

  // Get tools which should have bookings published
  _db->sp_tool_get_calendars(-1, &tool_list); 
  for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator) 
  {
    string calendar_url;
    dbrow row = *iterator;
    int tool_id = row["tool_id"].asInt();

    log->dbg("Processing tool: " + row["tool_id"].asStr() + "\t" + 
         row["tool_address"].asStr() + "\t" +
         row["tool_name"].asStr() + "\t" +
         row["tool_calendar"].asStr() + "\t" +
         row["tool_cal_poll_ival"].asStr());

    calendar_url = "http://www.google.com/calendar/ical/" + row["tool_calendar"].asStr() + "/public/basic.ics";
    //calendar_url = "http://localhost/basic.ics";
    log->dbg("calendar = " + calendar_url);
    curl_read_buffer = "";
    curl_easy_setopt(curl, CURLOPT_URL, calendar_url.c_str());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      log->dbg("cURL perform failed: " + (string)_errorBuffer);
      continue;
    } else
    {
      log->dbg("Got calendar data");
    }

    // Extract the booking information from the returned calendar (ical format) data
    _bookings[tool_id].clear();
    process_ical_data(curl_read_buffer, tool_id);
  }

  curl_easy_cleanup(curl);

  return;
}

int nh_tools::publish_now_next_bookings(int tool_id)
{
  dbrows tool_list;
  evtdata event_now;
  evtdata event_next;
  string booking_info;

  _db->sp_tool_get_calendars(tool_id, &tool_list); 
  for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator)
  {
    dbrow row = *iterator;

    // Find the current/next booking for the tool
    get_now_next_bookings(_bookings[row["tool_id"].asInt()], event_now, event_next);

    // Save the time of the nearest event that will require the display to be updated.
    if ((event_now.end_time > time(NULL)) && (event_now.end_time < _next_event))
      _next_event = event_now.end_time;
    
    if ((event_next.start_time > time(NULL)) && (event_next.start_time < _next_event))
      _next_event = event_next.start_time;
    

    booking_info = json_encode_booking_data(event_now, event_next);
    message_send(_tool_topic + row["tool_name"].asStr() + "/BOOKINGS", booking_info);
  }
  
  return 0;
}

string nh_tools::json_encode_booking_data(evtdata event_now, evtdata event_next)
{
  struct tm *timeinfo;
  char buf[100] = "";
  time_t current_time;  
  
  time(&current_time);
  
  json_object *jstr_now_display_time;
  json_object *jstr_now_display_name;
  json_object *jstr_next_display_time;
  json_object *jstr_next_display_name;
  
  json_object *j_obj_root = json_object_new_object();
  json_object *j_obj_now  = json_object_new_object();
  json_object *j_obj_next = json_object_new_object();
  

  // Current booking details
  if (event_now.start_time > 0)
  {
    jstr_now_display_time = json_object_new_string("now");
    jstr_now_display_name = json_object_new_string(event_now.full_name.c_str());
  } 
  else
  {
    jstr_now_display_time = json_object_new_string("now");
    jstr_now_display_name = json_object_new_string("none");
  }
  json_object_object_add(j_obj_now, "display_time", jstr_now_display_time);
  json_object_object_add(j_obj_now, "display_name", jstr_now_display_name);
  
  
  // Next booking details
  if 
    (
      (event_next.start_time > 0) &&
      ((event_next.start_time - current_time) < (60*60*20)) // To avoid confusion (as only the time, not date, is displayed), only 
    )                                                       // show the next booking if it starts within the next 20 hrs.
  {
    timeinfo = localtime (&event_next.start_time);
    strftime (buf, sizeof(buf), "%R", timeinfo); // HH:MM  
    jstr_next_display_time = json_object_new_string(buf);
    jstr_next_display_name = json_object_new_string(event_next.full_name.c_str()); 
  }
  else
  {
    jstr_next_display_time = json_object_new_string("next");
    jstr_next_display_name = json_object_new_string("none");
  }
  json_object_object_add(j_obj_next, "display_time", jstr_next_display_time);
  json_object_object_add(j_obj_next, "display_name", jstr_next_display_name);  
  
  
  json_object_object_add(j_obj_root, "now",  j_obj_now);
  json_object_object_add(j_obj_root, "next", j_obj_next);
  
  string json_encoded = json_object_to_json_string(j_obj_root);
  
  return json_encoded;
}
    
// static callback used by cURL
size_t nh_tools::s_curl_write(char *data, size_t size, size_t nmemb, void *p)
{
  ((string*)p)->append(data, size * nmemb);
  return size*nmemb;
}

int nh_tools::process_ical_data(string ical_data, int tool_id)
{
  evtdata current_event;


  icalcomponent *root = icalparser_parse_string(ical_data.c_str());
  if (!root)
  {
    log->dbg("Failed to parse ical data!");
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
      log->dbg("Summary not found!");
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
      log->dbg("No start time");
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
      log->dbg("No end time");
      continue;
    }

    // To have got this far, all the required data should have been found / extracted, so add to buffer
    _bookings[tool_id].push_back(current_event);

    component = icalcomponent_get_next_component(root, ICAL_VEVENT_COMPONENT);
  }

  icalcomponent_free(root);

  // Sort events in descening order by start datetime
  sort (_bookings[tool_id].begin(), _bookings[tool_id].end(), event_by_start_time_sorter);

  return 0;
}

int nh_tools::get_now_next_bookings(vector<evtdata> const& tool_bookings, evtdata &evt_now, evtdata &evt_next)
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

// Decending sort by start time
bool nh_tools::event_by_start_time_sorter(evtdata const& i, evtdata const& j)
{
  return i.start_time > j.start_time;
}

/* Must only be used from the cal thread (it uses class buffer_errorBuffer) */
bool nh_tools::google_get_auth_token(string& auth_token)
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
    log->dbg("sp_tool_get_google_info failed..");
    return false;
  }

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
  
  post_fields =  "client_id="     + http_escape(curl, _client_id)
              + "&client_secret=" + http_escape(curl, _client_secret)
              + "&refresh_token=" + http_escape(curl, refresh_token)
              + "&grant_type=refresh_token";

  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());
  log->dbg("get_auth_token> post fields: [" + post_fields + "]");


  string oauth_url = "https://accounts.google.com/o/oauth2/token";
  
  log->dbg("get_auth_token> Using URL: [" + oauth_url + "]");
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, oauth_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    log->dbg("get_auth_token> cURL perform failed: " + (string)_errorBuffer);
    curl_easy_cleanup(curl);
    return false;
  } else
  {
    log->dbg("get_auth_token> Got data");
  }

  auth_token = extract_value(curl_read_buffer, "access_token");
  curl_easy_cleanup(curl);
 
  if (auth_token.length() > 2)
    return true;
  else
    return false;
}

bool nh_tools::google_renew_channels()
{
  string auth_token;
  dbrows tool_list;
  
  // get auth_token
  if (!google_get_auth_token(auth_token))
  {
    log->dbg("Failed to get auth token, unable to renew push notification channels");
    return false;
  }

  _db->sp_tool_get_calendars(-1, &tool_list); 
  for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator)
  {
    dbrow row = *iterator;  
    int tool_id = row["tool_id"].asInt();
    
    // Delete any push notification channels that may have previously been set up
    google_delete_channels(tool_id, auth_token);
    
    // Set up new channel
    google_add_channel(tool_id, auth_token, row["tool_calendar"].asStr());
  }
  
  return true;
}

bool nh_tools::google_delete_channels(int tool_id, string auth_token)
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

bool nh_tools::google_delete_channel(string auth_token, string channel_id, string resource_id)
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
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1);     // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
  
  // Setup post data
  post_fields = json_encode_id_resourse_id(channel_id, resource_id);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());

  log->dbg("google_delete_channel> post fields: [" + post_fields + "]");

  // setup http headers
  string auth_header = "Authorization: Bearer " + auth_token;
  list = curl_slist_append(list, auth_header.c_str());
  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  
  string stop_url = "https://www.googleapis.com/calendar/v3/channels/stop";
  log->dbg("google_delete_channel> Using URL: [" + stop_url + "]");
  
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, stop_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    log->dbg("google_delete_channel> cURL perform failed: " + (string)_errorBuffer);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_HTTP_RETURNED_ERROR)
    {
      // Going to assume that this means we've been able to contact google, but 
      // there was an error deleting the channel (e.g. becasue it's already
      // expired). Therefore, just delete it from the DB so we don't try endlessly
      // to delete it.
      log->dbg("HTTP error from google... delete record of channel from DB");
      _db->sp_tool_delete_google_channel(channel_id);
    }
    
    return false;
  } else
  {
    log->dbg("google_delete_channel> Got data: [" + curl_read_buffer + "]");
    _db->sp_tool_delete_google_channel(channel_id);
  }

  curl_slist_free_all(list); // free header list
  curl_easy_cleanup(curl);
 
  return true;
}



bool nh_tools::google_add_channel(int tool_id, string auth_token, string tool_calendar)
{
  string curl_read_buffer;
  string post_fields;
  string refresh_token;
  string identity;
  int res;
  CURL *curl;
  struct curl_slist *list = NULL;

  //string tool_calendar = "3gq3oi2rgf3831geiv63nldk8s@group.calendar.google.com"; // TODO: get this from DB

  // Init cURL
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER  , _errorBuffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR  , 1); // On error (e.g. 404), we want curl_easy_perform to return an error
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nh_tools::s_curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA    , &curl_read_buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT      , 20L);   // wait a maximum of 20 seconds before giving up
  
  // Generate channel_id
  uuid_t uuid;
  uuid_generate_time_safe(uuid);
  char uuid_str[37];
  uuid_unparse_lower(uuid, uuid_str);
  
  // Setup post data
  string channel_token = CNHmqtt_irc::itos(tool_id);
  string channel_id = (string)uuid_str;
  post_fields = json_encode_for_add_chan(channel_id, channel_token, "https://lspace.nottinghack.org.uk/temp/google.php");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_fields.length());
  curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_fields.c_str());
  log->dbg("google_add_channel> post fields: [" + post_fields + "]");

  // setup http headers
  string auth_header = "Authorization: Bearer " + auth_token;
  list = curl_slist_append(list, auth_header.c_str());
  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  
  string watch_url = "https://www.googleapis.com/calendar/v3/calendars/" + tool_calendar + "/events/watch";
  log->dbg("google_add_channel> Using URL: [" + watch_url + "]");
  
  curl_read_buffer = "";
  curl_easy_setopt(curl, CURLOPT_URL, watch_url.c_str());
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    log->dbg("google_add_channel> cURL perform failed: " + (string)_errorBuffer);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return false;
  } else
  {
    log->dbg("google_add_channel> Got data: [" + curl_read_buffer + "]");
    string resource_id    = extract_value(curl_read_buffer, "resourceId");
    string expiration_s   = extract_value(curl_read_buffer, "expiration");

    long long int expiration_i = atoll(expiration_s.c_str());
    expiration_i /= 1000;  // Expiration time is a Unix timestamp (in ms), convert to seconds
    _db->sp_tool_set_google_channel(tool_id, resource_id, channel_token, channel_id, expiration_i);

  }

  curl_slist_free_all(list); // free header list
  curl_easy_cleanup(curl);
 
  return true;
}

string nh_tools::json_encode_id_resourse_id(string channel_id, string resource_id)
{
  json_object *j_obj_root       = json_object_new_object();
  json_object *jstr_id          = json_object_new_string(channel_id.c_str() );
  json_object *jstr_resource_id = json_object_new_string(resource_id.c_str());

  json_object_object_add(j_obj_root, "id"        , jstr_id);
  json_object_object_add(j_obj_root, "resourceId", jstr_resource_id);

  string json_encoded = json_object_to_json_string(j_obj_root);
// TODO: free?
  return json_encoded;
  
}


string nh_tools::json_encode_for_add_chan(string channel_id, string token, string url)
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
// TODO: free?
  return json_encoded;
}


string nh_tools::extract_value(string json_in, string param)
{
  json_object *jobj = json_tokener_parse(json_in.c_str());
  if (jobj == NULL)
  {
    log->dbg("extract_value> json_tokener_parse failed. JSON data: [" + json_in + "]");
    return "";
  }
  
  json_object *obj_param = json_object_object_get(jobj, param.c_str());
  if (obj_param == NULL)
  {
    log->dbg("extract_value> json_object_object_get failed. JSON data: [" + json_in + "]");
    json_object_put(jobj);
    return "";
  }
  
  const char *param_val = json_object_get_string(obj_param);
  if (param_val == NULL)
  {
    log->dbg("extract_value> json_object_get_string failed. JSON data: [" + json_in + "]");
    json_object_put(obj_param);
    json_object_put(jobj);
    return "";
  }
  
  log->dbg("extract_value> got [" + param + "] = [" + (string)param_val + "]");
  string result = (string)param_val;
  
  json_object_put(obj_param);
  json_object_put(jobj);
  return result;
}

string nh_tools::http_escape(CURL *curl, string parameter)
{
  string escaped_str;
  char   *escaped_curl;

  escaped_curl = curl_easy_escape(curl, parameter.c_str(), parameter.length());
  if (escaped_curl == NULL)
  {
    log->dbg("curl_easy_escape failed");
    return "";
  }
  
  escaped_str = (string)escaped_curl;
  curl_free(escaped_curl);
  return escaped_str;
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
