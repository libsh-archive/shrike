// -*- C++ -*-
#include "Timer.hpp"

#ifdef WIN32
ShTimer::ShTimer(void)
  {
  t.QuadPart = 0;
  }

ShTimer::~ShTimer(void)
  {
  }

float ShTimer::value(void)
  {
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return (float)(t.QuadPart)/(freq.QuadPart/1000);
  }

ShTimer ShTimer::now(void)
  {
  ShTimer ret;
  QueryPerformanceCounter(&ret.t);
  return ret;
  }

ShTimer ShTimer::zero(void)
  {
  ShTimer ret;
  ret.t.QuadPart = 0;
  return ret;
  }

ShTimer operator-(const ShTimer& a, const ShTimer& b)
  {
  ShTimer ret;
  ret.t.QuadPart = a.t.QuadPart + b.t.QuadPart;
  return ret;
  }

ShTimer operator+(const ShTimer& a, const ShTimer& b)
  {
  ShTimer ret;
  ret.t.QuadPart = a.t.QuadPart - b.t.QuadPart;
  return ret;
  }

#else

ShTimer::ShTimer(void)
  {
  t.tv_sec = 0;
  t.tv_usec = 0;
  }

ShTimer::~ShTimer(void)
  {
  }

float ShTimer::value(void)
  {
  float sec = (float)t.tv_sec*1000;
  float msec = (float)t.tv_usec/1000;
  return (sec + msec);
  }

ShTimer ShTimer::now(void)
  {
  ShTimer ret;
  gettimeofday(&ret.t, 0);
  return ret;
  }

ShTimer ShTimer::zero(void)
  {
  ShTimer ret;
  ret.t.tv_sec = 0;
  ret.t.tv_usec = 0;
  return ret;
  }

ShTimer operator-(const ShTimer& a, const ShTimer& b)
  {
  ShTimer ret;
  
  ret.t.tv_sec = a.t.tv_sec - b.t.tv_sec;
  ret.t.tv_usec = a.t.tv_usec - b.t.tv_usec;

  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec < 0)
    {
    ret.t.tv_sec -= 1;
    ret.t.tv_usec += 1000000;
    }

  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec > 1000000)
    {
    ret.t.tv_sec += 1;
    ret.t.tv_usec -= 1000000;
    }

  return ret;
  }

ShTimer operator+(const ShTimer& a, const ShTimer& b)
  {
  ShTimer ret;

  ret.t.tv_sec = a.t.tv_sec + b.t.tv_sec;
  ret.t.tv_usec = a.t.tv_usec + b.t.tv_usec;

  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec < 0)
    {
    ret.t.tv_sec -= 1;
    ret.t.tv_usec += 1000000;
    }

  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec > 1000000)
    {
    ret.t.tv_sec += 1;
    ret.t.tv_usec -= 1000000;
    }

  return ret;
  }
#endif /* WIN32 */
