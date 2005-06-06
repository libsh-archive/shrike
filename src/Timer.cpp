// Sh: A GPU metaprogramming language.
//
// Copyright 2003-2005 Serious Hack Inc.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must
// not claim that you wrote the original software. If you use this
// software in a product, an acknowledgment in the product documentation
// would be appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must
// not be misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
//////////////////////////////////////////////////////////////////////////////
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
  return ((float)t.QuadPart)/((float)freq.QuadPart/1000.0);
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
  if (ret.t.tv_usec < 0) {
    ret.t.tv_sec -= 1;
    ret.t.tv_usec += 1000000;
  }

  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec > 1000000) {
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
  if (ret.t.tv_usec < 0) {
    ret.t.tv_sec -= 1;
    ret.t.tv_usec += 1000000;
  }
  
  // adjust microsecond if the value is out of range
  if (ret.t.tv_usec > 1000000) {
    ret.t.tv_sec += 1;
    ret.t.tv_usec -= 1000000;
  }
  
  return ret;
}
#endif /* WIN32 */
