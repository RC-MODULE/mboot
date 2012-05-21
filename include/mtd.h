#ifndef _MTD_H_
#define _MTD_H_

#include <linux/mtd/mtd.h>
#include <nand.h>

int mtd_init(void);
int board_mtd_init(struct mtd_info *mtd, ulong *base_addr, int num);

extern struct mtd_info mtd_info[];
/*extern int mtd_curr_device;*/

static inline int mtd_is_valid(struct mtd_info *mtd)
{
	return (mtd->name != NULL);
}

static inline int mtd_get_index(struct mtd_info* mtd)
{
	return (mtd - mtd_info) / sizeof(struct mtd_info);
}

static inline int mtd_read(struct mtd_info *info, loff_t ofs, size_t *len, u_char *buf)
{
	return info->read(info, ofs, *len, (size_t *)len, buf);
}

static inline int mtd_write(struct mtd_info *info, loff_t ofs, size_t *len, u_char *buf)
{
	return info->write(info, ofs, *len, (size_t *)len, buf);
}

static inline int mtd_block_isbad(struct mtd_info *info, loff_t ofs)
{
	return info->block_isbad(info, ofs);
}

static inline int mtd_block_markbad(struct mtd_info *info, loff_t ofs)
{
	if(info->block_markbad)
		return info->block_markbad(info, ofs);
	else
		return -1;
}


#define for_all_mtd(mtd) \
	for(mtd=mtd_info; mtd<mtd_info+CONFIG_SYS_MAX_MTD_DEVICE; mtd++) \
		if(mtd && mtd_is_valid(mtd))

static inline struct mtd_info * mtd_by_index(int index)
{
	struct mtd_info *mtd;
	for_all_mtd(mtd) {
		if(mtd_get_index(mtd) == index)
			return mtd;
	}
	return NULL;
}

enum mtd_op_flags {
	mtd_verbose = 1,
	mtd_ignore_bad = 2,
	mtd_stop_on_bad = 4,
};

int mtd_erase(struct mtd_info *mtd, uint64_t offset, uint64_t size, int skipbb);

static inline size_t mtd_full_pages(struct mtd_info *mtd, size_t len)
{
	return ((len / mtd->writesize) + 1) * mtd->writesize;
}

int mtd_write_pages(struct mtd_info *mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size);

int mtd_read_pages(struct mtd_info *mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size);

#endif

