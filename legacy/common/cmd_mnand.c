/*
 * Copyright 2010
 * Sergey Mironov (ierton@gmail.com)
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
#include <mtd.h>
#include <mnand.h>

int do_mnand(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	if (argc < 2) {
		return cmd_usage(cmdtp);
	}

	ret = 0;
	if(0 == strcmp(argv[1], "init")) {
		ret = mnand_init(&mtd_info[0]); 
	}
	else if(0 == strcmp(argv[1], "verbose")) {
		int verb = 1;
		if(argc >= 3) {
			verb = simple_strtoul(argv[2], NULL, 16);
		}
		g_mnand_debug = verb;
	}
	else if(0 == strcmp(argv[1], "spare-ignore")) {
		int val = 1;
		if(argc >= 3) {
			val = simple_strtoul(argv[2], NULL, 10);
		}
		g_mnand_ignore_ecc_in_spare = val;
	}
	else if(0 == strcmp(argv[1], "spare-skip-if-empty")) {
		int val = 1;
		if(argc >= 3) {
			val = simple_strtoul(argv[2], NULL, 10);
		}
		g_mnand_check_for_empty_spare = val;
	}
	else if(0 == strcmp(argv[1], "calc-ecc")) {
		int val = 1;
		if(argc >= 3) {
			val = simple_strtoul(argv[2], NULL, 10);
		}
		g_mnand_calculate_ecc = val;
		printf("use 'mnand reset' to apply\n");
	}
	else if(0 == strcmp(argv[1], "reset")) {
		struct mtd_info *mtd;
		for_all_mtd(mtd) {
			printf("mnand: resetting %s\n", mtd->name);
			mnand_reset(mtd);
		}
	}
	else {
		printf("mnand: invalid argument: '%s'\n", argv[1]);
		ret = -1;
	}

	if(ret != 0) 
		printf("mnand command failed with error: %d\n", ret);
	return ret;
}

U_BOOT_CMD(
	mnand,	3,	1,	do_mnand,
	"configures mnand subsytem",
	"\n"
	"init\n"
	"	  Initializes mnand subsystem\n"
	"     base_addr: mnand registers base address\n"
	"     Warning! There will be memory leaks if reinitializing. Beter use mnand reset.\n"
	"mnand reset\n"
	"     resets mnand\n"
	"mnand verbose [n]\n"
	"     Sets verbose flag to n (default - 1)\n"
	"mnand calc-ecc [0|1]\n"
	"     Calculate ecc with hardware (default - 1)\n"
	"mnand spare-ignore [0|1]\n"
	"     Ignore ECC from NAND spare-memory, if 1 (default)\n"
	"mnand spare-skip-if-empty [0|1]\n"
	"     Skip ECC check if spare contains FF (default - 1)\n"
);


