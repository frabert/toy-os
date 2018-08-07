#pragma once

/**
 * \brief Writes a string to the Bochs' console
 * 
 * \param c The string to write
 */
void debug_write(const char *c);

/**
 * \brief Causes Bochs to pause execution
 * 
 */
void debug_break();

/**
 * \brief Prints a stack trace
 * 
 * \param MaxFrames The maximum number of frames to climb
 */
void debug_stacktrace(unsigned int MaxFrames);

#define MAGIC_BREAK asm volatile("xchgw %bx, %bx")