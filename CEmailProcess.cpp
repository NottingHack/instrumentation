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

/* Class to extract to/from/subject/text body (skipping over any attachemnts, 
 * HTML portion, etc) of an email */

#include "CEmailProcess.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

CEMailProcess::CEMailProcess()
{
  _header_complete = false;
}

bool CEMailProcess::add_line(std::string msgdata)
{
  string header_field;
  string header_body;
  
  if (!_header_complete)
    _header_complete = header_line(msgdata, _headers);
  else
    _body.push_back(msgdata);
  
  return true;
}

/* qp = Quoted Printable */
string CEMailProcess::qp_decode(string msg_line)
{
  string retstr;
  retstr = "";
  unsigned char enc_char = '\0';
  unsigned int n;
  
  if (msg_line.length() == 0)
    return "\n";
  
  /* decode escaped characters */
  for (n = 0; n < msg_line.size()-1; n++)
  {
    if (msg_line[n] != '=')
    {
      retstr += msg_line[n];
    } else
    {
      /* the next two characters should be an ASCII code */
      if (n+3 > msg_line.size())
        break;
      
      sscanf(msg_line.substr(n+1, 2).c_str(), "%2hhx", &enc_char);
      
      if (enc_char != '\0')
        retstr += enc_char; 
      
      n += 2;
    }
  }
  /* Add last character. Specical case, as it may be a '='  *
   * without a two digit ascii code following it            */
  if (n==msg_line.size()-1)
    retstr += msg_line[msg_line.size()-1];
    
  /* if last char isn't =, then it's not a soft line break */
  if (retstr.substr(retstr.length()-1, 1) != "=")
    retstr += "\n";
  else /* remove trailing = */
    retstr = retstr.substr(0, retstr.length()-1);
  
  return retstr;
}

bool CEMailProcess::process()
{
  string boundary;
  map<string,string>::iterator it;
  bool qp;
  
  /* is this a MIME email? */
  if (_headers["content-type"].find("multipart") != string::npos)
  {
    boundary = get_boundary(_headers["content-type"]);
    
    if (boundary != "")
      _boundaries.push_back(boundary);
   
    for (unsigned int n=0; n < _body.size(); n++)
    {
      /* Search for boundary */
      if (!is_boundary(_body[n]))
        continue;
      
      /* Make sure there's actaully something to read */
      if ((n+3) > _body.size())
        break;
      
      /* read mime header */
      _mime_header.clear();
      while(!header_line(_body[++n], _mime_header));
      
      /* Only interested in the plain text element */
      if (_mime_header["content-type"].substr(0, sizeof("text/plain")-1) == "text/plain")
      {
        n++; /* skip over crlf seperator between header and body */
        if (_mime_header["content-transfer-encoding"] == "quoted-printable")
          qp = true;
        else
          qp = false;
        
        while (n < _body.size() && (!is_boundary(_body[n])))
        {
          if (qp)
            _text_body.push_back(qp_decode(_body[n++]));
          else
            _text_body.push_back(_body[n++] + "\n");     
        }
      } else if (_mime_header["content-type"].find("multipart") != string::npos)
      {
        /* Multipart within multipart. Also want to serach this multipart for a 
         * text/plain component */
        boundary = get_boundary(_mime_header["content-type"]);
        
        if (boundary != "")
          _boundaries.push_back(boundary);        
      }
    }
  }
  else 
  {
    /* Not a MIME encoded email */
    
    if (_headers["content-transfer-encoding"] == "quoted-printable")
      qp = true;
    else
      qp = false; 
    
    for (unsigned int n = 0; n < _body.size(); n++)
    {
      if (qp)
        _text_body.push_back(qp_decode(_body[n]));
      else
        _text_body.push_back(_body[n] + "\n");     
    }
  }
  
  return true;
}

/* returns false when the head of the header section is reached, true otherwise */
bool CEMailProcess::header_line(string msgdata,  map<string, string> &headers)
{
  unsigned int sep;
  string field;
  string body;
  
  /* If the first character is a space/tab, it's a continuation of the previous line */
  if ((msgdata[0] == ' ') || (msgdata[0] == '\t'))
  {
    if (msgdata.length() > 1)
      headers[_last_field] += msgdata.substr(1);
    return false;
  }
  
  /* If it's a crlf on it's own, we're done with the header */
  if (msgdata.size() == 0)
  {
    return true;
  }
  
  /* seperate field/body, and store the result */
  sep = msgdata.find_first_of(":");
  if ((sep == string::npos) || (msgdata.size() < (sep + 2)))
  {
    printf("Error - no seperator. msgdata=[%s]\n", msgdata.c_str());
    return false;
  }
  
  _last_field = field = sTo_lower(msgdata.substr(0, sep));
  body  = msgdata.substr(sep+2, string::npos);
  headers[field] = body;
  return false;  
}

