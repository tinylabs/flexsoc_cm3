/**
 *  Circular buffer
 *
 *  All rights reserved.
 *  Tiny Labs Inc.
 *  2020
 */
#include "Cbuf.h"
#include <stdio.h>
#include <string.h>


int Cbuf::SpaceAvail (void)
{
  return full ? 0 : size - ((widx - ridx) % size);
}

int Cbuf::DataAvail (void)
{
  return size - SpaceAvail();
}

int Cbuf::Write (const uint8_t *buf, int len)
{
  int written = 0, sz;

  // Lock mutex
  pthread_mutex_lock (&lock);

  // If full then return
  if (!SpaceAvail ())
    pthread_cond_wait (&space_avail, &lock);

  // Calculate size to copy
  if (ridx > widx)
    sz = (widx + len) < ridx ? len : ridx - widx;
  else
    sz = (widx + len) < size ? len : size - widx;
  
  // Copy to buffer
  memcpy (&_buf[widx], &buf[written], sz);
  
  // Increment pointers
  widx += sz;
  written += sz;
  
  // Handle wrap around
  if (widx >= size)
    widx = 0;
  
  // Check if full
  if (widx == ridx) {
    printf ("full=1\n");
    full = 1;
  }

  // Set data available
  if (written)
    pthread_cond_signal (&data_avail);

  // Release mutex
  pthread_mutex_unlock (&lock);
  return written;
}

int Cbuf::Read (uint8_t *buf, int len)
{
  int read = 0, sz;

  // Lock mutex
  pthread_mutex_lock (&lock);

  // Check data
  if (!DataAvail ())
    pthread_cond_wait (&data_avail, &lock);
  
  // Calculate size to copy
  if (ridx < widx)
    sz = (ridx + len) < widx ? len : widx - ridx;
  else
    sz = (ridx + len) < size ? len : size - ridx;
  
  // Copy to buffer
  memcpy (&buf[read], &_buf[ridx], sz);

  // Increment pointers
  ridx += sz;
  read += sz;
  
  // Handle wrap around
  if (ridx >= size)
    ridx = 0;
  
  // Set data available
  if (read)
    full = 0;

  // Check if more data available
  if (full || (ridx != widx))
    pthread_cond_signal (&space_avail);

  // Release mutex
  pthread_mutex_unlock (&lock);
  return read;
}
