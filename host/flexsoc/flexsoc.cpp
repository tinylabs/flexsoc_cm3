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
#include "Cbuf.h"

#define DEBUG         0

// Read transfer size
#define READ_SEND_SZ   (180)
#define WRITE_SEND_SZ  (READ_SEND_SZ * 5)

// Write transfer size
#define WRITE_RECV_SZ (WRITE_SEND_SZ / 2)
#define READ_RECV_SZ  (READ_SEND_SZ * 5)

// Master buf size
#define MBUF_SZ   (16 * 1024)

// Local variables
static Transport *dev = NULL;
static pthread_t read_tid;
static bool kill_thread = false;

// Circular buffer
static Cbuf *mbuf;

// Ping-pong transaction buffers
static uint8_t *tbuf[2];

// Protect outgoing writes
static pthread_mutex_t write_lock, api_lock; 

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
  int rv, pread;
  uint8_t pkt[17];
  
  // Loop forever reading packets
  while (1) {

    // Read from target interface
    rv = dev->Read (pkt, 1);
    
    // Device closed - kill thread
    if ((rv == DEVICE_NOTAVAIL) || kill_thread)
      return NULL;

    // Write to buffer
    if (rv > 0) {
      int written = 0, sz = rv;

      // Get size
      sz = cmd2payload (pkt[0]);
      pread = 0;
    
      // Read rest of packet
      while (pread < sz) {
        rv = dev->Read (&pkt[1 + pread], sz - pread);
        pread += rv;
      }

      // Dump if debug
      if (DEBUG) dump ("<=", pkt, sz + 1);
    
      // Copy to mbuf or dispatch to slave
      if (pkt[0] & CMD_INTERFACE_MASTER) {
        int written = 0;
        
        while (written < sz + 1) {
          rv = mbuf->Write (&pkt[written], sz + 1 - written);
          written += rv;
        }
      }
      // Dispatch to slave
      else if (recv_cb) {
        recv_cb (pkt, sz + 1);
      }
    }
  }
}


int flexsoc_open (char *id)
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

  // Create cirular buffer
  mbuf = new Cbuf (MBUF_SZ);
  
  // Create write lock (mux master/slave)
  pthread_mutex_init (&write_lock, NULL);

  // API lock
  pthread_mutex_init (&api_lock, NULL);

  // Malloc tbuf
  tbuf[0] = (uint8_t *)malloc (WRITE_SEND_SZ);
  tbuf[1] = (uint8_t *)malloc (WRITE_SEND_SZ);
  if (!tbuf[0] || (!tbuf[1]))
    err ("Failed to malloc tbuf");

  // Spin up thread to read transport
  rv = pthread_create (&read_tid, NULL, &flexsoc_listen, NULL);
  if (rv)
    err ("Failed to spawn flexsoc thread!");

  // Success
  return 0;
}

void flexsoc_send (const uint8_t *buf, int len)
{
  int rv, written = 0;
  // Lock write mutex
  pthread_mutex_lock (&write_lock);
  if (dev) {
    if (DEBUG) dump ("=>", (uint8_t *)buf, len);
    while (written < len) {
      rv = dev->Write (buf, len);
      if (rv < 0) {
        err ("flexsoc_send() failed");
      }
      written += rv;
    }
  }
  pthread_mutex_unlock (&write_lock);
}

static int flexsoc_recv (uint8_t *buf, int len)
{
  int read = 0, rv;

  while (read < len) {
    rv = mbuf->Read (&buf[read], len - read);
    read += rv;
  }
  return read;
}

void flexsoc_close (void)
{
  // Kill thread
  kill_thread = true;
  
  // Wait for thread
  pthread_join (read_tid, NULL);

  // Close transport
  if (dev)
    dev->Close ();

  // Free buffers
  free (tbuf[0]);
  free (tbuf[1]);

  // Free cbuf
  delete mbuf;
}

static void host16_to_buf (uint8_t *buf, const uint8_t *host)
{
  *((uint16_t *)buf) = htons (*((uint16_t *)host));
}

