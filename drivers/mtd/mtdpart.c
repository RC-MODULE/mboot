/*
 * Simple MTD partitioning layer
 *
 * (C) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This code is GPL
 *
 * 	02-21-2002	Thomas Gleixner <gleixner@autronix.de>
 *			added support for read_oob, write_oob
 */

#include <common.h>
#include <errno.h>
#include <mtd.h>
#include <mtdpart.h>

#include <linux/types.h>
#include <linux/list.h>
#include <malloc.h>
#include <linux/mtd/compat.h>

/* Our partition linked list */
LIST_HEAD(g_mtdpats_list);

/*
 * Given a pointer to the MTD object in the mtd_part structure, we can retrieve
 * the pointer to that structure with this macro.
 */
#define PART(x)  ((struct mtd_part *)(x))

/*
 * MTD methods which simply translate the effective address and pass through
 * to the _real_ device.
 */

static int part_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	struct mtd_ecc_stats stats;
	int res;

	stats = part->master->ecc_stats;

	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	res = part->master->read(part->master, from + part->offset,
				   len, retlen, buf);
	if (unlikely(res)) {
		if (res == -EUCLEAN)
			mtd->ecc_stats.corrected += part->master->ecc_stats.corrected - stats.corrected;
		if (res == -EBADMSG)
			mtd->ecc_stats.failed += part->master->ecc_stats.failed - stats.failed;
	}
	return res;
}

static int part_read_oob(struct mtd_info *mtd, loff_t from,
		struct mtd_oob_ops *ops)
{
	struct mtd_part *part = PART(mtd);
	int res;

	if (from >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && from + ops->len > mtd->size)
		return -EINVAL;
	res = part->master->read_oob(part->master, from + part->offset, ops);

	if (unlikely(res)) {
		if (res == -EUCLEAN)
			mtd->ecc_stats.corrected++;
		if (res == -EBADMSG)
			mtd->ecc_stats.failed++;
	}
	return res;
}

static int part_read_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->read_user_prot_reg(part->master, from,
					len, retlen, buf);
}

static int part_get_user_prot_info(struct mtd_info *mtd,
		struct otp_info *buf, size_t len)
{
	struct mtd_part *part = PART(mtd);
	return part->master->get_user_prot_info(part->master, buf, len);
}

static int part_read_fact_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->read_fact_prot_reg(part->master, from,
					len, retlen, buf);
}

static int part_get_fact_prot_info(struct mtd_info *mtd, struct otp_info *buf,
		size_t len)
{
	struct mtd_part *part = PART(mtd);
	return part->master->get_fact_prot_info(part->master, buf, len);
}

static int part_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return part->master->write(part->master, to + part->offset,
				    len, retlen, buf);
}

static int part_panic_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return part->master->panic_write(part->master, to + part->offset,
				    len, retlen, buf);
}

static int part_write_oob(struct mtd_info *mtd, loff_t to,
		struct mtd_oob_ops *ops)
{
	struct mtd_part *part = PART(mtd);

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	if (to >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && to + ops->len > mtd->size)
		return -EINVAL;
	return part->master->write_oob(part->master, to + part->offset, ops);
}

static int part_write_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	return part->master->write_user_prot_reg(part->master, from,
					len, retlen, buf);
}

static int part_lock_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len)
{
	struct mtd_part *part = PART(mtd);
	return part->master->lock_user_prot_reg(part->master, from, len);
}

static int part_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct mtd_part *part = PART(mtd);
	int ret;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (instr->addr >= mtd->size)
		return -EINVAL;
	instr->addr += part->offset;
	ret = part->master->erase(part->master, instr);
	if (ret) {
		if (instr->fail_addr != MTD_FAIL_ADDR_UNKNOWN)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	return ret;
}

void mtd_erase_callback(struct erase_info *instr)
{
	if (instr->mtd->erase == part_erase) {
		struct mtd_part *part = PART(instr->mtd);

		if (instr->fail_addr != MTD_FAIL_ADDR_UNKNOWN)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	if (instr->callback)
		instr->callback(instr);
}

static int part_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct mtd_part *part = PART(mtd);
	if ((len + ofs) > mtd->size)
		return -EINVAL;
	return part->master->lock(part->master, ofs + part->offset, len);
}

static int part_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct mtd_part *part = PART(mtd);
	if ((len + ofs) > mtd->size)
		return -EINVAL;
	return part->master->unlock(part->master, ofs + part->offset, len);
}

static void part_sync(struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);
	part->master->sync(part->master);
}

static int part_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	return part->master->block_isbad(part->master, ofs);
}

