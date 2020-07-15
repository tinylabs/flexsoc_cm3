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
  int Read (char *buf, int len);
  int Write (const char *buf, int len);
  void Flush (void);
};

#endif /* FTDITRANSPORT_H */