string CEMailProcess::get_boundary(string line)
{
  unsigned int pos;
  string boundary;
  
  // Sanity check - if we think we've already found more than 10 
  // differant boundaries, something's probably gone horribly wrong.
  if (_boundaries.size() > 10)
    return "";
  
  // MIME email, get boundary 
  pos = line.find("boundary=");
  if (pos == string::npos)
  {
    printf("Something went wrong getting boundary. Content-Type=[%s]\n", line.c_str());
    return "";
  }
    
  boundary = line.substr(pos + sizeof("boundary=")-1);
    
  // Remove any quotes around the boundary string 
  if ((boundary[0] == '"') && (boundary.length ()> 2))
    boundary = boundary.substr(1, boundary.length()-2);
  boundary = "--" + boundary;
  
  return boundary;
}
    
bool CEMailProcess::is_boundary(string line)
{
  for (unsigned int i = 0; i < _boundaries.size(); i++)  
    if (line == _boundaries[i])
      return true;
  
  return false;    
}

string CEMailProcess::get_subject()
{
  return _headers["subject"];
}

string CEMailProcess::get_from()
{
  return _headers["from"];
}

string CEMailProcess::get_message_id()
{
  return _headers["message-id"];
}

string CEMailProcess::get_reply_to()
{
  return _headers["in-reply-to"];
}

string CEMailProcess::get_body()
{
  string body="";
  
  for (unsigned int i=0; i < _text_body.size(); i++)
    body += _text_body[i];  
  
  return body;
}

string CEMailProcess::get_list_id()
{
  return _headers["list-id"];
}
    
string CEMailProcess::sTo_lower(string s)
{
  string ret = s;
  
  for(unsigned int i = 0; i < s.length(); i++)
  {
    ret[i] = tolower(s[i]);
  }
  
  return ret;
}


unsigned int CEMailProcess::get_msg_word_count(string msg_body)
{
  vector<string> msg_lines;
  string msg_line;
  unsigned int pos;
  unsigned int word_count = 0;
    
  // Split string into lines
  stringstream msg_body_ss(msg_body);
  while (getline(msg_body_ss, msg_line))
    msg_lines.push_back(msg_line);
  
  for (unsigned int i = 0; i < msg_lines.size(); i++)
  {
    if (msg_lines[i].size() <= 1)
      continue;
     
    if ((pos = msg_lines[i].find_first_not_of(" ")) == string::npos)
      continue;
    
    if (msg_lines[i][pos] == '>')
      continue;
    
    if (msg_lines[i].find("You received this message because you are subsc") != string::npos)
      break;
    
    word_count += get_word_count(msg_lines[i]); 
  }
  return word_count;
}

unsigned int CEMailProcess::get_word_count(string line)
{
  vector<string> words;  
  unsigned int word_count = 0;
  string word;
  
  // Split string into words
  stringstream line_ss(line);
  while (getline(line_ss, word, ' '))
    words.push_back(sTo_lower(word));
  
  // Loop through and count words 
  for (unsigned int i = 0; i < words.size(); i++)
  {  
    if (words[i].length() == 0)
      continue;
    
    // The only single letter word we're going to count is A or I
    if ((words[i].length() == 1) && (words[i].find_first_of("ai") == string::npos))
      continue;
    
    // If the "word" doesn't have letters in it, then it's not a word
    if (words[i].find_first_of("abcdefghijklmnopqrstuvwxyz") == string::npos)
      continue;
    
    word_count++;
  }
    
 return word_count; 
}


/*
int main()
{
  CEMailProcess mp;
  string input_line;
  
  while (cin)
  {
    getline(cin, input_line);
    mp.add_line(input_line);
  }
  
  mp.process();
  cout << "Subject: " << mp.get_subject() << endl;
  cout << "From: " << mp.get_from() << endl;
  cout << "Message id: " << mp.get_message_id() << endl;
  cout << "Reply to: " << mp.get_reply_to() << endl;
  cout << "Body: " << endl << mp.get_body() << endl;
  return 0;
}
*/
