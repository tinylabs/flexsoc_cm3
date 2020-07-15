/**
 *  FTDI transport implementationn
 *
 *  All rights reserved
 *  Tiny Labs Inc
 *  2019
 */
#include "FTDITransport.h"
#include "err.h"

// VID/PID of FT2232H chipset
#define USB_VID  0x0403
#define USB_PID  0x6010

FTDITransport::FTDITransport (void)
  : Transport ()
{
  int rv;
  
  // Create FTDI structure
  ftdi = ftdi_new ();
  if (!ftdi)
    err ("Failed to create FTDI struct!");

  // Set interface to B
  rv = ftdi_set_interface (ftdi, INTERFACE_B);
  if (rv)
    err ("Failed to set interface!");
}

FTDITransport::~FTDITransport ()
{
  if (ftdi)
    ftdi_free (ftdi);
}

int FTDITransport::Open (char *id)
{
  int rv;
  
  // Look for device by serial # if available
  rv = ftdi_usb_open_desc (ftdi, USB_VID, USB_PID, NULL, *id == '0' ? NULL : id);
  if (rv < 0)
    // No devices found
    return -1;
  
  // Purge buffers
  rv = ftdi_usb_purge_buffers (ftdi);
  if (rv)
    err ("Failed to purge FTDI buffers!");

  // Reset controller
  rv = ftdi_set_bitmode (ftdi, 0, BITMODE_RESET);
  if (rv)
    err ("Failed to reset bitmode");
  
  // Decrease latency timer to minimum
  rv = ftdi_set_latency_timer (ftdi, 1);
  if (rv)
    err ("Failed to set latency timer");

  return 0;
}

void FTDITransport::Close (void)
{
  ftdi_usb_close (ftdi);
}

void FTDITransport::Flush (void)
{
  if (ftdi_usb_purge_buffers (ftdi))
    err ("Failed to purge buffers");
}

int FTDITransport::Read (char *buf, int len)
{
  int rv;
  
  rv = ftdi_read_data (ftdi, (unsigned char *)buf, len);
  if (rv < 0)
    err ("FTDI read failed: rv=%d", rv);
  return rv;
}

int FTDITransport::Write (const char *buf, int len)
{
  int rv;
  
  rv = ftdi_write_data (ftdi, (unsigned char *)buf, len);
  if (rv < 0)
    err ("FTDI write failed: rv=%d", rv);
  return rv;
}

