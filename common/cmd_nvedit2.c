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

#define SEP '\0'
#define SSEP "\0"
#define EOE '\3'
#define SEOE "\3"

struct estring {
	char *lstr;
	int len;
};

typedef struct estring estring_t;

inline void estr_clear(struct estring *s) { memset(s,0,sizeof(struct estring)); }
inline void estr_init(struct estring *s, char *v) { s->lstr=v; s->len=strlen(v); }

inline int estr_eq(const struct estring *s, const char *v)
{
	int len = strlen(v);
	if(len != s->len) return 0;
	return 0 == strncmp(s->lstr,v,len);
}


#define _MK_STR(x)	#x
#define MK_STR(x)	_MK_STR(x)

static char default_environment[] = {
#ifdef	CONFIG_BOOTARGS
	"bootargs="	CONFIG_BOOTARGS			SSEP
#endif
#ifdef	CONFIG_BOOTCOMMAND
	"bootcmd="	CONFIG_BOOTCOMMAND		SSEP
#endif
#ifdef	CONFIG_RAMBOOTCOMMAND
	"ramboot="	CONFIG_RAMBOOTCOMMAND		SSEP
#endif
#ifdef	CONFIG_NFSBOOTCOMMAND
	"nfsboot="	CONFIG_NFSBOOTCOMMAND		SSEP
#endif
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	"bootdelay="	MK_STR(CONFIG_BOOTDELAY)	SSEP
#endif
#if defined(CONFIG_BAUDRATE) && (CONFIG_BAUDRATE >= 0)
	"baudrate="	MK_STR(CONFIG_BAUDRATE)		SSEP
#endif
#ifdef	CONFIG_LOADS_ECHO
	"loads_echo="	MK_STR(CONFIG_LOADS_ECHO)	SSEP
#endif
#ifdef	CONFIG_ETHADDR
	"ethaddr="	CONFIG_ETHADDR		SSEP
#endif
#ifdef	CONFIG_ETH1ADDR
	"eth1addr="	CONFIG_ETH1ADDR		SSEP
#endif
#ifdef	CONFIG_ETH2ADDR
	"eth2addr="	CONFIG_ETH2ADDR		SSEP
#endif
#ifdef	CONFIG_ETH3ADDR
	"eth3addr="	CONFIG_ETH3ADDR		SSEP
#endif
#ifdef	CONFIG_ETH4ADDR
	"eth4addr="	CONFIG_ETH4ADDR		SSEP
#endif
#ifdef	CONFIG_ETH5ADDR
	"eth5addr="	CONFIG_ETH5ADDR		SSEP
#endif
#ifdef	CONFIG_IPADDR
	"ipaddr="	CONFIG_IPADDR		SSEP
#endif
#ifdef	CONFIG_SERVERIP
	"serverip="	CONFIG_SERVERIP		SSEP
#endif
#ifdef	CONFIG_SYS_AUTOLOAD
	"autoload="	CONFIG_SYS_AUTOLOAD			SSEP
#endif
#ifdef	CONFIG_PREBOOT
	"preboot="	CONFIG_PREBOOT			SSEP
#endif
#ifdef	CONFIG_ROOTPATH
	"rootpath="	CONFIG_ROOTPATH		SSEP
#endif
#ifdef	CONFIG_GATEWAYIP
	"gatewayip="	CONFIG_GATEWAYIP	SSEP
#endif
#ifdef	CONFIG_NETMASK
	"netmask="	CONFIG_NETMASK		SSEP
#endif
#ifdef	CONFIG_HOSTNAME
	"hostname="	CONFIG_HOSTNAME		SSEP
#endif
#ifdef	CONFIG_BOOTFILE
	"bootfile="	CONFIG_BOOTFILE		SSEP
#endif
#ifdef	CONFIG_LOADADDR
	"loadaddr="	MK_STR(CONFIG_LOADADDR)		SSEP
#endif
#ifdef  CONFIG_CLOCKS_IN_MHZ
	"clocks_in_mhz=1"                       SSEP
#endif
	SEOE
};

#ifdef  CONFIG_EXTRA_ENV_SETTINGS
#error Not in UEMD
#endif

static int env_read_default(char* buf, size_t *len, void *priv)
{
	size_t sz = MIN(sizeof(default_environment), *len);
	memcpy(buf, default_environment, sz);
	*len = sz;
	return 0;
}

static struct env_ops g_default_ops = {
	.readenv = env_read_default,
	.writeenv = NULL,
};

static struct env_ops *g_ops;
static char* g_env = NULL;
static size_t g_env_sz = 0;
static void* g_env_priv = NULL;

static int g_env_id = 1;

int get_env_id (void)
{
	return g_env_id;
}

int env_init(struct env_ops *ops, void *priv)
{
	int ret;
	size_t sz;

	g_env_id = 1;
	g_env_priv = priv;
	g_env = (char*)CONFIG_SYS_ENV_ADDR;
	g_env_sz = (size_t)CONFIG_SYS_ENV_SIZE;

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
	sz = (size_t)CONFIG_SYS_ENV_SIZE;
	ret = env_read_default(g_env, &sz, NULL);
	if(ret < 0) {
		return ret;
	}

	g_env[MIN(sz, g_env_sz-1)] = EOE;
	return 0;
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

int setenv (const char *rname, const char *rval)
{
	char *i;
	struct estring name, val;
	size_t free, len;

	if(g_env == NULL)
		return -1;

	if(rval > g_env && rval <= g_env+g_env_sz) {
		error("BUG: recursive environment pointers: name %s val %s",
			rname, rval);
	}

	for_all_env(g_env, i, &name, &val) {
		if(estr_eq(&name, rname)) {
			memcpy(name.lstr, i, g_env_sz - (i - g_env));
			for(i=name.lstr; *i!=EOE; i++);
			break;
		}
	}

	if(rval == NULL) goto out;

	free = g_env_sz - (i - g_env) - 1;

	len = MIN(free,strlen(rname));
	memcpy(i, rname, len);
	i += len;
	free -= len;

	len = MIN(free,1);
	memcpy(i, "=", len);
	i += len;
	free -= len;

	len = MIN(free,strlen(rval));
	memcpy(i, rval, len);
	i += len;
	free -= len;

	len = MIN(free,1);
	memcpy(i, SSEP, len);
	i += len;
	free -= len;

	*i = EOE;

out:
	g_env_id++;

	return 0;
}

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

static int do_env_print(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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

static int do_env_set(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	switch(argc) {
		case 2: return setenv(argv[1], NULL);
		case 3: return setenv(argv[1], argv[2]);
		default: break;
	}

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	setenv, 3, 0, do_env_set,
	"set environment variables",
	"name value ...\n"
	"    - set environment variable 'name' to 'value ...'\n"
	"setenv name\n"
	"    - NOT IMPLEMENTED"
);

int do_env_save (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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

