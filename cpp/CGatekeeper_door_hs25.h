#include <string>
#include <time.h>
#include <string.h>
#include <list>
#include <queue>

#include "CLogging.h"
#include "CNHDBAccess.h"
#include "nh-cbi.h"
#include "CGatekeeper_door.h"

class CGatekeeper_door_hs25 : public CGatekeeper_door
{
  public:
    CGatekeeper_door_hs25();
    ~CGatekeeper_door_hs25();

    void set_opts(int id, std::string base_topic, CLogging *log, CNHDBAccess *db, InstCBI *cb, std::string entry_announce, int read_timeout, std::string door_state);
    void process_door_event(std::string type, std::string payload);

    enum DoorState {DS_OPEN, DS_CLOSED, DS_UNKNWON};

  private:
    struct door_bell
    {
      std::string mqtt_topic;
      std::string mqtt_message;
    };

    struct member_arrival
    {
      int member_id;
      int new_zone_id;
      time_t card_read_time;
    };

    void dbg(std::string msg);
    void display_message_lcd(char side, std::string message, int duration);
    void beep(char side, int tone = 5000, int duration = 50);
    DoorState get_door_state_from_str(std::string door_state);
    int set_member_zone(int member_id, int new_zone_id);

    std::list<door_bell> _door_bells;
    int _id;
    std::string _base_topic;
    std::string _handle;
    std::string _last_seen;
    std::string _entry_announce;
    time_t _last_valid_read;
    int _read_timeout;
    DoorState _door_state;
    std::queue<member_arrival> _pending_arrivals; // members who have swiped their card on the read, but not opened the door yet (entries should only exist in here for a few seconds normally)

    CLogging *_log;
    CNHDBAccess *_db;
    InstCBI *_cb;
};

