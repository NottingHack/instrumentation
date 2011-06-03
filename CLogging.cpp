#include "CLogging.h"

CLogging::CLogging()
{

}

CLogging::~CLogging()
{
  if (logfile.is_open())
    logfile.close();
  
}

bool CLogging::open_logfile(string log_file)
{
  if (log_file == "")
  {
    cout << "No logfile specified!\n";
    return false;
  }
  
  logfile.open (log_file.c_str(), ios::out | ios::app);
  
  if (logfile.is_open())
    return true;
  else
  {
    cout << "\nError opening logfile! (" << log_file << ")\n";
    return false;
  }
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
  
  // Write to log file if open, otherwise output to stdout
  if (logfile.is_open())
    logfile << buf << msg << endl;
  else
    cout << buf << msg << endl;
}