static int part_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	int res;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	res = part->master->block_markbad(part->master, ofs);
	if (!res)
		mtd->ecc_stats.badblocks++;
	return res;
}

static int add_one_partition(struct mtd_info *master,
	struct mtd_part *part, int partno, uint64_t cur_offset)
{
	struct mtd_part *slave = part;

	/* set up the MTD object for this partition */
	slave->mtd.type = master->type;
	slave->mtd.flags = master->flags; // & ~part->mask_flags;
	slave->mtd.size = part->size;
	slave->mtd.writesize = master->writesize;
	slave->mtd.oobsize = master->oobsize;
	slave->mtd.oobavail = master->oobavail;
	slave->mtd.subpage_sft = master->subpage_sft;

	slave->mtd.name = part->name;
	slave->mtd.owner = master->owner;

	slave->mtd.read = part_read;
	slave->mtd.write = part_write;

	if (master->panic_write)
		slave->mtd.panic_write = part_panic_write;

	if (master->read_oob)
		slave->mtd.read_oob = part_read_oob;
	if (master->write_oob)
		slave->mtd.write_oob = part_write_oob;
	if (master->read_user_prot_reg)
		slave->mtd.read_user_prot_reg = part_read_user_prot_reg;
	if (master->read_fact_prot_reg)
		slave->mtd.read_fact_prot_reg = part_read_fact_prot_reg;
	if (master->write_user_prot_reg)
		slave->mtd.write_user_prot_reg = part_write_user_prot_reg;
	if (master->lock_user_prot_reg)
		slave->mtd.lock_user_prot_reg = part_lock_user_prot_reg;
	if (master->get_user_prot_info)
		slave->mtd.get_user_prot_info = part_get_user_prot_info;
	if (master->get_fact_prot_info)
		slave->mtd.get_fact_prot_info = part_get_fact_prot_info;
	if (master->sync)
		slave->mtd.sync = part_sync;
	if (master->lock)
		slave->mtd.lock = part_lock;
	if (master->unlock)
		slave->mtd.unlock = part_unlock;
	if (master->block_isbad)
		slave->mtd.block_isbad = part_block_isbad;
	if (master->block_markbad)
		slave->mtd.block_markbad = part_block_markbad;
	slave->mtd.erase = part_erase;
	slave->master = master;
	//slave->offset = part->offset;
	//slave->index = partno;

	if (slave->offset == MTDPART_OFS_APPEND)
		slave->offset = cur_offset;
	if (slave->offset == MTDPART_OFS_NXTBLK) {
		slave->offset = cur_offset;
		if (mtd_mod_by_eb(cur_offset, master) != 0) {
			/* Round up to next erasesize */
			slave->offset = (mtd_div_by_eb(cur_offset, master) + 1) * master->erasesize;
			MTDDEBUG(2, "MTD moving partition %d: "
			       "0x%012llx -> 0x%012llx\n", partno,
			       (unsigned long long)cur_offset, (unsigned long long)slave->offset);
		}
	}
	if (slave->mtd.size == MTDPART_SIZ_FULL)
		slave->mtd.size = master->size - slave->offset;

	MTDDEBUG(2, "MTD 0x%012llx-0x%012llx : \"%s\"\n", (unsigned long long)slave->offset,
		(unsigned long long)(slave->offset + slave->mtd.size), slave->mtd.name);

	/* let's do some sanity checks */
	if (slave->offset >= master->size) {
		/* let's register it anyway to preserve ordering */
		slave->offset = 0;
		slave->mtd.size = 0;
		printk("MTD: partition \"%s\" is out of reach -- disabled\n",
			part->name);
		goto out_register;
	}
	if (slave->offset + slave->mtd.size > master->size) {
		slave->mtd.size = master->size - slave->offset;
		printk("MTD: partition \"%s\" extends beyond the end of device "
			"\"%s\" -- size truncated to %#llx\n",
			part->name, master->name, (unsigned long long)slave->mtd.size);
	}
	if (master->numeraseregions > 1) {
		/* Deal with variable erase size stuff */
		int i, max = master->numeraseregions;
		u64 end = slave->offset + slave->mtd.size;
		struct mtd_erase_region_info *regions = master->eraseregions;

		/* Find the first erase regions which is part of this
		 * partition. */
		for (i = 0; i < max && regions[i].offset <= slave->offset; i++)
			;
		/* The loop searched for the region _behind_ the first one */
		i--;

		/* Pick biggest erasesize */
		for (; i < max && regions[i].offset < end; i++) {
			if (slave->mtd.erasesize < regions[i].erasesize) {
				slave->mtd.erasesize = regions[i].erasesize;
			}
		}
		BUG_ON(slave->mtd.erasesize == 0);
	} else {
		/* Single erase size */
		slave->mtd.erasesize = master->erasesize;
	}

