/**
 *  Target wrapper to access specific CSRs from com link
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <unistd.h>

#include "Target.h"
#include "flexsoc.h"
#include "hwreg.h"
#include "err.h"

// Singleton pointer
Target *Target::inst = NULL;

Target::Target (char *id)
{
  int rv;
  rv = flexsoc_open (id);
  if (rv)
    err ("Failed to open: %s", id);

  // Create autogen CSR classes
  irq = new irq_slave (IRQ_BASE, &flexsoc_reg_read, &flexsoc_reg_write);
  if (!irq)
    err ("Failed to inst irq_slave");
  csr = new flexsoc_csr (CSR_BASE, &flexsoc_reg_read, &flexsoc_reg_write);
  if (!csr)
    err ("Failed to inst flexsoc_csr");
}

Target::~Target ()
{
  // Disable slave interface
  //SlaveEn (false);
  //SlaveUnregister ();

  // Delete CSR classes
  delete irq;
  delete csr;
    
  // Close comm link
  flexsoc_close ();
}

Target *Target::Ptr (char *id)
{
  if (!inst) 
    inst =  new Target (id);
  return inst;
}

Target *Target::Ptr (void)
{
  return inst;
}

// General APIs
void Target::ReadW (uint32_t addr, uint32_t *data, uint32_t cnt)
{
  if (flexsoc_readw (addr, data, cnt))
    err ("flexsoc_readw failed!");
}

void Target::ReadH (uint32_t addr, uint16_t *data, uint32_t cnt)
{
  if (flexsoc_readh (addr, data, cnt))
    err ("flexsoc_readh failed!");
}

void Target::ReadB (uint32_t addr, uint8_t *data, uint32_t cnt)
{
  if (flexsoc_readb (addr, data, cnt))
    err ("flexsoc_readb failed!");
}

void Target::WriteW (uint32_t addr, const uint32_t *data, uint32_t cnt)
{
  if (flexsoc_writew (addr, data, cnt))
    err ("flexsoc_writew failed!");
}

void Target::WriteH (uint32_t addr, const uint16_t *data, uint32_t cnt)
{
  if (flexsoc_writeh (addr, data, cnt))
    err ("flexsoc_writeh failed!");
}

void Target::WriteB (uint32_t addr, const uint8_t *data, uint32_t cnt)
{
  if (flexsoc_writeb (addr, data, cnt))
    err ("flexsoc_writeb failed!");
}

uint32_t Target::ReadReg (uint32_t addr)
{
  return flexsoc_reg_read (addr);
}

void Target::WriteReg (uint32_t addr, uint32_t val)
{
  flexsoc_reg_write (addr, val);
}

void Target::SlaveRegister (void (*cb)(uint8_t *data, int len))
{
  flexsoc_register (cb);
}

void Target::SlaveUnregister (void)
{
  // Disable interface first
  SlaveEn (false);
  flexsoc_unregister ();
}

void Target::SlaveSend (const uint8_t *data, int len)
{
  flexsoc_send (data, len);
}

// Access IRQs
void Target::IRQ (uint8_t n)
{
  irq->edge (n >> 5, 1 << (n & 0x1f));
}

void Target::IRQSet (uint8_t n)
{
  irq->level (n >> 5, 1 << (n & 0x1f));
}

void Target::IRQClr (uint8_t n)
{
  uint32_t cur = irq->level (n >> 5);
  cur &= ~(1 << (n & 0x1f));
  irq->level (n >> 5, cur);
}

// Access CSRs
uint32_t Target::FlexsocID (void)
{
  return csr->flexsoc_id ();
}

uint32_t Target::MemoryID (void)
{
  return csr->memory_id ();
}

void Target::CPUReset (bool reset)
{
  csr->cpu_reset (reset);
}

bool Target::CPUReset (void)
{
  return csr->cpu_reset ();
}

void Target::SlaveEn (bool en)
{
  csr->slave_en (en);
}

bool Target::SlaveEn (void)
{
  return csr->slave_en ();
}


