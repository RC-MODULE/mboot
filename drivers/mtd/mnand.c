/*
 * MTD device concatenation layer
 *
 * (C) 2010 Sergey Mironov <smironov@gmail.com>
 *
 * Driver fot mtd-like NAND controller by "Module" company
 *
 * This code is GPL
 */
#include <common.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/cache.h>
#include <mnand.h>

#ifndef CONFIG_CMD_NAND
struct nand_flash_dev nand_flash_ids[] = {

#ifdef CONFIG_MTD_NAND_MUSEUM_IDS
	{"NAND 1MiB 5V 8-bit",		0x6e, 256, 1, 0x1000, 0},
	{"NAND 2MiB 5V 8-bit",		0x64, 256, 2, 0x1000, 0},
	{"NAND 4MiB 5V 8-bit",		0x6b, 512, 4, 0x2000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xe8, 256, 1, 0x1000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xec, 256, 1, 0x1000, 0},
	{"NAND 2MiB 3,3V 8-bit",	0xea, 256, 2, 0x1000, 0},
	{"NAND 4MiB 3,3V 8-bit", 	0xd5, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe3, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe5, 512, 4, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xd6, 512, 8, 0x2000, 0},

	{"NAND 8MiB 1,8V 8-bit",	0x39, 512, 8, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xe6, 512, 8, 0x2000, 0},
	{"NAND 8MiB 1,8V 16-bit",	0x49, 512, 8, 0x2000, NAND_BUSWIDTH_16},
	{"NAND 8MiB 3,3V 16-bit",	0x59, 512, 8, 0x2000, NAND_BUSWIDTH_16},
#endif

