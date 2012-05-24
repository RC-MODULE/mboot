#ifndef UEMD_H
#define UEMD_H

#include <asm/u-boot.h>

struct uemd_nand_state {
    int size;
    int oobsize;
    int pagesize;
    int erasesize;
};

enum uemd_boot_source
{
    BOOT_FROM_RAM = 0,
	BOOT_FROM_SPI,
	BOOT_FROM_NAND
};

typedef struct KeyS{
    char data[16];
} Key;

struct uemd_otp
{
	/* need to stop in JTAG wait point*/
	int jtag_stop;

	enum uemd_boot_source source;

	/* image is signed */
	int image_signed; 

	/* image length */
	unsigned int words_length;

	unsigned int spi_speed;

	/* if not null, image is encrypted with key */
	Key *pSignature; 

	/* if not null, image is encrypted with key*/
	Key *pKey; 
};

void uemd_init(struct uemd_otp *otp);

void uemd_em_init(void);
int uemd_em_init_check(bi_dram_t *pdram, int nbank);

void uemd_timer_init(void);

int uemd_console_init(void);

#define uemd_barrier() __asm__ __volatile__("": : :"memory")

#define uemd_error(fmt, args...) do {	\
		printf("%s: " fmt "\n",			\
			__func__, ##args);			\
	} while(0)

#define MEM(addr)  (*((volatile unsigned long int*)(addr)))

extern unsigned long g_early_start;
extern unsigned long g_signature_start;

extern unsigned long g_text_start;
extern unsigned long g_bss_start;
extern unsigned long g_data_start;
extern unsigned long g_bss_end;

#endif
