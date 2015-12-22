/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Add to readline cmdline-editing by
 * (C) Copyright 2005
 * JinHua Luo, GuangDong Linux Center, <luo.jinhua@gd-linux.com>
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
#include <main.h>
#include <watchdog.h>
#include <errno.h>

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
void __show_boot_progress (int val) {
}
void show_boot_progress (int val) __attribute__((weak, alias("__show_boot_progress")));

extern int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#undef DEBUG_PARSER

char console_buffer[CONFIG_SYS_CBSIZE + 1];	/* console I/O buffer	*/
static char lastcommand[CONFIG_SYS_CBSIZE] = { 0, };

static char *delete_char (char *buffer, char *p, int *colp, int *np, int plen);
static char erase_seq[] = "\b \b";   /* erase sequence	*/
static char tab_seq[]   = "        ";/* used to expand TABs	*/

/* Process one user command */
int main_process_command(struct main_state *s)
{
	int len;
	int ret;

	len = readline(CONFIG_SYS_PROMPT);
	if(len < 0) {
		if(len != -EINTR) {
			printf("MAIN: readline error: ret %d\n", len);
		}
		return len;
	}
	else {
		if (len > 0) {
			ret = run_command(s, console_buffer, 0, &s->want_repeat);
			if(ret == 0 && s->want_repeat) {
				strncpy_s(lastcommand, console_buffer, CONFIG_SYS_CBSIZE);
			}
		}
		else { /*len == 0 */
			if(s->want_repeat) {
				ret = run_command(s, lastcommand, 1, &s->want_repeat);
				if(ret != 0) {
					s->want_repeat = 0;
				}
			}
		}
	}

	/* Ignore any errors for now */
	return 0;
}

/****************************************************************************/

/*
 * Prompt for input and read a line.
 * If  CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
 * time out when time goes past endtime (timebase time in ticks).
 * Return:	number of read characters
 *		-EINTR if break
 */
int readline (const char *const prompt)
{
	/*
	 * If console_buffer isn't 0-length the user will be prompted to modify
	 * it instead of entering it from scratch as desired.
	 */
	console_buffer[0] = '\0';

	return readline_into_buffer(prompt, console_buffer);
}


int readline_into_buffer (const char *const prompt, char * buffer)
{
	char *p = buffer;
	char * p_buf = p;
	int	n = 0;				/* buffer index		*/
	int	plen = 0;			/* prompt length	*/
	int	col;				/* output column cnt	*/
	char	c;

	/* print prompt */
	if (prompt) {
		plen = strlen (prompt);
		puts (prompt);
	}
	col = plen;

	for (;;) {
		WATCHDOG_RESET();		/* Trigger watchdog, if needed */

		c = getc();

		/*
		 * Special character handling
		 */
		switch (c) {
		case '\r':				/* Enter		*/
		case '\n':
			*p = '\0';
			puts ("\r\n");
			return (p - p_buf);

		case '\0':				/* nul			*/
			continue;

		case 0x03:				/* ^C - break		*/
			p_buf[0] = '\0';	/* discard input */
			return -EINTR;

		case 0x15:				/* ^U - erase line	*/
			while (col > plen) {
				puts (erase_seq);
				--col;
			}
			p = p_buf;
			n = 0;
			continue;

		case 0x17:				/* ^W - erase word	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			while ((n > 0) && (*p != ' ')) {
				p=delete_char(p_buf, p, &col, &n, plen);
			}
			continue;

		case 0x08:				/* ^H  - backspace	*/
		case 0x7F:				/* DEL - backspace	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			continue;

		default:
			/*
			 * Must be a normal character then
			 */
			if (n < CONFIG_SYS_CBSIZE-2) {
				if (c == '\t') {	/* expand TABs		*/
#ifdef CONFIG_AUTO_COMPLETE
					/* if auto completion triggered just continue */
					*p = '\0';
					if (cmd_auto_complete(prompt, console_buffer, &n, &col)) {
						p = p_buf + n;	/* reset */
						continue;
					}
#endif
					puts (tab_seq+(col&07));
					col += 8 - (col&07);
				} else {
					++col;		/* echo input		*/
					putc (c);
				}
				*p++ = c;
				++n;
			} else {			/* Buffer full		*/
				putc ('\a');
			}
		}
	}
}

/****************************************************************************/

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putc (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}

int parse_line (char *line, char *argv[])
{
	int nargs = 0;

#ifdef DEBUG_PARSER
	printf ("parse_line: \"%s\"\n", line);
#endif
	while (nargs < CONFIG_SYS_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
			printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		*line++ = '\0';	/* terminate current arg */
	}

	printf ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);

#ifdef DEBUG_PARSER
	printf ("parse_line: nargs=%d\n", nargs);
#endif
	return nargs;
}

