#pragma once

#include <iostream>
#include <string>
#include <stdlib.h>
#include "CLogging.h"

#define VAL_TYPE_INT 1
#define VAL_TYPE_VARCHAR 2
#define VAL_TYPE_FLOAT 3
#define VAL_TYPE_TEXT 4


class CDBValue
{
  public:
    CDBValue(MYSQL_BIND *bind);
    CDBValue();
    ~CDBValue();

    std::string asStr();
    float asFloat();
    bool isNull();
    int asInt();
    void print();
    
    operator std::string  () {return asStr();};
    operator int          () {return asInt();};

  private:
    MYSQL_BIND  _bind;
    bool        _val_set;
    bool        _null;
    short       _val_type;

    std::string _strVal;
    int         _intVal;
    float       _fltVal;

    std::string itos(long n);
};
