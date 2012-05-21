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
#include <netdev.h>
#include <mtd.h>
#include <mnand.h>
#include <timestamp.h>
#include <version.h>
#include <malloc.h>
#include <serial.h>
#include <command.h>
#include <compiler.h>
#include <errno.h>

#include "uemd.h"

void reset_cpu(ulong addr)
{
	MEM(0x20025000 + 0xf00) = 1;
	MEM(0x20025000 + 0xf04) = 1;
}

int board_eth_init(bd_t *bis)
{
	int ret = -1;
#ifdef CONFIG_GRETH
	ret = greth_initialize(bis);
#endif
	return ret;
}

int board_mtd_init(struct mtd_info *mtd, ulong *base_addr, int cnt)
{
	int ret = -1;
	int num = 0;
#if defined(CONFIG_MTD_MNAND)
	ret = mnand_init(&mtd[0], base_addr[0]); 
	if(ret < 0) return ret;
	num++;
#endif
	return num;
}

void hang (void)
{
	puts ("ERROR: Please reset the board\n");
	for (;;);
}

static void uemd_hang(void)
{
	for (;;);
}

const char version_string[] =
	U_BOOT_VERSION" (" U_BOOT_DATE " - " U_BOOT_TIME ")";

static int uemd_gd_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	if(sizeof(gd_t) > CONFIG_SYS_GD_SIZE)
		return -1;
	if(sizeof(bd_t) > CONFIG_SYS_BD_SIZE)
		return -2;

	memset((void*)(CONFIG_SYS_GD_ADDR), 0, CONFIG_SYS_GD_SIZE);
	memset((void*)(CONFIG_SYS_BD_ADDR), 0, CONFIG_SYS_BD_SIZE);
	gd = (gd_t *) (CONFIG_SYS_GD_ADDR);

	gd->bd = (bd_t *) (CONFIG_SYS_BD_ADDR);
	gd->bd->bi_arch_number = CONFIG_UEMD_MACH_TYPE;
	gd->bd->bi_boot_params = CONFIG_SYS_PARAM_ADDR;
	return 0;
}

static int uemd_env_read(char* buf, size_t *len, void *priv)
{
	struct mtd_info *mtd = (struct mtd_info*) priv;
	int ret;
	
	*len = MIN(*len,CONFIG_MNAND_ENV_SIZE);

	ret = mtd_read_pages(mtd,
		CONFIG_MNAND_ENV_OFF, CONFIG_MNAND_ENV_OFF + CONFIG_MNAND_ENV_SIZE,
		(u8*)buf, *len);
	if(ret < 0) {
		*len = 0;
		uemd_error("Failed to read env from MTD: ret %d", ret);
		return ret;
	}
	return 0;
}

static int uemd_env_write(const char* buf, size_t len, void *priv)
{
	struct mtd_info *mtd = (struct mtd_info*) priv;
	int ret;

	ret = mtd_erase(mtd, CONFIG_MNAND_ENV_OFF, CONFIG_MNAND_ENV_SIZE, 1);
	if(ret < 0) {
		uemd_error("Failed to erase env section of MTD: ret %d", ret);
		return ret;
	}

	ret = mtd_write_pages(mtd,
		CONFIG_MNAND_ENV_OFF, CONFIG_MNAND_ENV_OFF + CONFIG_MNAND_ENV_SIZE,
		(u8*)buf, mtd_full_pages(mtd,len));
	if(ret < 0) {
		uemd_error("Failed to write env to MTD: ret %d", ret);
		return ret;
	}
	return 0;
}

static struct env_ops g_uemd_env_ops = {
	.readenv = uemd_env_read,
	.writeenv = uemd_env_write,
};

