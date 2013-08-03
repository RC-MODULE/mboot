/*
 * drivers/mtd/nand/nand_util.c
 *
 * Copyright (C) 2006 by Weiss-Electronic GmbH.
 * All rights reserved.
 *
 * @author:	Guido Classen <clagix@gmail.com>
 * @descr:	NAND Flash support
 * @references: borrowed heavily from Linux mtd-utils code:
 *		flash_eraseall.c by Arcom Control System Ltd
 *		nandwrite.c by Steven J. Hill (sjhill@realitydiluted.com)
 *			       and Thomas Gleixner (tglx@linutronix.de)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
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
 *
 * Copyright 2010 Freescale Semiconductor
 * The portions of this file whose copyright is held by Freescale and which
 * are not considered a derived work of GPL v2-only code may be distributed
 * and/or modified under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <common.h>
#include <command.h>
#include <watchdog.h>
#include <malloc.h>
#include <errno.h>
#include <mtd.h>

inline loff_t mtd_current_block(struct mtd_info *mtd, loff_t off)
{
	return off & ~((loff_t)(mtd->erasesize-1));
}

/*
 * Reads user data from the [flash_offset, flash_offset + flash_size) area.
 * Skips bad blocks. Returns 0 on succcess (buffer_size bytes have been
 * transferred).
 */
int mtd_read_pages(struct mtd_info *mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size)
{
	int ret;
	u8 *buffer_start = buffer;
	u8 *buffer_end = buffer + buffer_size;
	loff_t flash_start = flash_offset;

	if(flash_end < flash_offset) {
		printf("MTD error: start (0x%08llX) > end (0x%08llX)\n",
			flash_offset, flash_end);
		return -EINVAL;
	}

	ret = 0;
	while((buffer<buffer_end) && (flash_offset<flash_end)) {
		uint64_t curr_eb = mtd_current_block(mtd, flash_offset);
		/* distance to next erase block */
		uint64_t nb = MIN(flash_end - flash_offset, mtd->erasesize-(flash_offset - curr_eb));
		/* size of user data left */
		size_t sz = MIN(buffer_end - buffer, nb);

		ret = mtd_block_isbad(mtd, curr_eb);
		if(ret < 0) {
			printf("MTD read_pages isbad failure: ret %d\n", ret);
			goto err;
		}
		if(ret == 1) {
			printf("MTD skipping bad block: off 0x%08llX\n", curr_eb);
			flash_offset += nb;
		}
		else {
			ret = mtd_read (mtd, flash_offset, buffer, sz);
			if (ret < 0) {
				printf("MTD read_pages read failure: off 0x%08llX ret %d\n",
					flash_offset, ret);
				goto err;
			}

			flash_offset += sz;
			buffer += sz;
		}
	}

	if(buffer < buffer_end) {
		printf(
			"MTD read_pages unable to read all the data: "
			"off 0x%08llX sz 0x%08X read 0x%X\n",
			flash_start, buffer_size, buffer - buffer_start);
		return -EFAULT;
	}

err:
	return ret;
}

/*
 * Fits user data to the [flash_offset, flash_offset + flash_size) MTD area.
 * Skips bad blocks. All sizes should be page-aligned.
 *
 * Returns 0 on succcess (all user data has been transfered).
 */
