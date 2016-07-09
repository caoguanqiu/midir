/*
 * Copyright (C) 2008-2013, Marvell International Ltd.
 * All Rights Reserved.
 */

/* Entry Point */
ENTRY(Reset_IRQHandler)

/* Start address of main stack pointer
 * Note: Stack grows towards lower addresses.
 */
_estack = 0x20020000;    /* end of 128K SRAM1 */

/* Minimum required heap size in bytes */
_heap_size = (72 * 1024);


MEMORY
{
	SRAM0 (rwx)  : ORIGIN = 0x00100000, LENGTH = 384K
	SRAM1 (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
	NVRAM (rw)   : ORIGIN = 0x480C0000, LENGTH = 4K
}

/* End of SRAM0 */
_e_sram0 = ORIGIN(SRAM0) + LENGTH(SRAM0);

/* The size of the main stack that is used for interrupt processing. */
_main_stack_size = 0x800;
/* Start of main stack*/
_main_stack_start = ORIGIN(SRAM1) + LENGTH(SRAM1) - _main_stack_size;

SECTIONS
{
	.text :
	{
		. = ALIGN(256);
		KEEP(*(.isr_vector))
		. = ALIGN(4);

		*(.text.Reset_IRQHandler)
		*(.text .text.* .gnu.linkonce.t.*)
		*(.rodata .rodata.* .gnu.linkonce.r.*)

		. = ALIGN(4);
		_etext = .;
	} > SRAM0

	/* Heap Section. */
	. = ALIGN(4);
	_heap_start = .;
	. = _e_sram0;
	_heap_end = .;

	.data :
	{
		_data = .;
		*(.data)
		*(.data.*)
		_edata = .;

       /*add Function Symble Table*/
		. = ALIGN(4);
        addlib_fsymtab_start = .;
        KEEP(*(FSymTab));
        addlib_fsymtab_end = .;

		. = ALIGN(4);
        addlib_vsymtab_start = .;
        KEEP(*(VSymTab))
        addlib_vsymtab_end = .;

       /*add user Function Symble Table*/
		. = ALIGN(4);
        addlib_user_fsymtab_start = .;
        KEEP(*(UserFSymTab));
        addlib_user_fsymtab_end = .;

		. = ALIGN(4);
        addlib_user_vsymtab_start = .;
        KEEP(*(UserVSymTab))
        addlib_user_vsymtab_end = .;

	} > SRAM1

	/*
	 * NOTE: Please do not move the below section ".iobufs" to SRAM0.
	 * Some of the peripherals (e.g. USB, SDIO) with their own DMA engines
	 * have a requirement that the data buffers for DMA need to be in the
	 * "Data" memory (SRAM1). The peripherals that use internal DMA engine
	 * of MC200 (e.g. UART) can use data buffers from SRAM0 or SRAM1.
	 */
	.iobufs (NOLOAD):
	{
		. = ALIGN(4);
		_iobufs_start = .;
		*(.iobufs.page_aligned)
		*(.iobufs)
		*(.iobufs.*)
		_iobufs_end = .;
	} > SRAM1

	.bss (NOLOAD):
	{
		_bss = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		_ebss = .;
	} > SRAM1

	/* Add the free space in SRAM1 to our heap */
	_heap_2_start = .;
	. = _main_stack_start;
	_heap_2_end = .;

	/* The main stack */
        ._main_stack _main_stack_start :
        {
                . = ALIGN(4);
                . = . + _main_stack_size;
                . = ALIGN(4);
        }

	_actual_heap_size = _heap_end - _heap_start + _heap_2_end - _heap_2_start;
	 ASSERT(_actual_heap_size >= _heap_size, "Insufficient space left for heap.")

	.nvram (NOLOAD):
	{
		/* BootROM uses first few bytes of retention ram */
		_nvram_start = .;
		. = . + 64;
		. = ALIGN(4);
		/* Zero initialized on bootup */
		_nvram_begin = .;
		*(.nvram)
		*(.nvram.*)
		_nvram_end = .;
		/* Un-initialized nvram section */
		. = ALIGN(4);
		*(.nvram_uninit)
		*(.nvram_uninit.*)
	} > NVRAM
}

PROVIDE ( addlib_fsymtab_area_size = addlib_fsymtab_end - addlib_fsymtab_start);
PROVIDE ( addlib_vsymtab_area_size = addlib_vsymtab_end - addlib_vsymtab_start);
PROVIDE ( addlib_user_fsymtab_area_size = addlib_user_fsymtab_end - addlib_user_fsymtab_start);
PROVIDE ( addlib_user_vsymtab_area_size = addlib_user_vsymtab_end - addlib_user_vsymtab_start);