void uemd_init(struct uemd_otp *otp)
{
	DECLARE_GLOBAL_DATA_PTR;
	struct mtd_info *mtd;
	int ret;
	int netn;

	uemd_timer_init();

	ret = uemd_console_init();
	if(ret < 0)
		goto err;

	printf("U-boot (UEMD mode): %s\n", version_string);
	printf("OTP info: boot_source %u jtag_stop %u words_len %u\n",
		otp->source, otp->jtag_stop, otp->words_length);

	/* After that it is safe to use gd */
	ret = uemd_gd_init();
	if(ret != 0) {
		uemd_error("GD init failed: ret %d", ret);
		goto err;
	}

	printf("Hardcoded MachineID 0x%x (may be changed via env)\n", gd->bd->bi_arch_number);

	ret = uemd_em_init_check(&gd->bd->bi_dram[0], CONFIG_NR_DRAM_BANKS);
	if(ret != 0) {
		uemd_error("SDRAM init failed: ret %d", ret);
		goto err;
	}

	mem_malloc_init(CONFIG_SYS_MALLOC_ADDR, CONFIG_SYS_MALLOC_SIZE);

	printf("Memory layout\n");
	printf("\t0x%08X  early\n", (uint)g_early_start);
	printf("\t0x%08X  text\n", (uint)g_text_start);
	printf("\t0x%08X  data\n", (uint)g_data_start);
	printf("\t0x%08X  signature\n", (uint)g_signature_start);
	printf("\t0x%08X  bss_start\n", (uint)g_bss_start);
	printf("\t0x%08X  stack_start\n", (uint)g_bss_end);
	printf("\t0x%08X^ stack_ptr\n", CONFIG_SYS_SP_ADDR);
	printf("\t0x%08X  malloc\n", CONFIG_SYS_MALLOC_ADDR);
	printf("\t0x%08X  env\n", CONFIG_SYS_ENV_ADDR);
	printf("\t0x%08X  bd\n", CONFIG_SYS_BD_ADDR);
	printf("\t0x%08X  gd\n", CONFIG_SYS_GD_ADDR);

#ifdef CONFIG_GENERIC_MTD
	ret = mtd_init();
	if(ret < 0) {
		uemd_error("Failed to register MTD device: ret %d", ret);
		goto err;
	}

	printf("NAND layout\n");
	printf("\t0x%08llX  u-boot\n", CONFIG_MNAND_UBOOT_OFF);
	printf("\t0x%08llX  env\n",    CONFIG_MNAND_ENV_OFF);
	printf("\t0x%08llX  kernel\n", CONFIG_MNAND_KERNEL_OFF);
	printf("\t0x%08llX  userdata\n",   CONFIG_MNAND_USERDATA_OFF);
#endif

	mtd = mtd_by_index(0);
	if(mtd == NULL) {
		uemd_error("Failed to get MTD dev to read env from");
		goto err;
	}

	ret = env_init(&g_uemd_env_ops, mtd);
	if(ret < 0) {
		uemd_error("Failed to initialise env: ret %d", ret);
		goto err;
	}

#ifdef CONFIG_NET_MULTI
	netn = eth_initialize(gd->bd);
	if(netn <= 0) {
		uemd_error("Failed to initialise ethernet");
	}
#endif

	for (;;) {
		main_loop ();
	}

err:
	uemd_hang();
}


#ifdef CONFIG_SERIAL_MULTI
#error Not in UEMD
#endif
#ifdef CONFIG_LOGBUFFER
#error Not in UEMD
#endif
#ifdef CONFIG_POST
#error Not in UEMD
#endif
#if defined(CONFIG_CMD_NAND)
#error Not in UEMD
#endif
#if defined(CONFIG_CMD_ONENAND)
#error Not in UEMD
#endif
#ifdef CONFIG_GENERIC_MMC
#error Not in UEMD
#endif
#ifdef CONFIG_HAS_DATAFLASH
#error Not in UEMD
#endif
#ifdef CONFIG_VFD
#error Not in UEMD
#endif
#if defined(CONFIG_API)
#error Not in UEMD
#endif
#if defined(CONFIG_ARCH_MISC_INIT)
#error Not in UEMD
#endif
#if defined(CONFIG_MISC_INIT_R)
#error Not in UEMD
#endif
#ifdef BOARD_LATE_INIT
#error Not in UEMD
#endif
#ifdef CONFIG_BITBANGMII
#error Not in UEMD
#endif

