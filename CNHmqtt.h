#pragma once
#include <string>
#include <sstream>
#include "mosquitto.h"
#include "CLogging.h"
#include "inireader/INIReader.h"

using namespace std;


class CNHmqtt
{
  public:
    CNHmqtt(int argc, char *argv[]);
    ~CNHmqtt();
    string get_topic();
    struct mosquitto *mosq; //needs to be public so can be accessed by static callback
    void dbg(string msg);
    int message_loop(void);
    static int daemonize();
         
    int  mosq_connect();
    virtual void process_message(string topic, string message);
   
    int subscribe(string topic);
    bool mosq_connected;
    static bool debug_mode;
    static bool daemonized;
    
    static string itos(int n);
    
  protected:
    
    struct irc_dest {
      string nick;
      string channel;
    };
    
    string mqtt_topic;
    string mqtt_rx;
    string mqtt_tx;
    string status; // returned in response to status mqtt message (so should be short!)
    string mosq_server;
    int mosq_port;
    CLogging *log;
    string irc_out;
    string irc_in;

    int message_send(string topic, string message);
    int get_int_option(string section, string option, int def_value);
    string get_str_option(string section, string option, string def_value);
    static bool decode_irc_topic(string irc_in, string topic, string &nick, string &channel);
    int irc_send(string message, irc_dest dst);
    int irc_send_nick (string message, string nick);
    int irc_send_channel (string message, string channel);
    bool is_irc(string topic, struct irc_dest *dst);
        
  private:
    static void connect_callback(void *obj, int result);
    static void message_callback(void *obj, const struct mosquitto_message *message);  
    bool terminate;
    bool reset;
    bool config_file_parsed;
    INIReader *reader;
    uid_t uid;
    pthread_mutex_t mosq_mutex;
};


