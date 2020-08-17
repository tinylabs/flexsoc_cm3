/**
 *  FTDI transport implementationn
 *
 *  All rights reserved
 *  Tiny Labs Inc
 *  2019
 */
#ifndef FTDITRANSPORT_H
#define FTDITRANSPORT_H

#include <ftdi.h>

#include "Transport.h"

class FTDITransport : public Transport {
 private:
  struct ftdi_context *ftdi = NULL;
  
 public:
  FTDITransport (void);
  ~FTDITransport ();

  // Implement interface
  int Open (char *id);
  void Close (void);
  int Read (uint8_t *buf, int len);
  int Write (const uint8_t *buf, int len);
  void Flush (void);

  // Manage buffer sizes
  void ReadSize (uint32_t sz);
  void WriteSize (uint32_t sz);
};

#endif /* FTDITRANSPORT_H */

