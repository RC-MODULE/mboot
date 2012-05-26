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

static int mtd_load_image(cmd_tbl_t *cmdtp, struct mtd_info *mtd,
			   loff_t offset, uint64_t maxsz, ulong addr, char *cmd)
{
	int fmt;
	int ret;
	char *ep;
	size_t cnt;
	image_header_t *hdr;
#if defined(CONFIG_FIT)
	const void *fit_hdr = NULL;
#endif

	printf("MTD Loading kernel image: dev %s offset 0x%08llX maxsz 0x%08llX\n",
		mtd->name, offset, maxsz);

	cnt = mtd->writesize;
	ret = mtd_read_pages(mtd, offset, offset + maxsz, (u8*)addr, cnt);
	if (ret) {
		printf("MTD Read image header failed: ret %d\n", ret);
		return ret;
	}

	fmt = genimg_get_format((void *)addr);
	switch (fmt) {
		case IMAGE_FORMAT_LEGACY:
			hdr = (image_header_t *)addr;

			image_print_contents (hdr);

			cnt = image_get_image_size (hdr);
			break;
#if defined(CONFIG_FIT)
		case IMAGE_FORMAT_FIT:
			fit_hdr = (const void *)addr;
			puts ("MTD fit image detected\n");

			cnt = fit_get_size (fit_hdr);
			break;
#endif
		default:
			printf("MTD Unknown image format: fmt %d\n", fmt);
			return 1;
	}

	ret = mtd_read_pages(mtd, offset, offset + maxsz, (u8*)addr, cnt);
	if (ret) {
		printf("MTD read image failed: ret %d\n", ret);
		return ret;
	}

#if defined(CONFIG_FIT)
	/* This cannot be done earlier, we need complete FIT image in RAM first */
	if (genimg_get_format ((void *)addr) == IMAGE_FORMAT_FIT) {
		if (!fit_check_format (fit_hdr)) {
			puts ("MTD Bad FIT image format\n");
			return 1;
		}
		fit_print_contents (fit_hdr);
	}
#endif

	/* Loading ok, update default load address */
	setenv_ul("loadaddr", "0x%08lX", addr);

	/* Check if we should attempt an auto-start */
	if (((ep = getenv("autostart")) != NULL) && (strcmp(ep, "yes") == 0)) {
		char *local_args[2];
		extern int do_bootm(cmd_tbl_t *, int, int, char *[]);

		local_args[0] = cmd;
		local_args[1] = NULL;

		printf("MTD: Automatic boot of image at addr 0x%08lX\n", addr);

		do_bootm(cmdtp, 0, 1, local_args);
		return 1;
	}
	return 0;
}

int do_mtdboot(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	ulong addr;
	loff_t offset = 0;

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
		return cmd_usage(cmdtp);
	}

	return mtd_load_image(cmdtp, mtd, offset, mtd->size, addr, argv[0]);
}

U_BOOT_CMD(mtdboot, 4, 0, do_mtdboot,
	"boot from MTD device",
	"[[loadAddr] offset]\n"
	"  Set " MTD_ENV_NAME " var to change a boot device"
);

