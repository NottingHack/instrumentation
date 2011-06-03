#include "CNHmqtt.h"
#include "nh-test.h"

 

class nh_test : public CNHmqtt
{
  public :nh_test(int argc, char *argv[]) : CNHmqtt(argc, argv)
   {
   }
   
  void process_message(string topic, string message)
  {
    
    
    CNHmqtt::process_message(topic, message);
  }
  
};



int main(int argc, char *argv[])
{
 
  nh_test nh = nh_test(argc, argv);
  
  nh.mosq_connect();
  
  // run with "-d" flag to avoid daemonizing
  nh_test::daemonize(); // will only work on first run
  nh.message_loop();
  return 0;
  
}