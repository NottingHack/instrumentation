#include "CLogging.h"

CLogging::CLogging(string logfile)
{
  
  
  
}

void CLogging::dbg(string msg)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buf[100];

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  // Jun  2 18:47:14 
  strftime (buf,sizeof(buf),"%b %d %H:%M:%S: ",timeinfo);
  
  cout << buf << msg << endl;
}