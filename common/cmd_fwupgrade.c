#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <mtd.h>

struct fwu_tftp_ctx {
	struct mtd_info *mtd;
	u8* page;
	loff_t flash_offset;
	loff_t flash_end;
};

static struct fwu_tftp_ctx fwu_ctx;
static int fwu_tftp_cb(uint32_t off, u8* buf, size_t len, void* priv)
{
	struct fwu_tftp_ctx * ctx = (struct fwu_tftp_ctx*)priv;
	if(ctx == NULL) {
		return -EINVAL;
	}

	printf("FWU got data: off 0x%08X len %d\n", off, len);
	return 0;
}

static int fwupgrade(struct mtd_info *mtd, char *file, loff_t from, loff_t size)
{
	int ret;
	struct fwu_tftp_ctx *ctx = &fwu_ctx;
	u8 *page;

	page = malloc(mtd->writesize);
	if(page == NULL) {
		printf("FWU unable to allocate memory\n");
		return -ENOMEM;
	}

	memset(ctx, 0, sizeof(struct fwu_tftp_ctx));
	ctx->mtd = mtd;
	ctx->page = page;
	ctx->flash_offset = from;
	ctx->flash_end = from + size;

	TftpCallback = fwu_tftp_cb;
	TftpCtx = ctx;
	setenv_s("bootfile", file);

	ret = NetLoop(TFTP);
	if (ret < 0) {
		printf("FWU net trasfer failed: ret %d\n", ret);
		goto out;
	}

out:
	TftpCallback = NULL;
	TftpCtx = NULL;
	free(page);
	return 0;
}

static int do_fwupgrade(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	int idx;
	char file[MAXPATH];
	int ret;

	idx = simple_strtoul(getenv("bootdevice"), NULL, 0);
	mtd = mtd_by_index(idx);
	if(mtd == NULL) {
		printf("FWU mtd device %d not available\n", idx);
		return -1;
	}
	
	switch(argc) {
		//case 1:
			//break;
		case 2:
			if(0 == strcmp(argv[1],"boot")) {
				getenv_s("mbootfile", file, (CONFIG_BOOTDIR "/mboot.bin"));
				ret = fwupgrade(mtd, file,
					CONFIG_MNAND_UBOOT_OFF,
					CONFIG_MNAND_UBOOT_SIZE);
			}
			else if(0 == strcmp(argv[1],"kernel")) {
				getenv_s("bootfile", file, CONFIG_BOOTFILE);
				ret = fwupgrade(mtd, file,
					CONFIG_MNAND_KERNEL_OFF,
					CONFIG_MNAND_KERNEL_SIZE);
			}
			else {
				return cmd_usage(cmdtp);
			}
			//else if(0 == strcmp(argv[1],"fs")) {
			//}
			break;

		default:
			return cmd_usage(cmdtp);
	}

	return ret;
}

//// 	printf("Starting incremental nand update\n");
	//if (NULL==getenv("fwaddr"))
	//{
		//printf("fwaddr not set\n");
		//return 0;
	//}
	//ulong addr = simple_strtoul(getenv("loadaddr"), NULL, 16);
	//ulong fw_offset = simple_strtoul(getenv("fwaddr"), NULL, 16);
	//nand_info_t *nand = &mtd_info[0];
	//char* bootfile = strdup(getenv("bootfile"));
	//char* tmp = malloc(128);
	////Erase all the flash
	//#ifdef CONFIG_FWU_ERASE
	//nand_erase_options_t nand_erase_options;
	//memset(&nand_erase_options, 0, sizeof(nand_erase_options));
	//nand_erase_options.offset = fw_offset;
//// 	nand_erase_options.spread =1;
	//nand_erase_options.length = nand->size - fw_offset;	
 	//printf("Cleaning [%d ==> %d / %d]\n", fw_offset, nand_erase_options.length, nand->size);
	//if (nand_erase_opts(nand, &nand_erase_options))
					//return 1;
	//#endif
	//int size;
	//size_t pos = fw_offset;
	//size_t end;
	//size_t sz;
	//int i=0;
	//while (i<5)
	//{
		//sprintf(tmp, "%s.%d", bootfile, i++);
//// 		printf("Loading: %s\n", tmp);
		//setenv("bootfile",tmp);
		//size = NetLoop(TFTP);
		//if (size <= 0)
		//{
			//break;
		//}
		
		//else if (size > 0)
			//flush_cache(addr, size);
		//size -= size % nand->erasesize;
		//size += nand->erasesize;
		//sz=size;
		//end = pos + size;
		//printf("Writing at 0x%x sz 0x%x\n", pos, sz);
			//if (nand_write_skip_bad(nand, pos, &sz,
					//(char*) addr))
				//goto bailout;
		//pos+=sz;
		//if (pos % nand->erasesize)
		 		//printf("Pos not aligned: 0x%x\n", pos);
		//}
		
	//bailout:
		//setenv("bootfile", bootfile);
	//return 0;

U_BOOT_CMD(
	fwupgrade,	2,	0,	do_fwupgrade,
	"Perform a system upgrade",
	""
);