	if ((slave->mtd.flags & MTD_WRITEABLE) &&
	    mtd_mod_by_eb(slave->offset, &slave->mtd)) {
		/* Doesn't start on a boundary of major erase size */
		/* FIXME: Let it be writable if it is on a boundary of
		 * _minor_ erase size though */
		slave->mtd.flags &= ~MTD_WRITEABLE;
		printk("MTD: partition \"%s\" doesn't start on an "
			"erase block boundary -- force read-only\n",
			part->name);
	}
	if ((slave->mtd.flags & MTD_WRITEABLE) &&
	    mtd_mod_by_eb(slave->mtd.size, &slave->mtd)) {
		slave->mtd.flags &= ~MTD_WRITEABLE;
		printk("MTD: partition \"%s\" doesn't end on "
			"an erase block -- force read-only\n",
			part->name);
	}

	slave->mtd.ecclayout = master->ecclayout;

#if 0
	if (master->block_isbad) {
		uint64_t offs = 0;

		while (offs < slave->mtd.size) {
			if (master->block_isbad(master,
						offs + slave->offset))
				slave->mtd.ecc_stats.badblocks++;
			offs += slave->mtd.erasesize;
		}
	}
#endif

out_register: 
	list_add(&slave->list, &g_mtdpats_list);
	return 0;
}

/*
 * This function, given a master MTD object and a partition table, creates
 * and registers slave MTD objects which are bound to the master according to
 * the partition definitions.
 *
 * We don't register the master, or expect the caller to have done so,
 * for reasons of data integrity.
 */

int mtdparts_add(struct mtd_info *master, struct mtd_part *parts)
{
	uint64_t cur_offset;
	int i;
	int ret;

	cur_offset = 0;
	for(i=0, parts; !parts->last; parts++,i++) {
		ret = add_one_partition(master, parts, i, cur_offset);
		if(ret != 0) {
			printf("MTD failed to create partition: mtd %s partno %d\n",
				master->name, i);
			return ret;
		}
		ret = mtd_add(&parts->mtd);
		if(ret != 0) {
			printf("MTD failed to register partition: part \"%s\"\n",
				parts->name);
			return ret;
		}

		cur_offset = parts->offset + parts->mtd.size;
	}

	return 0;
}



static size_t get_size_from_env(const char* name, size_t def)
{
	const char* tmp = getenv(name);
	if (strcmp(tmp,"-")==0)
		return 0;

	if (tmp) {
		size_t ksize = simple_strtoul(tmp, NULL, 16);
		if (ksize > 0)
			return ksize;
	}
	return def;
}


int mtdparts_add_fromenv(struct mtd_info *master, struct mtd_part *parts, char* env)
{
	uint64_t cur_offset;
	int i;
	int ret;
	char* env_parts = getenv("parts"); 
	cur_offset = 0;
	for(i=0, parts; !parts->last; parts++,i++) {
		ret = add_one_partition(master, parts, i, cur_offset);
		if(ret != 0) {
			printf("MTD failed to create partition: mtd %s partno %d\n",
				master->name, i);
			return ret;
		}
		ret = mtd_add(&parts->mtd);
		if(ret != 0) {
			printf("MTD failed to register partition: part \"%s\"\n",
				parts->name);
			return ret;
		}

		cur_offset = parts->offset + parts->mtd.size;
	}
	if (!env_parts)
		return 0;
	/* Now, parse parts from env */
	env_parts = strdup(env_parts);
	char* name = strtok(env_parts,",");
	while (name) {
		char tmp[32];
		size_t size;
		sprintf(tmp,"%s_size", name);
		size = get_size_from_env(tmp, 0x40000);
		struct mtd_part *p = malloc(sizeof(struct mtd_part));
		if (!p)
			BUG_ON(1);
		p->name = name;
		p->offset = MTDPART_OFS_NXTBLK;
		if (size)
			p->size = size;
		else
			p->size= MTDPART_SIZ_FULL;
		p->last = 0;
		/* Now, register it! */
		name = strtok(NULL, ",");
		i++;
		ret = add_one_partition(master, p, i, cur_offset);
		if(ret != 0) {
			printf("MTD failed to create partition: mtd %s partno %d\n",
			       master->name, i);
			return ret;
		}
		ret = mtd_add(&p->mtd);
		if(ret != 0) {
			printf("MTD failed to register partition: part \"%s\"\n",
				p->name);
			return ret;
		}

		cur_offset = p->offset + p->mtd.size;
	}
	return 0;
}
