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
#include <malloc.h>
#include <mtd.h>
#include <errno.h>

#ifndef CONFIG_SYS_MTD_BASE_LIST
#define CONFIG_SYS_MTD_BASE_LIST { CONFIG_SYS_MTD_BASE }
#endif

LIST_HEAD(g_mtd_list);

/* Register mtd device. Return 0 on not-error (success or no device)
 * MTD subsystem uses malloc, it should be accessable at the moment of call
 */
int mtd_add(struct mtd_info *mtd)
{
	BUG_ON(mtd->name == NULL);
	list_add(&mtd->list, &g_mtd_list);
	return 0;
}

