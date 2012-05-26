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

static void replace_filename(char* path, const char *to)
{
	char* p = strrchr(path,'/');
	if(p == NULL)
		p = path;
	else
		p = p+1;
	strncpy_s(p, to, MAXPATH - (p-path));
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
	ctx->flash_offset = 0;
	ctx->flash_end = mtd->size;

	struct NetTask task;
	net_init_task_def(&task, TFTP);
	task.u.tftp.data_cb = fwu_tftp_cb;
	task.u.tftp.data_ctx = ctx;
	replace_filename(task.bootfile, mtd->name);

	ret = NetLoop(&task);
	if (ret < 0) {
		printf("FWU net trasfer failed: ret %d\n", ret);
		goto out;
	}

out:
	free(page);
	return 0;
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
	"Firmware upgrade via TFTP",
	"\n"
	"fwupgrade - upgrades current mtd device \n"
	"fwupgrade MTD - upgrades mtd device MTD (can use a partition)\n"
	" NOTE: TFTP file name is generated as follows:\n"
	"       \"dirname($bootfile)/name_of(MTD)\"\n"
);

