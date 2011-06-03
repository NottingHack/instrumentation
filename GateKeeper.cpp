#include "CNHmqtt.h"
#include "GateKeeper_dbaccess.h"

#include <stdio.h>

bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 

class GateKeeper : public CNHmqtt
{
  public:
    
    string irc_in;
    string irc_out;
    string door_buzzer;
    string door_contact;
    string door_button;
    string rfid;
    string unlock;
    int bell_duration;
    CDBAccess *db;
    
    GateKeeper(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      irc_in = get_str_option("gatekeeper", "irc_in", "irc/rx");
      irc_out = get_str_option("gatekeeper", "irc_out", "irc/tx");
      door_buzzer = get_str_option("gatekeeper", "door_buzzer", "nh/gk/buzzer");
      bell_duration = get_int_option("gatekeeper", "bell_duration", 100);
      door_contact = get_str_option("gatekeeper", "door_contact", "nh/gk/contact");
      door_button = get_str_option("gatekeeper", "door_button", "nh/gk/button");
      rfid = get_str_option("gatekeeper", "rfid", "nh/gk/RFID");
      rfid = get_str_option("gatekeeper", "unlock", "nh/gk/Unlock");
      db = new CDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"));   
    }
    
    ~GateKeeper()
    {
      delete db;
    }
    
    void process_message(string topic, string message)
    {
      pthread_attr_t tattr;
      pthread_t bell_thread;
      string handle;
    //int ret;

      if(topic==irc_in)
      {
        if (message=="!bell")
        {
          pthread_attr_init(&tattr);
          pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
          pthread_create(&bell_thread, &tattr, &ring_bell, this);
        } 
      }
      
      if (topic==door_contact)
      {
        if (message=="LOW")
          message_send(irc_out, "Door opened");
        else if (message=="HIGH")
          message_send(irc_out, "Door closed");
      }   
          
      if (topic==door_button)
      {
        if (message=="HIGH")
          message_send(door_buzzer, "HIGH");
        else
          message_send(door_buzzer, "LOW");
      }          
      
      if (topic==rfid)
      {
        if(db->validate_rfid_tag(message, handle))
        {
          // access denied
        //db->log_access(rfid, "denied") // TODO: write this
          message_send(unlock, "Access denied");
        } else
        {
          // Ok - unlock
        //db->log_access(rfid, "unlock") // TODO: write this          
          message_send(unlock, "Unlock " + handle);
        }
      }
       
        
      CNHmqtt::process_message(topic, message);
    }
    
  int db_connect()
  {
    db->dbConnect();
    return 0;
  }

  static void *ring_bell(void *arg)
  {
    GateKeeper *gk;
    int n;
      
    gk = (GateKeeper*) arg;
    
    // Ring door bell
    gk->message_send(gk->door_buzzer, "HIGH");
          
    for (n=0; n < (gk->bell_duration); n++)
      usleep(1000); // 1ms
          
    gk->message_send(gk->door_buzzer, "LOW");
    
    return NULL;
  }
    
    void setup()
    {
      subscribe(irc_in);
      subscribe(door_contact);
      subscribe(door_button);
      subscribe(rfid);
    }
};



int main(int argc, char *argv[])
{
 
  string handle="";
  GateKeeper nh = GateKeeper(argc, argv);
  
  
  nh.db_connect();  
  
  GateKeeper::daemonize(); // will only work on first run
  
  nh.mosq_connect();
  nh.setup();
  nh.message_loop(); 
  return 0;
  
}