static void host32_to_buf (uint8_t * buf, const uint8_t *host)
{
  *((uint32_t *)buf) = htonl (*((uint32_t *)host));
}

static void buf_to_host16 (uint8_t *host, const uint8_t *buf)
{
  *((uint16_t *)host) = ntohs (*((uint16_t *)buf));
}

static void buf_to_host32 (uint8_t *host, const uint8_t * buf)
{
  *((uint32_t *)host) = ntohl (*((uint32_t *)buf));
}

static int read_process (uint8_t width, uint8_t *data, int rcnt)
{
  int i, read = 0;
  uint8_t rbuf[READ_RECV_SZ];

  // Debug only
  if (rcnt > READ_RECV_SZ) {
    printf ("rcnt=%d READ_RECV_SZ=%d\n", rcnt, READ_RECV_SZ);
    while (1) ;
  }
  
  // Read results
  flexsoc_recv (rbuf, rcnt);
  for (i = 0; i < (rcnt / (1 + width)); i++) {
    
    // Verify there wasn't an error
    if (rbuf[i * (1 + width)] & 1)
      err ("Read failed: %02X", rbuf[i * (1 + width)]);

    // Convert back to host endian
    switch (width) {
      case 1: data[read] = rbuf[(i * (1 + width)) + 1]; break;
      case 2: buf_to_host16 (&data[read], &rbuf[(i * (1 + width)) + 1]); break;
      case 4: buf_to_host32 (&data[read], &rbuf[(i * (1 + width)) + 1]); break;
    }
    
    // Increment read
    read += width;
  }
  
  // Return read
  return read;
}

static int write_process (int rcnt)
{
  int i, written = 0;
  uint8_t rbuf[WRITE_RECV_SZ];

  // Debug only
  if (rcnt > WRITE_RECV_SZ) {
    printf ("rcnt=%d READ_RECV_SZ=%d\n", rcnt, WRITE_RECV_SZ);
    while (1) ;
  }

  // Read results
  flexsoc_recv (rbuf, rcnt);
  for (i = 0; i < rcnt; i++) {
    
    // Verify there wasn't an error
    if (rbuf[i] & 1)
      err ("Write failed: %02X", rbuf[i]);    
    
    // Increment read
    written++;
  }
  
  // Return read
  return written;
}

static int flexsoc_read (uint8_t width, uint32_t addr, uint8_t *data, int len)
{
  int rv, i, bi = 0, idx = 0, read = 0;
  bool process = false;
  int rcnt[2] = {0, 0};
  
  // Handle empty reads
  if (len <= 0)
    return 0;

  // Lock API lock
  pthread_mutex_lock (&api_lock);
  
  // Set read/write size
  dev->WriteSize (READ_SEND_SZ);
  dev->ReadSize (READ_RECV_SZ);

  // Read all data
  for (i = 0; i < len; i++) {

    // If we've hit buffer size then flush
    if (idx == READ_SEND_SZ) {        
      flexsoc_send (tbuf[bi], idx);
      bi = !bi; // Switch buffers
      if (!bi) // Start processing responses
        process = true;
      idx = 0;

      // If we've already sent two buffers then start processing
      if (process) {
        read += read_process (width, &data[read], rcnt[bi]);
        rcnt[bi] = 0;
      }
    }

    // Send full read with address
    if (i == 0) {
      tbuf[bi][idx] = CMD_INTERFACE_MASTER | payload2cmd (4) | CMD_READ | CMD_WIDTH (width);
      idx++;
      host32_to_buf (&tbuf[bi][idx], (uint8_t *)&addr);
      idx += 4;
    }

    // Send incrementing read after first command
    else {
      tbuf[bi][idx] = CMD_INTERFACE_MASTER | payload2cmd (0) | CMD_READ | CMD_AUTOINC | CMD_WIDTH (width);
      idx++;
    }

    // Update bytes expected back
    rcnt[bi] += (1 + width);
  }
  
  // Flush any remaining data
  if (idx != 0)
    flexsoc_send (tbuf[bi], idx);
  
  // Process any remaining data
  if (rcnt[!bi])
    read += read_process (width, &data[read], rcnt[!bi]);
  if (rcnt[bi])
    read += read_process (width, &data[read], rcnt[bi]);
  
  // Unlock API lock
  pthread_mutex_unlock (&api_lock);
  
  // Return success
  return 0;
}

