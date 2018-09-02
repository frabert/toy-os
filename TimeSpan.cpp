#include "time.h"

#include <stdint.h>
#include <stddef.h>
#include <kassert.h>

using namespace os::Time;

static bool div64by32eq64(uint64_t dividend, uint32_t divisor, uint64_t* res)
{
  if (divisor == 0)
    return false;

  *res = 0;
  while(dividend >= divisor) {
    (*res) = (*res) + 1;
    dividend -= divisor;
  }

  return true;
}


TimeSpan os::Time::operator+(const TimeSpan& a, const TimeSpan& b) {
  return TimeSpan{a.nanoseconds() + b.nanoseconds()};
}
TimeSpan os::Time::operator-(const TimeSpan& a, const TimeSpan& b) {
  return TimeSpan(a.nanoseconds() - b.nanoseconds());
}
TimeSpan os::Time::operator*(const TimeSpan& a, uint64_t v) {
  return TimeSpan(a.nanoseconds() * v);
}
TimeSpan os::Time::operator/(const TimeSpan& a, uint32_t v) {
  uint64_t d = a.nanoseconds();
  uint64_t res;
  if(div64by32eq64(d, v, &res)) return TimeSpan(res);
  else panic("Division by 0");
}

bool os::Time::operator>(const TimeSpan& a, const TimeSpan& b) {
  return a.nanoseconds() > b.nanoseconds();
}
bool os::Time::operator<(const TimeSpan& a, const TimeSpan& b) {  
  return b > a;
}
bool os::Time::operator>=(const TimeSpan& a, const TimeSpan& b) {
  return !(a < b);
}
bool os::Time::operator<=(const TimeSpan& a, const TimeSpan& b) {
  return !(a > b);
}

TimeSpan& TimeSpan::operator+=(const TimeSpan& a) {
  m_nanos += a.m_nanos;
  return *this;
}

TimeSpan& TimeSpan::operator-=(const TimeSpan& a) {
  m_nanos -= a.m_nanos;
  return *this;
}

TimeSpan& TimeSpan::operator*=(uint64_t a) {
  m_nanos *= a;
  return *this;
}

TimeSpan& TimeSpan::operator/=(uint32_t a) {
  if(div64by32eq64(m_nanos, a, &m_nanos)) return *this;
  else panic("Division by 0");
}