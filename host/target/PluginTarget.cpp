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
#include "log.h"

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

uint32_t PluginTarget::ReadW (uint32_t addr)
{
  return flexsoc_reg_read (addr);
}

void PluginTarget::WriteW (uint32_t addr, uint32_t data, uint32_t mask)
{
  uint8_t bval;
  uint16_t hval;
  
  // Decode mask
  switch (mask) {
    // Handle byte accesses
    case 0xff:
      bval = data & 0xff;
      flexsoc_writeb (addr, &bval, 1);
      break;
    case 0xff00:
      bval = (data >> 8) & 0xff;
      flexsoc_writeb (addr + 1, &bval, 1);
      break;
    case 0xff0000:
      bval = (data >> 16) & 0xff;
      flexsoc_writeb (addr + 2, &bval, 1);
      break;
    case 0xff000000:
      bval = (data >> 24) & 0xff;
      flexsoc_writeb (addr + 3, &bval, 1);
      break;
    // Handle hword accesses
    case 0xffff:
      hval = data & 0xffff;
      flexsoc_writeh (addr, &hval, 1);
      break;
    case 0xffff0000:
      hval = (data  >> 16) & 0xffff;
      flexsoc_writeh (addr + 2, &hval, 1);
      break;      
    // Handle word accesses
    case 0xffffffff:
      flexsoc_writew (addr, &data, 1);
      break;
  }
}

void PluginTarget::Log (uint8_t lvl, const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  vlog (lvl, fmt, va);
  va_end (va);
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
