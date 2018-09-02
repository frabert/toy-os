#pragma once

#include <stdint.h>
#include <stddef.h>
#include <shared_ptr.h>

namespace os {
  namespace Tasking {
    class Waitable;
  }

  namespace Time {
    class TimeSpan {
    public:
      constexpr TimeSpan(uint64_t ns) : m_nanos(ns) {}

      constexpr uint64_t nanoseconds() const { return m_nanos; }
      constexpr uint64_t microseconds() const { return m_nanos / 1'000; }
      constexpr uint64_t milliseconds() const { return microseconds() / 1'000; }
      constexpr uint64_t seconds() const { return milliseconds() / 1'000; }
      constexpr uint64_t minutes() const { return seconds() / 60; }
      constexpr uint64_t hours() const { return minutes() / 60; }

      TimeSpan& operator+=(const TimeSpan& a);
      TimeSpan& operator-=(const TimeSpan& a);
      TimeSpan& operator*=(uint64_t a);
      TimeSpan& operator/=(uint32_t a);
    private:
      uint64_t m_nanos;
    };
      
    constexpr TimeSpan operator""_ns(uint64_t v) { return TimeSpan{v}; }
    constexpr TimeSpan operator""_us(uint64_t v) { return TimeSpan{v * 1'000}; }
    constexpr TimeSpan operator""_ms(uint64_t v) { return TimeSpan{v * 1'000'000}; }
    constexpr TimeSpan operator""_s(uint64_t v) { return TimeSpan{v * 1'000'000'000}; }
    constexpr TimeSpan operator""_min(uint64_t v) { return TimeSpan{v * 60'000'000'000}; }
    constexpr TimeSpan operator""_h(uint64_t v) { return TimeSpan{v * 3'600'000'000'000}; }

    TimeSpan operator+(const TimeSpan& a, const TimeSpan& b);
    TimeSpan operator-(const TimeSpan& a, const TimeSpan& b);
    TimeSpan operator*(const TimeSpan& a, uint64_t v);
    TimeSpan operator/(const TimeSpan& a, uint32_t v);

    bool operator>(const TimeSpan& a, const TimeSpan& b);
    bool operator<(const TimeSpan& a, const TimeSpan& b);
    bool operator>=(const TimeSpan& a, const TimeSpan& b);
    bool operator<=(const TimeSpan& a, const TimeSpan& b);

    TimeSpan since_boot();
    void wait_for(TimeSpan duration);
    void wait_until(TimeSpan t);
  }
}