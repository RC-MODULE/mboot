
#include <common.h>
#include <command.h>
#include <bzlib.h>

#define DDR_EM0				      0x40000000
#define XDMAC_BASE			      0x10070000			
#define MEM(addr)  (*((volatile unsigned long int*)(addr)))


extern int do_mtest_bzip2 (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret,i,ec, sz, checker;
	u32 dstLen,srcLen,dstLen2;
	char *src,*dst,*dst2;
	char *endptr;
	int verbose;

	sz = 31213;

	if(argc > 1) {
		if (0 == strcmp(argv[1],"large")) {
			sz = 3159400;
		} else if (0 == strcmp(argv[1],"small")) {
			sz = 31213;
		} else {
			printf("Invalid size\n");
			goto usage;
		}
	}

	verbose = 0;

	printf("Assuming file size: %d\n", sz);

	src = (char*)PHYS_EM1;
	srcLen = sz;

	dst =  (char*)PHYS_EM0 + 0x01000000;
	dst2 = (char*)PHYS_EM0 + 0x07000000;

	printf("Resetting malloc subsystem\n");
	mem_malloc_init(PHYS_EM0, 0x01000000);

	for(i=0,ec=0;;) {

		dstLen = 0x01000000;
		dstLen2 = 0x01000000;

		printf("Round %d c1. Errors: %d\n", i++, ec);

		ret = BZ2_bzBuffToBuffDecompress(
			/*        char*         dest,*/
			dst,
			/*        unsigned int* destLen,*/
			&dstLen,
			/*        char*         source,*/
			src,
			/*        unsigned int  sourceLen,*/
			srcLen,
			/*        int           small*/
			0,
			/*        int           verbosity,*/
			verbose
			);

		if (ret != BZ_OK) {
			printf("Failed to decompress 1 (%d)\n", ret);
			ec++;
			continue;
		}

		printf("Round %d c2. Errors: %d\n", i++, ec);

		ret = BZ2_bzBuffToBuffDecompress(
			/*        char*         dest,*/
			dst2,
			/*        unsigned int* destLen,*/
			&dstLen2,
			/*        char*         source,*/
			src,
			/*        unsigned int  sourceLen,*/
			srcLen,
			/*        int           small*/
			0,
			/*        int           verbosity,*/
			verbose
			);

		if (ret != BZ_OK) {
			printf("Failed to decompress 2 (%d)\n", ret);
			ec++;
			continue;
		}
		if (dstLen != dstLen2) {
			printf("Len mismatch (%d)\n", ret);
			ec++;
			continue;
		}

		printf("Lenghts are %d, checking... ", dstLen);
		for(checker=0; checker<dstLen; checker++) {
			if(dst[checker] != dst2[checker]) {
				printf("mismatch at 0x%08X:0x%08X \n", dst+checker, dst2+checker);
				ec++;
			}
		}
		printf("OK \n");
	}

	return ec;

usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	mtest_bzip2, 3, 0, do_mtest_bzip2 ,
	"run BZIP-based EM0 test",
	"LARGE|SMALL"
);

