/**
 *  flexsoc communication library. Send/receive commands to flexsoc using 
 *  various transport layers.
 *
 *  All rights reserved.
 *  Tiny Labs Inc.
 *  2020
 */
#ifndef FLEXSOC_H
#define FLEXSOC_H

#include <stdint.h>

// Callback for slave interface
typedef void (*recv_cb_t) (uint8_t *buf, int len);

// Open/close flexsoc
int flexsoc_open (char *id);
void flexsoc_close (void);

//
// Raw interface - send/receive bytes
//
void flexsoc_send (const uint8_t *buf, int len);

// Master read/write interface
int flexsoc_readw (uint32_t addr, uint32_t *data, int len);
int flexsoc_readh (uint32_t addr, uint16_t *data, int len);
int flexsoc_readb (uint32_t addr, uint8_t  *data, int len);
int flexsoc_writew (uint32_t addr, const uint32_t *data, int len);
int flexsoc_writeh (uint32_t addr, const uint16_t *data, int len);
int flexsoc_writeb (uint32_t addr, const uint8_t  *data, int len);

// Simplified register access
uint32_t flexsoc_reg_read (uint32_t addr);
void flexsoc_reg_write (uint32_t addr, const uint32_t data);

// Register fn pointer with slave interface
void flexsoc_register (recv_cb_t cb);
void flexsoc_unregister (void);

// Read/write return code
int flexsoc_read_returnval (void);
void flexsoc_write_returnval (int val);

// Enable/disable high speed mode
void flexsoc_hispeed (bool en);

#endif /* FLEXSOC_H */
