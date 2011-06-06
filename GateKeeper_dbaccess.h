#include <mysql/mysql.h>
#include <iostream>
#include <stdlib.h>
#include "CLogging.h"

#define STATE_ACTIVE 10
#define STATE_EXPIRED 20
#define STATE_CANCELLED 30

#define ACCESS_DENIED 10
#define ACCESS_GRANTED 20

using namespace std; 
    
class CDBAccess
{
  public:   
    CDBAccess(string server, string username, string password, string database, CLogging *log);
    int validate_rfid_tag(string rfid_serial, string &unlock_text);
    int validate_pin(string pin, string &unlock_text);
    int dbConnect();
    void dbDisconnect();
    int log_rfid_access(string rfid, int access);
    int log_pin_access(string rfid, int access);
    
  private:
      string server;
      string username;
      string password;
      string database;
      MYSQL mysql;
      MYSQL_RES *result; 
      CLogging *log;
};