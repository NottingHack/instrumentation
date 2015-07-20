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
      _tool_topic = get_str_option("tools", "_tool_topic", "nh/tools/"); // tool name is appended to this, e.g. laser's topic is nh/tools/laser/
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
    
      while (1)
      {
        
        pthread_mutex_lock(&_cal_mutex); 
        to.tv_sec = time(NULL) + timeout; 
        to.tv_nsec = 0; 

        while (_do_poll == FALSE) 
        {  
          err = pthread_cond_timedwait(&_condition_var, &_cal_mutex, &to); 
          if (err == ETIMEDOUT) 
          { 
            log->dbg("Timeout");
            _do_poll = TRUE;
          }
        }
        _do_poll = FALSE;
        pthread_mutex_unlock(&_cal_mutex);


        get_publish_cal_data();
      }
    }
    
    
    void nh_tools::get_publish_cal_data()
    {
      dbrows tool_list;
      CURL *curl;
      string curl_read_buffer;
      string booking_info;
      int res;
      
      
      log->dbg("Entered cal_thread");
      
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

      // Get tools which should have bookings published
      _db->sp_tool_get_calendars(-1, &tool_list); 
      for (dbrows::const_iterator iterator = tool_list.begin(), end = tool_list.end(); iterator != end; ++iterator) 
      {
        string calendar_url;
        dbrow row = *iterator;
        evtdata event_now;
        evtdata event_next;
      
        log->dbg("Processing tool: " + row["tool_id"].asStr() + "\t" + 
             row["tool_address"].asStr() + "\t" +
             row["tool_name"].asStr() + "\t" +
             row["tool_calendar"].asStr() + "\t" +
             row["tool_cal_poll_ival"].asStr());
        
        calendar_url = "http://www.google.com/calendar/ical/" + row["tool_calendar"].asStr() + "/public/basic.ics";
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
        
        // Extract the current/next bookings from the returned calendar (ical format) data
        process_ical_data(curl_read_buffer, event_now, event_next);

        booking_info = json_encode_booking_data(event_now, event_next);
        message_send(_tool_topic + row["tool_name"].asStr() + "/BOOKINGS", booking_info);

      }
      
      curl_easy_cleanup(curl);
      
      
      return;
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

int nh_tools::process_ical_data(string ical_data, evtdata &evt_now, evtdata &evt_next)
{
  vector<evtdata> event_buffer;
  evtdata current_event;
  time_t current_time;
  int event_now_idx = -1;
  int event_next_idx = -1;
  
  time(&current_time);
  
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
    event_buffer.push_back(current_event);

    component = icalcomponent_get_next_component(root, ICAL_VEVENT_COMPONENT);
  }

  icalcomponent_free(root);

  // Sort events in descening order by start datetime
  sort (event_buffer.begin(), event_buffer.end(), event_by_start_time_sorter);

  for (unsigned int j=0; j < event_buffer.size(); j++) 
  {

    // Check if this is the current booking
    if (
         (event_buffer[j].start_time <= current_time) &&
         (event_buffer[j].end_time   >= current_time)
       )
    {
      event_now_idx = (int)j;
    }
    
    
    
    // Get next booking. Note: event_buffer is sorted decending by start time, so
    // event_next should end up holding the nearest event that's in the future.
    if ((event_buffer[j].start_time > current_time) && ((int)j != event_now_idx))
      event_next_idx = j;
    
    // Not interested in past bookings
    if (event_buffer[j].start_time < current_time)
      break;
  }

  if (event_next_idx >= 0)
    evt_next = event_buffer[event_next_idx];
  else
    evt_next.start_time = 0;

  if (event_now_idx >= 0)
     evt_now = event_buffer[event_now_idx];
  else
    evt_now.start_time = 0;
  return 0;
}

// Decending sort by start time
bool nh_tools::event_by_start_time_sorter(evtdata const& i, evtdata const& j)
{
  return i.start_time > j.start_time;
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
