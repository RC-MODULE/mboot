/*
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(asm_start)
SECTIONS
{
	. = 0x00000000;

	. = ALIGN(4);
	.text : {
		/* Early code, no more than 4K */
		lds_early_start = .;
		board/uemd/start.o	(.text)
		board/uemd/uemd_early.o (.text)

		/* Rest of the code */
		lds_text_start = .;
		*(.text)
	}

	lds_data_start = .;
	. = ALIGN(4);
	.rodata : {
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
	}

	. = ALIGN(4);
	.data : {
		*(.data)
	}

	/* U-boot commands array */
	. = ALIGN(4);
	lds_u_boot_cmd_start = .;
	.u_boot_cmd : {
		*(.u_boot_cmd)
	}
	lds_u_boot_cmd_end = .;

	/* End-of-code marker */
	. = ALIGN(4);
	lds_signature_start = .;
	.signature : {
		*(.signature)
	}
	lds_signature_end = .;

	/* Uninitialised data */
	. = ALIGN(4);
	lds_bss_start = .;
	.bss (NOLOAD) : {
		*(.bss) . = ALIGN(4);
	}
	lds_bss_end = .;

	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
}
