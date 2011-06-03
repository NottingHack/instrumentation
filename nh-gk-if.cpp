#include "CNHmqtt.h"
#include <usb.h>        /* this is libusb */
#include <math.h>
#include <string.h>
#include "usb/opendevice.h" /* common code moved to separate module */
 
#include "usb/requests.h"   /* custom request numbers */
#include "usb/usbconfig.h"  /* device's VID/PID and names */
 
  const unsigned char rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
  char                vendor[] = {USB_CFG_VENDOR_NAME, 0}, product[] = {USB_CFG_DEVICE_NAME, 0}; 

class nh_gk_if : public CNHmqtt
{
  public :
    usb_dev_handle      *usb_handle;
    pthread_t           usb_thread;
    
    char                buffer[8];
    int                 vid, pid;
    char                avr_inputs;
    char                avr_outputs;
    
    
    nh_gk_if(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      usb_handle = NULL;
      vid = rawVid[1] * 256 + rawVid[0];
      pid = rawPid[1] * 256 + rawPid[0];
      avr_inputs = 0;
      avr_outputs = 0;
    }
   
    void process_message(string topic, string message)
    {
      int n;
      char b[20];
      string str_low;
      string str_high;
      string b_topic;
      bool updated = false;
      
      for (n=0; n<=7; n++)
      {
        sprintf(b, "out_bit%d", n+1);
        b_topic   = get_str_option(b, "topic", mqtt_rx + "/" + b);
        str_high  = get_str_option(b, "high" , "HIGH");
        str_low   = get_str_option(b, "low"  , "LOW");

        if ((topic == b_topic) && (message==str_high))
        {
          updated = true;
          avr_outputs |= 1 << n;
        }
        
        if ((topic == b_topic) && (message==str_low))
        {
          updated = true; 
          avr_outputs &= ~(1 << n);
        }
      }    

      if (updated)
        usb_send_update();
      
      CNHmqtt::process_message(topic, message);
    }
    
    void usb_send_update()
    {
      int n;
      char buffer[8];
      
      // print out new state
      
      log->dbg("Transmit state update to AVR: [");
      for (n=0; n <= 7; n++)
      {
        if ( (avr_outputs & (1 << n)))
          log->dbg("1");
        else
          log->dbg("0");
      }
      log->dbg("]");

      
      memset(buffer,0, sizeof(buffer));

      
      n = usb_control_msg(usb_handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, CUSTOM_RQ_SET_STATUS, avr_outputs, 0, buffer, 0, 5000);
      if(n < 0)
        log->dbg("USB transmit error: " + (string)usb_strerror());

      
    }
    
    int init()
    {
      usb_init();
      char buffer[100];
      
      if(usbOpenDevice(&usb_handle, vid, vendor, pid, product, NULL, NULL, NULL) != 0)
      {
        sprintf(buffer, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x", product, vid, pid);
        log->dbg(buffer);
        return -1;
      }
          
      if(usb_set_configuration(usb_handle, 1) < 0)
      {
          sprintf(buffer,"error setting USB configuration: %s", usb_strerror());
          log->dbg(buffer);
      }
      
      if(usb_claim_interface(usb_handle, 0) < 0)
      {
          sprintf(buffer,"error setting USB interface: %s", usb_strerror());
          log->dbg(buffer);
      }
      return 0;
    }    
    
    void usb_loop_start()
    {
      pthread_create(&usb_thread, NULL, &usb_loop, this);
    }
    
    static void *usb_loop(void *arg)
    {
      int n;
      char buffer[8];
      char dbg_msg[32];
      nh_gk_if *nh;
      nh = (nh_gk_if*)arg;  
      
      while ((n = usb_control_msg(nh->usb_handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, CUSTOM_RQ_GET_STATUS, 0, 0, buffer, sizeof(buffer), 5000)) >= 0)
      {
        // Expecting to receive either 0 bytes (so wait), or 3:
        // 1) AVR's PINB (Inputs)
        // 2) AVR's PORTC status (outputs) (TODO in fw)
        // 3) 0
        // So only the first byte is of intrest
        
        if (n==3)
          nh->process_usb_msg(buffer, n);
        else if (n==0)
        {
          /* wait for interrupt, set timeout to more than a week */
          n = usb_interrupt_read(nh->usb_handle, USB_ENDPOINT_IN | 1 , (char *)buffer, sizeof(buffer), 700000 * 1000);
   
          if(n < 0)
          {
            sprintf(buffer,"error in USB interrupt read: %s", usb_strerror());
            nh->log->dbg(buffer);
          }   
        }
        
        else
        {
           sprintf(dbg_msg, "USB: only %d bytes received.", n);
           nh->log->dbg(dbg_msg);
        }
      } 
      
      nh->log->dbg("FATAL: USB DISCONNECT: " + (string)usb_strerror() + "");
      usb_close(nh->usb_handle);
      nh->usb_handle = NULL;
      
      // no usb = no point, so send ourselves a terminate message
      nh->message_send(nh->mqtt_rx, "TERMINATE");      
      return NULL;
    }
    
    int process_usb_msg(char *buffer, int len)
    {
      string topic;
      string message;
      char b[10];
      int n;

      
      if (len < 2)
        return -1;
      
      for (n=0; n < 8; n++)
      { 

        if ( (buffer[0] & (1 << n)) !=  (avr_inputs & (1 << n)) )
        {
          // bit n has changed!
          sprintf(b, "in_bit%d", n);
          topic   = get_str_option(b, "topic", mqtt_tx + "/" + b);
          
          if (buffer[0] & (1 << n))
            message = get_str_option(b, "high", "1");
          else
            message = get_str_option(b, "low", "0");
          
          // send change via mqtt
           printf("[%#x]",  buffer[0]);
          message_send(topic, message);
        }
      }
      
      avr_inputs = buffer[0];      
      return 0; 
    }
      
    int mosq_topic_subscribe()
    {
      int n;
      char b[20];
      string topic;
      
      for (n=0; n<=7; n++)
      {
        sprintf(b, "out_bit%d", n+1);
        topic   = get_str_option(b, "topic", mqtt_rx + "/" + b);
        subscribe(topic);
      }
      
      return 0;
    }
  
};



int main(int argc, char *argv[])
{
 
  nh_gk_if nh = nh_gk_if(argc, argv);
  
  if (nh.init())// init usb & connect to device
  {
    cout << "ERROR: Failed to connect to device!"; 
    return -1;
  }
  
  nh.usb_send_update(); // put everything into a known state
  nh.usb_loop_start();  // creates a second thread and returns
  
  nh.mosq_connect();
  nh.mosq_topic_subscribe();
  nh.message_loop(); // only returns if a terminate message received or something goes wrong
  return 0;
  
}