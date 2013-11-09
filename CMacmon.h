#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <pcap.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <map>
#include <unistd.h>

typedef void (*UpdateCallbackType)(void *obj, std::map<std::string, time_t>);

class CMacmon
{
  public:
    CMacmon(std::string interface, UpdateCallbackType updateCallBack, void *obj, int update_freq);
    void setUserId(uid_t uid);
    int start(std::string &errStr);
 
  private:
    std::map<std::string, time_t> _addresses;
    UpdateCallbackType _updateCallBack;
    std::string _interface;
    int _updateFrq;
    uid_t _uid;
    void *_obj;
};

