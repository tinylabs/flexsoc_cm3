/**
 *  Target interface for plugins. This is a subset of the Target interface
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <signal.h>
#include <unistd.h>

#include "PluginTarget.h"

#include "hwreg.h"
#include "flexsoc.h"
#include "err.h"

PluginTarget::PluginTarget (void)
{
  irq = new irq_slave (IRQ_BASE, &flexsoc_reg_read, &flexsoc_reg_write);
  if (!irq)
    err ("Failed to inst irq_slave");

  // Save host tid
  host_tid = pthread_self ();
}

PluginTarget::~PluginTarget ()
{
  delete irq;
}

void PluginTarget::IRQ (uint32_t n)
{
  irq->edge (n >> 5, 1 << (n & 0x1f));
}

void PluginTarget::IRQSet (uint8_t n)
{
  irq->level (n >> 5, 1 << (n & 0x1f));
}

void PluginTarget::IRQClr (uint8_t n)
{
  uint32_t cur = irq->level (n >> 5);
  cur &= ~(1 << (n & 0x1f));
    irq->level (n >> 5, cur);
}

void PluginTarget::Exit (int status)
{
  // Set return code
  flexsoc_write_returnval (status);

  // Send host interrupt to stop
  pthread_kill (host_tid, SIGINT);
}
