/*
 * (C) Copyright 2013 RC Module
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
#include <main.h>
#include <errno.h>
#include <mtd.h>
#include <mtdpart.h>
#include <mnand.h>
#include <malloc.h>

extern char console_buffer[];


static int do_pmgr (struct cmd_ctx *ctx, int argc, char * const argv[])
{
	int len;
	char* name; 
	char parts[512];

	parts[0]=0;

	printf("This command will repartition your NAND device\n");
	printf("And you will likely loose all data, except uboot and env. \n");

	len = readline("Type 'OKAY' if you are really sure: ");
	if ( strcmp(console_buffer,"OKAY")!=0 ) {
		printf("Cancelled\n");
		goto bailout;
	}
	struct mtd_part* part = NULL;
	for_all_mtdparts(part) {
		sprintf(console_buffer, "setenv %s_size\n",
		       part->name);
		run_command(ctx->ms, console_buffer, 0, NULL);
	}
	run_command(ctx->ms, "setenv parts", 0, NULL);

	//struct mtd_info *mtd = mtd_get_default();
	
	while (1) {
		loff_t sz;
		int stop=0;
		len = readline("Part name: ");
		if (len == -EINTR)
			goto bailout;
		name = strdup(console_buffer);
		len = readline("Size in MiBS or '-' to use all that's left: ");
		if (len == -EINTR)
			goto bailout;

		sz = simple_strtoull(console_buffer, NULL, 10);
		sz *= 1024 * 1024;

		strcat(parts, name);
		strcat(parts, ",");
		
		if (sz)
			sprintf(console_buffer, "setenv %s_size 0x%llx", name, sz);
		else {
			sprintf(console_buffer, "setenv %s_size -", name );	
			stop = 1;
		}
		run_command(ctx->ms, console_buffer, 0, NULL);
		free(name);
		if(stop)
			break;
	}
	sprintf(console_buffer, "setenv parts %s", parts);
	run_command(ctx->ms, console_buffer, 0, NULL);

	printf("Environment has been altered. Run saveenv and reset to commit changes.\n");

bailout:
	return 0;
}

/* -------------------------------------------------------------------- */


U_BOOT_CMD(
	pmgr, 1, 0,	do_pmgr,
	"NAND partition manager",
	""
);



