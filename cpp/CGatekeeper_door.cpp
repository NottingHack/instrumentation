/* TODO: 
 *  - maintain door state in database
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
      _db->sp_log_event("DOOR_OPENED", CNHmqtt::itos(_id));

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
      _db->sp_log_event("DOOR_CLOSED", CNHmqtt::itos(_id));
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
      _db->sp_log_event("DOOR_OPENED", CNHmqtt::itos(_id));
    }
    else _cb->cbiSendMessage(_entry_announce, payload); // Else just pass the message on verbatim
  }

  else if (type=="DoorButton")
    _db->sp_log_event("DOORBELL", CNHmqtt::itos(_id));

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
