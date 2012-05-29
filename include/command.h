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

#ifndef COMMAND_H
#define COMMAND_H

#ifndef	__ASSEMBLY__

struct cmd_tbl;

struct cmd_ctx {
	const char *cmdline;
	int repeat;
	struct main_state *ms;
	struct cmd_tbl *cmdtp;
};

struct cmd_tbl {
	char *name;		/* Command Name	*/
	int	maxargs;	/* maximum number of arguments */
	int	repeatable;	/* autorepeat allowed? */
	int	(*cmd)(struct cmd_ctx * ctx, int argc, char * const argv[]);
	char *usage;	/* Usage message (short) */
#ifdef	CONFIG_SYS_LONGHELP
	char *help;		/* Help  message (long)	*/
#endif
};

typedef struct cmd_tbl cmd_tbl_t;

extern cmd_tbl_t lds_u_boot_cmd_start;
extern cmd_tbl_t lds_u_boot_cmd_end;

cmd_tbl_t *find_cmd(const char *cmd);
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len);

int cmd_usage(struct cmd_tbl *cmdtp);

int cmd_get_data_size(char* arg, int default_size);

#endif	/* __ASSEMBLY__ */

#define Struct_Section  __attribute__ ((unused,section (".u_boot_cmd")))

#ifdef  CONFIG_SYS_LONGHELP

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage, help}

#define U_BOOT_CMD_MKENT(name,maxargs,rep,cmd,usage,help) \
{#name, maxargs, rep, cmd, usage, help}

#else	/* no long help info */

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage}

#define U_BOOT_CMD_MKENT(name,maxargs,rep,cmd,usage,help) \
{#name, maxargs, rep, cmd, usage}

#endif	/* CONFIG_SYS_LONGHELP */

#endif	/* COMMAND_H */
