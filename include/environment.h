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
 *
 * The environment is preceeded by a 32 bit CRC over the data part.
 */

struct env_ops {
	/* Reads the len bytes of env. Handler may shorten the len. */
	int (*readenv) (char* str, size_t *len, void *priv);
	/* Write len bytes of env to store. */
	int (*writeenv) (const char* str, size_t len, void *priv);
};

int env_init(struct env_ops *ops, void *priv);

int	setenv(const char *n, const char *v);

char *getenv(const char *);

static inline const char *getenv_def(const char *name, const char *def)
{
	const char *val = getenv(name);
	return val != NULL ? val : def;
}

int	saveenv(void);


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
