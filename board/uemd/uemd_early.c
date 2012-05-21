/*
 * (C) Copyright 2010
 * Sergey Mironov <ierton@gmail.com>
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

#include <common.h>
#include "uemd.h"

inline void* early_memset(void * s,int c,size_t count)
{
	char *xs = (char *) s;
	while (count--)
		*xs++ = c;
	return s;
}

typedef int (*otp_create_f)(struct uemd_otp *otp);

static void early_otp_init(struct uemd_otp *otp)
{
	/* Those functions are in ARM's boot ROM (factory programmed)
	 * returns 1 on success, 0 on error */
	otp_create_f early_otp_create = (otp_create_f)0x00001978;

	early_memset(otp, 0, sizeof(struct uemd_otp));
	early_otp_create(otp);
}

/* Those functions are in ARM's boot ROM (factory programmed) */
typedef void (*nand_init_f)(struct uemd_nand_state *state);
typedef int (*nand_read_f)(struct uemd_nand_state *state,
	int start, void *dest, int words_len);

#define UEMD_IMAGE_SIGNATURE_1 0xDEADB001
#define UEMD_IMAGE_SIGNATURE_2 0xDEADB002

/* Image entry point, defined in start.S */
void asm_start (void) __attribute__ ((noreturn)); 

static void early_nand_reread_and_restart(void)
{
	/* UEMD's built in boot loader can read image from NAND, but no more
	 * that 4K. Following code is required to re-read full image and place
	 * it into Internal Memory Bank0 (IM0).
	 */
	struct uemd_nand_state nand;
	nand_init_f early_nand_init = (nand_init_f)0x000007e0;
	nand_read_f early_nand_read = (nand_read_f)0x000009f4;

	early_memset(&nand, 0, sizeof(struct uemd_nand_state));
	early_nand_init(&nand);

	/* Read the flash up to the start of bss section. */
	early_nand_read(&nand, 0,
		(void *)PHYS_IM0, ((g_bss_start - PHYS_IM0)/4) + 1);

	/* Restart the bootloader softly */
	asm_start();
}

#define __signature __attribute__ ((section(".signature")))
unsigned int g_tail_marker __signature = UEMD_IMAGE_SIGNATURE_1;

void early_init(void)
{
	struct uemd_otp otp;

	early_otp_init(&otp);

	if(otp.source == BOOT_FROM_NAND) {
		if(g_tail_marker != UEMD_IMAGE_SIGNATURE_1) {
			/* Image's tail is damaged due to incomplete NAND transfer. Call
			 * reread to fix it */
			early_nand_reread_and_restart();
		}
		else {
			/* Image looks good. Mark it as 'used' used and continue. */
			g_tail_marker = UEMD_IMAGE_SIGNATURE_2;
			uemd_init(&otp);
		}
	}
	else {
		uemd_init(&otp);
	}
}

