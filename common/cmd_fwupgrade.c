#include <common.h>
#include <command.h>
#include <errno.h>
#include <mtd.h>

struct fwu_tftp_ctx {
	struct mtd_info *mtd;
	u8* block;
	size_t block_sz;
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
				printf("FWU bad block detection failure: off 0x%012llX\n",
					ctx->flash_offset);
				goto err;
			}
			else if (ret == 1) {
				printf("FWU bad block detected: off 0x%012llX\n",
					ctx->flash_offset);
				bad = 1;
				ret = 0;
			}

			for(retry=0; (retry<5)&&(ctx->block_sz>0); retry++) {
				printf("FWU rewriting block: off 0x%012llX%s%s\n",
					ctx->flash_offset,
					bad ? " (BAD)" : "",
					retry == 0 ? "" : " (again)");

				ret = mtd_erase1(mtd, ctx->flash_offset);
				if(ret < 0) {
					printf("FWU erase err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					continue;
				}

				ret = mtd_write(mtd, ctx->flash_offset, ctx->block, mtd->erasesize);
				if(ret < 0) {
					printf("FWU write err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					continue;
				}

				/* read back to force hardware CRC checks */
				ret = mtd_read(mtd, ctx->flash_offset, ctx->block, mtd->erasesize);
				if(ret < 0) {
					printf("FWU checkread err: off 0x%012llX ret %d\n",
						ctx->flash_offset, ret);
					continue;
				}

				ctx->flash_offset += ctx->block_sz;
				ctx->block_sz = 0;
			}

			if(ctx->block_sz > 0) {
				printf("FWU failed to overwrite block 0x%012llX ret %d\n",
					ctx->flash_offset, ret);
				goto err;
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

static int do_fwupgrade(struct cmd_ctx *cmdctx, int argc, char * const argv[])
{
	struct mtd_info *mtd = NULL;
	const char *filename = NULL;
	int ret;

	switch(argc) {
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
		printf("FWU mtd device is not available\n");
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
	fwupgrade,	3,	0,	do_fwupgrade,
	"Firmware upgrade via TFTP",
	"fwupgrade MTD [FNAME] ...\n"
	" - rewrites mtd device MTD using TFTP\n"
	"     default TFTP file name is generated from MTD device name as follows:\n"
	"     boot ..... dir($(bootfile))/mboot.img\n" 
	"     kernel ... dir($(bootfile))/uImage\n" 
	"     NAME ..... dir($(bootfile))/mboot-NAME.bin\n"
);

