/* 
 * Copyright (c) 2013, Daniel Swann <hs@dswann.co.uk>, Matt Lloyd 
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
 * 3. Neither the name of nh-irc nor the names of its
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
#include "CNHDBAccess.h"
#include "nh-temperature.h"
#include <string>
#include <sstream>

#define MAX_TEMP 60
#define MIN_TEMP -30

using namespace std;

class nh_temperature : public CNHmqtt
{
public:
  CNHDBAccess *db;
  string temperature_topic;
  string temperature_topic_out;
  string light_level_topic;
  string humidity_topic;
  string barometric_pressure_topic;
  string sensor_battery_topic;

  nh_temperature(int argc, char *argv[]) : CNHmqtt(argc, argv)
  {
    temperature_topic         = get_str_option("temperature", "temperature_topic", "nh/temp");
    temperature_topic_out     = get_str_option("temperature", "temperature_topic_out", "nh/temperature");
    light_level_topic         = get_str_option("light_level", "light_level_topic", "nh/lightlevel");
    humidity_topic            = get_str_option("humidity", "humidity_topic", "nh/humidity");
    barometric_pressure_topic = get_str_option("barometric_pressure", "barometric_pressure_topic", "nh/barometric-pressure");
    sensor_battery_topic      = get_str_option("sensor_battery", "sensor_battery_topic", "nh/sensor-battery");

    db = new CNHDBAccess(get_str_option("mysql", "server", "localhost"), get_str_option("mysql", "username", "gatekeeper"), get_str_option("mysql", "password", "gk"), get_str_option("mysql", "database", "gk"), log);   ;
  }

  ~nh_temperature()
  {
    delete db;
  }

  void process_message(string topic, string message)
  {
    if (topic==temperature_topic)
    {
      char buf[50];
      string address;
      float temp;
      string desc;
      ostringstream ssTemp;

      if (message.length() < 17)
      {
        log->dbg("invalid message");
        return;
      }

      // break apart message into addres and temp
      address = message.substr(0,16);

      std::istringstream b( message.substr(17, message.length() - 17) );
      b >> temp;

      snprintf(buf, sizeof(buf), "temp: [%f]", temp);
      log->dbg(buf);
      
      // Sanity check the temperature
      if ((temp > MAX_TEMP) || (temp < MIN_TEMP))
      {
        snprintf(buf, sizeof(buf), "Ignoring excessive temperature: [%f]", temp);
        log->dbg(buf);
        return;
      }
      
      db->sp_temperature_update(address, temp);
      
      // publish room name / temperature to mqtt
      if (!db->sp_temperature_get_desc(address, desc))
        if (desc != "")
        {
          ssTemp << temp;
          string strTemp(ssTemp.str());
          message_send(temperature_topic_out + "/" + desc, strTemp);
        }
    }
    else if ((topic.length() > light_level_topic.length()+1) && topic.substr(0, light_level_topic.length()) == light_level_topic)
    {
      string room = topic.substr(light_level_topic.length()+1, string::npos);
      
      db->sp_light_level_update(room, atoi(message.c_str()));
    }
    else if ((topic.length() > humidity_topic.length()+1) && topic.substr(0, humidity_topic.length()) == humidity_topic)
    {
      string room = topic.substr(humidity_topic.length()+1, string::npos);

      db->sp_humidity_update(room, std::stof(message));
    }
    else if ((topic.length() > barometric_pressure_topic.length()+1) && topic.substr(0, barometric_pressure_topic.length()) == barometric_pressure_topic)
    {
      string room = topic.substr(barometric_pressure_topic.length()+1, string::npos);

      db->sp_barometric_pressure_update(room, std::stof(message));
    }
    else if ((topic.length() > sensor_battery_topic.length()+1) && topic.substr(0, sensor_battery_topic.length()) == sensor_battery_topic)
    {
      string room = topic.substr(sensor_battery_topic.length()+1, string::npos);

      db->sp_sensor_battery_update(room, std::stof(message));
    }
    

    CNHmqtt::process_message(topic, message);
  }

  bool setup()
  {
    subscribe(temperature_topic);
    subscribe(light_level_topic + "/#");
    subscribe(humidity_topic + "/#");
    subscribe(barometric_pressure_topic + "/#");
    subscribe(sensor_battery_topic + "/#");
    
    if (db->dbConnect())
      return false;
    
    return true;
  }
  
};


int main(int argc, char *argv[])
{
  
  nh_temperature nh = nh_temperature(argc, argv);
  
  if (nh.mosq_connect())
    return -1;
  
  if (!nh.setup())
    return -1;
  
  // run with "-d" flag to avoid daemonizing
  nh_temperature::daemonize(); // will only work on first run
  nh.message_loop();
  return 0;
  
}
