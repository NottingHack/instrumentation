#pragma once
#include <iostream>
#include <time.h>
#include <fstream>
#include <sys/file.h>
#include <pthread.h>

using namespace std;

class CLogging
{

  public:
    CLogging();
    ~CLogging();
    void dbg(string msg);
    bool open_logfile(string logfile);
    
  private:
    int logfile;
    pthread_mutex_t logfile_mutex;
    
};