static int flexsoc_write (uint8_t width, uint32_t addr, const uint8_t *data, int len)
{
  int rv, i, bi = 0, idx = 0, written = 0;
  bool process = false;
  int rcnt[2] = {0, 0};
  
  // Ignore empty writes
  if (len <= 0)
    return 0;
  
  // Lock API lock
  pthread_mutex_lock (&api_lock);

  // Set read/write size
  dev->WriteSize (WRITE_SEND_SZ);
  dev->ReadSize (WRITE_RECV_SZ);

  // Loop over data to write
  for (i = 0; i < len; i++) {

    // If we've hit buffer size then flush
    if (idx + 1 + width > WRITE_SEND_SZ) {
      flexsoc_send (tbuf[bi], idx);
      bi = !bi; // Switch buffers
      idx = 0;
      if (!bi) // Start processing responses
        process = true;

      // If we've already sent two buffers then start processing
      if (process) {
        written += write_process (rcnt[bi]);
        rcnt[bi] = 0;
      }
    }

    // Send full write with address
    if (i == 0) {
      tbuf[bi][idx] = CMD_INTERFACE_MASTER | payload2cmd (4 + width) | CMD_WRITE | CMD_WIDTH (width);
      idx++;
      host32_to_buf (&tbuf[bi][idx], (uint8_t *)&addr);
      idx += 4;
    }

    // Send incrementing write after first command
    else {
      tbuf[bi][idx] = CMD_INTERFACE_MASTER | payload2cmd (width) |
        CMD_WRITE | CMD_AUTOINC | CMD_WIDTH (width);
      idx++;
    }

    // Copy data to write buffer
    switch (width) {
      case 1: tbuf[bi][idx] = data[i]; break;
      case 2: host16_to_buf (&tbuf[bi][idx], &data[i * width]); break;
      case 4: host32_to_buf (&tbuf[bi][idx], &data[i * width]); break;
    }

    // Update idx
    idx += width;
    
    // Update bytes expected back
    rcnt[bi] += 1;
  }
  
  // Flush any remaining data
  if (idx != 0)
    flexsoc_send (tbuf[bi], idx);
    
  // Process any remaining data
  if (rcnt[!bi])
    written += write_process (rcnt[!bi]);
  if (rcnt[bi])
    written += write_process (rcnt[bi]);

  // Unlock API lock
  pthread_mutex_unlock (&api_lock);
  
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

int flexsoc_writew (uint32_t addr, const uint32_t *data, int len)
{
  return flexsoc_write (4, addr, (const uint8_t *)data, len);
}

int flexsoc_writeh (uint32_t addr, const uint16_t *data, int len)
{
  return flexsoc_write (2, addr, (const uint8_t *)data, len);
}

int flexsoc_writeb (uint32_t addr, const uint8_t *data, int len)
{
  return flexsoc_write (1, addr, (const uint8_t *)data, len);
}

uint32_t flexsoc_reg_read (uint32_t addr)
{
  int rv;
  uint32_t val;
  rv = flexsoc_readw (addr, &val, 1);
  if (rv)
    err ("Reg read failed: %08X", addr);
  return val;
}

void flexsoc_reg_write (uint32_t addr, const uint32_t data)
{
  int rv;
  rv = flexsoc_writew (addr, &data, 1);
  if (rv)
    err ("Reg write failed: %08X", addr);
}

void flexsoc_register (recv_cb_t cb)
{
  recv_cb = cb;
}

void flexsoc_unregister (void)
{
  recv_cb = NULL;
}
