#include <mysql/mysql.h>
#include <iostream>
#include <stdlib.h>

using namespace std; 
    
class CDBAccess
{
  public:   
    CDBAccess(string server, string username, string password, string database);
    int validate_rfid_tag(string rfid_serial, string &handle);
    int dbConnect();
    void dbDisconnect();
    
  private:
      string server;
      string username;
      string password;
      string database;
      MYSQL mysql;
      MYSQL_RES *result; 
};