int mtd_write_pages(struct mtd_info *mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size)
{
	int ret;
	u8 *buffer_start = buffer;
	u8 *buffer_end = buffer + buffer_size;
	loff_t flash_start = flash_offset;
	loff_t off;
	u8 *page;

	if(flash_end < flash_offset) {
		printf("MTD error: start (0x%08llX) > end (0x%08llX)\n",
			flash_offset, flash_end);
		return -EINVAL;
	}

	page = malloc(mtd->writesize);
	if(page == NULL) {
		printf("MTD write_pages unable to allocate memory\n");
		return -ENOMEM;
	}

	ret = 0;
	while((buffer<buffer_end) && (flash_offset<flash_end)) {
		uint64_t curr_eb = mtd_current_block(mtd, flash_offset);
		/* distance to next erase block */
		uint64_t nb = MIN(flash_end - flash_offset, mtd->erasesize-(flash_offset - curr_eb));
		/* size of user data left */
		size_t sz = MIN(buffer_end - buffer, nb);

		ret = mtd_block_isbad(mtd, curr_eb);
		if(ret < 0) {
			printf("MTD write_pages isbad failure: ret %d\n", ret);
			goto err;
		}
		if(ret == 1) {
			printf("MTD write_pages skipping bad block: off 0x%08llX\n", curr_eb);
			flash_offset += nb;
			ret = 0;
			continue;
		}

		if(sz % mtd->writesize != 0) {
			size_t wsz = sz - (sz % mtd->writesize);
			if(wsz > 0) {
				ret = mtd_write (mtd, flash_offset, buffer, wsz);
				if (ret < 0) {
					printf("MTD write_pages write1 failure: off 0x%08llX ret %d\n",
						flash_offset, ret);
					goto err;
				}
			}

			memcpy(page, buffer+wsz, sz-wsz);
			memset(page+(sz-wsz), 0xFE, mtd->writesize-(sz-wsz));

			assert(mtd->writesize < nb-wsz);
			ret = mtd_write (mtd, flash_offset+wsz, page, mtd->writesize);
			if (ret < 0) {
				printf("MTD write_pages write2 failure: off 0x%08llX ret %d\n",
					flash_offset+wsz, ret);
				goto err;
			}
		}
		else {
			ret = mtd_write (mtd, flash_offset, buffer, sz);
			if (ret < 0) {
				printf("MTD write_pages write failure: off 0x%08llX ret %d\n",
					flash_offset, ret);
				goto err;
			}
		}

		for(off=flash_offset; off<(flash_offset+nb); off+=mtd->writesize) {
			ret = mtd_read (mtd, off, page, mtd->writesize);
			if (ret < 0) {
				printf("MTD write touch_pages read failure: off 0x%08llX ret %d\n",
					off, ret);
				goto err;
			}
		}

		flash_offset += nb;
		buffer += sz;
	}

	if(buffer < buffer_end) {
		printf(
			"MTD write_pages unable to write all the data: "
			"off 0x%08llX sz 0x%08X written 0x%X\n",
			flash_start, buffer_size, buffer - buffer_start);
		ret = -EFAULT;
		goto err;
	}

err:
	free(page);
	return ret;
}


/* 
 * Erases the mtd in range [offset, offset+size).
 * skipbb == 1 - Don't erase blocks marked as bad.
 * markbad == 1 - Mark blocks failing to erase as bad
 * Returns: >=0  - ok, number of bad blocks (if skipbb), <0 - error code
 */
int mtd_erase_blocks(
	struct mtd_info *mtd, uint64_t offset, uint64_t size, int skipbb, int markbad)
{
	int bb;
	int ret;
	struct erase_info erase;

	if (offset % mtd->erasesize) {
		printf("MTD Attempt to erase non page aligned data\n");
		return -EINVAL;
	}

	if (size == 0) {
		printf("MTD Invalid erase size: 0\n");
		return -EINVAL;
	}

	if (size % mtd->erasesize) {
		printf("MTD Attempt to erase non page sized data\n");
		return -EINVAL;
	}

	memset(&erase, 0, sizeof(erase));

	erase.mtd = mtd;
	erase.len  = mtd->erasesize;
	erase.addr = offset;
	ret = 0;
	bb = 0;
	int n=0;
	uint64_t addr;
	for(addr=offset; addr<(offset+size); addr+=mtd->erasesize) {
		erase.addr = addr;
		WATCHDOG_RESET ();
		n++;
		if (skipbb) {
			ret = mtd->block_isbad(mtd, erase.addr);
			if(ret < 0) {
				printf("MTD bad block detection failure: offset 0x%08llX\n",
					erase.addr);
				goto err;
			}
			else if (ret == 1) {
				printf("MTD skipping bad block: off 0x%08llX\n",
					erase.addr);
				bb += 1;
				ret = 0;
				continue;
			}
		} 

		ret = mtd->erase(mtd, &erase);
		if (ret != 0) {
			printf("MTD erase failure: off 0x%08llX ret %d\n",
				erase.addr, ret);
			if (markbad) {
				mtd_block_markbad(mtd, erase.addr);
				printf("Marked as bad\n");
				bb+=1;
				continue;
			}
			goto err;
		}
	}

