#pragma once
#include <string>
#include <sstream>
#include "mosquitto.h"
#include "CLogging.h"
#include "inireader/INIReader.h"

#define EXIT_TERMINATE 1 
#define EXIT_RESET 2

using namespace std;


class nh_mail
{
  public:
    nh_mail(int argc, char *argv[]);
    ~nh_mail();
    string get_topic();
    struct mosquitto *mosq; //needs to be public so can be accessed by static callback
    void dbg(string msg);
    int message_send(string topic, string message);
    int message_send(string topic, string message, bool no_debug);    
         
    int  mosq_connect();
   
    bool mosq_connected;
    static string itos(int n);
    int message_loop(void);
    string mqtt_topic;
    
  protected:
    string mosq_server;
    int mosq_port;
    CLogging *log;


    int get_int_option(string section, string option, int def_value);
    string get_str_option(string section, string option, string def_value);
        
  private:
    static void connect_callback(void *obj, int result);
    static void message_callback(void *obj, const struct mosquitto_message *message);  
    bool config_file_parsed;
    INIReader *reader;
    pthread_mutex_t mosq_mutex;
};


