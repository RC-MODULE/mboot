#ifndef _MTD_H_
#define _MTD_H_

#include <linux/mtd/mtd.h>
#include <nand.h>

int mtd_add(struct mtd_info *mtd);

int board_mtd_init(struct mtd_info *mtd, ulong *base_addr, int num);

extern struct mtd_info mtd_info[];

static inline int mtd_is_valid(struct mtd_info *mtd)
{
	return (mtd->name != NULL);
}

static inline int mtd_read(struct mtd_info *info, loff_t ofs, u_char *buf, size_t len)
{
	size_t tmp;
	return info->read(info, ofs, len, &tmp, buf);
}

static inline int mtd_write(struct mtd_info *info, loff_t ofs, u_char *buf, size_t len)
{
	size_t tmp;
	return info->write(info, ofs, len, &tmp, buf);
}

static inline int mtd_writeoob(struct mtd_info *info, loff_t ofs, u_char *buf, size_t len)
{
//	printf("!\n");
	struct mtd_oob_ops ops = {
		.datbuf = (uint8_t*) buf,
		.len = len,
		.mode = MTD_OOB_AUTO,
		.oobbuf = (uint8_t*) buf + info->writesize,
		.ooblen = info->oobavail,
		.ooboffs = 0
	};
	int ret = info->write_oob(info, ofs, &ops);
//	printf("OK!\n");
	return ret;
}



static inline int mtd_erase1(struct mtd_info *mtd, loff_t ofs)
{
	struct erase_info erase;
	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.len = mtd->erasesize;
	erase.addr = ofs;
	return mtd->erase(mtd, &erase);
}

static inline int mtd_scrub1(struct mtd_info *mtd, loff_t ofs)
{
	struct erase_info erase;
	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.len = mtd->erasesize;
	erase.addr = ofs;
	return mtd->erase(mtd, &erase);
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

extern struct list_head g_mtd_list;

#define for_all_mtd(mtd) list_for_each_entry_reverse(mtd, &g_mtd_list, list)

static inline struct mtd_info * mtd_get_first(void)
{
	struct mtd_info *mtd;
	for_all_mtd(mtd) { return mtd; }
	return NULL;
}

static inline struct mtd_info * mtd_by_name(const char *name)
{
	struct mtd_info *mtd;
	for_all_mtd(mtd) {
		if(strcmp(mtd->name, name) == 0)
			return mtd;
	}
	return NULL;
}

#define MTD_ENV_NAME "bootdevice"

static inline struct mtd_info* mtd_get_default(void)
{
	const char *name = getenv(MTD_ENV_NAME);
	if(name) {
		return mtd_by_name(name);
	}
	else {
		return mtd_get_first();
	}
}

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

int mtd_overwrite_pages(struct mtd_info* mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size);

int mtd_erase_blocks(struct mtd_info *mtd,
	uint64_t offset,
        uint64_t size, int skipbb, int markbad);

#endif

