/**
 *  Target interface for plugins. This is a subset of the Target interface
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <pthread.h>
#include "irq_slave.h"

#ifndef PLUGINTARGET_H
#define PLUGINTARGET_H

class PluginTarget {

 private:
  irq_slave *irq;
  pthread_t host_tid;
 public:
  PluginTarget (void);
  ~PluginTarget ();
  void IRQ (uint32_t irq);
  void IRQSet (uint8_t irq);
  void IRQClr (uint8_t irq);
  // For unit testing
  void Exit (int status);
};

#endif /* PLUGINTARGET_H */

