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
  // IRQ access
  void IRQ (uint32_t irq);
  void IRQSet (uint8_t irq);
  void IRQClr (uint8_t irq);
  // Memory space access
  uint32_t ReadW (uint32_t addr);
  void WriteW (uint32_t addr, uint32_t data, uint32_t mask);
  // Log data
  void Log (uint8_t lvl, const char *fmt, ...);
  // For unit testing
  void Exit (int status);
};

#endif /* PLUGINTARGET_H */

