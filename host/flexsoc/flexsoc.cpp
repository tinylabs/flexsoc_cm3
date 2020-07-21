/**
 *  flexsoc communication library.
 *
 *  All rights reserved.
 *  Tiny Labs Inc.
 *  2020
 */
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "TCPTransport.h"
#include "FTDITransport.h"
#include "flexsoc.h"
#include "err.h"

#define DEBUG   1

// Local variables
static Transport *dev = NULL;
static pthread_t tid;

// One outstanding master transaction at a time
static pthread_mutex_t resplock, mlock;
static char *exp_buf;

// Callbacks for plugin interface
static recv_cb_t recv_cb = NULL;

// Must match fifo_host_pkg.sv
typedef enum {
              FIFO_D0  = 0,
              FIFO_D1  = 1,
              FIFO_D2  = 2,
              FIFO_D4  = 3,
              FIFO_D5  = 4,
              FIFO_D6  = 5,
              FIFO_D8  = 6,
              FIFO_D16 = 7
} cmd_payload_t;

// Command interface
#define CMD_INTERFACE_MASTER 0x80
#define CMD_INTERFACE_SLAVE  0x00
#define CMD_PAYLOAD_SHIFT    4
#define CMD_PAYLOAD_MASK     7
#define CMD_PAYLOAD(x)       ((x) << CMD_PAYLOAD_SHIFT)
#define CMD_WRITE            0x8
#define CMD_READ             0x0
#define CMD_AUTOINC          0x4
#define CMD_WIDTH(x)         ((x) >> 1)

static uint8_t payload2cmd (uint8_t len)
{
  switch (len) {
    case 0:  return CMD_PAYLOAD (FIFO_D0);
    case 1:  return CMD_PAYLOAD (FIFO_D1);
    case 2:  return CMD_PAYLOAD (FIFO_D2);
    case 4:  return CMD_PAYLOAD (FIFO_D4);
    case 5:  return CMD_PAYLOAD (FIFO_D5);
    case 6:  return CMD_PAYLOAD (FIFO_D6);
    case 8:  return CMD_PAYLOAD (FIFO_D8);
    case 16: return CMD_PAYLOAD (FIFO_D16);
    default: return CMD_PAYLOAD (FIFO_D0);
  }
}

static uint8_t cmd2payload (uint8_t cmd)
{
  switch ((cmd >> CMD_PAYLOAD_SHIFT) & CMD_PAYLOAD_MASK) {
    case FIFO_D0:  return 0;
    case FIFO_D1:  return 1;
    case FIFO_D2:  return 2;
    case FIFO_D4:  return 4;
    case FIFO_D5:  return 5;
    case FIFO_D6:  return 6;
    case FIFO_D8:  return 8;
    case FIFO_D16: return 16;
  }
  return 0;
}


static void dump (const char *str, const uint8_t *data, int len)
{
  int i;
  printf ("[%d] %s ", len, str);
  for (i = 0; i < len; i++) {
    printf ("%02X", data[i]);
  }
  printf ("\n");
}

static void *flexsoc_listen (void *arg)
{
  int rv;
  char buf[17];
  uint8_t cnt, read;
  uint32_t addr, data;
  
  // Loop forever reading packets
  while (1) {

    // Get command
    rv = dev->Read (&buf[0], 1);
    if (rv < 0)
      err ("Target connection broken: rv=%d", rv);
    else if (rv == 0) {
      /* libftdi doesn't block on sync call ftdi_read_data()
       * this eats up processor but without a
       * blocking API we need this to service
       * calls as fast as possible
       */
      continue;
    }

    // Read the rest of the data
    read = 0;
    cnt = cmd2payload (buf[0]);
    if (cnt) {
      while (read < cnt) {
        rv = dev->Read (&buf[1 + read], cnt);
        if (rv < 0)
          err ("Target connection broken: rv=%d", rv);
        read += rv;
      }
    }

    // Master response
    if ((buf[0] & 0x80) == 0x80) {

      // Copy response
      memcpy (exp_buf, buf, cnt + 1);
            
      // Unlock mutex
      pthread_mutex_unlock (&resplock);
    }
    // Pass to callback if registered
    else if (recv_cb) {
      if (DEBUG) dump ("<=", (uint8_t *)buf, cnt + 1);
      recv_cb ((uint8_t *)buf, cnt + 1);
    }
  }
}


int flexsoc_open (char *id, recv_cb_t cb)
{
  int rv;

  // If it looks like an IP address create TCP connection
  // Else try FTDI
  if (strchr (id, ':') || strchr (id, '.'))
    dev = new TCPTransport ();
  else
    dev = new FTDITransport ();

  // Open transport
  rv = dev->Open (id);
  if (rv)
    err ("Failed to open device: %s (rv=%d)", id, rv);

  // Initialize the mutexes
  pthread_mutex_init (&resplock, NULL);
  pthread_mutex_init (&mlock, NULL);

  // Default to locked position
  pthread_mutex_lock (&resplock);
  
  // Spin up thread to read transport
  rv = pthread_create (&tid, NULL, &flexsoc_listen, NULL);
  if (rv)
    err ("Failed to spawn flexsoc thread!");

  // Save callback
  recv_cb = cb;
  
  // Success
  return 0;
}

