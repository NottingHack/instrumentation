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

string CLogging::curLogfile()
{
  time_t rawtime;
  struct tm * timeinfo;
  char buf[100];
  string log_msg;
  
  time (&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (buf,sizeof(buf),patLogfile.c_str(),timeinfo);  
  
  return (string)buf;
}

bool CLogging::open_logfile(string log_file)
{
  string filename;
  
  if (log_file == "")
  {
    cout << "No logfile specified!\n";
    return false;
  }
  
  patLogfile = log_file;
  filename = curLogfile();

  logfile = open (filename.c_str(), O_WRONLY | O_APPEND );
  
  if (logfile==-1) // maybe the file doesn't exist yet. try creating it.
    logfile = open (filename.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE | S_IRGRP |S_IROTH);    
  
  if (logfile==-1)
  {
    cout << "Error opening logfile! (" << filename << ")\n";
    return false;
  }
  
  if (flock(logfile, LOCK_EX | LOCK_NB))
  {
    cout << "Failed to lock log file! (already locked by another instance?)\n";
    close(logfile);
    logfile = -1;
    return false;
  }
  
  opnLogfile = filename;
  return true;
}

void CLogging::dbg(string msg)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buf[100];
  string log_msg;
  
  int fdnewLogfile;
  string strnewLogfile;
  
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );  
  
  // See if we should be changing logfiles (e.g. date change since last write)
  strnewLogfile = curLogfile();
  pthread_mutex_lock(&logfile_mutex);
  if ((opnLogfile != strnewLogfile) && (logfile > 0))
  {
    strftime (buf,sizeof(buf),"%b %d %H:%M:%S: ",timeinfo);
    log_msg = buf + (string)"Attempting to switch logfile to [" + curLogfile() + "]\n";
    
    write(logfile, log_msg.c_str(), log_msg.length() );
    
    // Open new logfile
    fdnewLogfile = open (strnewLogfile.c_str(), O_WRONLY | O_APPEND );
    
    if (fdnewLogfile==-1) // maybe the file doesn't exist yet. try creating it.
      fdnewLogfile = open (strnewLogfile.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE | S_IRGRP |S_IROTH);    
    
    if (fdnewLogfile > 0)
    { 
      if (flock(fdnewLogfile, LOCK_EX | LOCK_NB))
      {
        close(fdnewLogfile);
      } else
      {
        // new logfile open & locked - so close old log file 
        flock(logfile, LOCK_UN | LOCK_NB);
        close(logfile);
        logfile = fdnewLogfile;
        opnLogfile = strnewLogfile;
        
      }
    }    
  }
  pthread_mutex_unlock(&logfile_mutex); 

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