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
#include "log.h"

// Singleton pointer
Target *Target::inst = NULL;

Target::Target (char *id)
{
  int rv;
  rv = flexsoc_open (id);
  if (rv)
    err ("Failed to open: %s", id);

  // Create autogen CSR classes
  csr = new flexsoc_csr (CSR_BASE, &flexsoc_reg_read, &flexsoc_reg_write);
  if (!csr)
    err ("Failed to inst flexsoc_csr");
}

Target::~Target ()
{
  // Delete CSR classes
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
  csr->irq_edge (n >> 5, 1 << (n & 0x1f));
}

void Target::IRQSet (uint8_t n)
{
  csr->irq_level (n >> 5, 1 << (n & 0x1f));
}

void Target::IRQClr (uint8_t n)
{
  uint32_t cur = csr->irq_level (n >> 5);
  cur &= ~(1 << (n & 0x1f));
  csr->irq_level (n >> 5, cur);
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

uint32_t Target::CoreFreq (void)
{
  return csr->core_freq ();
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


int Target::CodeAliasSet (uint8_t idx, alias_t *alias)
{
  if (!alias || (idx > 1))
    return -1;

  // Setup alias
  csr->code_remap_base (idx, alias->base);
  csr->code_remap_end (idx, alias->base + alias->size);
  csr->code_remap_off (idx, alias->remap);
  return 0;
}

int Target::SysAliasSet (uint8_t idx, alias_t *alias)
{
  if (!alias || (idx > 0))
    return -1;

  // Setup alias
  csr->sys_remap_base (alias->base);
  csr->sys_remap_end (alias->base + alias->size);
  csr->sys_remap_off (alias->remap);
  return 0;
}

int Target::CodeAliasGet (uint8_t idx, alias_t *alias)
{
  if (!alias || (idx > 1))
    return -1;

  // Get values
  alias->base = csr->code_remap_base (idx);
  alias->size = csr->code_remap_end (idx) - csr->code_remap_base (idx);
  alias->remap = csr->code_remap_off (idx);
  return 0;
}

int Target::SysAliasGet (uint8_t idx, alias_t *alias)
{
  if (!alias || (idx > 0))
    return -1;

  // Get values
  alias->base = csr->sys_remap_base ();
  alias->size = csr->sys_remap_end () - csr->sys_remap_base ();
  alias->remap = csr->sys_remap_off ();
  return 0;
}

uint32_t Target::RemoteBase (void)
{
  return csr->brg_base ();
}

uint8_t Target::RemoteStat (void)
{
  return csr->brg_stat ();
}

const char *Target::RemoteStatStr (void)
{
  switch (csr->brg_stat ()) {
    case SUCCESS: return "SUCCESS";
    case ERR_FAULT: return "ERR_FAULT";
    case ERR_PARITY: return "ERR_PARITY";
    case ERR_TIMEOUT: return "ERR_TIMEOUT";
    case ERR_NOMEMAP: return "ERR_NOMEMAP";
    case ERR_UNSUPSZ: return "ERR_UNSUPSZ";
    default: return "ERR_UNKNOWN";
  }
}

void Target::RemoteTimeout (int timeout)
{
  remote_timeout = timeout;
}

void Target::RemoteEn (bool en)
{
  csr->brg_en (en);
}

bool Target::RemoteEn (void)
{
  return csr->brg_en ();
}

void Target::RemoteClkDiv (uint8_t div)
{
  csr->brg_clkdiv (div & 0x1f);
}

uint8_t Target::RemoteClkDiv (void)
{
  return csr->brg_clkdiv ();
}

uint32_t Target::RemoteIDCODE (void)
{
  return csr->brg_idcode ();
}

uint32_t Target::RemoteRegRead (bool APnDP, uint8_t addr)
{
  int n = remote_timeout;
  
  // CTRL = APnDP, ADDR[1:0], WRnRD, START=1/DONE=0
  if (APnDP)
    csr->brg_ctrl ((1 << 4) | (addr & 0xc) | 1);
  else
    csr->brg_ctrl ((addr & 0xc) | 1);

  // Wait until command is complete
  while ((csr->brg_ctrl () & 1) && n)
    n--;
  
  // Return data read
  return n ? csr->brg_data () : 0;
}

void Target::RemoteRegWrite (bool APnDP, uint8_t addr, uint32_t data)
{
  int n = remote_timeout;

  // Setup data
  csr->brg_data (data);
  
  // CTRL = APnDP, ADDR[1:0], WRnRD, START=1/DONE=0
  if (APnDP)
    csr->brg_ctrl ((1 << 4) | (addr & 0xc) | 3);
  else
    csr->brg_ctrl ((addr & 0xc) | 3);

  // Wait until command is complete
  while ((csr->brg_ctrl () & 1) && n)
    n--;
}

uint32_t Target::RemoteAPRead (uint8_t addr, uint8_t ap)
{
  // Update AP if specified
  if (ap != 255)
    apsel = ap;
  
  // Select address using DP-select
  RemoteRegWrite (false, 8, (apsel << 24) | addr);

  // Read from AP - Must read RDBUFF after
  (void)RemoteRegRead (true, addr & 0xc);

  // Read buffer (RDBUFF)
  return RemoteRegRead (false, 0xc);
}

void Target::RemoteAPWrite (uint8_t addr, uint32_t data, uint8_t ap)
{
  // Update AP if specified
  if (ap != 255)
    apsel = ap;
  
  // Select address using DP-select
  RemoteRegWrite (false, 8, (apsel << 24) | addr);

  // Write to AP
  RemoteRegWrite (true, addr & 0xc, data);
}

void Target::RemoteAHBAP (uint8_t ap)
{
  // Save AP as AHB bridge AP
  csr->brg_apsel (ap);
}

uint32_t Target::RemoteReadW (uint32_t addr)
{
  // Setup CSW for word access
  RemoteAPWrite (0, 0xA2000002);

  // Setup address in TAR
  RemoteAPWrite (4, addr);

  // Wait for any previous transactions to finish
  while ((RemoteAPRead (0) & 0xC0) != 0x40)
  ;

  // Read from DRW
  return RemoteAPRead (0xc);
}

void Target::RemoteWriteW (uint32_t addr, uint32_t data)
{
  // Setup CSW for word access
  RemoteAPWrite (0, 0xA2000002);

  // Setup address in TAR
  RemoteAPWrite (4, addr);

  // Wait for any previous transactions to finish
  while ((RemoteAPRead (0) & 0xC0) != 0x40)
    ;

  // Write to DRW
  RemoteAPWrite (0xc, data);
}

void Target::RemoteAHBEn (bool en)
{
  csr->brg_ahb_en (en);
}

void Target::RemoteRemap32M (uint8_t idx, uint32_t remap)
{
  uint32_t v;
  
  // Verify only top 4 bits set
  if (remap & 0x01FFFFFF) {
    log (LOG_ERR, "-- remap32[%d] invalid address = 0x%08X", idx, remap);
    log (LOG_ERR, "-- remap32[%d] address mask    = 0xFE000000", idx);
    return;
  }

  // RMW remap reg
  v = csr->brg_remap32 (idx >> 2);
  v &= ~(0xff << ((idx & 3) * 8));
  v |= ((remap >> 24) & 0xfe) << ((idx & 3) * 8);
  
  // Setup remap
  csr->brg_remap32 (idx >> 2, v);
}

uint32_t Target::RemoteRemap32M (uint8_t idx)
{
  return (csr->brg_remap32(idx >> 2) >> ((idx & 3) * 8)) << 24;
}

void Target::RemoteRemap256M (uint32_t remap)
{
  // Verify only top 4 bits set
  if (remap & 0xFFFFFFF) {
    log (LOG_ERR, "-- remap256 invalid address = 0x%08X", remap);
    log (LOG_ERR, "-- remap256 address mask    = 0xF0000000");
    return;
  }
  
  // Set remap
  csr->brg_remap256 (remap >> 28);
}

uint32_t Target::RemoteRemap256M (void)
{
  return csr->brg_remap256 () << 28;
}