void flexsoc_send (const char *buf, int len)
{
  if (dev) {
    if (DEBUG) dump ("=>", (uint8_t *)buf, len);
    dev->Write (buf, len);
  }
}

void flexsoc_close (void)
{
  // Close transport
  if (dev)
    dev->Close ();
}

int flexsoc_send_resp (const char *wbuf, int wlen, char *rbuf, int rlen)
{
  int rv;
  
  if (!dev)
    return -1;

  // Grab master lock
  pthread_mutex_lock (&mlock);
  
  // Save num bytes/buffer
  exp_buf = rbuf;
  if (DEBUG) dump ("=>", (uint8_t *)wbuf, wlen);
  
  // Write to device
  rv = dev->Write (wbuf, wlen);
  if (rv != wlen)
    err ("Unexpected resp: rv=%d", rv);

  // Block until response received
  if (rbuf && rlen)
    pthread_mutex_lock (&resplock);
  if (DEBUG) dump ("<=", (uint8_t *)rbuf, rlen);
  
  // Release master lock
  pthread_mutex_unlock (&mlock);

  return 0;
}

static int flexsoc_read (uint8_t width, uint32_t addr, uint8_t *data, int len)
{
  int rv, i, wlen = 5;
  char buf[5];

  // Handle empty reads
  if (len <= 0)
    return 0;
  
  // Assemble command - payload is address
  buf[0] = CMD_INTERFACE_MASTER | payload2cmd (4) | CMD_READ | CMD_WIDTH (width);
  *((uint32_t *)&buf[1]) = htonl (addr);
  
  // Read all data
  for (i = 0; i < len; i++) {

    // Send incrementing read after first address with command
    if (i != 0) {
      buf[0] = CMD_INTERFACE_MASTER | payload2cmd (0) | CMD_READ | CMD_AUTOINC | CMD_WIDTH (width);
      wlen = 1;
    }

    // Start transaction with target
    rv = flexsoc_send_resp (buf, wlen, buf, 1 + width);
    if (rv)
      return rv;

    // If transaction failed return failure
    if (buf[0] & 1)
      return -1;

    // Convert back to host endian
    switch (width) {
      case 1:
        data[i] = buf[1];
        break;
      case 2:
        *((uint16_t *)&data[i * width]) = ntohs (*((uint16_t *)&buf[1]));
        break;
      case 4:
        *((uint32_t *)&data[i * width]) = ntohl (*((uint32_t *)&buf[1]));
        break;
    }
  }
  
  // Return success
  return 0;
}

static int flexsoc_write (uint8_t width, uint32_t addr, const uint8_t *data, int len)
{
  int rv, i;
  char buf[9];

  // Handle empty reads
  if (len <= 0)
    return 0;
  
  // Assemble command - payload is address + data
  buf[0] = CMD_INTERFACE_MASTER | payload2cmd (4 + width) | CMD_WRITE | CMD_WIDTH (width);
  *((uint32_t *)&buf[1]) = htonl (addr);

  // Loop through data
  for (i = 0; i < len; i++) {

    if (i != 0) {

      // Send incrementing write
      buf[0] = CMD_INTERFACE_MASTER | payload2cmd (width) | CMD_WRITE | CMD_AUTOINC | CMD_WIDTH (width);
    }
    
    // Convert to network endian
    switch (width) {
      case 1:
        buf[i ? 1 : 5] = data[i];
        break;
      case 2:
        *((uint16_t *)&buf[i ? 1 : 5]) = htons (*((uint16_t *)&data[i * width]));
        break;
      case 4:
        *((uint32_t *)&buf[i ? 1 : 5]) = htonl (*((uint32_t *)&data[i * width]));
        break;
    }

    // Start transaction
    rv = flexsoc_send_resp (buf, i ? 1 + width : 5 + width, buf, 1);
    if (rv)
      return rv;
    
    // If transaction failed return
    if (buf[0] & 0x1)
      return -1;
  }
  
  // Return success
  return 0;
}

int flexsoc_readw (uint32_t addr, uint32_t *data, int len)
{
  return flexsoc_read (4, addr, (uint8_t *)data, len);
}

int flexsoc_readh (uint32_t addr, uint16_t *data, int len)
{
  return flexsoc_read (2, addr, (uint8_t *)data, len);
}

int flexsoc_readb (uint32_t addr, uint8_t *data, int len)
{
  return flexsoc_read (1, addr, (uint8_t *)data, len);
}

int flexsoc_writew (uint32_t addr, uint32_t *data, int len)
{
  return flexsoc_write (4, addr, (const uint8_t *)data, len);
}

int flexsoc_writeh (uint32_t addr, uint16_t *data, int len)
{
  return flexsoc_write (2, addr, (const uint8_t *)data, len);
}

int flexsoc_writeb (uint32_t addr, uint8_t *data, int len)
{
  return flexsoc_write (1, addr, (const uint8_t *)data, len);
}
