#pragma once
#include <iostream>
#include <time.h>

using namespace std;

class CLogging
{

  public:
    CLogging(string logfile);
    void dbg(string msg);
    
};
