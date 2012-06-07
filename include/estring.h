#ifndef ESTRING_H
#define ESTRING_H

#include <linux/string.h>

/* A string with explicitly specified length */
struct estring {
	char *lstr;
	int len;
	int quote;
};

#define ESTR_EMPTY { NULL, 0, 0 }

typedef struct estring estring_t;

static inline void estr_clear(struct estring *s)
{
	memset(s,0,sizeof(struct estring));
}

static inline void estr_init(struct estring *s, const char *v)
{
	s->lstr=(char*)v; s->len=v?strlen(v):0;
}

static inline int estr_eq(const struct estring *s, const char *v)
{
	int len = v?strlen(v):0;
	if(len != s->len) return 0;
	return 0 == strncmp(s->lstr,v,len);
}

static inline int estr_eq_estr(const struct estring *s, const struct estring *v)
{
	if(v->len != s->len) return 0;
	return 0 == strncmp(s->lstr,v->lstr,s->len);
}

static inline void estr_mirror(struct estring *d, const struct estring *s)
{
	memcpy(d,s,sizeof(struct estring));
}

#endif
