/**
 *  Transport layer for host daemon. Abstracts TCP/FT2232H transport differences.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */
#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <pthread.h>

class Transport {
 protected:
  pthread_mutex_t rlock, wlock;
  
 public:
  Transport (void) {
    pthread_mutex_init (&rlock, NULL);
    pthread_mutex_init (&wlock, NULL);
  }
  ~Transport () {}

  // Interface to be met
  virtual int Open (char *id) = 0;
  virtual void Close (void) = 0;
  virtual int Read (char *buf, int len) = 0;
  virtual int Write (const char *buf, int len) = 0;
  virtual void Flush (void) = 0;
};

#endif /* TRANSPORT_H */

