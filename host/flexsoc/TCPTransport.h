/**
 *  TCP transport implementationn
 *
 *  All rights reserved
 *  Tiny Labs Inc
 *  2019
 */
#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include <netinet/in.h>

#include "Transport.h"

class TCPTransport : public Transport {
 private:
  struct sockaddr_in  a4;
  struct sockaddr_in6 a6;
  bool ipv6 = false;
  int sockfd;
  
 public:
  TCPTransport (void);
  ~TCPTransport ();

  // Implement interface
  int Open (char *id);
  void Close (void);
  int Read (uint8_t *buf, int len);
  int Write (const uint8_t *buf, int len);
  void Flush (void);
};

#endif /* TCPTRANSPORT_H */

