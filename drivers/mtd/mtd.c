/*
 * (C) Copyright 2010
 * Sergey Mironov <ierton@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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

/*
 * This file provides basic implementation of nand layer
 * which can be used with cmd_jffs2
 */

#include <common.h>
#include <nand.h>
#include <mtd.h>

#ifndef CONFIG_SYS_MTD_BASE_LIST
#define CONFIG_SYS_MTD_BASE_LIST { CONFIG_SYS_MTD_BASE }
#endif

nand_info_t mtd_info[CONFIG_SYS_MAX_MTD_DEVICE];

/* Register mtd device. Return 0 on not-error (success or no device) */
int mtd_init(void)
{
	int ret;
	ulong base_address[CONFIG_SYS_MAX_MTD_DEVICE] = CONFIG_SYS_MTD_BASE_LIST;

	memset(mtd_info, 0, sizeof(mtd_info));

	ret = board_mtd_init(mtd_info, base_address, CONFIG_SYS_MAX_MTD_DEVICE);
	if(ret < 0) {
		error("faild to register mtd devices: ret %d", ret);
		goto err;
	}

	return 0;

err:
	return ret;
}

