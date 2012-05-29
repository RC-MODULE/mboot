/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <command.h>
#include <image.h>
#include <environment.h>
#include <errno.h>
#include <main.h>

int do_bootm (struct cmd_ctx *ctx, int argc, char * const argv[])
{
	int ret;
	unsigned long addr;

	switch(argc) {
		case 1:
			getenv_ul("loadaddr", &addr, CONFIG_LOADADDR);
			break;
		case 2:
			addr = simple_strtoul(argv[1], NULL, 16);
			break;
		default:
			return cmd_usage(ctx->cmdtp);
	}

	ret = image_scan(addr, &ctx->ms->os_image);
	if(ret < 0) {
		printf("BOOTM scan image failed: addr 0x%08lX ret %d\n",
			addr, ret);
		return -1;
	}

	return 0;
}


U_BOOT_CMD(
	bootm,	CONFIG_SYS_MAXARGS,	1,	do_bootm,
	"boot application image from memory",
	""
);

#if defined(CONFIG_CMD_BOOTD)
int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return run_command (getenv ("bootcmd"), flag);
}

U_BOOT_CMD(
	bootd, 1,	1,	do_bootd,
	"boot default, i.e., run 'bootcmd'",
	""
);
#endif

