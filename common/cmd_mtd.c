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

#if defined(CONFIG_CMD_MTDPARTS)
#error Not in UEMD
#endif

static int mtd_dump(struct mtd_info *mtd, loff_t off, int only_oob)
{
	struct mtd_oob_ops ops;
	int i;
	u_char *datbuf, *oobbuf, *p;
	int ret;

	datbuf = malloc(mtd->writesize + mtd->oobsize);
	if(!datbuf) {
		puts("No memory for data buffer\n");
		return -1;
	}

	oobbuf = malloc(mtd->oobsize);
	if(!oobbuf) {
		free(datbuf);
		puts("No memory for oob buffer\n");
		return -1;
	}

	if(off != (off & ~(mtd->writesize - 1))) {
		off &= ~(mtd->writesize - 1);
		printf("Nearest page-aligned offset: off 0x%08llX\n", off);
	}

	memset(&ops, 0, sizeof(ops));
	ops.datbuf = datbuf;
	ops.oobbuf = oobbuf;
	ops.len = mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ops.mode = MTD_OOB_RAW;

	ret = mtd->read_oob(mtd, off, &ops);
	if (ret < 0) {
		printf("MTD error reading page: off 0x%08llX ret %d\n", off, ret);
	}

	printf("MTD page dump: name %s off 0x%08llX\n", mtd->name, off);
	i = mtd->writesize >> 4;
	p = datbuf;
	while (i--) {
		if (!only_oob)
			printf("\t%02x %02x %02x %02x %02x %02x %02x %02x"
			       "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
			       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
			       p[8], p[9], p[10], p[11], p[12], p[13], p[14],
			       p[15]);
		p += 16;
	}

	puts("OOB:\n");
	i = mtd->oobsize >> 3;
	p = oobbuf;
	while (i--) {
		printf("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
	}

	printf("MTD page dump complete: name %s off 0x%08llX next 0x%08llX\n",
		mtd->name, off, (off + mtd->writesize));

	free(datbuf);
	free(oobbuf);
	return ret;
}

static inline int str2off(const char *p, loff_t *num)
{
	char *endptr;
	*num = simple_strtoull(p, &endptr, 16);
	return (*p != '\0' && *endptr == '\0') ? 0 : -1;
}

static inline int str2sz(const char *p, unsigned long *l)
{
	char *endptr;
	*l = simple_strtoul(p, &endptr, 0);
	return (*p != '\0' && *endptr == '\0') ? 0 : -1;
}

static inline int str2szll(const char *p, uint64_t *ll)
{
	char *endptr;
	*ll = simple_strtoull(p, &endptr, 0);
	return (*p != '\0' && *endptr == '\0') ? 0 : -1;
}

static inline int str2addr(const char *p, unsigned long *num)
{
	char *endptr;
	*num = simple_strtoul(p, &endptr, 16);
	return (*p != '\0' && *endptr == '\0') ? 0 : -1;
}

static void mtd_print_info(struct mtd_info *mtd)
{
	printf("mtd_nfo: name       %s\n", mtd->name);
	printf("mtd_nfo: size       0x%08llX (%llu b / %llu M)\n",
		mtd->size, mtd->size, mtd->size/(1024*1024));
	printf("mtd_nfo: write_size 0x%08X (%u)\n", mtd->writesize, mtd->writesize);
	printf("mtd_nfo: oob_size   0x%08X (%u)\n", mtd->oobsize, mtd->oobsize);
	printf("mtd_nfo: erase_size 0x%08X (%u)\n", mtd->erasesize, mtd->erasesize);
	printf("mtd_nfo: pages_per_block %d\n", mtd->erasesize / mtd->writesize);
}

#define check(x, err) do { if(!(x)) { err ; } }while(0)
#define check_(x, str, err) do { if(!(x)) { printf("%s\n", str); err ; } }while(0)

int do_mtd(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	char *cmd;
	struct mtd_info *mtd;

	if (argc < 2)
		goto usage;

	cmd = argv[1];

	if(strcmp(cmd, "info") == 0) {
		for_all_mtd(mtd) {
			mtd_print_info(mtd); puts("\n");
		}
		return 0;
	}

	if(strcmp(cmd, "device") == 0) {
		check(argc == 3, goto usage);
		setenv_s(MTD_ENV_NAME, argv[2]);
		return 0;
	}

	mtd = mtd_get_default();
	if(mtd == NULL) {
		printf("No MTD devices available\n");
		return -1;
	}

	if (strcmp(cmd, "bad") == 0) {
		loff_t off;
		printf("MTD %s bad blocks:\n", mtd->name);
		for (off = 0; off < mtd->size; off += mtd->erasesize)
			if (mtd_block_isbad(mtd, off))
				printf("  0x%08llX\n",(unsigned long long)off);
		return 0;
	}

	if (strcmp(cmd, "erase") == 0 ||
		strcmp(cmd, "scrub") == 0 ) {

		uint64_t size;
		loff_t off;

		check(argc >= 3, goto usage);

		ret = str2off(argv[2], &off);
		check(ret == 0, goto usage);

		if(argc == 4) {
			ret = str2szll(argv[3], &size);
			check(ret == 0, goto usage);
		}
		else {
			size = mtd->erasesize;
		}

		ret = mtd_erase(mtd, off, size, (cmd[0] == 'e'));
		if(ret < 0)
			printf("mtd_erase_opts failed: ret %d\n", ret);

		return ret;
	}

	if (strcmp(cmd, "dump") == 0) {
		static loff_t last_off = 0;
		loff_t off;

		check(argc >= 3, goto usage);

		ret = str2off(argv[2], &off);
		check(ret == 0, goto usage);

		if(flag & CMD_FLAG_REPEAT)
			off = last_off + mtd->writesize;

		ret = mtd_dump(mtd, off, 0);
		if(ret < 0) {
			printf("mtd_dump failed: ret %d\n",ret);
			return ret;
		}

		last_off = off;
		return 0;
	}

	if (strcmp(cmd, "read") == 0 ||
	    strcmp(cmd, "ow") == 0 ||
		strcmp(cmd, "write") == 0 ) {

		ulong addr, size;
		loff_t off;
		check(argc == 5, goto usage);

		ret = str2addr(argv[2], &addr);
		check_(ret == 0, "Invalid addr", goto usage);

		ret = str2off(argv[3], &off);
		check_(ret == 0, "Invalid off", goto usage);

		ret = str2sz(argv[4], &size);
		check_(ret == 0, "Invalid size", goto usage);

		switch(cmd[0]) {
			case 'r':
				ret = mtd_read_pages(mtd, off, mtd->size, (u8*)addr, size);
				break;
			case 'w':
				ret = mtd_write_pages(mtd, off, mtd->size, (u8*)addr, size);
				break;
			case 'o':
				ret = mtd_overwrite_pages(mtd, off, mtd->size, (u8*)addr, size);
				break;
			default:
				goto usage;
		}
		if(ret < 0) {
			printf("MTD %s command failed: ret %d\n", cmd, ret);
		}
		return ret;
	}

	if (strcmp(cmd, "markbad") == 0) {
		int i;
		loff_t off;

		check(argc >= 3, goto usage);

		for (i=2; i<argc; i++) {

			ret = str2off(argv[i], &off);
			check(ret == 0, goto usage);

			ret = mtd_block_markbad(mtd, off);
			if(ret < 0) {
				printf("mtd_block_markbad failure: addr 0x%08llX ret %d\n",
					off, ret);
				continue;
			}
			printf("MTD block marked as bad: off 0x%08llx\n", off);
		}

		return 0;
	}

usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	mtd, CONFIG_SYS_MAXARGS, 0, do_mtd,
	"MTD subsystem",
	"info - show available MTD devices\n"
	"mtd device [dev] - show or set current device\n"
	"mtd read addr off size\n"
	"mtd write addr off size\n"
	"mtd ow addr off size\n"
	"mtd erase off [size]\n"
	"	- erases all the data\n"
	"mtd scrub off [size]\n"
	"	- erases all the data including bad blocks\n"
	"mtd bad\n"
	"	- show bad blocks\n"
	"mtd dump off\n"
	"	- dumps the page\n"
	"mtd markbad off [off ...]\n"
	"	- mark bad block(s) at offset\n"
);

static int mtd_load_image(cmd_tbl_t *cmdtp, struct mtd_info *mtd,
			   loff_t offset, uint64_t maxsz, ulong addr, char *cmd)
{
	int fmt;
	int ret;
	char *ep, *s;
	size_t cnt;
	image_header_t *hdr;
#if defined(CONFIG_FIT)
	const void *fit_hdr = NULL;
#endif

	s = strchr(cmd, '.');
	if (s != NULL &&
	    (strcmp(s, ".jffs2") && strcmp(s, ".e") && strcmp(s, ".i"))) {
		printf("MTD Unknown nand load suffix '%s'\n", s);
		return -1;
	}

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
			puts ("MTD: Bad FIT image format\n");
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
	struct mtd_info * mtd;
	char boot_device[MAXPATH];
	ulong addr;
	loff_t offset = CONFIG_SYS_KERNEL_OFF;
	loff_t sz = CONFIG_SYS_KERNEL_SIZE;

	switch (argc) {
	case 1:
		getenv_ul("loadaddr", &addr, CONFIG_LOADADDR);
		getenv_s(MTD_ENV_NAME, &boot_device[0], "mtd0");
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		getenv_s(MTD_ENV_NAME, &boot_device[0], "mtd0");
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		strncpy_s(boot_device, argv[2], MAXPATH);
		break;
	case 4:
		addr = simple_strtoul(argv[1], NULL, 16);
		strncpy_s(boot_device, argv[2], MAXPATH);
		offset = simple_strtoull(argv[3], NULL, 16);
		break;
	case 5:
		addr = simple_strtoul(argv[1], NULL, 16);
		strncpy_s(boot_device, argv[2], MAXPATH);
		offset = simple_strtoull(argv[3], NULL, 16);
		sz = simple_strtoull(argv[4], NULL, 16);
		break;
	default:
		return cmd_usage(cmdtp);
	}

	mtd = mtd_by_name(boot_device);
	if(mtd == NULL) {
		printf("MTD device '%s' not available\n", boot_device);
		return -1;
	}

	return mtd_load_image(cmdtp, mtd, offset, sz, addr, argv[0]);
}

U_BOOT_CMD(mtdboot, 4, 0, do_mtdboot,
	"boot from MTD device",
	"[[[[loadAddr] dev] offset] maxsz]"
);

#ifdef CONFIG_SYS_MTD_QUIET
#error Not in UEMD
#endif
