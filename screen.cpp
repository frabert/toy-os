#include "screen.h"

#include <stdint.h>
#include "ports.h"
#include <string.h>

using namespace os;

Screen Screen::m_instance;

void Screen::init() {
  m_instance = Screen();
}

void Screen::move_cursor(uint8_t x, uint8_t y) {
  uint16_t location = y * 80 + x;
  outb(0x3D4, 14);
  outb(0x3D5, location >> 8);
  outb(0x3D4, 15);
  outb(0x3D5, location);
}

void Screen::scroll() {
  // Get a space character with the default colour attributes.
  uint8_t attributeByte = (m_bg << 4) | (m_fg & 0x0F);
  uint16_t blank = 0x20 /* space */ | (attributeByte << 8);

  // Row 25 is the end, this means we need to scroll up
  if(m_cursor_y >= 25) {
    // Move the current text chunk that makes up the screen
    // back in the buffer by a line
    int i;
    for (i = 0*80; i < 24*80; i++) {
      m_video_memory[i] = m_video_memory[i+80];
    }

    // The last line should now be blank. Do this by writing
    // 80 spaces to it.
    for (i = 24*80; i < 25*80; i++) {
      m_video_memory[i] = blank;
    }
    // The cursor should now be on the last line.
    m_cursor_y = 24;
  }
} 

void Screen::clear() {
  // Make an attribute byte for the default colours
  uint8_t attributeByte = (m_bg << 4) | (m_fg & 0x0F);
  uint16_t blank = 0x20 /* space */ | (attributeByte << 8);

  int i;
  for (i = 0; i < 80*25; i++) {
    m_video_memory[i] = blank;
  }

  // Move the hardware cursor back to the start.
  m_cursor_x = 0;
  m_cursor_y = 0;
  move_cursor(m_cursor_x, m_cursor_y);
} 

void Screen::put(char c) {
  // The attribute byte is made up of two nibbles - the lower being the
  // foreground colour, and the upper the background colour.
  uint8_t  attributeByte = (m_bg << 4) | (m_fg & 0x0F);
  // The attribute byte is the top 8 bits of the word we have to send to the
  // VGA board.
  uint16_t attribute = attributeByte << 8;
  uint16_t *location;

  // Handle a backspace, by moving the cursor back one space
  if (c == 0x08 && m_cursor_x) {
    m_cursor_x--;
  } else if (c == 0x09) {
    m_cursor_x = (m_cursor_x+8) & ~(8-1);
  } else if (c == '\r') {
    m_cursor_x = 0;
  } else if (c == '\n') {
    m_cursor_x = 0;
    m_cursor_y++;
   } else if(c >= ' ') {
    location = m_video_memory + (m_cursor_y*80 + m_cursor_x);
    *location = c | attribute;
    m_cursor_x++;
   }

  // Check if we need to insert a new line because we have reached the end
  // of the screen.
  if (m_cursor_x >= 80)
  {
    m_cursor_x = 0;
    m_cursor_y ++;
  }

  // Scroll the screen if needed.
  scroll();
  // Move the hardware cursor.
  move_cursor(m_cursor_x, m_cursor_y);
} 

void Screen::write(const char *c) {
  int i = 0;
  while (c[i])
  {
    Screen::put(c[i++]);
  }
}

void Screen::write_decimal(uint32_t n) {
  if (n == 0) {
    put('0');
    return;
  }

  uint32_t acc = n;
  char c[32];
  int i = 0;
  while (acc > 0) {
    c[i] = '0' + acc%10;
    acc /= 10;
    i++;
  }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0) {
    c2[i--] = c[j++];
  }
  write(c2);
}

void Screen::write_hexadecimal(uint32_t n) {
  uint32_t tmp;
  write("0x");
  char noZeroes = 1;

  for (int i = 28; i > 0; i -= 4) {
    tmp = (n >> i) & 0xF;
    if (tmp == 0 && noZeroes != 0) {
      continue;
    }
    
    if (tmp >= 0xA) {
      noZeroes = 0;
      put(tmp-0xA+'a' );
    } else {
      noZeroes = 0;
      put(tmp+'0' );
    }
  }
  
  tmp = n & 0xF;
  if (tmp >= 0xA) {
    put(tmp-0xA+'a');
  } else {
    put(tmp+'0');
  }
}