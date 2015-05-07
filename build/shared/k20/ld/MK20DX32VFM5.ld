OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(Reset_Handler)

MEMORY {
	ROM (rx)  : ORIGIN = 3K, LENGTH = 29K
	RAM (rwx) : ORIGIN = 0x20000000 - 8K / 2, LENGTH = 8K
}

FTFL_CONFIG = 0x00000400;
FlexRAM     = 0x14000000;
DMA         = 0x40008000;
DMAMUX      = 0x40021000;
FTFL        = 0x40020000;
SPI0        = 0x4002c000;
I2S0        = 0x4002f000;
CRC         = 0x40032000;
PIT         = 0x40037000;
FTM0        = 0x40038000;
FTM1        = 0x40039000;
ADC0        = 0x4003b000;
RTC         = 0x4003d000;
LPTMR0      = 0x40040000;
TSI0        = 0x40045000;
SIM         = 0x40047000;
PORTA       = 0x40049000;
PORTB       = 0x4004a000;
PORTC       = 0x4004b000;
PORTD       = 0x4004c000;
PORTE       = 0x4004d000;
MCG         = 0x40064000;
I2C0        = 0x40066000;
UART0       = 0x4006A000;
UART1       = 0x4006B000;
UART2       = 0x4006C080;
USB0        = 0x40072000;
LLWU        = 0x4007c000;
PMC         = 0x4007d000;
SMC         = 0x4007e000;
RCM         = 0x4007f000;
GPIOA       = 0x400ff000;
GPIOB       = 0x400ff040;
GPIOC       = 0x400ff080;
GPIOD       = 0x400ff0C0;
GPIOE       = 0x400ff100;
SYSTICK     = 0xe000e010;
NVIC        = 0xe000e100;
SCB         = 0xe000ed00;

SECTIONS {

    .reset : {
        KEEP(*(.isr_vector .isr_vector.*))
        . = ALIGN(0x4);
    } >ROM

/*    
    .fastcode : {
        __fastcode_load = LOADADDR (.fastcode);
        __fastcode_start = .;

        *(.glue_7t) *(.glue_7)
        *(.text.fastcode)

        . = ALIGN (4);
        __fastcode_end = .;
    } >RAM AT>ROM
*/

    .text : {
        CREATE_OBJECT_SYMBOLS
        *(.text .text.* .gnu.linkonce.t.*)
        *(.plt)
        *(.gnu.warning)
        *(.glue_7t) *(.glue_7)         /* NOTE: placed already in .fastcode */

        . = ALIGN (4);
        /* These are for static constructors and destructors under ELF */
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))

        *(.rodata .rodata.* .gnu.linkonce.r.*)

        *(.ARM.extab* .gnu.linkonce.armextab.*)
        *(.gcc_except_table)
        *(.eh_frame_hdr)
        *(.eh_frame)

        *(.init)
        *(.fini)

        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >ROM

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } >ROM
    _etext = .;

    .usb_bdt (NOLOAD) : {
        . = ALIGN(512);
        __bss_start__ = .;
        *(.usb_bdt)
    } > RAM
    
    .bss : {
        . = ALIGN(4);
        *(.shbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN (4);
        __bss_end__ = .;
    } >RAM

    .data : {
        __data_load = LOADADDR (.data);
        __data_start = .;
        KEEP(*(.jcr))
        *(.got.plt) *(.got)
        *(.shdata)
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN (4);
        _edata = .;
    } >RAM AT>ROM

    .co_stack (NOLOAD) : {
        . = ALIGN(8);
        *(.co_stack .co_stack.*)
    } >RAM

    _end = . ;
    __end = . ;
    PROVIDE(end = .);

    .stab 0 (NOLOAD) : {
        *(.stab)
    }

    .stabstr 0 (NOLOAD) : {
        *(.stabstr)
    }

    /* DWARF debug sections.
    * Symbols in the DWARF debugging sections are relative to the beginning
    * of the section so we begin them at 0.
    */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
    .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }
    /DISCARD/ : { *(.note.GNU-stack)  }
}
