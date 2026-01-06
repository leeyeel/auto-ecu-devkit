/**
 * @file    s32k312.ld
 * @brief   Linker Script for NXP S32K312
 *
 * @details Memory layout and linker configuration for S32K312 MCU.
 *
 * @par     Target Device: NXP S32K312
 * @par     Flash: 256KB
 * @par     RAM: 32KB
 *
 * @note    Generated for SPI Demo application
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Heap and Stack Size */
__stack_size = 0x1000;      /* 4KB Stack */
__heap_size = 0x1000;       /* 4KB Heap */

/* Memory Regions */
MEMORY
{
    /* Flash: 0x00000000 - 0x00040000 (256KB) */
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K

    /* SRAM_L: 0x1FFF8000 - 0x1FFFFFFF (32KB) */
    SRAM (rwx)  : ORIGIN = 0x1FFF8000, LENGTH = 32K

    /* Code Flash (for storing data if needed) */
    CODE_FLASH (rx) : ORIGIN = 0x00010000, LENGTH = 192K
}

/* Section Definitions */
SECTIONS
{
    /* Interrupt Vector Table */
    .vectors :
    {
        __Vectors_start = .;
        *(.vectors)
        __Vectors_end = .;
    } > FLASH

    /* Program Code */
    .text :
    {
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP (*(.init))
        KEEP (*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only Data */
    .rodata :
    {
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM Exception Tables */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        *(.ARM.exidx*)
    } > FLASH

    /* Initialized Data (load in Flash, run in RAM) */
    .data :
    {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH

    /* Uninitialized Data */
    .bss :
    {
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM

    /* Heap */
    ._heap :
    {
        . = ALIGN(8);
        __heap_start = .;
        . += __heap_size;
        __heap_end = .;
    } > SRAM

    /* Stack */
    ._stack :
    {
        . = ALIGN(8);
        __stack_start = .;
        . += __stack_size;
        __stack_end = .;
    } > SRAM

    /* Debug Information (for debugging tools) */
    .debug_info     : { *(.debug_info) }
    .debug_abbrev   : { *(.debug_abbrev) }
    .debug_line     : { *(.debug_line) }
    .debug_frame    : { *(.debug_frame) }
    .debug_str      : { *(.debug_str) }
    .debug_loc      : { *(.debug_loc) }
    .debug_macinfo  : { *(.debug_macinfo) }
}

/* End of Script */
