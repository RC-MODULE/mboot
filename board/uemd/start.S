/*
 *  armboot - Startup Code for ARM1176 CPU-core
 *
 * Copyright (c) 2007	Samsung Electronics
 *
 * Copyright (C) 2008
 * Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * 2007-09-21 - Restructured codes by jsgood (jsgood.yang@samsung.com)
 * 2007-09-21 - Added MoviNAND and OneNAND boot codes by
 * jsgood (jsgood.yang@samsung.com)
 * Base codes by scsuh (sc.suh)
 */

#include <asm-offsets.h>
#include <config.h>
#include <version.h>

.globl asm_start
asm_start:

reset_excp:
    b reset
undef_excp:
    subs pc, r14, #4 	/* return */
swi_excp:
    subs pc, r14, #0 	/* return */
prefetch_excp:
    subs pc, r14, #4	/* return */
abort_excp:
    subs pc, r14, #4 	/* return */
    nop			/* reserver vector */	
irq_excp:
    subs pc, r14, #4	/* return */
fiq_excp:
    subs pc, r14, #4

.globl g_early_start
g_early_start:
	.word lds_early_start

.globl g_text_start
g_text_start:
	.word lds_text_start

.globl g_data_start
g_data_start:
	.word lds_data_start

.globl g_signature_start
g_signature_start:
	.word lds_signature_start

.globl g_signature_end
g_signature_end:
	.word lds_signature_end

.globl g_bss_start
g_bss_start:
	.word lds_bss_start

.globl g_bss_end
g_bss_end:
	.word lds_bss_end

reset:
	/* Set the cpu to SVC32 mode and disable interrupts */
	mrs	r0, cpsr
	bic	r0, r0, #0x3f
	orr	r0, r0, #0xd3 /* set I=1 F=1 T=0 M=supervisor */
	msr	cpsr, r0

cpu_init_crit:
	/* Flush v4 I/D caches */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/* Disable MMU stuff and caches */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache

	/* Prepare to disable the MMU */
	adr	r2, mmu_disabled
	sub	r2, r2, #(CONFIG_SYS_UBOOT_BASE - CONFIG_SYS_TEXT_BASE)
	b	mmu_disable

	.align 5
	/* Run in a single cache-line */
mmu_disable:
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	mov	pc, r2

mmu_disabled:
clear_bss:
	ldr	r0, g_bss_start
	ldr	r1, g_bss_end
	mov	r2, #0x00000000		/* clear */

clear_bss_loop:
	str	r2, [r0]
	add	r0, r0, #4
	cmp	r0, r1
	bne	clear_bss_loop

	/* Setup stack and call user code */
	ldr	sp, =(CONFIG_SYS_SP_ADDR)
	ldr	r0,=0x00000000
	bl	early_init