static void process_macros (char *output, const char *input)
{
	char c, prev;
	const char *varname_start = NULL;
	int inputcnt = strlen (input);
	int outputcnt = CONFIG_SYS_CBSIZE;
	int state = 0;		/* 0 = waiting for '$'  */

	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */
#ifdef DEBUG_PARSER
	char *output_start = output;

	printf ("[PROCESS_MACROS] INPUT len %d: \"%s\"\n", strlen (input),
		input);
#endif

	prev = '\0';		/* previous character   */

	while (inputcnt && outputcnt) {
		c = *input++;
		inputcnt--;

		if (state != 3) {
			/* remove one level of escape characters */
			if ((c == '\\') && (prev != '\\')) {
				if (inputcnt-- == 0)
					break;
				prev = c;
				c = *input++;
			}
		}

		switch (state) {
		case 0:	/* Waiting for (unescaped) $    */
			if ((c == '\'') && (prev != '\\')) {
				state = 3;
				break;
			}
			if ((c == '$') && (prev != '\\')) {
				state++;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		case 1:	/* Waiting for (        */
			if (c == '(' || c == '{') {
				state++;
				varname_start = input;
			} else {
				state = 0;
				*(output++) = '$';
				outputcnt--;

				if (outputcnt) {
					*(output++) = c;
					outputcnt--;
				}
			}
			break;
		case 2:	/* Waiting for )        */
			if (c == ')' || c == '}') {
				int i;
				char envname[CONFIG_SYS_CBSIZE], *envval;
				int envcnt = input - varname_start - 1;	/* Varname # of chars */

				/* Get the varname */
				for (i = 0; i < envcnt; i++) {
					envname[i] = varname_start[i];
				}
				envname[i] = 0;

				/* Get its value */
				envval = getenv (envname);

				/* Copy into the line if it exists */
				if (envval != NULL)
					while ((*envval) && outputcnt) {
						*(output++) = *(envval++);
						outputcnt--;
					}
				/* Look for another '$' */
				state = 0;
			}
			break;
		case 3:	/* Waiting for '        */
			if ((c == '\'') && (prev != '\\')) {
				state = 0;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		}
		prev = c;
	}

	if (outputcnt)
		*output = 0;
	else
		*(output - 1) = 0;

#ifdef DEBUG_PARSER
	printf ("[PROCESS_MACROS] OUTPUT len %d: \"%s\"\n",
		strlen (output_start), output_start);
#endif
}

int run_command(struct main_state *s, const char *cmd,
	int repeat, int *want_repeat)
{
	cmd_tbl_t *cmdtp;
	char cmdbuf[CONFIG_SYS_CBSIZE];
	/* token without macros */
	char token_nom[CONFIG_SYS_CBSIZE];
	/* token split for argv */
	char token_argv[CONFIG_SYS_CBSIZE];
	char *token; /* start of token in cmdbuf	*/
	char *sep;   /* end of token (separator) in cmdbuf */
	char *str;
	char *argv[CONFIG_SYS_MAXARGS + 1];	/* NULL terminated */
	int argc, inquotes;
	int rc = 0;
	int ask_repeat = 1;

	clear_ctrlc();		/* forget any previous Control C */

	if (!cmd || !*cmd) {
		return -EINVAL;
	}

	if (strlen(cmd) >= CONFIG_SYS_CBSIZE) {
		return -ENOBUFS;
	}

	strcpy(cmdbuf, cmd);

	for(str=cmdbuf; *str; ) {
		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
				(*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
				(*sep == ';') &&	/* separator		*/
				( sep != str) &&	/* past string start	*/
				(*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */

		/* find macros in this token and replace them */
		process_macros(token_nom, token);
		strncpy_s(token_argv, token_nom, CONFIG_SYS_CBSIZE);

		/* Extract arguments */
		argc = parse_line(token_argv, argv);
		if (argc == 0) {
			rc = 0;
			continue;
		}

		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			printf ("Unknown command '%s' - try 'help'\n", argv[0]);
			ask_repeat = 0;
			rc = -ENOENT;
			break;
		}

		struct cmd_ctx ctx = {
			.cmdline = token_nom,
			.repeat = repeat,
			.ms = s,
			.cmdtp = cmdtp,
		};

		/* found - check max args */
		if (argc > cmdtp->maxargs) {
			cmd_usage(ctx.cmdtp);
			ask_repeat = 0;
			rc = -E2BIG;
			break;
		}

		/* OK - call function to do the command */
		rc = (cmdtp->cmd) (&ctx, argc, argv);
		if(rc < 0) {
			ask_repeat = 0;
			break;
		}

		ask_repeat &= cmdtp->repeatable;

		/* Did the user stop this? */
		if (had_ctrlc ())
			return -EINTR;
	}

	if(want_repeat)
		*want_repeat = ask_repeat;

	return rc;
}

/****************************************************************************/

#if defined(CONFIG_CMD_RUN)
int do_run (struct cmd_ctx *ctx, int argc, char * const argv[])
{
	int i;
	int ret;

	if (argc < 2)
		return cmd_usage(ctx->cmdtp);

	for (i=1; i<argc; ++i) {
		char *arg = getenv(argv[i]);
		if (arg == NULL) {
			printf ("RUN variable not defined: name %s\n", argv[i]);
			return -EINVAL;
		}

		ret = run_command(ctx->ms, arg, 0, NULL);
		if(ret < 0) {
			printf("RUN failed: cmd %s ret %d\n", arg, ret);
			return ret;
		}
	}
	return 0;
}
#endif

