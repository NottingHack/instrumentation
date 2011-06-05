#include "CLogging.h"

// Logging.
// Not using ofstream becasue it can't open files in exclusive mode!
// (a crude way to stop the process from being running more than once).

CLogging::CLogging()
{
  logfile = -1;
  pthread_mutex_init (&logfile_mutex, NULL);
}

CLogging::~CLogging()
{
  if (logfile > 0)
  {
    // Relase lock on file then close
    flock(logfile, LOCK_UN | LOCK_NB);
    close(logfile);
  }
}

bool CLogging::open_logfile(string log_file)
{
  if (log_file == "")
  {
    cout << "No logfile specified!\n";
    return false;
  }

  logfile = open (log_file.c_str(), O_WRONLY | O_APPEND );
  
  if (logfile==-1)
  {
    cout << "Error opening logfile! (" << log_file << ")\n";
    return false;
  }
  
  if (flock(logfile, LOCK_EX | LOCK_NB))
  {
    cout << "Failed to lock log file! (already locked by another instance?)\n";
    close(logfile);
    logfile = -1;
    return false;
  }
    
  return true;
}

void CLogging::dbg(string msg)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buf[100];
  string log_msg;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  // format date, e.g. Jun  2 18:47:14 
  strftime (buf,sizeof(buf),"%b %d %H:%M:%S: ",timeinfo);
  
  // Write to log file if open, otherwise output to stdout
  if (logfile > 0)
  {
    log_msg = buf + msg + "\n";
    pthread_mutex_lock(&logfile_mutex);
    write(logfile, log_msg.c_str(), log_msg.length() );
    pthread_mutex_unlock(&logfile_mutex);
  }
  else
    cout << buf << msg << endl;
}