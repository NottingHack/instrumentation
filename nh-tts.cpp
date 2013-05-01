/* 
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>
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
#include "nh-tts.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>


class nh_tts : public CNHmqtt
{
  private:
    int _port;
    string _address;
    string _tts_topic;
    int _sck;
    pthread_t _rx_thread;
    
  public:
    nh_tts(int argc, char *argv[]) : CNHmqtt(argc, argv)
    {
      _port = get_int_option("festival", "port", 1314);
      _address = get_str_option("festival", "address", "localhost");
      _tts_topic = get_str_option("tts", "topic", "nh/tts/gk");
      _sck = -1;
      _rx_thread = -1;
    }
   
    void process_message(string topic, string message)
    {
      string msg;
      string msg_clean;
    
      if (topic == _tts_topic)
      {
        /* Try not to send anything that'll confuse festival, e.g. extra unescaped quotes */
        for (unsigned int a=0; a < message.length(); a++)
        {
          if (((message[a] > '#') && (message[a] < 'z')) || (message[a] == ' '))
            msg_clean += message[a];
        }
        
        msg = "(SayText \"" + msg_clean + "\")";
        log->dbg("> " + msg);
        write(_sck, msg.c_str(), msg.length());        
      }
    
      CNHmqtt::process_message(topic, message);
    }
    
    int init()
    {
      int ret;
      struct addrinfo hints;
      struct addrinfo *result, *pRes;
      
      subscribe(_tts_topic);
      
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      
      if((ret=getaddrinfo(_address.c_str(), CNHmqtt::itos(_port).c_str(), &hints, &result)))
      {
        log->dbg("getaddrinfo failed");
        return -1;
      }
      
      /* Connect to festival */
      log->dbg("Connecting to " + _address + ":" + itos(_port));
      pRes = result;
      do
      {
        _sck = socket(pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);
        if (_sck == -1)
          continue;
        
        if ((ret=connect(_sck, pRes->ai_addr, pRes->ai_addrlen)) == 0)
        {
          log->dbg("Connected to festival!");
          break;
        } else
        {
          close(_sck);
          _sck = -1;
        }
        pRes = pRes->ai_next;
      } while (pRes != NULL);
      
      freeaddrinfo(result);
      if (_sck==-1)
      {
        log->dbg("Failed to connect to festival.");
        return -1;
      }
      
      /* Start thread to monitor incoming data */
      pthread_create(&_rx_thread, NULL, rx_thread_w, (void *) this);
      
      return 0;
    }
    
    void rx_thread()
    {
      char buf[1024];
      
      while ( read(_sck, buf, sizeof(buf)) > 0)
      {
        log->dbg("< " + (string)buf);
      }
      
      log->dbg("Connection to festival lost.");
      exit(1);
    }
};

void *rx_thread_w(void *arg) 
{
  ((nh_tts*)arg)->rx_thread();
  return NULL;
}


int main(int argc, char *argv[])
{
  nh_tts nh = nh_tts(argc, argv);
  
  nh.mosq_connect();
  
  // run with "-d" flag to avoid daemonizing
  nh_tts::daemonize(); // will only work on first run
  
  if(nh.init())
    return -1;
  
  nh.message_loop();
  return 0;
}