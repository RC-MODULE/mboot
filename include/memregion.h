#ifndef MEMREGION_H
#define MEMREGION_H

#include <compiler.h>

struct memregion {
	ulong start;
	ulong end;
};

int memreg_check_overlap(const struct memregion *regs);

static inline size_t memreg_size(const struct memregion *reg)
{
	return reg->end - reg->start;
}

#endif
