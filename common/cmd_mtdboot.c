/*
 * Driver for NAND support, Rick Bronson
 * borrowed heavily from:
 * (c) 1999 Machine Vision Holdings, Inc.
 * (c) 1999, 2000 David Woodhouse <dwmw2@infradead.org>
 *
 * Ported 'dynenv' to 'nand env.oob' command
 * (C) 2010 Nanometrics, Inc.
 * 'dynenv' -- Dynamic environment offset in NAND OOB
 * (C) Copyright 2006-2007 OpenMoko, Inc.
 * Added 16-bit nand support
 * (C) 2004 Texas Instruments
 *
 * Copyright 2010 Freescale Semiconductor
 * The portions of this file whose copyright is held by Freescale and which
 * are not considered a derived work of GPL v2-only code may be distributed
 * and/or modified under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <common.h>
#include <linux/mtd/mtd.h>
#include <command.h>
#include <watchdog.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <mtd.h>
#include <image.h>

static struct mtd_info* mtd_get_boot(void)
{
	const char *name = getenv(MTD_ENV_NAME);
	if(name) {
		return mtd_by_name(name);
	}
	else {
		return mtd_by_name(CONFIG_MTD_BOOTNAME);
	}
}

int do_mtdboot(struct cmd_ctx *ctx, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	ulong addr;
	loff_t offset = 0;
	int ret;
	size_t image_size;

	mtd = mtd_get_boot();
	if(mtd == NULL) {
		printf("MTD wrong or invalid default device\n");
		return -1;
	}

	switch (argc) {
	case 1:
		getenv_ul("loadaddr", &addr, CONFIG_LOADADDR);
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		offset = simple_strtoull(argv[3], NULL, 16);
		break;
	default:
		return cmd_usage(ctx->cmdtp);
	}

	printf("MTD Loading kernel image: dev %s offset 0x%012llX addr 0x%08lX\n",
		mtd->name, offset, addr);

	ret = mtd_read_pages(mtd, offset, mtd->size, (u8*)addr, mtd->writesize);
	mboot_check_zero(ret, return ret,
		"MTD Read image header failed: ret %d\n", ret);

	ret = image_guess_size(addr, &image_size);
	mboot_check_zero(ret, return ret,
		"MTD failed to detect image size: ret %d\n", ret);

	ret = mtd_read_pages(mtd, offset, mtd->size, (u8*)addr, image_size);
	mboot_check_zero(ret, return ret,
		"MTD read image failed: ret %d\n", ret);

	/* Loading ok, update default load address */
	setenv_ul("loadaddr", "0x%08lX", addr);
	return 0;
}

U_BOOT_CMD(mtdboot, 4, 0, do_mtdboot,
	"boot from MTD device",
	"mtdboot [[loadAddr] offset]\n"
	"  Set " MTD_ENV_NAME " var to change a boot device"
);

