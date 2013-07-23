/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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
#include <estring.h>

#define SEP '\0'
#define SSEP "\0"
#define EOE '\3'
#define SEOE "\3"


#ifdef  CONFIG_EXTRA_ENV_SETTINGS
#error Not in UEMD
#endif

static struct env_ops *g_ops;
static char* g_env = NULL;
static size_t g_env_sz = 0;
static void* g_env_priv = NULL;
static int g_env_id = 1;



static int env_read_default(char* buf, size_t *len, void *priv)
{
	int free=0;
	struct env_var *defs = (struct env_var*)(priv);
	for(;defs->name!=NULL; defs++) {
		free = setenv(defs->name, defs->val);
		if(free < 0) {
			printf("ENV setenv failed: var %s red %d\n", defs->name, free);
			return free;
		}
	}
	printf("ENV loaded defaults: free 0x%08X\n", free);
	//size_t sz = MIN(sizeof(default_environment), *len);
	//memcpy(buf, default_environment, sz);
	*len = g_env_sz;
	return 0;
}

static struct env_ops g_default_ops = {
	.readenv = env_read_default,
	.writeenv = NULL,
};

int get_env_id (void)
{
	return g_env_id;
}

/* Initialises environment. defs is a NULL-terminated table of default env. priv
 * will be passed to the ops without changes. */
int env_init(struct env_ops *ops, struct env_var *defs, void *priv)
{
	int ret;
	size_t sz;

	g_env_id = 1;
	g_env_priv = priv;
	g_env = (char*)CONFIG_SYS_ENV_ADDR;
	g_env_sz = (size_t)CONFIG_SYS_ENV_SIZE;
	g_env[0] = EOE;

	g_ops = ops==NULL ? (&g_default_ops) : ops;
	if(g_ops->readenv == NULL) {
		printf("ENV read method not defined, using default\n");
		goto def;
	}

	sz = (size_t)CONFIG_SYS_ENV_SIZE;
	ret = g_ops->readenv(g_env, &sz, g_env_priv);
	if(ret < 0) {
		printf("ENV read failed, using default: ret %d\n", ret);
		goto def;
	}
	g_env[MIN(sz, g_env_sz-1)] = EOE;

	/* QuickCheck the environment */
	{
		char* p;
		size_t found_eq = 0;
		for(p=g_env; p<g_env+g_env_sz; p++) {
			if(*p == '=') found_eq++;
		}

		if(found_eq == 0) {
			printf("ENV looks empty, using default\n");
			goto def;
		}
	}

	return 0;

def:
	g_env[0] = EOE;
	sz = (size_t)CONFIG_SYS_ENV_SIZE;
	ret = env_read_default(g_env, &sz, defs);
	if(ret < 0) {
		return ret;
	}

	g_env[MIN(sz, g_env_sz-1)] = EOE;
	return 0;
}

void env_load_defaults(struct env_var *defs) {
	g_env[0] = EOE;
	size_t sz = (size_t) CONFIG_SYS_ENV_SIZE;
	env_read_default(g_env, &sz, defs);
	g_env[MIN(sz, g_env_sz-1)] = EOE;

}

static char* env_next(char *p, struct estring *pname, struct estring *pval)
{
	int cnt, skip;
	static struct estring bads = { NULL, 0 } ;

	cnt = 0;
	pname->lstr = NULL;
	pname->len = -1;
	pval->lstr = NULL;
	pval->len = -1;

	if(!p) return NULL;

	while(*p==SEP) p++;

	for(skip=0; *p!=EOE; p++) {
		if(skip > 0) {skip--; continue;}

		if(pname->lstr == NULL) {
			pname->lstr = p;
			cnt = 0;
		}

		if(pname->len < 0) {
			if(*p != '=' && *p != SEP) {
				cnt ++;
				continue;
			}
			else {
				pname->len = cnt;
				continue;
			}
		}

		if(pval->lstr == NULL) {
			pval->lstr = p;
			cnt = 0;
		}

		if(pval->len < 0) {
			if(*p != SEP) {
				cnt ++;
				continue;
			}
			else {
				pval->len = cnt;
				continue;
			}
		}
		return p;
	}

	/* Kill incomplete envs */
	if(pval->len<=0) {
		memcpy(pval, &bads, sizeof(struct estring));
		memcpy(pname, &bads, sizeof(struct estring));
	}
	else {
		if(pname->len<=0) memcpy(pname, &bads, sizeof(struct estring));
	}

	return p;
}

static void printf_env(struct estring* name, struct estring* val)
{
	nputc(name->lstr, name->len);
	putc('=');
	nputc(val->lstr, val->len);
	putc('\n');
}

