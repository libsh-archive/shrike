// -*- C++ -*-
#ifndef TIMER_H
#define TIMER_H

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif /* WIN32 */

class ShTimer
  {
  public:
    ShTimer(void);
    ~ShTimer(void);

    float value(void);

    static ShTimer now(void);
    static ShTimer zero(void);

    friend ShTimer operator-(const ShTimer& a, const ShTimer& b);
    friend ShTimer operator+(const ShTimer& a, const ShTimer& b);

  private:
#ifdef WIN32
	LARGE_INTEGER t;
#else
    struct timeval t;
#endif /* WIN32 */
  };

#endif /* TIMER_H */