	printf("erase: %d blocks processed; %d were bad\n", n, bb);
	return bb;

err:
	return ret;
}

/* 
 * Overwrites data on the mtd device. flash_offset and flash_end must be a
 * fraction of mtd->erasesize. buffer_size should be smaller than (flash_end -
 * flash_offset)
 *
 * Returns: 0 - ok, >0  - # of bad blocks, < 0 - error
 */
int mtd_overwrite_pages(struct mtd_info* mtd,
	loff_t flash_offset, loff_t flash_end,
	u8* buffer, size_t buffer_size)
{
	int bb;
	int ret;
	struct erase_info erase;
	u8 *buffer_start = buffer;
	u8 *buffer_end = buffer + buffer_size;
	u8 *page;
	loff_t flash_start = flash_offset;
	loff_t off;

	if (flash_offset % mtd->erasesize) {
		printf("MTD bad flash_offset\n");
		return -EINVAL;
	}

	if (flash_end % mtd->erasesize) {
		printf("MTD bad flash_size\n");
		return -EINVAL;
	}

	page = malloc(mtd->writesize);
	if(page == NULL) {
		printf("MTD overwrite unable to allocate memory\n");
		return -ENOMEM;
	}

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;

	ret = 0;
	bb = 0;

	while((buffer<buffer_end) && (flash_offset<flash_end)) {
		/* distance to next erase block */
		uint64_t nb = MIN(flash_end - flash_offset, mtd->erasesize);
		/* size of user data chunk */
		size_t sz = MIN(buffer_end - buffer, nb);

		WATCHDOG_RESET ();

		ret = mtd->block_isbad(mtd, flash_offset);
		if(ret < 0) {
			printf("MTD bad block detection failure: offset 0x%08llX\n",
				flash_offset);
			goto err;
		}
		else if (ret == 1) {
			printf("MTD skipping bad block: off 0x%08llX\n",
				flash_offset);
			bb += 1;
			flash_offset += nb;
			ret = 0;
			continue;
		}

		erase.addr = flash_offset;
		erase.len = MIN(nb, mtd->erasesize);
		ret = mtd->erase(mtd, &erase);
		if (ret != 0) {
			printf("MTD overwrite_pages erase failure: off 0x%08llX ret %d\n",
				erase.addr, ret);
			goto err;
		}

		if(sz % mtd->writesize != 0) {
			size_t wsz = sz - (sz % mtd->writesize);
			if(wsz > 0) {
				ret = mtd_write (mtd, flash_offset, buffer, wsz);
				if (ret < 0) {
					printf("MTD overwrite_pages write1 failure: off 0x%08llX ret %d\n",
						flash_offset, ret);
					goto err;
				}
			}

			memcpy(page, buffer+wsz, sz-wsz);
			memset(page+(sz-wsz), 0xFE, mtd->writesize-(sz-wsz));

			assert(mtd->writesize < nb-wsz);
			ret = mtd_write (mtd, flash_offset+wsz, page, mtd->writesize);
			if (ret < 0) {
				printf("MTD overwrite_pages write2 failure: off 0x%08llX ret %d\n",
					flash_offset+wsz, ret);
				goto err;
			}
		}
		else {
			ret = mtd_write (mtd, flash_offset, buffer, sz);
			if (ret < 0) {
				printf("MTD overwrite_pages write failure: off 0x%08llX ret %d\n",
					flash_offset, ret);
				goto err;
			}
		}

		for(off=flash_offset; off<(flash_offset+nb); off+=mtd->writesize) {
			ret = mtd_read (mtd, off, page, mtd->writesize);
			if (ret < 0) {
				printf("MTD overwrite touch_pages read failure: off 0x%08llX ret %d\n",
					off, ret);
				goto err;
			}
		}

		buffer += sz;
		flash_offset += nb;
	}

	if(buffer < buffer_end) {
		printf(
			"MTD overwrite_pages unable to write all the data: "
			"off 0x%08llX sz 0x%08X written 0x%X\n",
			flash_start, buffer_size, buffer - buffer_start);
		ret = -EFAULT;
		goto err;
	}

	ret = bb;

err:
	free(page);
	return ret;
}

