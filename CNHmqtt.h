#pragma once
#include <string>
#include <sstream>
#include "mosquitto.h"
#include "CLogging.h"
#include "inireader/INIReader.h"

#define EXIT_TERMINATE 1 
#define EXIT_RESET 2

using namespace std;


class CNHmqtt
{
  public:
    CNHmqtt(int argc, char *argv[]);
    ~CNHmqtt();
    string get_topic();
    struct mosquitto *_mosq; //needs to be public so can be accessed by static callback
    void dbg(string msg);
    int message_loop(void);
    static int daemonize();
         
    int  mosq_connect();
    virtual void process_message(string topic, string message);
   
    int subscribe(string topic);
    bool _mosq_connected;
    static bool debug_mode;
    static bool daemonized;
    static string itos(int n);
    
  protected:
    string _mqtt_topic;
    string _mqtt_rx;
    string _mqtt_tx;
    string _status_name; // process name to report in respose to status message
    string _status_topic;
    string _mosq_server;
    int _mosq_port;
    CLogging *log;

    int message_send(string topic, string message);
    int message_send(string topic, string message, bool no_debug);
    int get_int_option(string section, string option, int def_value);
    string get_str_option(string section, string option, string def_value);
        
  private:
    static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);  
    bool _config_file_parsed;
    bool _config_file_default_parsed;
    bool _no_staus_debug;
    INIReader *_reader;
    INIReader *_reader_default;
    uid_t _uid;
    pthread_mutex_t _mosq_mutex;
};


