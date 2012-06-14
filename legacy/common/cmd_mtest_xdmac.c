
#include <common.h>
#include <command.h>

#define DDR_EM0				      0x40000000
#define XDMAC_BASE			      0x10070000			
#define MEM(addr)  (*((volatile unsigned long int*)(addr)))

void xdmac_init()
{
	//Init XDMAC channel 0
	MEM(XDMAC_BASE+0x10)=0x02000000; 	//Read counter
	MEM(XDMAC_BASE+0x14)=0x40000000; 	//Source address
	MEM(XDMAC_BASE+0x18)=0x43000000; 	//Destination address
	MEM(XDMAC_BASE+0x1c)=0x00030f00; 	//Read burst size
	MEM(XDMAC_BASE+0x20)=0x00030f00; 	//Write burst size
	MEM(XDMAC_BASE+0x24)=0x0;			//No descriptors chain
	MEM(XDMAC_BASE+0x2c)=0x0;			//No interrupts
	MEM(XDMAC_BASE+0x28)=0x11100000;	//Enable channel 0
}

int do_mtest_xdmac(void) 
{
	long int tmp = 0;
	long int i = 0;
	long int ec = 0;
	long int loop = 0;

	unsigned long begin = DDR_EM0 + 0x00000000;
	unsigned long end = DDR_EM0 + 0x02000000;

	for(loop=0;;loop++) {

		//Write 32Mb
		while(begin < end) {
			MEM(begin) = begin;
			begin=begin+4;
		}

		//Enable XDMAC
		MEM(XDMAC_BASE)=0x10000000;

		xdmac_init();
		
		//Read 32Mb while XDMAC is working
		begin = DDR_EM0 + 0x00000000;
	  
		while(begin < end) {

			//Check and restart XDMAC if necessary
			if(MEM(XDMAC_BASE+0x30) == 0x8) {
				xdmac_init();
			}
			
			if (MEM(begin) != begin) {
				ec=ec+1;
				printf("on %08lx, %d errors\n", begin, ec);
			}
			
			if( (begin % 0x00400000) == 0) {
				printf("on %08lx, %d errors\n", begin, ec);
			}

			if (ctrlc()) {
				goto exit;
			}

			begin=begin+4;
		}

		printf("Loop %d done, %d errors detected\n", loop, ec);

	}// while(1)

exit:
	printf("Break on loop %d, %d errors detected\n", loop, ec);
	// Wait XMDAC to stop
	while(MEM(XDMAC_BASE+0x30) != 0x8) { }
	// Disable XDMAC
	MEM(XDMAC_BASE)=0x0;
	return ec;
}

U_BOOT_CMD(
	mtest_xdmac, 1, 0, do_mtest_xdmac ,
	"run XDMAC-based EM0 test",
	"<no arguments>"
);

