/*
 * (C) Copyright 2012
 * Sergey Mironov <ierton@gmail.com>
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

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

/*
 * The "environment" is stored as a list of '\0' terminated "name=value"
 * strings. The end of the list is marked by '\3'. New entries are always added
 * at the end. Deleting an entry shifts the remaining entries to the front.
 * Replacing an entry is a combination of deleting the old value and adding the
 * new one.
 */

struct env_ops {
	/* Read len bytes into str. Handler may shorten the len. */
	int (*readenv) (char* str, size_t *len, void *priv);
	/* Write len bytes to the store. */
	int (*writeenv) (const char* str, size_t len, void *priv);
};

struct env_var {
	const char *name;
	const char *val;
};

#define ENV_VAR(n,v) { .name = n, .val = v }
#define ENV_NULL { .name = NULL, .val = NULL }

int env_init(struct env_ops *ops, struct env_var *defs, void *priv);

int setenv(const char *n, const char *v);

char *getenv(const char *);

static inline const char *getenv_def(const char *name, const char *def)
{
	const char *val = getenv(name);
	return val != NULL ? val : def;
}

int saveenv(void);

int get_env_id (void);

static inline void getenv_ul(const char *name, ulong *res, ulong def)
{
	char *val = getenv(name);
	*res = (val ? simple_strtoul(val, NULL, 0) : def);
}

static inline void getenv_ull(const char *name, loff_t *res, loff_t def)
{
	char *val = getenv(name);
	*res = (val ? simple_strtoull(val, NULL, 0) : def);
}

#define MAXPATH (128+1)

static inline void getenv_s(const char *name, char *pval, const char* def)
{
	const char *val = getenv(name);
	if(!val)
		strncpy_s(pval, def, MAXPATH);
	else
		strncpy_s(pval, val, MAXPATH);
}

#define setenv_s setenv

static inline void setenv_ul(const char *name, const char* fmt, ulong val)
{
	char buf[12];
	sprintf(buf, fmt, val);
	setenv(name, buf);
}

static inline void setenv_ull(const char *name, const char* fmt, loff_t val)
{
	char buf[24];
	sprintf(buf, fmt, val);
	setenv(name, buf);
}



#if defined(CONFIG_ENV_IS_IN_FLASH)
#error Not in UEMD
#endif

#if defined(CONFIG_ENV_IS_IN_NAND)
#error Not in UEMD
#endif

#if defined(CONFIG_ENV_IS_IN_MG_DISK)
#error Not in UEMD
#endif

#ifdef CONFIG_ENV_IS_EMBEDDED
#error Not in UEMD
#endif

#ifdef CONFIG_PPC
#error Not in UEMD
#endif

#endif	/* _ENVIRONMENT_H_ */