	{"NAND 16MiB 1,8V 8-bit",	0x33, 512, 16, 0x4000, 0},
	{"NAND 16MiB 3,3V 8-bit",	0x73, 512, 16, 0x4000, 0},
	{"NAND 16MiB 1,8V 16-bit",	0x43, 512, 16, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 16MiB 3,3V 16-bit",	0x53, 512, 16, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 32MiB 1,8V 8-bit",	0x35, 512, 32, 0x4000, 0},
	{"NAND 32MiB 3,3V 8-bit",	0x75, 512, 32, 0x4000, 0},
	{"NAND 32MiB 1,8V 16-bit",	0x45, 512, 32, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 32MiB 3,3V 16-bit",	0x55, 512, 32, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 64MiB 1,8V 8-bit",	0x36, 512, 64, 0x4000, 0},
	{"NAND 64MiB 3,3V 8-bit",	0x76, 512, 64, 0x4000, 0},
	{"NAND 64MiB 1,8V 16-bit",	0x46, 512, 64, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 64MiB 3,3V 16-bit",	0x56, 512, 64, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 128MiB 1,8V 8-bit",	0x78, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 8-bit",	0x39, 512, 128, 0x4000, 0},
	{"NAND 128MiB 3,3V 8-bit",	0x79, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 16-bit",	0x72, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 1,8V 16-bit",	0x49, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x74, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x59, 512, 128, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 256MiB 3,3V 8-bit",	0x71, 512, 256, 0x4000, 0},

	/*
	 * These are the new chips with large page size. The pagesize and the
	 * erasesize is determined from the extended id bytes
	 */
#define LP_OPTIONS (NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR)
#define LP_OPTIONS16 (LP_OPTIONS | NAND_BUSWIDTH_16)

	/*512 Megabit */
	{"NAND 64MiB 1,8V 8-bit",	0xA2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xF2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 16-bit",	0xB2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC2, 0,  64, 0, LP_OPTIONS16},

	/* 1 Gigabit */
	{"NAND 128MiB 1,8V 8-bit",	0xA1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xF1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xD1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 1,8V 16-bit",	0xB1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 3,3V 16-bit",	0xC1, 0, 128, 0, LP_OPTIONS16},

	/* 2 Gigabit */
	{"NAND 256MiB 1,8V 8-bit",	0xAA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 3,3V 8-bit",	0xDA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 1,8V 16-bit",	0xBA, 0, 256, 0, LP_OPTIONS16},
	{"NAND 256MiB 3,3V 16-bit",	0xCA, 0, 256, 0, LP_OPTIONS16},

	/* 4 Gigabit */
	{"NAND 512MiB 1,8V 8-bit",	0xAC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 3,3V 8-bit",	0xDC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 1,8V 16-bit",	0xBC, 0, 512, 0, LP_OPTIONS16},
	{"NAND 512MiB 3,3V 16-bit",	0xCC, 0, 512, 0, LP_OPTIONS16},

	/* 8 Gigabit */
	{"NAND 1GiB 1,8V 8-bit",	0xA3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 3,3V 8-bit",	0xD3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 1,8V 16-bit",	0xB3, 0, 1024, 0, LP_OPTIONS16},
	{"NAND 1GiB 3,3V 16-bit",	0xC3, 0, 1024, 0, LP_OPTIONS16},

	/* 16 Gigabit */
	{"NAND 2GiB 1,8V 8-bit",	0xA5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 3,3V 8-bit",	0xD5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 1,8V 16-bit",	0xB5, 0, 2048, 0, LP_OPTIONS16},
	{"NAND 2GiB 3,3V 16-bit",	0xC5, 0, 2048, 0, LP_OPTIONS16},

	/*
	 * Renesas AND 1 Gigabit. Those chips do not support extended id and
	 * have a strange page/block layout !  The chosen minimum erasesize is
	 * 4 * 2 * 2048 = 16384 Byte, as those chips have an array of 4 page
	 * planes 1 block = 2 pages, but due to plane arrangement the blocks
	 * 0-3 consists of page 0 + 4,1 + 5, 2 + 6, 3 + 7 Anyway JFFS2 would
	 * increase the eraseblock size so we chose a combined one which can be
	 * erased in one go There are more speed improvements for reads and
	 * writes possible, but not implemented now
	 */
	{"AND 128MiB 3,3V 8-bit",	0x01, 2048, 128, 0x4000,
	 NAND_IS_AND | NAND_NO_AUTOINCR |NAND_NO_READRDY | NAND_4PAGE_ARRAY |
	 BBT_AUTO_REFRESH
	},

	{NULL,}
};
#endif

#define NAND_REG_STATUS 0
#define NAND_REG_STATUS_IDLE (1<<10)
#define NAND_REG_CONTROL 0x04
#define NAND_REG_COMMAND 0x08
#define NAND_REG_DATA 0x0C

#define NAND_REG_DATA_CHANNEL_CONFIG 0x1C
#define NAND_REG_ARLEN 0x24
#define NAND_REG_RENDIAN 0x38

#define NAND_REG_AWLEN 0x6C
#define NAND_REG_WENDIAN 0x74

#define NAND_REG_DMA_DESC_1_W 0x88
#define NAND_REG_DMA_DESC_2_W 0x8C
#define NAND_REG_DMA_DESC_3_W 0x90

#define NAND_REG_DMA_CONF_1_W 0x94

#define NAND_REG_DMA_BUF_END 0xA4
#define NAND_REG_IRQ_STATUS 0xB4

#define NAND_REG_TIMING_0 0xB8
#define NAND_REG_TIMING_1 0xBC
#define NAND_REG_TIMING_2 0xC0
#define NAND_REG_TIMING_3 0xC4
#define NAND_REG_TIMING_4 0xC8
#define NAND_REG_TIMING_5 0xCC
#define NAND_REG_TIMING_6 0xD0
#define NAND_REG_TIMING_7 0xD4

#define CONFIGH_BASE	(CONFIG_OTP_SHADOW_BASE+0x84)

static inline unsigned int read_configh(void) 
{
    return *(volatile unsigned int*)(CONFIGH_BASE);
}

static inline void write_configh(unsigned int v) 
{
    *(volatile unsigned int*)(CONFIGH_BASE) = v;
}

#define MNAND_NAME "mnand"
#ifndef CONFIG_MNAND_DEFAULT_TYPE
#define CONFIG_MNAND_DEFAULT_TYPE {"NAND 128MiB 3,3V 8-bit", 0xF1, 0, 128, 0, \
	NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR}
#endif

#define ERR(x, args...) \
	printf("%s: " x "\n", __func__, ## args)

#define INFO(x, args...) \
	printf("%s: " x "\n", __func__, ## args)

#define as32(d) ((unsigned long)((d)&0xffffffffULL))

int g_mnand_debug = 0;

int g_mnand_ignore_ecc_in_spare = 0;

int g_mnand_check_for_empty_spare = 1;

int g_mnand_calculate_ecc = 1;

#define MARK() \
	do{ if(g_mnand_debug) printf("%s \n", __func__); } while(0)
#define TRACE(format, args...) \
	do{ if(g_mnand_debug) printf("%s():%d: " format "\n", __func__, __LINE__, ## args); } while(0)

#define writel(b, addr) (void)((*(volatile unsigned int *) (addr)) = (b))
#define readl(addr) (*(volatile unsigned int*)(addr))

#define iowrite32(val, x) writel(val, x)
#define ioread32(x) readl(x)

static inline int is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

// Those two functions are backports from newer kernel
inline int mnand_erasesize_shift(struct mtd_info *mtd) 
{
	if (is_power_of_2(mtd->erasesize))
		return ffs(mtd->erasesize) - 1;
	else
		return 0;
}

inline int mnand_writesize_shift(struct mtd_info *mtd) 
{
	if (is_power_of_2(mtd->writesize))
		return ffs(mtd->writesize) - 1;
	else
		return 0;
}

enum {
	MNAND_IDLE = 0,
	MNAND_WRITE,
	MNAND_READ,
	MNAND_ERASE,
	MNAND_READID,
	MNAND_RESET,
	MNAND_ERROR
};

struct mnand_chip {

    void* io;

    uint32_t state;

    dma_addr_t dma_handle;
    size_t dma_size;
    void* dma_area;

    uint64_t active_page;
    struct mtd_info *mtd;

    uint32_t status1;
    uint32_t status2;

    uint32_t ecc_corrected;
    uint32_t ecc_failed;
};

static struct mnand_chip g_chip;

#ifdef CONFIG_MNAND_DEBUG_HIST
struct write_info {
    uint32_t address;
    uint32_t value;
};

struct write_info g_write_buffer[1024];
int g_write_pos = 0;

struct page_log {
    uint32_t address;
    uint8_t data[2048+64];
};

struct page_log g_write_page_log[100];
int g_write_page_pos = 0;

struct page_log g_read_page_log[100];
int g_read_page_pos = 0;
#endif /* CONFIG_MNAND_DEBUG_HIST */

#define MNAND_STATUS_ECC_ERROR 0x100

#define MNAND_ECC_BLOCKSIZE 256
#define MNAND_ECC_BYTESPERBLOCK 3
#define MNAND_DMA_SIZE 4096

static struct nand_ecclayout g_ecclayout = {
    .eccbytes = 24,
    .eccpos = {
           1, 2, 3, 4, 5, 6, 7, 8,
           9, 10, 11, 12, 13, 14, 15, 16,
           17, 18, 19, 20, 21, 22, 23, 24},
    .oobfree = {
        {.offset = 25,
         .length = 39}},
    .oobavail = 39
};

void mnand_set(uint32_t addr, uint32_t val) 
{
#ifdef CONFIG_MNAND_DEBUG_HIST
    struct write_info wi = { addr, val };
    
    if(g_write_pos == 1024)
        g_write_pos = 0;

    g_write_buffer[g_write_pos++] = wi;
#endif

    TRACE( "nand(0x%08X) <= 0x%08X", (uint32_t)(g_chip.io+addr), val);
    iowrite32(val, g_chip.io + addr);
}

uint32_t mnand_get(uint32_t addr)
{
    uint32_t r = ioread32(g_chip.io + addr);
    TRACE( "nand(0x%08X) => 0x%08X", (uint32_t)(g_chip.io+addr), r);
    return r;
} 

void mnand_reg_dump(void)
{
}

static inline unsigned char* mnand_get_spare_ecc(size_t i) 
{
    uint32_t ecc_offset = g_chip.mtd->writesize + 
		g_ecclayout.eccpos[MNAND_ECC_BYTESPERBLOCK * i];
	TRACE("ECC block %d offset is 0x%X", i, ecc_offset);
	return g_chip.dma_area + ecc_offset;
}

static void mnand_get_hardware_ecc(size_t i, unsigned char *ecc) 
{
    int data;
    unsigned char* cdata = (unsigned char*)&data;
    data = mnand_get(0xD8+i*sizeof(uint32_t));
	ecc[0] = cdata[2];
	ecc[1] = cdata[1];
	ecc[2] = cdata[0];
}

void mnand_reset_grabber(void) 
{
	//FIXME:
/*    mnand_set(0x1008,0x8);*/
/*    mnand_set(0x100C, 1);*/
/*    mnand_set(0x100C, 0);*/

/*    mnand_set(0x2008, 0xffffffff);*/
/*    mnand_set(0x200C, 1);*/
/*    mnand_set(0x200C, 0);*/

/*    mnand_set(0x3008, 0xffffffff);*/
/*    mnand_set(0x300C, 1);*/
/*    mnand_set(0x300C, 0);*/

/*    mnand_set(0x400C, 1);*/
/*    mnand_set(0x400C, 0);*/

/*    mnand_set(0x500C, 1);*/
/*    mnand_set(0x500C, 0);*/

/*    mnand_set(0x800C, 1);*/
/*    mnand_set(0x800C, 0);*/
/*    mnand_set(0x8008, 0xffffffff);*/
}

static int mnand_poll_read(void) 
{
	ulong t_start;

	MARK();
	if(! (g_chip.state==MNAND_READ || g_chip.state==MNAND_READID)) {
		BUG();
		return -EINVAL;
	}

	t_start = get_timer(0);
	while(g_chip.state != MNAND_IDLE) {

		if(get_timer(t_start) > CONFIG_MNAND_TIMEOUT_MS) {
			uint32_t old_state = g_chip.state;
			g_chip.state = MNAND_ERROR;
			ERR("timeout: state:%d, status1:0x%08X, status2:0x%08X", 
				old_state, g_chip.status1, g_chip.status2);
			return -EIO;
		}

		g_chip.status1 = mnand_get(0x10);
		g_chip.status2 = mnand_get(0xb4);

		/* Last AXI transaction flag */
		if(g_chip.status2 & 0x100) {
			g_chip.state = MNAND_IDLE;
		}
	}

	return 0;
}

static int mnand_poll(void) 
{
	ulong t_start;
	int status;

	MARK();

	status = 0;
	t_start = get_timer(0);
	while(g_chip.state != MNAND_IDLE) {

		if(get_timer(t_start) > CONFIG_MNAND_TIMEOUT_MS) {
			uint32_t old_state = g_chip.state;
			g_chip.state = MNAND_ERROR;
			ERR("timeout: state:%d, ready:%08X, status1:0x%08X, status2:0x%08X", 
				old_state, status, g_chip.status1, g_chip.status2);
			return -EIO;
		}

		status = mnand_get(NAND_REG_STATUS);
		if(! (status & NAND_REG_STATUS_IDLE) )
			continue;

		g_chip.status1 = mnand_get(0x10);
		g_chip.status2 = mnand_get(0xb4);

		switch(g_chip.state) {
			case MNAND_WRITE:
				if((g_chip.status1 & 0x2) || (g_chip.status2 & 0x2)) {
					g_chip.state = MNAND_IDLE;
				}
				break;
			case MNAND_ERASE:
				if((g_chip.status1 & 0x1) || (g_chip.status2 & 0x1) ) {
					g_chip.state = MNAND_IDLE;
				}
				break;
			case MNAND_RESET:
				if((g_chip.status1 & 0x20) || (g_chip.status2 & 0x20)) {
					g_chip.state = MNAND_IDLE;
				}
				break;
			default:
				return -EINVAL;
		}
	}

	return 0;
}

void mnand_hw_init(void) 
{
	int val;
	// check if we can write CONFIGH
	if((read_configh() & (1 << 23)) == 0)  {
		// enabling NAND pads
		write_configh(read_configh()|(1<<6));
	}

	// initializing timings for operations with nand flash
#ifdef CONFIG_MTD_MNAND_SLOW
	mnand_set(NAND_REG_TIMING_0, 0x02050211); // tADL=17, tALH=2, tALS=5, tCH=2
	mnand_set(NAND_REG_TIMING_1, 0x02060502); // tCLH=2, tCLS=5, tCS=6, tDH=2
	mnand_set(NAND_REG_TIMING_2, 0x05030904); // tWP=5, tWH=3, tWC=9, tDS=4
	mnand_set(NAND_REG_TIMING_3, 0x01020309); // tOH=1, tCLR=2, tAR=3, tWW=9
	mnand_set(NAND_REG_TIMING_4, 0x03090101); // tREH=3, tRC=9, tIR=1, tCOH=1
	mnand_set(NAND_REG_TIMING_5, 0x05011101); // tRP=5, tRLOH=1, tRHW=17, tRHOH=1
	mnand_set(NAND_REG_TIMING_6, 0x09090A04); // tCHZ=9, tCEA=9, tWHR=10, tRR=4
	mnand_set(NAND_REG_TIMING_7, 0x00111104); // tWB=17, tRHZ=17, tREA=4
#else
	mnand_set(NAND_REG_TIMING_0, 0x01010106);
	mnand_set(NAND_REG_TIMING_1, 0x01010201);
	mnand_set(NAND_REG_TIMING_2, 0x02010301);
	mnand_set(NAND_REG_TIMING_3, 0x03010102);
	mnand_set(NAND_REG_TIMING_4, 0x01030102);
	mnand_set(NAND_REG_TIMING_5, 0x02010902);
	mnand_set(NAND_REG_TIMING_6, 0x04040602);
	mnand_set(NAND_REG_TIMING_7, 0x000A0A03);
#endif
	// setting default values for nand_controller configuration registers to 
	// perform READ ID operation
	val = (1 << 25) | ((2048+64) << 11) | (1 << 10);
	val |= g_mnand_calculate_ecc ? (1<<7) : 0;
	mnand_set(NAND_REG_CONTROL, val);    

	// setting default values for AXI MASTER for read from system memory
	mnand_set(NAND_REG_DATA_CHANNEL_CONFIG, 0xC0);
	mnand_set(NAND_REG_ARLEN, 0xF);
	mnand_set(NAND_REG_RENDIAN, 1);

	// setting default values for AXI MASTER for write to system memory
	mnand_set(NAND_REG_AWLEN, 0xF);
	mnand_set(NAND_REG_WENDIAN, 1);
	mnand_set(NAND_REG_DMA_CONF_1_W, 0x30);

	// IRQ mask
	mnand_set(0x2C, 0);
}

static void mnand_prepare_dma_read(size_t bytes)
{
    TRACE( "bytes %d", bytes);
	if(g_mnand_debug)
		print_buffer(g_chip.dma_handle + 0x1f0, (void*)(g_chip.dma_handle + 0x1f0), 4, 8, 0);

    mnand_set(0x88, g_chip.dma_handle);
    mnand_set(0x8C, g_chip.dma_handle + bytes - 1);
    mnand_set(0x90, 0x80004000);

    while(mnand_get(0x90) & 0x80000000);
}

static void mnand_prepare_dma_write(size_t bytes)
{
    TRACE("bytes=%d", bytes);
	if(g_mnand_debug)
		print_buffer(g_chip.dma_handle + 0x1f0, (void*)(g_chip.dma_handle + 0x1f0), 4, 8, 0);

    mnand_set(0x40, g_chip.dma_handle);
    mnand_set(0x44, g_chip.dma_handle + bytes - 1);
    mnand_set(0x48, 0x80001000);
    mnand_set(0x4C, 0x2000);

    while(mnand_get(0x48) & 0x80000000);

    mnand_set(0x18, 0x40000001);
}

int mnand_ready(void)
{
    return (mnand_get(0) & 0x200) != 0;
}

static int mnand_core_reset(void)
{
	int ret = 0;

    g_chip.state = MNAND_RESET;

    mnand_set(0x08, 0x2C);

	ret = mnand_poll();
	if(ret != 0)
		return ret;

    BUG_ON(!mnand_ready());

    g_chip.state = MNAND_IDLE;
    
    return 0; 
}

static int mnand_core_erase(loff_t off) 
{
	int ret;
	MARK();

	if(g_chip.state != MNAND_IDLE) {
		ERR("flash is in error state");
		return -EINVAL;
	}
	BUG_ON(!mnand_ready());

    mnand_reset_grabber();

    g_chip.state = MNAND_ERASE;

    mnand_set(0x8, 0x2b);
    mnand_set(0xc, (off >> mnand_erasesize_shift(g_chip.mtd)) << 18);
    
	ret = mnand_poll();
	if(ret < 0) 
		return ret;

    BUG_ON(!mnand_ready());

    g_chip.state = MNAND_IDLE;

    return (mnand_get(0) & NAND_STATUS_FAIL) ? -EIO : 0;
}

static int mnand_core_read_id(size_t bytes) 
{
	int ret;

	MARK();

	if(g_chip.state != MNAND_IDLE) {
		ERR("flash is in error state");
		return -EINVAL;
	}

    mnand_reset_grabber();

    BUG_ON(!mnand_ready());  
    
    mnand_prepare_dma_read(bytes);    

    g_chip.state = MNAND_READID;

    mnand_set(0x08, 0x25);
    mnand_set(0x0C, bytes << 8);

	ret = mnand_poll_read();
	if(ret != 0)
		return ret;

    BUG_ON(!mnand_ready());
    g_chip.state = MNAND_IDLE;
	return 0;
}

/*{{{ ECC*/
/*
 * invparity is a 256 byte table that contains the odd parity
 * for each byte. So if the number of bits in a byte is even,
 * the array element is 1, and when the number of bits is odd
 * the array eleemnt is 0.
 */
static const char invparity[256] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};



static const char addressbits[256] = {
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
	0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
	0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
	0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f
};

static const char bitsperbyte[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

static int mnand_correct_data(unsigned char *buf, unsigned char *read_ecc, unsigned char *calc_ecc)
{
	unsigned char b0, b1, b2;
	unsigned char byte_addr, bit_addr;
	/* 256 or 512 bytes/ecc  */
	const uint32_t eccsize_mult = 1;

	/*
	 * b0 to b2 indicate which bit is faulty (if any)
	 * we might need the xor result  more than once,
	 * so keep them in a local var
	*/
#ifdef CONFIG_MTD_NAND_ECC_SMC
	b0 = read_ecc[0] ^ calc_ecc[0];
	b1 = read_ecc[1] ^ calc_ecc[1];
#else
	b0 = read_ecc[1] ^ calc_ecc[1];
	b1 = read_ecc[0] ^ calc_ecc[0];
#endif
	b2 = read_ecc[2] ^ calc_ecc[2];

	/* check if there are any bitfaults */

	/* repeated if statements are slightly more efficient than switch ... */
	/* ordered in order of likelihood */

	if ((b0 | b1 | b2) == 0)
		return 0;	/* no error */

	if ((((b0 ^ (b0 >> 1)) & 0x55) == 0x55) &&
	    (((b1 ^ (b1 >> 1)) & 0x55) == 0x55) &&
	    ((eccsize_mult == 1 && ((b2 ^ (b2 >> 1)) & 0x54) == 0x54) ||
	     (eccsize_mult == 2 && ((b2 ^ (b2 >> 1)) & 0x55) == 0x55))) {
	/* single bit error */
		/*
		 * rp17/rp15/13/11/9/7/5/3/1 indicate which byte is the faulty
		 * byte, cp 5/3/1 indicate the faulty bit.
		 * A lookup table (called addressbits) is used to filter
		 * the bits from the byte they are in.
		 * A marginal optimisation is possible by having three
		 * different lookup tables.
		 * One as we have now (for b0), one for b2
		 * (that would avoid the >> 1), and one for b1 (with all values
		 * << 4). However it was felt that introducing two more tables
		 * hardly justify the gain.
		 *
		 * The b2 shift is there to get rid of the lowest two bits.
		 * We could also do addressbits[b2] >> 1 but for the
		 * performace it does not make any difference
		 */
		if (eccsize_mult == 1)
			byte_addr = (addressbits[b1] << 4) + addressbits[b0];
		else
			byte_addr = (addressbits[b2 & 0x3] << 8) +
				    (addressbits[b1] << 4) + addressbits[b0];
		bit_addr = addressbits[b2 >> 2];
		/* flip the bit */
		buf[byte_addr] ^= (1 << bit_addr);
		return 1;

	}
	/* count nr of bits; use table lookup, faster than calculating it */
	if ((bitsperbyte[b0] + bitsperbyte[b1] + bitsperbyte[b2]) == 1)
		return 1;	/* error in ecc data; no action needed */

	return -1;
}

int mnand_calculate_ecc(const unsigned char *buf, unsigned char *code)
{
	int i;
	const uint32_t *bp = (uint32_t *)buf;
	/* 256 or 512 bytes/ecc  */
	const uint32_t eccsize_mult = 1;
	
    uint32_t cur;		/* current value in buffer */
	/* rp0..rp15..rp17 are the various accumulated parities (per byte) */
	uint32_t rp0, rp1, rp2, rp3, rp4, rp5, rp6, rp7;
	uint32_t rp8, rp9, rp10, rp11, rp12, rp13, rp14, rp15, rp16;
	uint32_t uninitialized_var(rp17);	/* to make compiler happy */
	uint32_t par;		/* the cumulative parity for all data */
	uint32_t tmppar;	/* the cumulative parity for this iteration;
				   for rp12, rp14 and rp16 at the end of the
				   loop */

	par = 0;
	rp4 = 0;
	rp6 = 0;
	rp8 = 0;
	rp10 = 0;
	rp12 = 0;
	rp14 = 0;
	rp16 = 0;

	/*
	 * The loop is unrolled a number of times;
	 * This avoids if statements to decide on which rp value to update
	 * Also we process the data by longwords.
	 * Note: passing unaligned data might give a performance penalty.
	 * It is assumed that the buffers are aligned.
	 * tmppar is the cumulative sum of this iteration.
	 * needed for calculating rp12, rp14, rp16 and par
	 * also used as a performance improvement for rp6, rp8 and rp10
	 */
	for (i = 0; i < eccsize_mult << 2; i++) {
		cur = *bp++;
		tmppar = cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= tmppar;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp8 ^= tmppar;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp10 ^= tmppar;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp8 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp8 ^= cur;

		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp6 ^= cur;
		cur = *bp++;
		tmppar ^= cur;
		rp4 ^= cur;
		cur = *bp++;
		tmppar ^= cur;

		par ^= tmppar;
		if ((i & 0x1) == 0)
			rp12 ^= tmppar;
		if ((i & 0x2) == 0)
			rp14 ^= tmppar;
		if (eccsize_mult == 2 && (i & 0x4) == 0)
			rp16 ^= tmppar;
	}

	/*
	 * handle the fact that we use longword operations
	 * we'll bring rp4..rp14..rp16 back to single byte entities by
	 * shifting and xoring first fold the upper and lower 16 bits,
	 * then the upper and lower 8 bits.
	 */
	rp4 ^= (rp4 >> 16);
	rp4 ^= (rp4 >> 8);
	rp4 &= 0xff;
	rp6 ^= (rp6 >> 16);
	rp6 ^= (rp6 >> 8);
	rp6 &= 0xff;
	rp8 ^= (rp8 >> 16);
	rp8 ^= (rp8 >> 8);
	rp8 &= 0xff;
	rp10 ^= (rp10 >> 16);
	rp10 ^= (rp10 >> 8);
	rp10 &= 0xff;
	rp12 ^= (rp12 >> 16);
	rp12 ^= (rp12 >> 8);
	rp12 &= 0xff;
	rp14 ^= (rp14 >> 16);
	rp14 ^= (rp14 >> 8);
	rp14 &= 0xff;
	if (eccsize_mult == 2) {
		rp16 ^= (rp16 >> 16);
		rp16 ^= (rp16 >> 8);
		rp16 &= 0xff;
	}

	/*
	 * we also need to calculate the row parity for rp0..rp3
	 * This is present in par, because par is now
	 * rp3 rp3 rp2 rp2 in little endian and
	 * rp2 rp2 rp3 rp3 in big endian
	 * as well as
	 * rp1 rp0 rp1 rp0 in little endian and
	 * rp0 rp1 rp0 rp1 in big endian
	 * First calculate rp2 and rp3
	 */
#ifdef __BIG_ENDIAN
	rp2 = (par >> 16);
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
	rp3 = par & 0xffff;
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
#else
	rp3 = (par >> 16);
	rp3 ^= (rp3 >> 8);
	rp3 &= 0xff;
	rp2 = par & 0xffff;
	rp2 ^= (rp2 >> 8);
	rp2 &= 0xff;
#endif

	/* reduce par to 16 bits then calculate rp1 and rp0 */
	par ^= (par >> 16);
#ifdef __BIG_ENDIAN
	rp0 = (par >> 8) & 0xff;
	rp1 = (par & 0xff);
#else
	rp1 = (par >> 8) & 0xff;
	rp0 = (par & 0xff);
#endif

	/* finally reduce par to 8 bits */
	par ^= (par >> 8);
	par &= 0xff;

	/*
	 * and calculate rp5..rp15..rp17
	 * note that par = rp4 ^ rp5 and due to the commutative property
	 * of the ^ operator we can say:
	 * rp5 = (par ^ rp4);
	 * The & 0xff seems superfluous, but benchmarking learned that
	 * leaving it out gives slightly worse results. No idea why, probably
	 * it has to do with the way the pipeline in pentium is organized.
	 */
	rp5 = (par ^ rp4) & 0xff;
	rp7 = (par ^ rp6) & 0xff;
	rp9 = (par ^ rp8) & 0xff;
	rp11 = (par ^ rp10) & 0xff;
	rp13 = (par ^ rp12) & 0xff;
	rp15 = (par ^ rp14) & 0xff;
	if (eccsize_mult == 2)
		rp17 = (par ^ rp16) & 0xff;

	/*
	 * Finally calculate the ecc bits.
	 * Again here it might seem that there are performance optimisations
	 * possible, but benchmarks showed that on the system this is developed
	 * the code below is the fastest
	 */
// FIXME: What is SMC
#ifdef CONFIG_MTD_NAND_ECC_SMC
	code[0] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[1] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#else
	code[1] =
	    (invparity[rp7] << 7) |
	    (invparity[rp6] << 6) |
	    (invparity[rp5] << 5) |
	    (invparity[rp4] << 4) |
	    (invparity[rp3] << 3) |
	    (invparity[rp2] << 2) |
	    (invparity[rp1] << 1) |
	    (invparity[rp0]);
	code[0] =
	    (invparity[rp15] << 7) |
	    (invparity[rp14] << 6) |
	    (invparity[rp13] << 5) |
	    (invparity[rp12] << 4) |
	    (invparity[rp11] << 3) |
	    (invparity[rp10] << 2) |
	    (invparity[rp9] << 1)  |
	    (invparity[rp8]);
#endif
	if (eccsize_mult == 1)
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    3;
	else
		code[2] =
		    (invparity[par & 0xf0] << 7) |
		    (invparity[par & 0x0f] << 6) |
		    (invparity[par & 0xcc] << 5) |
		    (invparity[par & 0x33] << 4) |
		    (invparity[par & 0xaa] << 3) |
		    (invparity[par & 0x55] << 2) |
		    (invparity[rp17] << 1) |
		    (invparity[rp16] << 0);
	return 0;
}
/*}}}*/

#define FORMAT_ECC(ecc) (ecc)[0], (ecc)[1], (ecc)[2]

int ecc_is_empty(unsigned char ecc[MNAND_ECC_BYTESPERBLOCK])
{
	int j;
	for(j=0;j<MNAND_ECC_BYTESPERBLOCK;j++) { 
		if(ecc[j]!=0xFF) return 0; 
	}
	return 1;
}

static int mnand_core_read(loff_t off)
{
	loff_t page = off & (~(g_chip.mtd->writesize-1));
	size_t corrected = 0;
	size_t failed = 0;
	int ret;

    if(g_chip.state != MNAND_IDLE) {
		ERR("flash is in error state");
		return -EINVAL;
	}
	BUG_ON(!mnand_ready());

	mnand_reset_grabber();

	if(g_chip.active_page != page) 
	{
		g_chip.active_page = -1;

		mnand_prepare_dma_read(g_chip.mtd->writesize + g_chip.mtd->oobsize);

		g_chip.state = MNAND_READ;

		mnand_set(0x08, 0x20);
		mnand_set(0x0C, (off >> mnand_writesize_shift(g_chip.mtd)) << 12);

		ret = mnand_poll_read();
		if(ret != 0)
			return ret;

		BUG_ON(!mnand_ready());
		g_chip.state = MNAND_IDLE;

#ifdef CONFIG_MNAND_DEBUG_HIST
		g_read_page_log[g_read_page_pos].address = page;
		memcpy(g_read_page_log[g_read_page_pos].data, g_chip.dma_area, 2048+64);

		++g_read_page_pos;

		if(g_read_page_pos == sizeof(g_read_page_log) / sizeof(g_read_page_log[0]))
			g_read_page_pos = 0;
#endif

		if(mnand_get(0) & NAND_STATUS_FAIL) {
			BUG_ON(1);
			return -EIO;
		}

		cache_flush();

		if(g_mnand_debug)
			print_buffer(g_chip.dma_handle + 0x1f0, (void*)(g_chip.dma_handle + 0x1f0), 4, 8, 0);

		size_t i;

		/*Check if we had a bad block*/
		if(*((char*)(g_chip.dma_area + g_chip.mtd->writesize)) == 0xFF) {

			for(i=0; i<g_chip.mtd->writesize/MNAND_ECC_BLOCKSIZE; ++i) {

				unsigned char ecc_software[MNAND_ECC_BYTESPERBLOCK];
				unsigned char ecc_hardware[MNAND_ECC_BYTESPERBLOCK];
				unsigned char *ecc_from_spare;
				unsigned char *ecc_block = g_chip.dma_area + i*MNAND_ECC_BLOCKSIZE;

				mnand_calculate_ecc(ecc_block, &ecc_software[0]);

				mnand_get_hardware_ecc(i, &ecc_hardware[0]);
				TRACE("ECC block %d", i);
				TRACE("ECC software: %02X %02X %02X", FORMAT_ECC(ecc_software));
				TRACE("ECC hardware: %02X %02X %02X", FORMAT_ECC(ecc_hardware));

				if(0!=memcmp(ecc_hardware,ecc_software,MNAND_ECC_BYTESPERBLOCK)) {
					ERR("ECC mismatch at block %d. Soft: %02X %02X %02X Hard: %02X %02X %02X", 
						i, FORMAT_ECC(ecc_software), FORMAT_ECC(ecc_hardware));
				}

				if(g_mnand_ignore_ecc_in_spare)
					continue;

				ecc_from_spare = mnand_get_spare_ecc(i);
				TRACE("ECC from spare: %02X %02X %02X", FORMAT_ECC(ecc_from_spare));

				if(g_mnand_check_for_empty_spare && ecc_is_empty(ecc_from_spare)) {
					TRACE("Spare ECC block %d is empty", i);
					continue;
				}

				if(0!=memcmp(ecc_from_spare, ecc_hardware, MNAND_ECC_BYTESPERBLOCK)) {

					if( mnand_correct_data(ecc_block, ecc_from_spare, ecc_hardware) < 0 ) {
						ERR("Data loss detected: off 0x%08llX ecc_block %d ecc_spare %02X%02X%02X ecc_sw %02X%02X%02X", 
							off, i, FORMAT_ECC(ecc_from_spare), FORMAT_ECC(ecc_hardware));
						++failed;
					}
					else {
						++corrected;
					}
				}
			}
		}

		g_chip.ecc_corrected += corrected;
		g_chip.ecc_failed += failed;

		/*if(!failed) 
		  g_chip.active_page = page;*/
	}

	return 0;
}

static int mnand_core_write(loff_t off) 
{
	int ret;
    if(g_chip.state != MNAND_IDLE) {
		ERR("flash is in error state");
		return -EINVAL;
	}
    BUG_ON(!mnand_ready());

    mnand_reset_grabber();

    g_chip.active_page = -1;

	cache_flush();

    mnand_prepare_dma_write(g_chip.mtd->writesize + g_chip.mtd->oobsize);

    g_chip.state = MNAND_WRITE;

#ifdef CONFIG_MNAND_DEBUG_HIST
    g_write_page_log[g_write_page_pos].address = off;
    memcpy(g_write_page_log[g_write_page_pos].data, g_chip.dma_area, 2048+64);

    ++g_write_page_pos;

    if((g_write_page_pos) == sizeof(g_write_page_log)/sizeof(g_write_page_log[0]))
        g_write_page_pos = 0;
#endif

    mnand_set(0x08, 0x27);
    mnand_set(0x0C, (off >> mnand_writesize_shift(g_chip.mtd)) << 12);
    BUG_ON(mnand_get(0x0C) != (off >> mnand_writesize_shift(g_chip.mtd)) << 12);
    
	ret = mnand_poll();
	if(ret<0)
		return ret;

    BUG_ON(!mnand_ready());
    g_chip.state = MNAND_IDLE;
    
    return (mnand_get(0) & NAND_STATUS_FAIL) ? -EIO : 0;
}

static int mnand_read_id(struct mtd_info *mtd) 
{
	int ret;
	uint8_t id;
	uint8_t extid;
	struct nand_flash_dev def_type = CONFIG_MNAND_DEFAULT_TYPE;
	struct nand_flash_dev* type = &def_type;

    if(g_chip.state != MNAND_IDLE) {
		ERR("flash is in error state");
		return -EINVAL;
	}

	ret = mnand_core_read_id(2);
	if(ret<0)
		return ret;

	id = *(((uint8_t*)g_chip.dma_area)+1);

	INFO("flash id 0x%02X", id); 

	for(type=&nand_flash_ids[0]; type->name!=0; type++) {
		if(type->id == id) {
			break;
		}
	}

	if(type->name == NULL) {
		type = &def_type;
		ERR("WARNING: Unknown flash ID. Using default (0x%02X)", type->id);
	}

	mtd->size = (uint64_t)type->chipsize << 20;

	ret = mnand_core_read_id(5);
	if(ret <0)
		return ret;

	extid = ((uint8_t*)g_chip.dma_area)[3];
	INFO( "flash ext_id 0x%02X", extid);

	mtd->writesize = 1024 << (extid & 0x3);
	extid >>= 2;

	mtd->oobsize = (8 << (extid & 0x01)) * (mtd->writesize >> 9);
	extid >>= 2;

	mtd->erasesize = (64 * 1024) << (extid & 0x03);

	INFO("%s size(%lu) writesize(%u) oobsize(%u) erasesize(%u)",
		type->name, type->chipsize, mtd->writesize, mtd->oobsize, mtd->erasesize);

	if(mtd->erasesize != 0x20000 || mtd->writesize != 2048) {
		ERR("WARNING: unsupported flash. This driver supports writesize 2048 erasesize 128K only");
	}

    return 0;
}


static int mnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int err;

	MARK();

    TRACE( "addr=0x%08llX, len=%lld", instr->addr, instr->len);
    
    if(instr->addr >= g_chip.mtd->size ||
        instr->addr + instr->len > g_chip.mtd->size  ||
        instr->len != g_chip.mtd->erasesize)
            return -EINVAL;

    err = mnand_core_erase(instr->addr);

    if(err) {
        printk(KERN_ERR "erase failed with %d at 0x%08llx\n", err, instr->addr);
        instr->state = MTD_ERASE_FAILED;
        instr->fail_addr = instr->addr;    
    }
    else {
        instr->state = MTD_ERASE_DONE;
    }
    
    /*mtd_erase_callback(instr);*/
   
    return 0;
}

#define PAGE_ALIGNED(x) (((x) & (g_chip.mtd->writesize-1)) ==0)

static uint8_t* mnand_fill_oob(uint8_t *oob, struct mtd_oob_ops *ops)
{
    size_t len = ops->ooblen;
        
    switch(ops->mode) {    
    case MTD_OOB_PLACE:
    case MTD_OOB_RAW:
        memcpy(g_chip.dma_area + g_chip.mtd->writesize + ops->ooboffs, oob, len);
        return oob + len;    
    case MTD_OOB_AUTO: {
        struct nand_oobfree *free = g_chip.mtd->ecclayout->oobfree;
        uint32_t boffs = 0, woffs = ops->ooboffs;
        size_t bytes = 0;
    
        for(; free->length && len; free++, len -= bytes) {
            /* Write request not from offset 0 ? */
            if (unlikely(woffs)) {
                if (woffs >= free->length) {
                    woffs -= free->length;
                    continue;
                }
                boffs = free->offset + woffs;
                bytes = min_t(size_t, len, (free->length - woffs));
                woffs = 0;
            } else {
                bytes = min_t(size_t, len, free->length);
                boffs = free->offset;
            }
            memcpy(g_chip.dma_area + g_chip.mtd->writesize + boffs, oob, bytes);
            oob += bytes;
        }
        return oob;
    }
    default:
        BUG();
    }
    return NULL;
}

static uint8_t* mnand_transfer_oob(uint8_t *oob, struct mtd_oob_ops *ops, size_t len)
{
    switch(ops->mode) {

    case MTD_OOB_PLACE:
    case MTD_OOB_RAW:
        TRACE( "raw transfer");
        memcpy(oob, g_chip.dma_area + g_chip.mtd->writesize + ops->ooboffs, len);
        return oob + len;

    case MTD_OOB_AUTO: {
        struct nand_oobfree *free = g_chip.mtd->ecclayout->oobfree;
        uint32_t boffs = 0, roffs = ops->ooboffs;
        size_t bytes = 0;
    
        for(; free->length && len; free++, len -= bytes) {
            /* Read request not from offset 0 ? */
            if (unlikely(roffs)) {
                if (roffs >= free->length) {
                    roffs -= free->length;
                    continue;
                }
                boffs = free->offset + roffs;
                bytes = min_t(size_t, len,
                          (free->length - roffs));
                roffs = 0;
            } else {
                bytes = min_t(size_t, len, free->length);
                boffs = free->offset;
            }
            memcpy(oob, g_chip.dma_area + g_chip.mtd->writesize + boffs, bytes);
            oob += bytes;
        }
        return oob;
    }
    default:
        BUG();
    }
    return NULL;
}

static int mnand_write_oob(struct mtd_info* mtd, loff_t to, struct mtd_oob_ops* ops) 
{
    uint8_t* data = ops->datbuf;
    uint8_t* dataend = data ? data + ops->len : 0;
    uint8_t* oob = ops->oobbuf;
    uint8_t* oobend = oob ? oob + ops->ooblen : 0;

    int err;
    
    TRACE("to=0x%08lX, ops.mode=%d, ops.len=%d",
		as32(to), ops->mode, ops->len);
	TRACE("ops.ooblen=%d ops.ooboffs=0x%X data=%p",
        ops->ooblen, ops->ooboffs, data);
    TRACE("oob=[%p..%p]", oob, oobend);

    ops->retlen = 0;    
    ops->oobretlen = 0;

    if(to >= g_chip.mtd->size || !PAGE_ALIGNED(to) || !PAGE_ALIGNED(dataend - data)) {
        printk(KERN_ERR "Can't write non page aligned data to 0x%08lX len:0x%08x\n", 
			as32(to), ops->len);
        return -EINVAL;
    }
    
    if(ops->ooboffs > mtd->oobsize) {
        printk(KERN_ERR "oob > mtd->oobsize" );
        return -EINVAL;
    }

	err = 0;
	for(;(data !=dataend) || (oob !=oobend);) {
		memset(g_chip.dma_area, 0xFF, g_chip.mtd->writesize + g_chip.mtd->oobsize);

		if(data != dataend) {
			memcpy(g_chip.dma_area, data, g_chip.mtd->writesize);
			data += g_chip.mtd->writesize;
		}

		if(oob != oobend) {
			oob = mnand_fill_oob(oob, ops);
			if(!oob) {
				err = -EINVAL;
				printk("MNAND oob einval\n");
				break;    
			}
		}

		err = mnand_core_write(to);

		if(err)
			break;

		to += g_chip.mtd->writesize;
	}


    if(err) {
        printk("MNAND core write returned error: off 0x%08lx err %d\n",
			as32(to), err);
        return err;
    }

    ops->retlen = ops->len;
    ops->oobretlen = ops->ooblen;

    return err;
}

static int mnand_read_oob(struct mtd_info* mtd, loff_t from, struct mtd_oob_ops* ops) 
{
    uint8_t* data = ops->datbuf;
    uint8_t* oob = ops->oobbuf;
    int err;

    uint8_t* dataend = data + ops->len;
    uint8_t* oobend = oob ? oob + ops->ooblen : 0;

    TRACE("from=0x%08lX, ops.mode=%d, ops.len=%d",
		as32(from), ops->mode, ops->len);
	TRACE("ops.ooblen=%d ops.ooboffs=0x%X data=%p",
        ops->ooblen, ops->ooboffs, data);
    TRACE("oob=[%p..%p]", oob, oobend);

    if(ops->len != 0 && data == 0) {
        ops->len = 0;
        dataend = 0;
    }

    ops->retlen = 0;
    ops->oobretlen = 0;

	for(;;) {
		err = mnand_core_read(from);        

		if(err != 0) {
			break;
		}

		if(data != dataend) {
			size_t shift = (from & (mtd->writesize -1));
			size_t bytes = min_t(size_t,dataend-data, mtd->writesize-shift);

			TRACE("data %p dataend %p shift 0x%X bytes 0x%X", 
				data, dataend, shift, bytes);

			memcpy(data, g_chip.dma_area + shift, bytes);
			data += bytes;
		}

		if(oob != oobend) {
			TRACE("oob %p, oobend %p", oob, oobend);

			oob = mnand_transfer_oob(oob, ops, oobend-oob);
			if(!oob) {
				err = -EINVAL;
				break;
			}
		}

		if(data == dataend && oob == oobend)
			break;

		from &= ~((1 << mnand_writesize_shift(mtd))-1);
		from += mtd->writesize;
	}

	if(data) {
		if(g_chip.mtd->ecc_stats.failed != g_chip.ecc_failed) {
			g_chip.mtd->ecc_stats.failed = g_chip.ecc_failed;
			g_chip.mtd->ecc_stats.corrected = g_chip.ecc_corrected;
			err = -EBADMSG;
		} 
		else if(g_chip.mtd->ecc_stats.corrected != g_chip.ecc_corrected) {
			g_chip.mtd->ecc_stats.failed = g_chip.ecc_failed;
			g_chip.mtd->ecc_stats.corrected = g_chip.ecc_corrected;
			err = -EUCLEAN;
		}
	}

	ops->retlen = ops->len;
	ops->oobretlen = ops->ooblen;

    return err;
}

static int mnand_isbad(struct mtd_info* mtd, loff_t off) 
{
    uint8_t f=0;
    int err;
    struct mtd_oob_ops ops = {
        .mode = MTD_OOB_RAW,
        .ooblen = 1,
        .oobbuf = &f,
        .ooboffs = 0
    };

    TRACE("offset 0x%08lX", as32(off));

    err = mnand_read_oob(mtd, off, &ops);

    TRACE("err %d, data=0x%02X", err, f);
    
    return ((err == 0 || err == -EUCLEAN) && f == 0xFF) ? 0 : 1;
}

static int mnand_markbad(struct mtd_info* mtd, loff_t off) 
{
    uint8_t f=0;
    struct mtd_oob_ops ops = {
        .mode = MTD_OOB_RAW,
        .ooblen = 1,
        .oobbuf = &f,
        .ooboffs = 0
    };

    TRACE("offset 0x%08lX", as32(off));

    return mnand_write_oob(mtd, off, &ops);
}


static int mnand_write(struct mtd_info* mtd, loff_t off, size_t len, 
	size_t* retlen, const u_char* buf) 
{
    struct mtd_oob_ops ops = {
        .datbuf = (uint8_t*)buf,
        .len = len
    };
    int err;

    TRACE("off=0x%08lX, len=%d", as32(off), len);

    err = mnand_write_oob(mtd, off, &ops);
    *retlen = ops.retlen;
    return err;
}

static int mnand_read(struct mtd_info* mtd, loff_t off, size_t len, 
	size_t* retlen, u_char* buf) 
{
    struct mtd_oob_ops ops = {
        .datbuf = buf,
        .len = len,
    };
    int err;

    TRACE("off=0x%08lX, len=%d", as32(off), len);

    if(buf == 0)
        return -EINVAL;

    err = mnand_read_oob(mtd, off, &ops);
    *retlen = ops.retlen;
    return err;
}

/* MNAND shouldn't use ENV vars 
 *
 * Uses console, uses malloc
 */
int mnand_init(struct mtd_info* mtd) 
{
    int ret;
	uint32_t base = CONFIG_MNAND_BASE;

	MARK();

    memset(&g_chip, 0, sizeof(struct mnand_chip));

    g_chip.active_page = -1;
	g_chip.io = (void*)base;

    g_chip.dma_area = memalign(0x100, MNAND_DMA_SIZE);
    if(!g_chip.dma_area) {
        ret = -ENOMEM;
		ERR("Unable to allocate memory. Check malloc pool settings.");
        goto out;
    }
	g_chip.dma_handle = (unsigned int)g_chip.dma_area;

	TRACE("Dma buffer allocated: 0x%08lX", (unsigned long)g_chip.dma_area);
    memset(g_chip.dma_area, 0xFF, MNAND_DMA_SIZE);

	g_chip.mtd = mtd;
    mtd->type = MTD_NANDFLASH;
    mtd->flags = MTD_WRITEABLE;
    mtd->erase = mnand_erase;
    mtd->read = mnand_read;
    mtd->write = mnand_write;
    mtd->block_isbad = mnand_isbad;
    mtd->block_markbad = mnand_markbad;
    mtd->read_oob = mnand_read_oob;
    mtd->write_oob = mnand_write_oob;
    mtd->ecclayout = &g_ecclayout;
	mtd->priv = &g_chip;

	mnand_reset(mtd);

    ret = mnand_read_id(mtd);
	if(ret != 0) {
		ERR("Unable to read id: %d", ret);
		goto err_dma;
	}

#if defined(CONFIG_MTD_DEVICE)
	ret = add_mtd_device(mtd);
	if(ret != 0) {
		ERR("Unable to register mtd device: %d", ret);
		goto err_dma;
	}
#endif
    return 0;

err_dma:
	free(g_chip.dma_area);
out:
	return ret;
}

int mnand_reset(struct mtd_info *mtd)
{
	if(mtd->priv != &g_chip) {
		ERR("not a mnand device");
		return -1;
	}
    mnand_hw_init();
    mnand_reset_grabber();
    mnand_core_reset();
	return 0;
}

