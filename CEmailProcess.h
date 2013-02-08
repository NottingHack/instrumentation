#include <string.h>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

class CEMailProcess
{
  public:
    CEMailProcess();
    bool add_line(std::string msgdata); /* Call this with email line of the received email */
    bool process(); /* Call after complete email has been passed into add_line */
    unsigned int get_msg_word_count(std::string msg_body);
    std::string get_subject();
    std::string get_from();
    std::string get_message_id();
    std::string get_reply_to();
    std::string get_body();
    std::string get_list_id();
    
  private:
    std::string sTo_lower(std::string s);
    std::string qp_decode(std::string msg_line);
    std::string get_boundary(std::string line);
    unsigned int get_word_count(std::string line);
    bool is_boundary(std::string line);
    bool header_line(std::string msgdata,  std::map<std::string, std::string> &headers);
    bool _header_complete;
    std::vector<std::string> _body;
    std::string _last_field;
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _mime_header;
    std::vector<std::string> _text_body;  
    std::vector<std::string> _boundaries;
};  
