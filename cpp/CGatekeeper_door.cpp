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


#include "CGatekeeper_door.h"
#include "CNHmqtt.h"

using namespace std;


CGatekeeper_door::CGatekeeper_door()
{
  _id = 0;
  _base_topic = "";
  _log = NULL;
  _db = NULL;
  door_short_name = "";
  _handle = "";
  _entry_announce = "";
  memset(&_last_valid_read, 0, sizeof(_last_valid_read));
}

CGatekeeper_door::~CGatekeeper_door()
{
  dbg("Deleted");
}

void CGatekeeper_door::set_opts(int id, string base_topic, CLogging *log, CNHDBAccess *db, InstCBI *cb, string entry_announce, int read_timeout)
{
  _id  = id;
  _base_topic = base_topic;
  _log = log;
  _db  = db;
  _cb  = cb;
  _entry_announce = entry_announce;
  _read_timeout = read_timeout;
  
  // Now the door_id has been set, get the list of door bells that should be rang when the button is pushed
  _door_bells.clear();
  
  // Get a list of all bells to be rung...
  dbrows bells;
  db->sp_gatekeeper_get_door_bells(_id, &bells);

  dbg("Door button will ring:");
  for (dbrows::const_iterator iterator = bells.begin(), end = bells.end(); iterator != end; ++iterator) 
  {
    door_bell bell;
    dbrow row = *iterator;

    bell.mqtt_topic   = row["bell_topic"].asStr();
    bell.mqtt_message = row["bell_message"].asStr();
    _door_bells.push_back(bell);

    dbg("\t" + bell.mqtt_topic + "\t" + bell.mqtt_message);
  }


  dbg("Configured");
}

void CGatekeeper_door::dbg(string msg)
{
  if (_log == NULL)
    return;

  _log->dbg("CGatekeeper_door(" + door_short_name + ")", msg);
}


void CGatekeeper_door::process_door_event(string type, string payload)
{
  if ((_db == NULL) || (_cb == NULL))
    return;

  time_t current_time;
  string unlock_text;
  string err;
  string unlock_topic = _base_topic + "/" + CNHmqtt::itos(_id) + "/Unlock";

  if (type=="DoorState")
  {
    if ((payload.substr(0, ((string)("Door Opened by:")).length() ) == "Door Opened by:") && (_handle != ""))
    {
      _db->sp_set_door_state(_id, "OPEN");

      if (_last_seen.length() > 1)
      {
        _cb->cbiSendMessage(_entry_announce + "/known", "Door opened by: " + _handle + " (last seen " + _last_seen + " ago)");
      }
      else
      {
        dbg("No last seen time set");
        _cb->cbiSendMessage(_entry_announce, payload + " " + _handle);
      }
      _handle = "";
      _last_seen = "";
    }
    else if (payload=="Door Closed")
    {
      dbg("Ignoring door closed message");
      _db->sp_set_door_state(_id, "CLOSED");
    }
    else if (payload=="Door Time Out")
    {
      _handle = "";
      _last_seen = "";
      _db->sp_log_event("DOOR_TIMEOUT", CNHmqtt::itos(_id));
    }
    else if (payload=="Door Opened")
    {
      _cb->cbiSendMessage(_entry_announce + "/unknown", "Door opened");
      _db->sp_set_door_state(_id, "OPEN");
    }
    else _cb->cbiSendMessage(_entry_announce, payload); // Else just pass the message on verbatim
  }

  else if (type=="DoorButton")
  {
    // Send message to all bells that need to be rang
    for (list<door_bell>::iterator i=_door_bells.begin(); i != _door_bells.end(); ++i)
      _cb->cbiSendMessage((*i).mqtt_topic, (*i).mqtt_message);

    // Send a message with the door name for the matrix displays / IRC / slack
    _cb->cbiSendMessage(_base_topic + "/DoorButton", door_short_name);

    // Log an event recording this door button was pushed
    _db->sp_log_event("DOORBELL", CNHmqtt::itos(_id));
  }

  else if (type=="RFID")
  {
    time(&current_time);
    if (difftime(current_time, _last_valid_read) > _read_timeout) // If there's been an unlock message sent in the
    {                                                             // last few seconds, do nothing (door is already open)
      if(_db->sp_check_rfid(payload, _id, unlock_text, _handle, _last_seen, err))
      {
        dbg("Call to sp_check_rfid failed");
        _cb->cbiSendMessage(unlock_topic, "Access Denied");
      } else
      {
        _cb->cbiSendMessage(unlock_topic, unlock_text); 

        if (unlock_text.substr(0, 7) == "Unlock:")
          time(&_last_valid_read);
      }
    } else
    {
      dbg("Ignoring message: came too soon after previous valid card");
    }
  }

  else if (type=="Keypad")
  {
    _db->sp_check_pin(payload, _id, unlock_text, _handle, err);
    dbg("err = [" + err + "]");
    _cb->cbiSendMessage(unlock_topic, unlock_text);
  }
  
}
