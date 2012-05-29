#include <asm/setup.h>
#include <asm/types.h>
#include <linux/ctype.h>
#include <stdarg.h>
#include <common.h>

void linux_tag_start(struct tag **tag, struct tag *tag_base)
{
	(*tag) = tag_base;
	(*tag)->hdr.tag = ATAG_CORE;
	(*tag)->hdr.size = tag_size (tag_core);
	(*tag)->u.core.flags = 0;
	(*tag)->u.core.pagesize = 0;
	(*tag)->u.core.rootdev = 0;
	*tag = tag_next(*tag);
}

void linux_tag_memory(struct tag **tag, const struct memregion *reg)
{
	(*tag)->hdr.tag = ATAG_MEM;
	(*tag)->hdr.size = tag_size (tag_mem32);
	(*tag)->u.mem.start = reg->start;
	(*tag)->u.mem.size = memreg_size(reg);
	*tag = tag_next(*tag);
}

void linux_tag_cmdline_start(struct tag **tag, char **cmdline)
{
	(*tag)->hdr.tag = ATAG_CMDLINE;
	(*tag)->hdr.size = 0; /* we don't know it yet */
	(*tag)->u.cmdline.cmdline[0] = '\0';
	(*cmdline) = ((*tag)->u.cmdline.cmdline);
}

void linux_tag_cmdline_add(char **cmdline, const char *val_fmt, ...)
{
	va_list args;
	int len;

	va_start(args, val_fmt);
	len = vsprintf(*cmdline,val_fmt,args);
	va_end(args);

	(*cmdline) += len;
}

void linux_tag_cmdline_add_space(char **cmdline)
{
	if(**cmdline != ' ') {
		**cmdline = ' ';
		(*cmdline)++;
		**cmdline = '\0';
	}
}

void linux_tag_cmdline_end(struct tag **tag, char **cmdline)
{
	u32 len = (u32)((*cmdline) - ((*tag)->u.cmdline.cmdline));
	(*tag)->hdr.size = (sizeof (struct tag_header) + len + 1 + 4) >> 2;
	*tag = tag_next (*tag);
}

void linux_tag_end(struct tag **tag)
{
	(*tag)->hdr.tag = ATAG_NONE;
	(*tag)->hdr.size = 0;
}

