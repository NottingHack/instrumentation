/* 
 * Copyright (c) 2015, Daniel Swann <hs@dswann.co.uk>
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

/*
 * For Tools access control (e.g. laser cutter) at Nottingham Hackspace
 */


#include "CNHmqtt_irc.h"
#include "CNHDBAccess.h"
#include "nh-cbi.h"
#include "nh-tools-bookings.h"

#include <stdio.h>
#include <time.h>
#include <string.h>


bool CNHmqtt::debug_mode = false;
bool CNHmqtt::daemonized = false; 
std::string CNHmqtt::_pid_file = "";


class nh_tools : public CNHmqtt_irc, public InstCBI
{
  public:
    nh_tools(int argc, char *argv[]);
    ~nh_tools();

    void process_message(std::string topic, std::string message);
    void process_irc_message(irc_msg msg);
    int cbiSendMessage(std::string topic, std::string message);
    int db_connect();
    void setup();

    /* split - taken from Alec Thomas's answer to http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
    void split(std::vector<std::string> &tokens, const std::string &text, char sep);

  private:
    CNHDBAccess *_db;

    std::string _tool_topic;
    std::string _bookings_topic;
    std::string _db_server;
    std::string _db_username;
    std::string _db_password;
    std::string _db_name;
    std::string _status_topic;
    bool _setup_done;
    CLogging *_bookings_log;

    std::map<std::string,nh_tools_bookings*> _bookings;

    // nh_tools_bookings *_bookings;
};
