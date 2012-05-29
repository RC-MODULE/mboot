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
#include <errno.h>

/* find command table entry for a command */
cmd_tbl_t *find_cmd_tbl(const char *cmd, cmd_tbl_t *table, int table_len)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = table; /*Init value */
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = table;
	     cmdtp != table + table_len;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp; /* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {	/* exactly one match */
		return cmdtp_temp;
	}

	return NULL; /* not found or ambiguous command */
}

cmd_tbl_t *find_cmd(const char *cmd)
{
	return find_cmd_tbl(cmd,
		&lds_u_boot_cmd_start,
		&lds_u_boot_cmd_end - &lds_u_boot_cmd_start);
}

/* Always fails */
int cmd_usage(struct cmd_tbl *cmdtp)
{
	printf("%s - %s\n\n", cmdtp->name, cmdtp->usage);
#ifdef	CONFIG_SYS_LONGHELP
	puts("Usage:\n");

	if (!cmdtp->help) {
		puts ("- No additional help available.\n");
		return 1;
	}

	puts (cmdtp->help);
	putc ('\n');
#endif	/* CONFIG_SYS_LONGHELP */
	return -EINVAL;
}

int cmd_get_data_size(char* arg, int default_size)
{
       /* Check for a size specification .b, .w or .l.
        */
       int len = strlen(arg);
       if (len > 2 && arg[len-2] == '.') {
               switch(arg[len-1]) {
               case 'b':
                       return 1;
               case 'w':
                       return 2;
               case 'l':
                       return 4;
               case 's':
                       return -2;
               default:
                       return -1;
               }
       }
       return default_size;
}

