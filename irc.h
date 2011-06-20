#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <netdb.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <signal.h>  
#include "CLogging.h"

using namespace std;

class irc 
{
  
      struct callback {
      string trigger;
      int (*ircCallback)(string, string, string, void*); // username, channel, message
      void *obj;
    };
  
  public:
    irc (string address, int port, string nick, string nickserv_password, CLogging *l);
    ~irc();
    int ircConnect();
    string lastError;
    void processMessage(string message);
    int join(string room);
    int send(string room, string message);
    int send(string message);
    int addCallback(string trigger, int (*ircCallback)(string, string, string, void*), void *obj);
    void wait();
    int state;
    int skt;
    void readThread();
    void activityThread();
    enum {CLOSED, ERROR, CONNECTED, DISCONNECTED, UNLOADING};

  private:

    int port;
    string address;
    string nick;
    string nickserv_password;
    pthread_t rThread;
    pthread_t aThread;
    struct sockaddr_in skt_addr;
    struct hostent *server;
    vector<string> channels;
    vector<callback> callbacks;
    bool pw_sent;
    bool alt_nick; // connected using alternative name as nick in use
    int write(string msg);
    time_t last_rx;
    time_t ping_sent;
    pthread_mutex_t socket_mutex;
    
    static void *readThread(void *arg);
    static void *activityThread(void *arg);
    
    int rx_privmsg(string prefix, string params);
    CLogging *log;
    
};



