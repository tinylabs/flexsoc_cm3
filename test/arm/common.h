/**
 *  Common test lib header
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */


#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

extern uint32_t lfsr (uint32_t initial, uint32_t poly, uint8_t *ptr, size_t len);
#define max32(val)      lfsr(val, 0xE0000200, NULL, 32)
uint32_t rand32 (uint32_t *state);

#endif /* COMMON_H */

