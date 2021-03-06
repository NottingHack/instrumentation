#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <mysql/mysql.h>

#include "CDBValue.h"


using namespace std;



CDBValue::CDBValue()
{
  _val_set = false;
  _null = true;
}

CDBValue::CDBValue(MYSQL_BIND *bind)
{
  char *buf      =   (char* )bind->buffer;
  int lng        = *((int*  )bind->buffer); /* MySQL long = C++ int */
  int flt        = *((float*)bind->buffer);
  MYSQL_TIME tim = *((MYSQL_TIME*)bind->buffer);

  if (*bind->is_null)
  {
    _null = true;
  } else
  {
    _null = false;
    _val_set = true;
    switch (bind->buffer_type)
    {
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
        _val_type = VAL_TYPE_VARCHAR;
        _strVal = buf;
        break;

      case MYSQL_TYPE_LONG:
        _val_type = VAL_TYPE_INT;
        _intVal = lng;
        break;

      case MYSQL_TYPE_FLOAT:
        _val_type = VAL_TYPE_FLOAT;
        _fltVal = flt;
        break;

      case MYSQL_TYPE_DATETIME:
        _val_type = VAL_TYPE_DATETIME;
        mysql_time2timet(&tim, &_timVal);
        break;

      default:
        _val_set = false;
    }
  }
}

CDBValue::~CDBValue()
{
}

string CDBValue::asStr()
{
  ostringstream ss;

  if (!_val_set)
    return "<NOVAL>";
  else if (_null)
    return "<NULL>";

  switch (_val_type)
  {
    case VAL_TYPE_VARCHAR:
      return _strVal;
      break;

    case VAL_TYPE_INT:
      return itos(_intVal);
      break;

    case VAL_TYPE_FLOAT:
      ss << _fltVal;
      return ss.str();
      break;

    case VAL_TYPE_DATETIME:
    {
      char outstr[200];
      struct tm *tmp = localtime(&_timVal);
      if (tmp == NULL)
        return "<TIME:ERR>";

      if (strftime(outstr, sizeof(outstr), "%a, %d %b %y %T %z", tmp) == 0)
        return "<TIME:ERR>";
      return outstr;
    }
      break;

    default:
      break;
  }

  return "<ERR>";
}

int CDBValue::asInt()
{

  if ((!_val_set) or (_null))
    return 0;

  switch (_val_type)
  {
    case VAL_TYPE_VARCHAR:
      return atoi(_strVal.c_str());
      break;

    case VAL_TYPE_INT:
      return _intVal;
      break;

    case VAL_TYPE_FLOAT:
      return _fltVal;
      break;

    case VAL_TYPE_DATETIME:
      return _timVal;
      break;

    default:
      break;
  }

  return 0;
}

float CDBValue::asFloat()
{
  if ((!_val_set) or (_null))
    return 0;

  switch (_val_type)
  {
    case VAL_TYPE_VARCHAR:
      return atof(_strVal.c_str());
      break;

    case VAL_TYPE_INT:
      return _intVal;
      break;

    case VAL_TYPE_FLOAT:
      return _fltVal;
      break;

    case VAL_TYPE_DATETIME:
      return _timVal;
      break;

    default:
      break;
  }

  return 0;
}

time_t CDBValue::asTime()
{
  if ((!_val_set) or (_null))
    return 0;

  switch (_val_type)
  {
    case VAL_TYPE_DATETIME:
      return _timVal;
      break;

    default:
      return 0;
      break;
  }
}

// Convert MYSQL_TIME to time_t
void CDBValue::mysql_time2timet(MYSQL_TIME *my_time, time_t *c_time)
{
  struct tm t;

  t.tm_sec  = my_time->second;
  t.tm_min  = my_time->minute;
  t.tm_hour = my_time->hour;
  t.tm_mday = my_time->day;
  t.tm_mon  = my_time->month - 1; // mysql is 1-12, c is 0-11
  t.tm_year = my_time->year - 1900;

  *c_time = timegm(&t);
}

bool CDBValue::isNull()
{
  if (_val_set && !_null)
    return false;
  else
    return true;
}

void CDBValue::print()
{
  if (_val_set)
  {
    cout << _strVal;

  } else
    cout << "NOVAL";
}

string CDBValue::itos(long n)
{
  string s;
  stringstream out;
  out << n;
  return out.str();
}
