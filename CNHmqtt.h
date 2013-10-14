#pragma once
#include <string>
#include <sstream>
#include <list>
#include "mosquitto.h"
#include "CLogging.h"
#include "inireader/INIReader.h"

#define EXIT_TERMINATE 1 
#define EXIT_RESET 2

class CNHmqtt
{
  public:
    CNHmqtt(int argc, char *argv[]);
    ~CNHmqtt();
    std::string get_topic();
    struct mosquitto *_mosq; //needs to be public so can be accessed by static callback
    void dbg(std::string msg);
    int message_loop(void);
    static int daemonize();
         
    int  mosq_connect();
    virtual void process_message(std::string topic, std::string message);
   
    int subscribe(std::string topic);
    bool _mosq_connected;
    static bool debug_mode;
    static bool daemonized;
    static std::string itos(int n);
    
  protected:
    std::string _mqtt_topic;
    std::string _mqtt_rx;
    std::string _mqtt_tx;
    std::string _status_name; // process name to report in respose to status message
    std::string _status_req_topic; // topic status requests are sent to
    std::string _status_res_topic; // topic status responses are to be published to
    std::string _mosq_server;
    int _mosq_port;
    CLogging *log;

    int message_send(std::string topic, std::string message);
    int message_send(std::string topic, std::string message, bool no_debug);
    int get_int_option(std::string section, std::string option, int def_value);
    std::string get_str_option(std::string section, std::string option, std::string def_value);
        
  private:
    static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
    static void connect_callback(struct mosquitto *mosq, void *obj, int result);
    void connected();
    bool _config_file_parsed;
    bool _config_file_default_parsed;
    bool _no_staus_debug;
    INIReader *_reader;
    INIReader *_reader_default;
    uid_t _uid;
    pthread_mutex_t _mosq_mutex;
    list<std::string> _topic_list;
};


