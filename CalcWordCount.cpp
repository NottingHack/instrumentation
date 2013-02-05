#include <stdio.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

#include "db/lib/CNHDBAccess.h"
#include "CLogging.h"

unsigned int get_word_count(string line);
unsigned int get_msg_word_count(string msg_body);
string sTo_lower(string s);
string _itos(int n);
CLogging log;
  
int main()
{  

  CNHDBAccess db = CNHDBAccess("localhost", "gk", "gk", "instrumentation", &log);
  string msg_body;

  unsigned int word_count = 0;
  int email_id;
  
  db.dbConnect();  
  db.sp_gg_get_email(email_id, msg_body);
  
  while (email_id > 0)
  {
    word_count = get_msg_word_count(msg_body);
    
    log.dbg("Email id = " + _itos(email_id) + ". Word count = " + _itos(word_count));
    
    if (db.sp_gg_set_auto_wc(email_id, word_count))
      break;
    
    if (db.sp_gg_get_email(email_id, msg_body))
      break;
  }
  
  return 0;
}

unsigned int get_msg_word_count(string msg_body)
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
    if ((pos = msg_lines[i].find_first_not_of(" ")) == string::npos)
      continue;
    
    if (msg_lines[i][pos] == '>')
      continue;
    
    if (msg_lines[i].find("You received this message because you are subsc") != string::npos)
      break;
    
    word_count += get_word_count(msg_lines[i]); 
  //log.dbg(msg_lines[i]);
  }
  return word_count;
}

unsigned int get_word_count(string line)
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

string sTo_lower(string s)
{
  string ret = s;
  
  for(unsigned int i = 0; i < s.length(); i++)
  {
    ret[i] = tolower(s[i]);
  }
  
  return ret;
}

string _itos(int n)
{
  string s;
  stringstream out;
  out << n;
  return out.str();
}

