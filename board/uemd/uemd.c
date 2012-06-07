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

#define CONFIG_BOOTARGS    "earlyprintk=serial console=ttyS0,38400n8 root=/dev/nfs nfsroot=10.0.0.1:/home/smironov/arm-module2-linux-gnueabi ip=10.0.0.2:10.0.0.1:10.0.0.1:255.255.255.0:UEMD:eth0:off greth.pure10Mbit=1 debug"

#define CONFIG_BOOTARGS_MTD "earlyprintk=serial console=ttyS0,38400n8 root=/dev/mtdblock3 ip=10.0.0.2:10.0.0.1:10.0.0.1:255.255.255.0:UEMD:eth0:off debug"

static struct env_var g_env_def[] = {
	ENV_VAR("bootargs",   CONFIG_BOOTARGS_MTD),
	ENV_VAR("bootcmd",    CONFIG_BOOTCOMMAND),
	ENV_VAR("bootdelay",  STR(CONFIG_BOOTDELAY)),
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

void board_reset(void)
{
	MEM(0x20025000 + 0xf00) = 1;
	MEM(0x20025000 + 0xf04) = 1;
}

int board_eth_init(void)
{
	return greth_initialize();
}

void board_hang (void)
{
	for (;;);
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

#define MTDALL "mnand"
#define MTDENV "env"
#define MTDBOOT CONFIG_MTD_BOOTNAME

void uemd_init(struct uemd_otp *otp)
{
	//DECLARE_GLOBAL_DATA_PTR;
	int ret;
	int netn;

	/* TIMERS */
	uemd_timer_init();

	/* SERIAL */
	ret = uemd_console_init();
	if(ret < 0)
		goto err_noconsole;

	printf("MBOOT (UEMD mode): Version %s (Built %s)\n",
		MBOOT_VERSION, MBOOT_DATE);
	printf("OTP info: boot_source %u jtag_stop %u words_len %u\n",
		otp->source, otp->jtag_stop, otp->words_length);

	/* SDRAM */
	struct memregion sdram;
	ret = uemd_em_init_check(&sdram);
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

	/* MTD/MNAND */
	struct mtd_info mtd_mnand = MTD_INITIALISER(MTDALL);

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
	netn = eth_initialize();
	if(netn <= 0) {
		uemd_error("Failed to initialise ethernet");
	}

	/* MAIN LOOP */
	struct main_state ms;
	main_state_init(&ms);

	ulong bootdelay_sec;
	const char *bootcmd;

	bootcmd = getenv("bootcmd");
	getenv_ul("bootdelay", &bootdelay_sec, 0);
	if(bootdelay_sec>0 && bootcmd) {
		printf("Hit any key (in %lu sec) to skip autoload...",
			bootdelay_sec);
		ulong base = get_timer(0);
		while (get_timer(base) < bootdelay_sec*1000) {
			if(tstc()) break;
		}
		if(tstc()) {
			printf("skipped\n");
		}
		else {
			printf("\nRunning autoload command '%s'\n",
				bootcmd);
			run_command(&ms,bootcmd,0,NULL);
		}
	}

	while(! MAIN_STATE_HAS_ENTRY(&ms)) {
		ret = main_process_command(&ms);
		if(ret < 0) {
			if (ret == -EINTR) {
				puts("<INTR>\n");
			}
			else {
				printf("process_command failed: ret %d\n", ret);
				goto err;
			}
		}
	}

	ulong machid;
	getenv_ul("machid", &machid, CONFIG_UEMD_MACH_TYPE);
	printf("Linux preparing to boot the kernel: machid 0x%lx\n", machid);

	struct tag *tag;
	struct tag *tag_base = (struct tag *)CONFIG_SYS_PARAM_ADDR;
	const char *bootargs = getenv_def("bootargs", CONFIG_BOOTARGS);
	char *cmdline;
	linux_tag_start(&tag, tag_base);
	linux_tag_memory(&tag, &sdram);
	linux_tag_cmdline_start(&tag, &cmdline);
	linux_tag_cmdline_add(&cmdline, "%s", bootargs);
	linux_tag_cmdline_add_space(&cmdline);
	int first = 1;
	for_all_mtdparts_of(part, MTDALL) {
		if(first) {
			linux_tag_cmdline_add(&cmdline, "mtdparts=%s:0x%llX",
				MTDALL,part->mtd.size);
			first = 0;
		}
		else
			linux_tag_cmdline_add(&cmdline, ",0x%llX",
				part->mtd.size);
	}
	linux_tag_cmdline_end(&tag, &cmdline);
	linux_tag_end(&tag);

	union image_entry_point ep;
	ret = image_move_unpack(&ms.os_image, &ep);
	uemd_check_zero(ret, goto err, "image move-unpack failed");
	printf("Linux tags start 0x%p end 0x%p\n", tag_base, tag);
	printf("Linux entry 0x%08lX\n", ep.addr);


	ep.linux_ep(0, machid, tag_base);

err:
	board_hang();
	return;

err_noconsole:
	board_hang();
	return;
}

