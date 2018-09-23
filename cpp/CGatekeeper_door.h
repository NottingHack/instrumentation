#ifndef GKDOOR_H
#define GKDOOR_H

#include "CLogging.h"
#include "CNHDBAccess.h"
#include "nh-cbi.h"

class CGatekeeper_door
{
  public:
    std::string door_short_name;
    virtual ~CGatekeeper_door() {};
    virtual void process_door_event(std::string type, std::string payload) = 0;
    virtual void set_opts(int id, std::string base_topic, CLogging *log, CNHDBAccess *db, InstCBI *cb, std::string entry_announce, int read_timeout, std::string door_state, std::string exit_message) = 0;
};

#endif
