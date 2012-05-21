#include <common.h>
#include <command.h>

extern void uemd_em_init(void);

extern int do_uemd_dram (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("Reintializing EM0 and EM1\n");
	uemd_em_init();
	return 0;
}


U_BOOT_CMD(
	uemd_dram, 1, 0, do_uemd_dram ,
	"initialize UEMD dram",
	""
);

