/**
 *   Circular buffer for multithread read/write access. Buffer will block when full or empty
 *
 *   All rights reserved.
 *   Tiny Labs Inc
 *   2020
 */

#ifndef CBUF_H
#define CBUF_H

#include <pthread.h>
#include <stdint.h>
#include "err.h"

class Cbuf {

 private:
  pthread_mutex_t lock;
  unsigned int widx, ridx, size;
  bool full;
  uint8_t *_buf;
  pthread_cond_t space_avail, data_avail;
  int SpaceAvail (void);
  int DataAvail (void);
  
public:
  int Write (const uint8_t *buf, int len);
  int Read (uint8_t *buf, int len);
  
  Cbuf (int size) {
    int rv;
    
    this->size = size;
    widx = ridx = 0;
    full = false;
    _buf = (uint8_t *)malloc (size);
    if (!_buf)
      err ("Failed to malloc cbuf");

    // init conditions
    space_avail = data_avail = PTHREAD_COND_INITIALIZER;
    
    // Init lock
    pthread_mutex_init (&lock, NULL);
  }
  ~Cbuf () {}
};

#endif /* CBUF_H */

