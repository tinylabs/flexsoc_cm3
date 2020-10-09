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
  uint8_t stat, apsel = 255;
  int timeout = 100, i, n;
  float freqMHz;
  
  // Get target pointer
  Target *targ = Target::Ptr ();

 retry_connect:
  if (div > 31) {
    log (LOG_ERR, "Remote connect failed");
    return -1;
  }
  
  // Calculate frequency
  freqMHz = (float)(targ->CoreFreq() / ((div + 1) * 2)) / 1000000;

  // Set clock divisor
  targ->RemoteClkDiv (div);
  
  // Enable remote interface
  targ->RemoteEn (true);

  // Poll for valid IDCODE
  n = timeout;
  do {
    idcode = targ->RemoteIDCODE ();
  } while (--n && (idcode == 0));
  
  // Check status
  stat = targ->RemoteStat ();
  
  // Check if not connected
  if (stat == ERR_NOCONNECT) {
    log (LOG_NORMAL, "-- Remote DebugPort not found! Check connections.");
    goto cleanup;
  }
  
  // If IDcode doesn't look correct retry at lower speed
  if (stat || (((idcode >> 1) & 0xff) != 0x3b)) {
    targ->RemoteEn (false);
    div++;
    goto retry_connect;
  }

  // Read IDcode of DP to check if valid
  log (LOG_NORMAL, "Remote connected - %s %s DPv%d [%08X] @ %.03fMHz",
       jedec_manufacturer ((idcode >> 1) & 0xff),
       debugport_id ((idcode >> 12) & 0xffff),
       (idcode >> 12) & 0xf,
       idcode, freqMHz);

  // Enable debug and overrun detection
  log_nonl (LOG_DEBUG, "  Enable debug... ");
  targ->RemoteRegWrite (false, DP_CTLSTAT, 0x50000000); n = timeout;
  while (((targ->RemoteRegRead (false, DP_CTLSTAT) >> 28) != 0xf) && n)
    n--;
  if (!n) {
    log (LOG_ERR, "remote timeout.");
    goto cleanup;
  }
  log (LOG_DEBUG, "SYS|DBG powered up.");

  // Scan all APs for MEM-AP
  for (i = 0; i < 256; i++) {

    // Get IDR register
    uint32_t idr = targ->RemoteAPRead (AP_IDR, i);
    if (idr == 0)
      break;
    log (LOG_NORMAL, "  [%d] Found AP: %s %s%s [%08X]", i,
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
    goto cleanup;
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
      goto cleanup;
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

    // Set value for state machine
    targ->RemoteCSWFixed (true);
  }
  else // CSW not fixed (WORD/HWRD/BYTE) supported over remote bridge
    targ->RemoteCSWFixed (false);

  // Set mem-ap for remote bridge
  targ->RemoteAHBAP (apsel);
  
  // Success
  return 0;

  // Cleanup
 cleanup:
  // Disable bridge
  targ->RemoteAHBEn (false);
  targ->RemoteEn (false);
  return -1;
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

void remote_dump (uint32_t base)
{
  int i;
  uint8_t reg = 0;
  
  // Get target pointer
  Target *targ = Target::Ptr ();

  // Dump each register
  for (i = 0; i < 0x14; i++) {
    
    // Select register for read (DCRSR)
    targ->WriteReg (base + 0xedf4, i);

    // Wait for it to complete (DHCSR)
    while (targ->ReadReg (base + 0xedf0) & (1 << 16) == 0)
      ;

    // Print reg (DCRDR)
    log (LOG_NORMAL, "[%02X] = %08X", i, targ->ReadReg (base + 0xedf8));
  }
}
