#pragma once
#include <iostream>
#include <time.h>
#include <fstream>
#include <sys/file.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>


class CLogging
{

  public:
    CLogging();
    ~CLogging();
    void dbg(std::string msg);
    void dbg(std::string area, std::string msg);
    bool open_logfile(std::string logfile);
    
  private:
    int logfile;
    pthread_mutex_t logfile_mutex;
    std::string sLogfile;
    
    std::string patLogfile;
    std::string opnLogfile;
    std::string curLogfile();
    
};
