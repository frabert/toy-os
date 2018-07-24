#pragma once

#include <stdint.h>

namespace os {
  class Screen {
  public:
    enum class Color : uint8_t {
      Black   = 0x0,
      Blue    = 0x1,
      Green   = 0x2,
      Cyan    = 0x3,
      Red     = 0x4,
      Magenta = 0x5,
      Brown   = 0x6,
      Gray    = 0x7,
      DarkGray      = 0x8,
      BrightBlue    = 0x9,
      BrightGreen   = 0xA,
      BrightCyan    = 0xB,
      BrightRed     = 0xC,
      BrightMagenta = 0xD,
      Yellow        = 0xE,
      White         = 0xF
    };

    void clear();

    void write_decimal(uint32_t i);
    void write_hexadecimal(uint32_t i);

    void write(const char *c);

    void put(char c);

    void write(uint32_t v) {
      write_decimal(v);
    }

    void write(const void* v) {
      write_hexadecimal((uint32_t)v);
    }

    template<typename T, typename... Targs>
    void write(const char *format, T a, Targs... b) {
      while(*format) {
        if(*format == '%') {
          format++;

          if(*format == '%') {
            put('%');
            format++;
            continue;
          }
          
          write(a);
          write(format, b...);
          break;
        } else {
          put(*format);
          format++;
        }
      }
    }

    void setColors(Color foreground, Color background) {
      m_fg = static_cast<uint8_t>(foreground);
      m_bg = static_cast<uint8_t>(background);
    }
    
    static Screen& getInstance() { return m_instance; }

    static void init();

  private:
    Screen()
      : m_video_memory((uint16_t*)0xB8000)
      , m_cursor_x(0)
      , m_cursor_y(0)
      , m_fg(0x7)
      , m_bg(0x0) {}
    uint16_t *m_video_memory;
    uint8_t m_cursor_x;
    uint8_t m_cursor_y;
    uint8_t m_fg, m_bg;
    void scroll();
    void move_cursor(uint8_t x, uint8_t y);

    static Screen m_instance;
  };
}