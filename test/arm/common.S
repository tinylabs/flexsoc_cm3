/**
 *   All rights reserved.
 *   Tiny Labs Inc
 *   2018-2020
 *
 */
    .syntax     unified
    .arch       armv7-m

    /* Core ISR vector table */
    .section    .vector0
    .globl      __isr_vector
__isr_vector:
    .align      2
    .word       _estack
    .word       Reset_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .long       Default_Handler
    .size       __isr_vector, . - __isr_vector


    /* Default handler - just loop */
    .text
    .thumb_func
    .align  1
    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b  Default_Handler
    .size   Default_Handler, . - Default_Handler

   /*     Loop to copy data from read only memory to RAM. The ranges
    *     of copy from/to are specified by following symbols evaluated in
    *     linker script.
    *     __etext: End of code section, i.e., begin of data sections to copy from.
    *     __data_start__/__data_end__: RAM address range that data should be
    *     copied to. Both must be aligned to 4 bytes boundary.
    */
    .text
    .thumb_func
    .align  1
    .globl  Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    ldr     r1, =__etext
    ldr     r2, =__data_start__
    ldr     r3, =__data_end__

    /* Calculate size */
    subs    r3, r2
    ble     .flash_to_ram_loop_end
.flash_to_ram_loop:
    subs    r3, #4
    ldr     r0, [r1, r3]
    str     r0, [r2, r3]
    bgt     .flash_to_ram_loop
.flash_to_ram_loop_end:
    
    /* ZI .bss region */
    ldr     r1, =__bss_start__
    ldr     r2, =__bss_end__
    /* Get size */
    subs    r2, r1
    beq     .boot_main
    mov     r0, #0
.bss_loop:
    subs    r2, #4
    str     r0, [r1, r2]
    bgt     .bss_loop

    /* Boot app main */
.boot_main:
    ldr     r0, =main
    blx     r0
    
    /* Check return code
    * If zero write PASS to SimExit
    * Else write fail */
    ldr     r1, =0xf0000000
    cmp     r0, #0
    ite     eq
    ldreq   r2, =0x20026
    movne   r2, #1
    str     r2, [r1]
.loop_forever:
    b       .loop_forever
    .size   Reset_Handler, . - Reset_Handler
    
/**
 *   All rights reserved.
 *   Tiny Labs Inc
 *   2018-2020
 *
 * Galois internal LFSR implementation
 * Args are:
 * R0 = Initial value
 * R1 = polynomial in standard form
 * R2 = pointer to data or NULL (no data)
 * R3 = length in bytes (bits if R2 is NULL)
 */
   /*
    *   LFSR routine - Allows arbitrary CRC calc
    */
    .text
    .thumb_func
    .align  1
    .globl  lfsr
    .type   lfsr, %function
lfsr:
    push    {r4}
    /* Check if length is zero */
    tst     r3, #-1
    beq     lfsr_done
    /* Convert to bits */
    tst     r2, #-1
    ite     ne
    lslne   r3, #3
    beq     lfsr_loop
    
lfsr_load:
    /* Load new data byte every 8 cycles */
    tst     r3, #7
    itt     eq
    ldrbeq  r4, [r2], #1
    eoreq   r0, r4
lfsr_loop:
    /* Shift and store LSB in carry flag */
    lsrs    r0, #1
    it      cs
    eorcs   r0, r1
    /* Increment and loop */
    subs    r3, #1
    beq     lfsr_done
    /* Check whether we process data */
    tst     r2, #-1
    it      eq
    beq     lfsr_loop
    b       lfsr_load
lfsr_done:
    /* Return result */
    pop     {r4}
    bx      lr
    .size   lfsr, . - lfsr
.end
