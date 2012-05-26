#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <mtd.h>

struct fwu_tftp_ctx {
	struct mtd_info *mtd;
	u8* page;
	size_t page_sz;
	loff_t flash_offset;
	loff_t flash_end;
	int bb;
};

static int fwu_tftp_cb(uint32_t off, u8* buf, size_t len, int last, void* priv)
{
	int ret;
	struct fwu_tftp_ctx *ctx = (struct fwu_tftp_ctx*)priv;

	if(ctx == NULL) {
		return -EINVAL;
	}

	struct mtd_info *mtd = ctx->mtd;

	ret = 0;
	while(len > 0) {
		size_t len1 = MIN(mtd->writesize - ctx->page_sz, len);
		memcpy(&ctx->page[ctx->page_sz], buf, len1);

		ctx->page_sz += len1;
		buf += len1;
		len -= len1;

		if((ctx->page_sz == mtd->writesize) || last) {
			if(ctx->flash_offset % mtd->erasesize == 0) {
				ret = mtd_block_isbad(mtd, ctx->flash_offset);
				if(ret < 0) {
					printf("FWU bad block detection failure: off 0x%012llX\n",
						ctx->flash_offset);
					goto err;
				}
				else if (ret == 1) {
					printf("FWU skipping bad block: off 0x%012llX\n",
						ctx->flash_offset);
					ctx->flash_offset += mtd->erasesize;
					ctx->bb ++;
					continue;
				}

				printf("FWU rewriting block: off 0x%012llX\n",
					ctx->flash_offset);

				ret = mtd_erase1(mtd, ctx->flash_offset);
				if(ret < 0) {
					printf("FWU erase err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					goto err;
				}
			}

			ret = mtd_write(mtd, ctx->flash_offset, ctx->page, mtd->writesize);
			if(ret < 0) {
				printf("FWU write err: off 0x%012llX ret %d\n",
					ctx->flash_offset, ret);
				goto err;
			}

			/* read back to force hardware CRC checks */
			ret = mtd_read(mtd, ctx->flash_offset, ctx->page, mtd->writesize);
			if(ret < 0) {
				printf("FWU checkread err: off 0x%012llX ret %d\n",
					ctx->flash_offset, ret);
				goto err;
			}

			ctx->flash_offset += mtd->writesize;
			ctx->page_sz = 0;
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

static int do_fwupgrade(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	int ret;

	switch(argc) {
		case 1:
			mtd = mtd_get_default();
			break;
		case 2:
			mtd = mtd_by_name(argv[1]);
			break;
		default:
			return cmd_usage(cmdtp);
	}

	if(mtd == NULL) {
		printf("FWU mtd device is not available\n");
		return -1;
	}

	u8 *page;
	page = malloc(mtd->writesize);
	if(page == NULL) {
		printf("FWU unable to allocate memory\n");
		return -ENOMEM;
	}

	struct fwu_tftp_ctx ctx;
	memset(&ctx, 0, sizeof(struct fwu_tftp_ctx));
	ctx.mtd = mtd;
	ctx.page = page;
	ctx.flash_offset = 0;
	ctx.flash_end = mtd->size;

	struct NetTask task;
	net_init_task_def(&task, TFTP);
	task.u.tftp.data_cb = fwu_tftp_cb;
	task.u.tftp.data_ctx = &ctx;

	char dir[MAXPATH];
	extract_dirname(dir, task.bootfile);
	if(0 == strcmp(mtd->name, "boot")) {
		sprintf(task.bootfile, "%smboot.img", dir);
	} else if(0 == strcmp(mtd->name, "kernel")) {
		sprintf(task.bootfile, "%suImage", dir);
	} else {
		sprintf(task.bootfile, "%smboot-%s.bin", dir, mtd->name);
	}

	ret = NetLoop(&task);
	if (ret < 0) {
		printf("FWU net trasfer failed: ret %d\n", ret);
		goto out;
	}

	printf("FWU complete: tftp_bytes %lu hex 0x%08lX flash_area 0x%012llX flash_bb %d\n",
		task.out_filesize, task.out_filesize, ctx.flash_offset, ctx.bb);

out:
	free(page);
	return 0;
}

U_BOOT_CMD(
	fwupgrade,	2,	0,	do_fwupgrade,
	"Firmware upgrade via TFTP",
	"\n"
	"fwupgrade - upgrades current mtd device \n"
	"fwupgrade MTD - upgrades mtd device MTD (can use a partition)\n"
	"  TFTP file names by partition:\n"
	"  boot ..... dir($(bootfile))/mboot.img\n" 
	"  kernel ... dir($(bootfile))/uImage\n" 
	"  NAME ..... dir($(bootfile))/mboot-NAME.bin\n"
);

