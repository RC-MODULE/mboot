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
#include <mtdpart.h>
#include <mnand.h>
#include <timestamp.h>
#include <version.h>
#include <malloc.h>
#include <serial.h>
#include <command.h>
#include <compiler.h>
#include <errno.h>
#include <main.h>

#include "uemd.h"

#define _MK_STR(x)	#x
#define STR(x)	_MK_STR(x)

static struct env_var g_env_def[] = {
	ENV_VAR("bootargs",   CONFIG_BOOTARGS),
#ifdef CONFIG_BOOTCMD
	ENV_VAR("bootcmd",    CONFIG_BOOTCMD),
#endif
#ifdef CONFIG_BOOTDELAY
	ENV_VAR("bootdelay",  CONFIG_BOOTDELAY),
#endif 
#ifdef CONFIG_AUTOLOAD
	ENV_VAR("autoload",   CONFIG_AUTOLOAD),
#endif
	ENV_VAR("ethaddr",    CONFIG_ETHADDR),
	ENV_VAR("ipaddr",     CONFIG_IPADDR),
	ENV_VAR("serverip",   CONFIG_SERVERIP),
	ENV_VAR("gatewayip",  CONFIG_GATEWAYIP),
	ENV_VAR("netmask",    CONFIG_NETMASK),
#ifdef CONFIG_HOSTNAME
	ENV_VAR("hostname",   CONFIG_HOSTNAME),
#endif
	ENV_VAR("bootfile",   CONFIG_BOOTFILE),
	ENV_VAR("loadaddr",   STR(CONFIG_LOADADDR)),
	ENV_NULL
};

void reset_cpu(ulong addr)
{
	MEM(0x20025000 + 0xf00) = 1;
	MEM(0x20025000 + 0xf04) = 1;
}

int board_eth_init(bd_t *bis)
{
	int ret = -1;
#ifdef CONFIG_GRETH
	ret = greth_initialize();
#endif
	return ret;
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

	if(mtd == NULL) {
		uemd_error("Environment partition is not defined");
		return -EINVAL;
	}

	*len = MIN(*len, mtd->size);

	ret = mtd_read_pages(mtd, 0, mtd->size, (u8*)buf, *len);
	if(ret < 0) {
		*len = 0;
		uemd_error("Failed to read env from MTD: name %s ret %d",
			mtd->name, ret);
		return ret;
	}
	return 0;
}

static int uemd_env_write(const char* buf, size_t len, void *priv)
{
	struct mtd_info *mtd = (struct mtd_info*) priv;
	int ret;

	if(mtd == NULL) {
		uemd_error("Environment partition is not defined");
		return -EINVAL;
	}

	ret = mtd_overwrite_pages(mtd, 0, mtd->size, (u8*)buf, len);
	if(ret < 0) {
		uemd_error("Failed to overwrite env section of MTD: name %s ret %d",
			mtd->name, ret);
		return ret;
	}

	return 0;
}

static struct env_ops g_uemd_env_ops = {
	.readenv = uemd_env_read,
	.writeenv = uemd_env_write,
};

#define MTDENV "env"
#define MTDBOOT CONFIG_MTD_BOOTNAME

void uemd_init(struct uemd_otp *otp)
{
	DECLARE_GLOBAL_DATA_PTR;
	int ret;
	int netn;

	uemd_timer_init();

	/* SERIAL */
	ret = uemd_console_init();
	if(ret < 0)
		goto err;

	printf("MBOOT (UEMD mode): Version %s (Built %s)\n",
		MBOOT_VERSION, MBOOT_DATE);
	printf("OTP info: boot_source %u jtag_stop %u words_len %u\n",
		otp->source, otp->jtag_stop, otp->words_length);

	/* Global data (U-boot legacy) */
	ret = uemd_gd_init();
	uemd_check_zero(ret, goto err, "GD init failed");

	printf("Hardcoded MachineID 0x%x (may be changed via env)\n", gd->bd->bi_arch_number);

	ret = uemd_em_init_check(&gd->bd->bi_dram[0], CONFIG_NR_DRAM_BANKS);
	uemd_check_zero(ret, goto err, "SDRAM init failed");

	/* MALLOC */
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

	/* MTD/MNAND */
	struct mtd_info mtd_mnand = MTD_INITIALISER("mnand");

	ret = mnand_init(&mtd_mnand);
	uemd_check_zero(ret, goto err, "MNAND init failed");

	ret = mtd_add(&mtd_mnand);
	uemd_check_zero(ret, goto err, "MTD add failed");

	struct mtd_part mtd_parts[] = {
		MTDPART_INITIALIZER("boot",   0,                  0x40000),
		MTDPART_INITIALIZER(MTDENV,   MTDPART_OFS_NXTBLK, 0x40000),
		MTDPART_INITIALIZER(MTDBOOT,  MTDPART_OFS_NXTBLK, 0x800000),
		MTDPART_INITIALIZER("user",   MTDPART_OFS_NXTBLK, MTDPART_SIZ_FULL),
		MTDPART_NULL,
	};

	ret = mtdparts_add(&mtd_mnand, mtd_parts);
	uemd_check_zero(ret, goto err, "MTD failed to register parts");

	struct mtd_part *part = NULL;
	struct mtd_part *part_env = NULL;
	for_all_mtdparts(part) {
		const char *envmsg = "";
		if(!part_env && 0 == strcmp(part->name, MTDENV)) {
			part_env = part;
			envmsg = " (env)";
		}
		printf("MTD Partition: name %10s off 0x%08llX size 0x%08llX %s\n",
			part->name, part->offset, part->mtd.size, envmsg);
	}

	/* ENV */
	ret = env_init(&g_uemd_env_ops, g_env_def, part_env);
	uemd_check_zero(ret, goto err, "Env init failed");

	/* ETH */
#ifdef CONFIG_NET_MULTI
	netn = eth_initialize(gd->bd);
	if(netn <= 0) {
		uemd_error("Failed to initialise ethernet");
	}
#endif

	struct main_state ms;
	memset(&ms, 0, sizeof(struct main_state));

	while(ms.imageaddr == NULL) {
		ret = main_process_command(&ms);
		uemd_check_zero(ret, goto err, "process command failed");
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

