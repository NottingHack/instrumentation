/*   
 * Copyright (c) 2012, Daniel Swann <hs@dswann.co.uk>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the owner nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "CNHmqtt.h"
#include "nh-vend.h"
#include "db/lib/CNHDBAccess.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#define BUFLEN 512

class nh_vend : public CNHmqtt
{
  public:
    CNHDBAccess *db;
    string temperature_topic;
    string twitter;
    int debug_level;
    
    nh_vend(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      sck = -1;
      port = get_int_option("vend", "port", 11023);
      temperature_topic = get_str_option("temperature", "temperature_topic", "nh/temp");
      debug_level = get_int_option("vend", "debug", 2);
      twitter = get_str_option("vend", "twitter_out", "nh/twitter/tx/status");
      db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   
    }
   
    ~nh_vend()
    {
      delete db;
    }
   
    void process_message(string topic, string message)
    {
    
    
      CNHmqtt::process_message(topic, message);
    }
    
    int setup()
    {
      struct sockaddr_in addr;
      
      if(db->dbConnect())
        return -1;
      
      // Create socket
      if ((sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      {
        log->dbg("Failed to create socket.");
        return -1;        
      }
      
      // Bind socket
      memset((char *) &addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      if (bind(sck, (struct sockaddr *)&addr, sizeof(addr))==-1)
      {
        log->dbg("Failed to bind to socket (port = " + itos(port) + ")");
        return -1;        
      } else
        log->dbg("Listening on port " + itos(port) + ".");
      
      // Start receive thread
      pthread_create(&rThread, NULL, &nh_vend::s_receive_thread, this);
      
     return 0; 
    }
    
    static void *s_receive_thread(void *arg)
    {      
      ((nh_vend*)arg)->receive_thread();
      return NULL;
    }
    
    void receive_thread()
    {
      char buf[BUFLEN];
      char response[BUFLEN];
      char dbgbuf[BUFLEN+256];
      struct sockaddr_in addr;
      socklen_t addrlen;
      int len;
      
      log->dbg("entered receive_thread()");      
      
      memset(buf, 0, sizeof(buf));
      addrlen = sizeof(addr);
      while ((len=recvfrom(sck, buf, sizeof(buf)-1, 0, (struct sockaddr *)&addr, &addrlen)) != -1)
      {
        if (buf[strlen(buf)-1] == '\n') // If the last character is a newline, remove it.
          buf[strlen(buf)-1] = 0;
            
        sprintf(dbgbuf, "%s:%d < [%s]", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
        log->dbg(dbgbuf);
        memset(response, 0, sizeof(response));
        process_message(buf, len, response);
        if (strlen(response) > 0)
        {      
          sprintf(dbgbuf, "%s:%d > [%s]", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), response);
          log->dbg(dbgbuf);
          if (sendto(sck, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen) == -1)
            log->dbg("Send failed!");
        }
          
          
        
        
        memset(buf, 0, sizeof(buf));  
      }

      perror("Done.");
     return;         
    }

  void process_message(char *msgbuf, unsigned int len, char *response)
  {
    char rfid_serial[20];
    char addr[20];
    float temp;
    char dbgbuf[256];
    char tran_id[7];
    string ret;
    string err;
    int args;
    int amount_scaled;
    int vend_ok;
    char position[20];
    string tweet;
    
    // expected message format:
    // XXXX:YYYY...
    if (len < 6)
        return;
    
    if (!strncmp(msgbuf, "INFO", 4))
    {
      // Info message only. Has already been written to the logfile on receiption, so do nothing
      return;
    }
    
    if (!strncmp(msgbuf, "AUTH", 4))
    {
      memset(rfid_serial, 0, sizeof(rfid_serial));
      if ((len-5) > sizeof(rfid_serial))
        strncpy(rfid_serial, msgbuf+5, sizeof(rfid_serial)-1);
      else
        strncpy(rfid_serial, msgbuf+5, len-5);
      
      if (db->sp_vend_check_rfid (rfid_serial, ret, err))
      {
        sprintf(response, "DENY:%s", rfid_serial);
        log->dbg("Card lookup failed!");
        return;
      }
      
      sprintf(dbgbuf, "Serial = [%s], ret = [%s], err = [%s]", rfid_serial, ret.c_str(), err.c_str());      
      log->dbg(dbgbuf);
      
      if (ret.length() <= 0)
      {
        sprintf(response, "DENY:%s", rfid_serial);
        log->dbg("Card lookup failed! (strlen)");
        return;
      }        
      
      if (ret == "0")
      {
        sprintf(response, "DENY:%s", rfid_serial);
        log->dbg("Card rejected.");
      }  else
      {
        // Card good
        sprintf(response, "GRNT:%s:%s", rfid_serial, ret.c_str());        
      }
      
      return;
    }  // end of AUTH message
    
    // Vend REQuest
    if (!strncmp(msgbuf, "VREQ", 4))
    {
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      if ((args=sscanf(msgbuf+5, "%19[^:]:%6[^:]:%d", rfid_serial, tran_id, &amount_scaled)) != 3)
      {
        log->dbg("Failed to split string");
        sprintf(response, "VDNY");
        return;
      }
      
      sprintf(dbgbuf, "Serial = [%s], transaction id = [%s], amount = [%d]", rfid_serial, tran_id, amount_scaled);      
      log->dbg(dbgbuf);      
      
      vend_ok = 0;
      if (db->sp_vend_request (rfid_serial, tran_id, amount_scaled, err, vend_ok))
      {
        sprintf(response, "VDNY:%s:%s", rfid_serial, tran_id); // Vend DeNY
        log->dbg("Card lookup failed!");
        return;
      }

      sprintf(response, "err = [%s], vend_ok = [%d]", err.c_str(), vend_ok);   
      log->dbg(response);

      if (!vend_ok)
      {
        sprintf(response, "VDNY:%s", rfid_serial);
        log->dbg("Card rejected.");
      }  else
      {
        // Card good
        sprintf(response, "VNOK:%s:%s", rfid_serial, tran_id);        
      }
      
      return;
    }  // end of VREQ message 
    
    // Vend SUCcess
    if (!strncmp(msgbuf, "VSUC", 4))
    {    
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      memset(position, 0, sizeof(position));
      if ((args=sscanf(msgbuf+5, "%19[^:]:%6[^:]:%s", rfid_serial, tran_id, position)) != 3)
      {
        log->dbg("Failed to split string. BUG **** Vend success not recored ****");
        return;
      }      
      
      sprintf(dbgbuf, "Vend succces: Serial = [%s], transaction id = [%s], position = [%s]", rfid_serial, tran_id, position);      
      log->dbg(dbgbuf);           
    
      db->sp_vend_success(rfid_serial, tran_id, position, err);
      if (err != "")
        log->dbg((string)"err = [" + err + (string)"]");   
            
      // Now tweet the vend if the product is in the database
      db->sp_vend_twitter_txt(tran_id, tweet);
      log->dbg((string)"Tweet=[" + tweet + (string)"]");   
      if (tweet != "")
      {
        message_send(twitter, tweet);
      } 
      
    }
    
    // Vend FAiL
    if (!strncmp(msgbuf, "VFAL", 4))
    {    
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      if ((args=sscanf(msgbuf+5, "%19[^:]:%6[^:]", rfid_serial, tran_id)) != 2)
      {
        log->dbg("Failed to split string. BUG **** Vend failure not recored ****");
        return;
      }      
      
      sprintf(dbgbuf, "Vend failure: Serial = [%s], transaction id = [%s]", rfid_serial, tran_id);      
      log->dbg(dbgbuf);           
    
      db->sp_vend_failure(rfid_serial, tran_id, err);
      if (err != "")
        log->dbg((string)"err = [" + err + (string)"]");   
    }    
    
    // Vend CANcel
    if (!strncmp(msgbuf, "VCAN", 4))
    {    
      memset(rfid_serial, 0, sizeof(rfid_serial));
      memset(tran_id, 0, sizeof(tran_id));
      if ((args=sscanf(msgbuf+5, "%19[^:]:%6[^:]", rfid_serial, tran_id)) != 2)
      {
        log->dbg("Failed to split string. BUG **** Vend cancel not recored ****");
        return;
      }      
      
      sprintf(dbgbuf, "Vend cancel: Serial = [%s], transaction id = [%s]", rfid_serial, tran_id);      
      log->dbg(dbgbuf);           
    
      db->sp_vend_cancel(rfid_serial, tran_id, err);
      if (err != "")
        log->dbg((string)"err = [" + err + (string)"]");   
    }   
    
    // TEMPerature report
    if (!strncmp(msgbuf, "TEMP", 4))
    {    
      temp = 0;
      memset(addr, 0, sizeof(addr));
      memset(tran_id, 0, sizeof(tran_id));
      if ((args=sscanf(msgbuf+5, "%19[^:]:%f6[^:]", addr, &temp)) != 2)
      {
        log->dbg("Failed to split temperature string.");
        return;
      }      
      
      sprintf(dbgbuf, "Temperature report: Address = [%s], temperature = [%f]", addr, temp);      
      log->dbg(dbgbuf);           
    
      sprintf(dbgbuf, "%s:%.2f", addr, temp);
      if ((temp < -30) || (temp > 80))
        log->dbg("Out of range temperature - not transmitting"); 
      else  
        message_send(temperature_topic, dbgbuf); 
    }   
    
    // DeBUG -request from vending machine for debug level
    if (!strncmp(msgbuf, "DBUG", 4))
    {   
      sprintf(response, "DBUG:%d", debug_level); 
    }
    
    // CASH - Cash sale
    if (!strncmp(msgbuf, "CASH", 4))
    {   
      log->dbg("TODO: Record cash sale");
    }    
    
  }
  
  void test()
  {
    string rfid_serial;
    string tran_id;
    int amount_scaled;
    string err;
    int vend_ok;
    
    rfid_serial = "1234567";
    tran_id = "10";
    amount_scaled = 60;
    
    
    db->sp_vend_request (rfid_serial, tran_id, amount_scaled, err, vend_ok);
  }
    
  private:
    int sck;
    int port;
    pthread_t rThread;
  
};



int main(int argc, char *argv[])
{
 
  nh_vend nh = nh_vend(argc, argv);
  
  nh.mosq_connect();
  
  // run with "-d" flag to avoid daemonizing
  nh_vend::daemonize(); // will only work on first run  
  
  if (nh.setup())
    return -1;

  nh.message_loop();  
  
  return 0;
  
}