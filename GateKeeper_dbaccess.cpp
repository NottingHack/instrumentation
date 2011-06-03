#include "GateKeeper_dbaccess.h"
#include <string.h>
#include <cstdio>
#include <string>

CDBAccess::CDBAccess(string server, string username, string password, string database)
{
  CDBAccess::username = username;
  CDBAccess::password = password;
  CDBAccess::database = database;
  CDBAccess::server   = server; 
}

int CDBAccess::dbConnect()
{
  
  mysql_init(&mysql);
  
  if (!mysql_real_connect(&mysql,server.c_str(),username.c_str(),password.c_str(),database.c_str(),0,0,0))
  {
    cout << mysql_error(&mysql) << endl;
    return -1;
  }
  
  return 0;
}

void CDBAccess::dbDisconnect()
{
  mysql_close(&mysql);
  return;
}

int CDBAccess::validate_rfid_tag(string rfid_serial, string &handle)
{
  MYSQL_BIND    bind[1];
  MYSQL_STMT    *stmt;
  char          str_data[1000];
  my_bool       is_null[1];
  my_bool       error[1];
  unsigned long length[1];
  string        myQuery;
  bool          serial_found = false;
  
  memset(bind, 0, sizeof(bind));

  // rfid serial
  bind[0].buffer_type= MYSQL_TYPE_STRING;
  bind[0].buffer= (char *)rfid_serial.c_str();
  bind[0].buffer_length= rfid_serial.length();
  bind[0].is_null= 0;  
  
  stmt = mysql_stmt_init(&mysql);
  if (!stmt)
    return -1;   
  
  myQuery = "select m.handle from members m inner join rfid_tags r on r.member_id = m.member_id where r.rfid_serial = ?";
  
  if (mysql_stmt_prepare(stmt, myQuery.c_str(), myQuery.length()))
  {
    cout << "pre-fail\n";
    return -1;  
  }
 
  if (mysql_stmt_bind_param(stmt, bind))
  {
    cout << "Bind error\n";
    return -1;
  }
 
  if (mysql_stmt_execute(stmt))
  {
    printf("mysql_stmt_execute error: %s\n", mysql_error(&mysql));
    return -1;
  }
  
  // Handle column
  bind[0].buffer_type= MYSQL_TYPE_STRING;
  bind[0].buffer= (char *)str_data;
  bind[0].buffer_length= sizeof(str_data);
  bind[0].is_null= &is_null[0];
  bind[0].length= &length[0];
  bind[0].error= &error[0];
  
  if (mysql_stmt_bind_result(stmt, bind))
  {
    cout << "Failed to bind output params\n";
    return -1;
  }
  
  if (mysql_stmt_store_result(stmt))
  {
    cout << "mysql_stmt_store_result failed\n";
    return -1;
  }
  
  
  while (!mysql_stmt_fetch(stmt))
  {
    serial_found = true;
    handle = str_data;
  }

  mysql_stmt_close(stmt);
  
  if (serial_found)
    return 0;
  else
    return 1;
}

