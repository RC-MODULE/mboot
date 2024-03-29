#include <common.h>
#include <command.h>
#include <errno.h>
#include <mtd.h>
#include <asm/sizes.h>

struct fwu_tftp_ctx {
	struct mtd_info *mtd;
	u8* block;
	size_t block_sz;
	loff_t flash_offset;
	loff_t op_start;
	loff_t flash_end;
	int bb;
	int last;
	int oob;
};

static int fwu_tftp_cb(uint32_t off, u8* buf, size_t len, int last, void* priv)
{
	int ret;
	struct fwu_tftp_ctx *ctx = (struct fwu_tftp_ctx*)priv;

	if(ctx == NULL) {
		return -EINVAL;
	}

	struct mtd_info *mtd = ctx->mtd;

	BUG_ON(ctx->last);
	ctx->last = last;

	ret = 0;	
	while(len > 0) {
		size_t len1 = MIN(mtd->erasesize - ctx->block_sz, len);
		memcpy(&ctx->block[ctx->block_sz], buf, len1);

		ctx->block_sz += len1;
		buf += len1;
		len -= len1;

		if((ctx->block_sz == mtd->erasesize) || last) {

			int retry;
			int bad = 0;

			ret = mtd_block_isbad(mtd, ctx->flash_offset);
			if(ret < 0) {
				printf("\nFWU bad block detection failure: off 0x%012llX\n",
					ctx->flash_offset);
				goto err;
			}
			else if (ret == 1) {
				printf("\nFWU bad block detected: off 0x%012llX\n",
					ctx->flash_offset);
				bad = 1;
				ret = 0;
			}

			for(retry=0; ((retry<3) && (ctx->block_sz>0)); retry++) {
				printf("FWU rewriting block: off 0x%012llX / %lldM %s%s %d\t\t\r",
				       ctx->flash_offset,
				       ctx->flash_offset / 1024 / 1024,
				       bad ? " \t(BAD)\n" : "",
				       retry == 0 ? "" : " \t(again)\n", retry);

				if (retry) {
					ret = mtd_erase1(mtd, ctx->flash_offset);
					if(ret < 0) {
						printf("\nFWU erase err: off 0x%012llX ret %d\n",
						       ctx->flash_offset, ret);
						continue;
					}
				}

				if (ctx->oob==0)
					ret = mtd_write(mtd, ctx->flash_offset, ctx->block, mtd->erasesize);
				else {
					int tmp; 
					for (tmp=0; tmp < (mtd->erasesize / mtd->writesize); tmp ++ ) {
						ret = mtd_writeoob(mtd, 
								   ctx->flash_offset + (tmp * mtd->writesize), 
								   ctx->block + tmp * (mtd->writesize + mtd->oobsize), 
								   mtd->writesize);
						if (ret<0)
							break;
					}
				}

				if(ret < 0) {
					printf("\nFWU write err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					continue;
				}

				/* read back to force hardware CRC checks */
				ret = mtd_read(mtd, ctx->flash_offset, ctx->block, mtd->erasesize);
				if(ret < 0) {
					printf("\nFWU checkread err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					continue;
				}
				ctx->flash_offset += mtd->erasesize;
				ctx->block_sz = 0;
			}

			if(ctx->block_sz > 0) {
				printf("\nFWU failed to overwrite block, skipping it and marking bad: off 0x%012llX ret %d\n",
					ctx->flash_offset, ret);
				mtd_erase1(mtd, ctx->flash_offset); /* Don't allow just to screw us */
				mtd_block_markbad(mtd, ctx->flash_offset);
				ctx->flash_offset += mtd->erasesize;
			}
		}
	}

err:
	return ret;
}

/* Extracts directory name with trailing '/' symbol or empty str if no dirs */
static void extract_dirname(char *dir, const char* path)
{
	const char* p = strrchr(path,'/');
	if(p == NULL)
		p = path;
	else
		p = p+1;
	strncpy(dir, path, p-path);
	dir[p-path]='\0';
}


#define COLS 30 
static int erase_region(struct mtd_info *mtd, uint64_t offset, uint64_t length, int scrub, int markbad)
{
	printf("REGIONERASE: erasing NAND from 0x%llx to 0x%llx...\n", offset, length);
	printf("REGIONERASE: scrub %d markbad %d\n", scrub, markbad);
	int ret = mtd_erase_blocks(mtd, offset, length, !scrub, markbad);
	if (ret>0)
		return 0;
	return ret;
}


static int erase_part(struct mtd_info *mtd, int scrub, int markbad)
{
	printf("PARTERASE: erasing NAND, please stand by...\n");
	printf("PARTERASE: scrub %d markbad %d\n", scrub, markbad);
	int ret = mtd_erase_blocks(mtd, 0, mtd->size, !scrub, markbad);
	if (ret>0)
		return 0;
	return ret;	
}

static int do_perase(struct cmd_ctx *cmdctx, int argc, char * const argv[])
{
	int scrub = 0;
	int ret; 
	int markbad =1;
	struct mtd_info *mtd = NULL;

	switch (argc) {
	case 4:
		markbad = (argv[3][0] == 'y');
	case 3: 
		scrub   = (argv[2][0] == 'y');
	case 2:
		mtd = mtd_by_name(argv[1]);
		break;
	default:
		printf("Invalid args\n");
		return 1;
	}

	if(mtd == NULL) {
		printf("FWU mtd device is not available (bad part name?)\n");
		return -1;
	}

	ret = erase_part(mtd, scrub, markbad);
	if (ret)
	{
		printf("FW: %d bad blocks encountered...\n", ret);
	}
	return 0;
}

extern volatile uint32_t *g_uemd_magic;

#define MAGIC_READY 0xa1
#define MAGIC_DONE  0x0




/* 
   SLAVE: these are the settings es
   HOST: Okay
   SLAVE: Buffer
   Host: Ready
   SLAVE: Buffer
   Host: Ready
   ....
   Slave: Buffer
   Host: Done
 */

struct efw_settings {
	uint32_t erase_size;
	uint32_t oob_size;
	uint64_t size; 
	uint32_t writesize; 
} __attribute__((packed));



static int write_block(struct mtd_info* mtd, uint64_t* offset, unsigned char* buf, unsigned char* tmpbuf, int has_oob)
{
	
	int ret=0;
	int retry;
	int markbad=0;
next:
	markbad = 0;
	ret = mtd_block_isbad(mtd, *offset);
	if(ret < 0) {
		printf("\nFWU bad block detection failure: off 0x%012llX\n",
		       *offset);
		return -1;
	}
	else if (ret == 1) {
		printf("\nFWU bad block detected: off 0x%012llX\n",
		       *offset);
		*offset += mtd->erasesize;
		goto next;
	}
	
	for(retry=0; (retry<3); retry++) {
		printf("FWU rewriting block: off 0x%012llX / %lldM %s %d\t\t\r",
		       *offset,
		       *offset / 1024 / 1024,
		       retry == 0 ? "" : " \t(again)\n", retry);
		
		if (retry) {
			ret = mtd_erase1(mtd, *offset);
			if(ret < 0) {
				printf("\nFWU erase err: off 0x%012llX ret %d\n",
				       *offset, ret);
				markbad = 1;
				continue;
			}
		}
		
		if (!has_oob)
			ret = mtd_write(mtd, *offset, buf, mtd->erasesize);
		else {
			int tmp; 
			for (tmp=0; tmp < (mtd->erasesize / mtd->writesize); tmp++ ) {
				ret = mtd_writeoob(mtd, 
						   *offset + (tmp * mtd->writesize), 
						   buf + tmp * (mtd->writesize + mtd->oobsize), 
						   mtd->writesize);
				if (ret<0)
					break;
			}
		}
		
		if(ret < 0) {
			printf("\nFWU write err: off 0x%012llX ret %d\n",
			       *offset, ret);
			markbad = 1;
			continue;
		}
		
		/* read back to force hardware CRC checks */
		ret = mtd_read(mtd, *offset, tmpbuf, mtd->erasesize);
		if(ret < 0) {
			printf("\nFWU checkread err: off 0x%012llX ret %d\n",
			       *offset, ret);
			markbad = 1;
			continue;
		}
		*offset += mtd->erasesize;
		break;
	}

	if (markbad) {
		printf("\nFWU failed to overwrite block, skipping it and marking bad: off 0x%012llX ret %d\n",
		       *offset, ret);
		mtd_erase1(mtd, *offset); /* Try to make sure block is somewhat clean */
		mtd_block_markbad(mtd, *offset);
		*offset += mtd->erasesize;
		goto next;
	}
	return 0;
}


#define e_do_sync(buffer)						\
	while (1) {							\
		volatile uint32_t tmp = *g_uemd_magic;			\
		if (*g_uemd_magic == MAGIC_DONE)			\
			goto out;					\
									\
		if (tmp != (volatile uint32_t) buffer)	\
			break;						\
									\
	}


static inline int str2szll(const char *p, uint64_t *ll)
{
	char *endptr;
	*ll = simple_strtoull(p, &endptr, 0);
	return (*p != '\0' && *endptr == '\0') ? 0 : -1;
}


static int do_eupgrade(struct cmd_ctx *cmdctx, int argc, char * const argv[])
{
	struct mtd_info *mtd = mtd_by_name(argv[1]);
	int ret;
	struct efw_settings es; 
	unsigned long laddr; 
	unsigned char* buffer0;
	unsigned char* buffer1; 
	unsigned char* buffer2; 
	uint64_t offset = 0;
	getenv_ul("loadaddr", &laddr, 0);
	if (laddr == 0)
	{
		printf("Bad loadaddr\n");
		return -1;
	}
	buffer0 = ((unsigned char*) laddr);
	buffer1 = buffer0 + SZ_1M;
	buffer2 = buffer1 + SZ_1M;
	printf("buffer   0 @ %x\n", (unsigned int) buffer0);
	printf("buffer   1 @ %x\n", (unsigned int) buffer1);
	printf("readback   @ %x\n", (unsigned int) buffer2);

	if (mtd == NULL) {
		printf("FWU mtd device is not available, assuming it's an offset\n");
		mtd = mtd_by_name("firmware");
		str2szll(argv[1], &offset);
	}

	if (mtd == NULL) {
		printf("FWU internal error, check nand chip\n");
		return -1;
	}

	es.erase_size = mtd->erasesize;
	es.oob_size   = mtd->oobsize;
	es.size       = mtd->size;
	es.writesize  = mtd->writesize; 

	if (argc > 2) {
		uint64_t length; 
		str2szll(argv[2], &length);
		ret = erase_region(mtd, offset, length, 0, 1);
		if (ret)
		{
			printf("FW: Region erase failed, aborting...\n");
			return -1;
		}
	} else {
		ret = erase_part(mtd, 0, 1);
		if (ret)
		{
			printf("FW: Partition erase failed, aborting...\n");
			return -1;
		}
	}
	
	*g_uemd_magic = (uint32_t) &es;
	while (*g_uemd_magic != MAGIC_READY);;
	printf("FW: sent info\n");

	*g_uemd_magic = (uint32_t) buffer0;
	while (*g_uemd_magic != MAGIC_READY);;

	printf("FW: got b0\n");

	while (1) 
	{	
		*g_uemd_magic = (uint32_t) buffer1;

		write_block(mtd, &offset, buffer0, buffer2, 0);
		
		e_do_sync(buffer1);

		*g_uemd_magic = (uint32_t) buffer0;

		write_block(mtd, &offset, buffer1, buffer2, 0);

		e_do_sync(buffer0);
		
	}
out:
	printf("\nFW: done \n");
	*g_uemd_magic = 0x0;
	return 0;
}


static int do_fwupgrade(struct cmd_ctx *cmdctx, int argc, char * const argv[])
{
	struct mtd_info *mtd = NULL;
	const char *filename = NULL;
	int ret;
	int oob = 0;
	switch(argc) {
	        case 4:
			oob = (argv[3][0] == 'y');;
			if (oob) printf("FWU: Will write oob data!\n");
			/* pass through */
		case 3:
			filename = argv[2];
			/* pass through */
		case 2:
			mtd = mtd_by_name(argv[1]);
			break;
		default:
			return cmd_usage(cmdctx->cmdtp);
	}

	if(mtd == NULL) {
		printf("FWU mtd device is not available (bad part name?)\n");
		return -1;
	}

	if(mtd->erasesize > CONFIG_CMD_FWUPGRADE_BUFFER_SIZE) {
		printf("FWU internal buffer is smaller than size of eraseblock\n");
		return -ENOMEM;
	};
	u8 *block = (u8*)CONFIG_CMD_FWUPGRADE_BUFFER_ADDR;
	if(block == NULL) {
		printf("FWU unable to allocate memory\n");
		return -ENOMEM;
	}

	struct fwu_tftp_ctx ctx;
	memset(&ctx, 0, sizeof(struct fwu_tftp_ctx));
	ctx.mtd = mtd;
	ctx.block = block;
	ctx.flash_offset = 0;
	ctx.flash_end = mtd->size;
	ctx.oob = oob;

	struct NetTask task;
	net_init_task_def(&task, TFTP);
	task.u.tftp.data_cb = fwu_tftp_cb;
	task.u.tftp.data_ctx = &ctx;

	char dir[MAXPATH];
	extract_dirname(dir, task.bootfile);
	if(filename) {
		if(filename[0] == '/') {
			strncpy_s(task.bootfile, filename, MAXPATH);
		}
		else {
			sprintf(task.bootfile, "%s%s", dir, filename);
		}
	} else {
		if(0 == strcmp(mtd->name, "boot")) {
			sprintf(task.bootfile, "%smboot.img", dir);
		} else if(0 == strcmp(mtd->name, "kernel")) {
			sprintf(task.bootfile, "%suImage", dir);
		} else {
			sprintf(task.bootfile, "%smboot-%s.bin", dir, mtd->name);
		}
	}
	
	ret = erase_part(mtd, 0, 1);
	if (ret)
	{
		printf("FW: Partition erase failed, aborting...");
		goto out;
	}

	ret = NetLoop(&task);
	if (ret < 0) {
		printf("FWU net trasfer failed: ret %d\n", ret);
		goto out;
	}

	printf("FWU complete: tftp_bytes %lu hex 0x%08lX flash_area 0x%012llX flash_bb %d\n",
		task.out_filesize, task.out_filesize, ctx.flash_offset, ctx.bb);

out:
	return 0;
}



U_BOOT_CMD(
	parterase,	4,	0,	do_perase,
	"Erase an mtd part",
	"parterase MTD [scrub] [markbad]\n"
);


U_BOOT_CMD(
	fwupgrade,	4,	0,	do_fwupgrade,
	"Firmware upgrade via TFTP",
	"fwupgrade MTD [FNAME] [OOB] ...\n"
	" - rewrites mtd device MTD using TFTP\n"
	"     default TFTP file name is generated from MTD device name as follows:\n"
	"     boot ..... dir($(bootfile))/mboot.img\n" 
	"     kernel ... dir($(bootfile))/uImage\n" 
	"     NAME ..... dir($(bootfile))/mboot-NAME.bin\n"
	" OOB == y instructs to write oob data from image as well"
);

U_BOOT_CMD(
	eupgrade,	4,	0,	do_eupgrade,
	"Firmware upgrade via EDCL",
	"fwupgrade MTD\n"
	" - rewrites mtd device MTD using EDCL\n"
);


