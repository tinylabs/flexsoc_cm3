/**
 *  Test the master interface of flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include "target.h"
#include "err.h"

// Save slave response
static uint8_t cmd;
static uint32_t addr, data;
static int slave_err = 0;
static pthread_mutex_t slave_lock;
static Target *target;

static void slave_cb (uint8_t *buf, int len)
{
  int i;
  uint8_t resp[5];
  uint16_t tmps;
  uint32_t tmpl;
  
  // Save command
  cmd = buf[0];

  // OKAY response
  resp[0] = 0;

  // Get address
  addr = ntohl (*((uint32_t *)&buf[1]));
  data = 0x11223344;
  
  // Decode request
  switch (len) {
    case 5: // Read operation
      switch (cmd & 3) {
        case 0:
          resp[0] = 0x10;
          resp[1] = (data >> (8 * (addr & 3))) & 0xff;
          break;
        case 1:
          resp[0] = 0x20;
          tmps = htons (data >> (8 * (addr & 3)));
          memcpy (&resp[1], &tmps, 2);
          break;
        default:
          resp[0] = 0x30;
          tmpl = htonl (data);
          memcpy (&resp[1], &tmpl, 4);
          break;
      }
      break;
    case 6: // Write byte
      data = buf[5];
      break;
    case 7: // Write hwrd
      data = ntohs (*((uint16_t *)&buf[5]));
      break;
    case 9: // Write word
      data = ntohl (*((uint32_t *)&buf[5]));
      break;
    default: slave_err = -1;      
  }

  // Send response
  if (cmd & 0x08) // Write command
    target->SlaveSend (resp, 1);
  else { // Read command
    switch (cmd & 3) {
      case 0: target->SlaveSend (resp, 2); break;
      case 1: target->SlaveSend (resp, 3); break;
      default: target->SlaveSend (resp, 5); break;
    }
  }
    
  // Release mutex
  pthread_mutex_unlock (&slave_lock);
}

static int slave_check (Target *target, int write, int width, uint32_t add, void *dat)
{
  int rv;
  uint8_t bdat = *((uint8_t *)dat);
  uint16_t hdat = *((uint16_t *)dat);
  uint32_t wdat = *((uint32_t *)dat);

  if (write) {
    switch (width) {
      case 1: target->WriteB (add, (uint8_t *)dat, 1); break;
      case 2: target->WriteH (add, (uint16_t *)dat, 1); break;
      case 4: target->WriteW (add, (uint32_t *)dat, 1); break;      
    }
  }
  else {
    switch (width) {
      case 1: target->ReadB (add, (uint8_t *)dat, 1); break;
      case 2: target->ReadH (add, (uint16_t *)dat, 1); break;
      case 4: target->ReadW (add, (uint32_t *)dat, 1); break;      
    }    
  }
  pthread_mutex_lock (&slave_lock);

  printf ("cmd=%X\n", cmd);
  printf ("addr: %X == %X\n", add, addr);

  if (write && ((cmd & 0x8) != 0x8))
    return -1;
  else if (!write && ((cmd & 0x8) != 0x0))
    return -1;
  
  // Check results
  if ((slave_err != 0) ||
      (addr != add))
    return -1;

  // Check data if write
  if (write) {
    switch (width) {
      case 1: 
        printf ("data: %X == %X\n", bdat, data);
        if (data != bdat) 
          return -1;
        break;
      case 2: 
        printf ("data: %X == %X\n", hdat, data);
        if (data != hdat) 
          return -1;
        break;
      case 4: 
        printf ("data: %X == %X\n", wdat, data);
        if (data != wdat) 
          return -1;
        break;
    }
  }

  // Success
  return 0;
}

static int write_test (Target *target)
{
  uint32_t wdat = 0x11223344;
  uint16_t hdat[2] = {0x3344, 0x1122};
  uint8_t  bdat[4] = {0x44, 0x33, 0x22, 0x11};

  // Word write
  if (slave_check (target, 1, 4, 0x40000000, &wdat))
    return -1;

  // Hwrd write
  if (slave_check (target, 1, 2, 0x40000000, &hdat[0]))
    return -1;
  if (slave_check (target, 1, 2, 0x40000002, &hdat[1]))
    return -1;

  // Byte write
  if (slave_check (target, 1, 1, 0x40000000, &bdat[0]))
    return -1;
  if (slave_check (target, 1, 1, 0x40000001, &bdat[1]))
    return -1;
  if (slave_check (target, 1, 1, 0x40000002, &bdat[2]))
    return -1;
  if (slave_check (target, 1, 1, 0x40000003, &bdat[3]))
    return -1;

  return 0;
}

static int read_test (Target *target)
{
  uint32_t wdat[1];
  uint16_t hdat[2];
  uint8_t  bdat[4];

  // Word read
  if (slave_check (target, 0, 4, 0x40000000, &wdat))
    return -1;

  // Hwrd read
  if (slave_check (target, 0, 2, 0x40000000, &hdat[0]))
    return -1;
  if (slave_check (target, 0, 2, 0x40000002, &hdat[1]))
    return -1;

  // Byte read
  if (slave_check (target, 0, 1, 0x40000000, &bdat[0]))
    return -1;
  if (slave_check (target, 0, 1, 0x40000001, &bdat[1]))
    return -1;
  if (slave_check (target, 0, 1, 0x40000002, &bdat[2]))
    return -1;
  if (slave_check (target, 0, 1, 0x40000003, &bdat[3]))
    return -1;

  return 0;
}

int main (int argc, char **argv)
{
  int rv;
  uint32_t val = 3;
  
  if (argc != 2)
    err ("Must pass interface");
  
  // Open target
  target = new Target (argv[1]);

  // Install slave callback
  target->SlaveRegister (&slave_cb);
  
  // Init mutex
  pthread_mutex_init (&slave_lock, NULL);
  pthread_mutex_lock (&slave_lock);
  
  // Enable slave interface
  target->SlaveEn (true);
  
  // Run write tests
  if (write_test (target))
    err ("Write test failed");
  
  // Run read tests
  if (read_test (target))
    err ("Read test failed");

  // Close target
  delete target;
  return 0;
}
