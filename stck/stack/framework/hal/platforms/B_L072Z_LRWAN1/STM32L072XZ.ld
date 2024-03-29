MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 192K
  EEPROM (r) : ORIGIN = 0x08080000, LENGTH = 6K
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

ENTRY(Reset_Handler)

_stack_size = 0x500;
_heap_size = 0x300;

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)

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
    __exidx_end = .;

    __etext = .;
    _sidata = .;

    /* store D7AP FS in EEPROM */
    .d7ap_fs_metadata :
    {
      . = ALIGN(4);
      __d7ap_fs_metadata_start = .;
      KEEP(*(.d7ap_fs_metadata))
      __d7ap_fs_metadata_end = .;
    } > EEPROM

    .d7ap_fs_file_headers :
    {
      . = ALIGN(4);
      __d7ap_fs_file_headers_start = .;
      KEEP(*(.d7ap_fs_metadata))
      __d7ap_fs_file_headers_end = .;
    } > EEPROM

    .d7ap_fs_systemfiles :
    {
      . = ALIGN(4);
      __d7ap_fs_systemfiles_start = .;
      KEEP(*(.d7ap_fs_systemfiles))
      __d7ap_fs_systemfiles_end = .;
    } > EEPROM

    /* stack section contains no symbols, used to check that there is enough RAM left.
        Place at start of RAM before data, so that is the stack overflows we would get a HardFault
        exception instead of silently overwriting other data, so this is easier to notice */
    .stack :
    {
      . = ALIGN(8);
      __stack_start = .;
      . = . + _stack_size;
      . = ALIGN(8);
      PROVIDE(_estack = .);
    } >RAM

    .data : AT (__etext)
    {
        __data_start__ = .;
        _sdata = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;
        _edata = .;

    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss = .;
    } > RAM

    .heap : {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _heap_size;
        _eheap = .; /*ORIGIN(RAM) + LENGTH(RAM);*/
    } > RAM
}
