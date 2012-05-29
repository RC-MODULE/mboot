/*
 * MTD partitioning layer definitions
 *
 * (C) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This code is GPL
 *
 * $Id: partitions.h,v 1.17 2005/11/07 11:14:55 gleixner Exp $
 */

#ifndef _MTDPARTITIONS_H
#define _MTDPARTITIONS_H

#include <linux/list.h>
#include <linux/mtd/mtd.h>

/*
 * Partition definition structure:
 *
 * An array of struct partition is passed along with a MTD object to
 * add_mtd_partitions() to create them.
 *
 * For each partition, these fields are available:
 * name: string that will be used to label the partition's MTD device.
 * size: the partition size; if defined as MTDPART_SIZ_FULL, the partition
 * 	will extend to the end of the master MTD device.
 * offset: absolute starting position within the master MTD device; if
 * 	defined as MTDPART_OFS_APPEND, the partition will start where the
 * 	previous one ended; if MTDPART_OFS_NXTBLK, at the next erase block.
 * mask_flags: contains flags that have to be masked (removed) from the
 * 	master MTD flag set for the corresponding MTD partition.
 * 	For example, to force a read-only partition, simply adding
 * 	MTD_WRITEABLE to the mask_flags will do the trick.
 *
 * Note: writeable partitions require their size and offset be
 * erasesize aligned (e.g. use MTDPART_OFS_NEXTBLK).
 */

/* Our partition node structure */
struct mtd_part {
	struct mtd_info mtd; /* keep it first */
	struct mtd_info *master;
	struct list_head list;
	int index;

	/* Filled by the user */
	const char *name;
	uint64_t offset;
	uint64_t size;
	int last;
};

#define MTDPART_INITIALIZER(nm, off, sz) { .name = nm, .offset = off, .size = sz, .last = 0 }
#define MTDPART_NULL { .last = 1 }

#define MTDPART_OFS_NXTBLK	(-2)
#define MTDPART_OFS_APPEND	(-1)
#define MTDPART_SIZ_FULL	(0)

int mtdparts_add(struct mtd_info* master, struct mtd_part* parts);

extern struct list_head g_mtdpats_list;

/* List partitions in order they were created */
#define for_all_mtdparts(part) list_for_each_entry_reverse(part, &g_mtdpats_list, list)

/* List partitions of a specific MTD device, in order they were created */
#define for_all_mtdparts_of(part, mtdname) \
	for_all_mtdparts(part) if(0 == strcmp(part->master->name, mtdname))

#endif
