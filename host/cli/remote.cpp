/**
 *  API to connect to remote target via SWD/JTAG bridge
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <unistd.h>

#include "Target.h"
#include "remote.h"
#include "log.h"


// CSW config word
#define CSW_CFG_WORD 0xA2000002
#define CSW_CFG_BYTE 0xA2000000
#define DEBUG_HALT   0xA05F0003

// Address to halt
#define SCB_DHCSR    0xE000EDF0

// DP regs
#define DP_CTLSTAT  4

// AP Regs
#define AP_CSW   0
#define AP_TAR   4
#define AP_DRW   0xc
#define AP_IDR   0xfc

static const char *jedec_manufacturer (uint8_t id)
{
  switch (id) {
    case 0x3b: return "ARM";
    default: return "UNKNOWN";
  }
}

static const char *debugport_id (uint16_t id)
{
  switch (id) {
    case 0xBA00: return "JTAG-DP";
    case 0xBA01: case 0xBA02: return "SW-DP";
    default: return "xx-xx";
  }
}

static const char *ap_idcode (uint8_t id)
{
  switch (id) {
    case 0: return "JTAG-AP";
    case 1: return "AHB-";
    case 2: return "APB-";
    case 3: return "AXI-";
    default: return "";
  }
}

int remote_open (uint8_t div, bool halt)
{
  uint32_t idcode;
  uint8_t apsel = 255;
  int timeout = 100, i, n;
  float freqMHz;
  
  // Get target pointer
  Target *targ = Target::Ptr ();

  // Calculate frequency
  freqMHz = (float)(targ->CoreFreq() / ((div + 1) * 2)) / 1000000;

  // Set clock divisor
  targ->RemoteClkDiv (div);
  
  // Enable remote interface
  targ->RemoteEn (true);

  // Poll IDcode until valid
  n = timeout;
  do {
    idcode = targ->RemoteIDCODE ();
    n--;
  } while ((idcode == 0) && n);
  
  // Check status
  if (!n || targ->RemoteStat ()) {
    log (LOG_ERR, "Remote connect failed: %s", targ->RemoteStatStr ());
    return -1;
  }
  
  // Read IDcode of DP to check if valid
  log (LOG_NORMAL, "Remote connected - %s %s DPv%d [%08X] @ %.03fMHz",
       jedec_manufacturer ((idcode >> 1) & 0xff),
       debugport_id ((idcode >> 12) & 0xffff),
       (idcode >> 12) & 0xf,
       idcode, freqMHz);

  // Enable debugger
  log_nonl (LOG_DEBUG, "  Enable debug... ");
  targ->RemoteRegWrite (false, DP_CTLSTAT, 0x50000000); n = timeout;
  while ((targ->RemoteRegRead (false, DP_CTLSTAT) != 0xf0000000) && n)
    n--;
  if (!n) {
    log (LOG_ERR, "remote timeout.");
    return -1;
  }
  log (LOG_DEBUG, "SYS|DBG powered up.");

  // Scan all APs for MEM-AP
  for (i = 0; i < 256; i++) {

    // Get IDR register
    uint32_t idr = targ->RemoteAPRead (AP_IDR, i);
    if (idr == 0)
      break;
    log (LOG_NORMAL, "  [%d] Found: %s %s%s [%08X]", i,
         jedec_manufacturer ((idr >> 17) & 0x7f),
         ap_idcode (idr & 0xf),
         ((idr >> 13) & 0xf) == 0x8 ? "MEMAP" : "",
         idr);

    // Save MEM-AP
    if (((idr >> 13) & 0xf) == 0x8)
      apsel = i;
  }

  // Check if we failed to find MEM-AP
  if (apsel == 256) {
    log (LOG_ERR, "  Failed to find MEM-AP!");
    return -1;
  }

  // Set CSW for word access
  targ->RemoteAPWrite (AP_CSW, CSW_CFG_WORD, apsel);
  
  // Halt processor if requested
  if (halt) {
    log_nonl (LOG_NORMAL, "  Halt remote CPU... ");

    // Write SCB_DHCSR to TAR
    targ->RemoteAPWrite (AP_TAR, SCB_DHCSR);

    // Try and halt processor
    n = timeout;
    do {
      // Write key + halt to DHCSR
      targ->RemoteAPWrite (AP_DRW, DEBUG_HALT);
      n--;
    } while ((targ->RemoteAPRead (AP_DRW) & (1 << 17) == 0) && n);

    // Check if we succeeded
    if (!n) {
      log (LOG_NORMAL, "timed out.");
      return -1;
    }
    log (LOG_NORMAL, "OK.");
  }
  else
    log (LOG_NORMAL, "  Remote CPU left running...");

  // Check if byte/hwrd access is supported
  targ->RemoteAPWrite (AP_CSW, CSW_CFG_BYTE);
  if ((targ->RemoteAPRead (AP_CSW) & 3) != 0) {
    log (LOG_NORMAL, "  -- BYTE/HWRD access not supported on remote.");
    log (LOG_NORMAL, "  -- AHB bridge will not work for these accesses");
  }
  
  // Set mem-ap for remote bridge
  targ->RemoteAHBAP (apsel);
  
  // Success
  return 0;
}
  
void remote_close (void)
{
  // Get target pointer
  Target *targ = Target::Ptr ();

  // Disable bridge
  targ->RemoteAHBEn (false);

  // Disable remote interface
  targ->RemoteEn (false);
}

