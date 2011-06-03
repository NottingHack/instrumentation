#pragma once
#include <iostream>
#include <time.h>
#include <fstream>

using namespace std;

class CLogging
{

  public:
    CLogging();
    ~CLogging();
    void dbg(string msg);
    bool open_logfile(string logfile);
    
  private:
    ofstream logfile;
    
};
