/**
 *  FPGA sysctl hardware interface regs
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#ifndef FLEXSOC_REG_H
#define FLEXSOC_REG_H

// Register definitions
#define SYSCTL_BASE       (0xE0000000)
#define SYS_HWID          (SYSCTL_BASE + 0)
#define SYS_CTRL          (SYSCTL_BASE + 4)
#define SYS_BRGCTRL       (SYSCTL_BASE + 8)
#define SYS_BRGIDC        (SYSCTL_BASE + 0xc)
#define SYS_BRGDCTL       (SYSCTL_BASE + 0x10)
#define SYS_BRGDATA       (SYSCTL_BASE + 0x14)
#define SYS_EXECBASE      (SYSCTL_BASE + 0x18)
#define SYS_ROMBASE       (SYSCTL_BASE + 0x1c)
#define SYS_ROMMASK       (SYSCTL_BASE + 0x20)
#define SYS_RAMBASE       (SYSCTL_BASE + 0x24)
#define SYS_RAMMASK       (SYSCTL_BASE + 0x28)
#define SYS_BRGBASE       (SYSCTL_BASE + 0x2c)
#define SYS_BRGMASK       (SYSCTL_BASE + 0x30)
#define SYS_REMAP32(n)    (SYSCTL_BASE + 0x34 + (n * 4))
#define SYS_IRQMAP(n)     (SYSCTL_BASE + 0x54 + n) // n=0-240 byte access
#define SYS_RIRQMASK(n)   (SYSCTL_BASE + 0x144 + (n * 4))

// Helper macros
#define MAGIC_VAL         0x1e050000
#define MAGIC_VALID(x)    ((x & 0xffff0000) == MAGIC_VAL)
#define HWVERSION(x)      (x & 0xffff)

#endif /* FLEXSOC_REG_H */

