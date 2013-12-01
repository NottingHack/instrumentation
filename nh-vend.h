bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false;

#include "db/lib/CNHDBAccess.h"
//#include "CVMC.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <list>



class nh_vend;


    class vm_msg
    {
    public:
      std::string vm_type;
      std::string msg;
      std::string vm_desc;
      int vm_id;
      nh_vend *nhvend;
      vm_msg(nh_vend *nhv, dbrow row, std::string m)
      {
        vm_type = row["vmc_type"].asStr();
        vm_desc = row["vmc_description"].asStr();
        vm_id   = row["vmc_id"].asInt();
        nhvend  = nhv;
        msg = m;
      }

      virtual void send(std::string)  = 0;
    };

    class vm_msg_mqtt : public vm_msg
    {
    public:
      vm_msg_mqtt(nh_vend *nhv, dbrow row, std::string msg);
      void send(std::string msg);
      std::string topic;
    };

    
    class vm_msg_udp : public vm_msg
    {
    public:
      vm_msg_udp(nh_vend *nhv, dbrow row, std::string msg, int sck, struct sockaddr_in remote_addr);
      void send(std::string);
      std::string ipaddr;
      int port;
      int sock;
      struct sockaddr_in remote_addr;
    };










class nh_vend : public CNHmqtt
{
  private:
    int sck;
    int port;
    pthread_t rThread;
    dbrows vm_list;
  //  std::list<CVMC*> _VMs;
  //  static int s_process_vm_msg_cb(CVMC* cvmc, void* obj, std::string msg);
 //   int process_vm_msg_cb(CVMC* cvmc, std::string msg);

  public:
    CNHDBAccess *db;
    std::string temperature_topic;
    std::string twitter;
    int debug_level;

    nh_vend(int argc, char *argv[]);
    ~nh_vend();
    void process_message(std::string topic, std::string message);
    int setup();
    static void *s_receive_thread(void *arg);
    void receive_thread();
    int vend_message_send(std::string msg, struct sockaddr_in *addr);
    void process_message(vm_msg* vmmsg);
    void test();
    void mqtt_send(std::string topic, std::string msg)
    {
      message_send(topic, msg);
    }
    void dbg(std::string msg) {log->dbg(msg);}
};


    
        