#define for_all_env(addr,i,pname,pval) \
	for(i=env_next(addr,pname,pval); (pname)->lstr; i=env_next(i,pname,pval))

static int setenv_e (const struct estring *ename, const struct estring *eval)
{
	char *i, *eoe;
	struct estring name, val;
	size_t free=0, len;

	if(g_env == NULL)
		return -EPERM;

	if(eval->lstr > g_env && eval->lstr <= g_env+g_env_sz) {
		error("BUG: recursive environment pointers: name %s val %s",
			ename->lstr, eval->lstr);
	}

	for_all_env(g_env, i, &name, &val) {
		if(estr_eq_estr(&name, ename)) {
			memcpy(name.lstr, i, g_env_sz - (i - g_env));
			for(i=name.lstr; *i!=EOE; i++);
			break;
		}
	}

	/* Now i points to EOE */
	eoe = i;

	if(eval->len == 0) goto out;

	free = g_env_sz - (i - g_env) - 1;

	len = MIN(free,ename->len);
	memcpy(i, ename->lstr, len);
	i += len;
	free -= len;

	len = MIN(free,1);
	memcpy(i, "=", len);
	i += len;
	free -= len;

	len = MIN(free,eval->len);
	memcpy(i, eval->lstr, len);
	i += len;
	free -= len;

	len = MIN(free,1);
	if(len == 0) goto bad;
	memcpy(i, SSEP, len);
	i += len;
	free -= len;

	*i = EOE;

out:
	g_env_id++;
	return (int)MIN(free,0x7fffffff);

bad:
	*eoe = EOE;
	return -ENOMEM;
}

/* Sets a value of a variable.
 * WARNING: values obtained from getenv calls are no longer valid */
int setenv (const char *rname, const char *rval)
{
	struct estring eval, ename;
	estr_init(&ename, rname);
	estr_init(&eval, rval);
	return setenv_e(&ename, &eval);
}

/* Gets a value of a rname variable.
 * WARNING: value is only valid until next setenv call*/
char *getenv (const char *rname)
{
	char *i;
	struct estring name, val;

	for_all_env(g_env, i, &name, &val) {
		if(estr_eq(&name, rname)) {
			/* Null - terminated */
			return val.lstr;
		}
	}
	return NULL;
}

static int do_env_print(struct cmd_ctx *ctx, int argc, char * const argv[])
{
	char *i;
	int j;
	struct estring name, val;

	if(argc == 1) {
		for_all_env(g_env, i, &name, &val) {
			printf_env(&name, &val);
		}
	}
	else {
		for_all_env(g_env, i, &name, &val) {
			for(j=1; j<argc; j++) {
				if(estr_eq(&name, argv[j]))
					printf_env(&name, &val);
			}
		}
	}
	return 0;
}

U_BOOT_CMD(
	printenv, CONFIG_SYS_MAXARGS, 1, do_env_print,
	"print environment variables",
	"\n    - print values of all environment variables\n"
	"printenv name ...\n"
	"    - print value of environment variable 'name'"
);

static int do_env_set(struct cmd_ctx *ctx, int argc, char * const argv[])
{
	struct estring cmd, name, val;
	char *next = (char*)ctx->cmdline;

	next = cmd_arg_next(next, &cmd);
	next = cmd_arg_next(next, &name);
	estr_init(&val, next);

	if(name.len == 0)
		return cmd_usage(ctx->cmdtp);

	return setenv_e(&name, &val);
}

U_BOOT_CMD(
	setenv, CONFIG_SYS_MAXARGS, 0, do_env_set,
	"set environment variables",
	"setenv NAME VALUE\n"
	"    - sets environment variable NAME to a value of VALUE\n"
	"setenv NAME\n"
	"    - deletes variable NAME"
);

int do_env_save (struct cmd_ctx* ctx, int argc, char * const argv[])
{
	if(g_ops == NULL || g_ops->writeenv == NULL) {
		printf("writeenv handler is not defined\n");
		return -1;
	}

	return g_ops->writeenv(g_env, g_env_sz, g_env_priv);
}

U_BOOT_CMD(
	saveenv, 2, 0,	do_env_save,
	"save environment variables to persistent storage",
	""
);

#if defined(CONFIG_PCI_BOOTDELAY) && (CONFIG_PCI_BOOTDELAY > 0)
#error Not in UEMD
#endif


extern int do_run (struct cmd_ctx *ctx, int argc, char * const argv[]);
U_BOOT_CMD(run, CONFIG_SYS_MAXARGS, 1, do_run, "", "");
