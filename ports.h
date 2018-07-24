#pragma once

#include <stdint.h>

template<typename T>
inline void port_write(uint16_t port, T value);

template<>
inline void port_write(uint16_t port, uint8_t value) {
  asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

template<>
inline void port_write(uint16_t port, uint16_t value) {
  asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

template<>
inline void port_write(uint16_t port, uint32_t value) {
  asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

inline uint8_t port_read8(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

inline uint16_t port_read16(uint16_t port) {
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

inline uint32_t port_read32(uint16_t port) {
  uint32_t ret;
  asm volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}