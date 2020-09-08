/* Define the end of RAM and limit of stack memory */
/* (4KB SRAM on the STM32F031x6 line, 4096 = 0x1000) */
/* (RAM starts at address 0x20000000) */
_estack = 0x20004000;

MEMORY
{
    FLASH ( rx )      : ORIGIN = 0x00000000, LENGTH = 16k
    RAM ( rxw )       : ORIGIN = 0x20000000, LENGTH = 16k
}

SECTIONS
{
    .text :
    {
        *(.vector0)
        *(.vector1)
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(SORT(.ctors.*))
        *(.ctors)
        *(SORT(.dtors.*))
        *(.dtors)
        KEEP(*(.rodata*))
        KEEP(*(.eh_frame*))
    } > FLASH

    .ARM.extab :
    {   
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
     } > FLASH

    __etext = .;
    .data :
    {
        __data_start__ = .;
        *(.data)
        . = ALIGN (4);
        __data_end__ = .;
    } > RAM AT> FLASH

    .bss (NOLOAD) :
    {
        __bss_start__ = .;
        *(.bss)
        *(COMMON)
        . = ALIGN (4);
        __bss_end__ = .;
    } > RAM
}
