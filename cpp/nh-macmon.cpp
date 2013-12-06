#include "CNHmqtt.h"
#include "nh-macmon.h"
#include "CMacmon.h"
#include "db/lib/CNHDBAccess.h"

class nh_macmon : public CNHmqtt
{
  string _interface;
  int _update_freq;
  CNHDBAccess *_db;
  pthread_t _mon_thread;
  string _topic_known, _topic_unknown;
  
  public:
    nh_macmon(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      _interface = get_str_option("macmon", "interface", "eth0");
      _topic_known   = get_str_option("macmon", "topic_known"  , "nh/addrcount/known"  );
      _topic_unknown = get_str_option("macmon", "topic_unknown", "nh/addrcount/unknown");
      _update_freq = get_int_option("macmon", "update_freq", 30);
      _db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);
    }

    void process_message(string topic, string message)
    {

      CNHmqtt::process_message(topic, message);
    }

    int dbupdatecallback(map<string, time_t> addresses)
    {
      map<string, time_t>::iterator itr;
      int addr_known, addr_unknown;

      /* Record address seen */
      for (itr = addresses.begin(); itr != addresses.end(); ++itr) 
        _db->sp_update_address(itr->first, &itr->second);

      /* Publish stats */
      _db->sp_get_address_stats(_update_freq, addr_known, addr_unknown);
      message_send(_topic_known  , itos(addr_known  ));
      message_send(_topic_unknown, itos(addr_unknown));

      return 0;
    }

    static void s_dbupdatecallback(void *obj, map<string, time_t> addresses)
    {
      nh_macmon *mm;

      mm = (nh_macmon*)obj;
      mm->dbupdatecallback(addresses);
      return;
    }

    void start_mon()
    {
      string err;
      CMacmon *mon = new CMacmon(_interface, &nh_macmon::s_dbupdatecallback, (void*)this, _update_freq);
      mon->start(err);
      log->dbg("mon done, err = [" + err + "]");
    }

    static void *s_mon_thread(void *obj)
    {
      nh_macmon *mm;

      mm = (nh_macmon*)obj;
      mm->start_mon();
      return 0;
    }

    int start()
    {
      if(_db->dbConnect())
        return -1;

      pthread_create(&_mon_thread, NULL, &nh_macmon::s_mon_thread, this);
      return 0;
    }
};



int main(int argc, char *argv[])
{
 
  nh_macmon nh = nh_macmon(argc, argv);
  nh.mosq_connect();
  nh.start();
  
  
  
  // run with "-d" flag to avoid daemonizing
  nh_macmon::daemonize(); // will only work on first run
  nh.message_loop();
  return 0;
  
}