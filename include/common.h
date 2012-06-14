/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#undef	_LINUX_CONFIG_H
#define _LINUX_CONFIG_H 1	/* avoid reading Linux autoconf.h file	*/

#ifndef __ASSEMBLY__		/* put C only stuff in this section */

typedef unsigned char		uchar;
typedef volatile unsigned long	vu_long;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char	vu_char;

#include <config.h>
#include <compiler.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/ptrace.h>
#include <stdarg.h>

#ifdef	DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif

#define error(fmt, args...) do {					\
		printf("ERROR: " fmt "\nat %s:%d/%s()\n",		\
			##args, __FILE__, __LINE__, __func__);		\
} while (0)

#ifndef BUG
#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif /* BUG */

#define mboot_check_zero(ret, err, fmt, args...) do { \
		if((ret) != 0) { printf(fmt, ##args); err ; } \
	}while(0)

#include <asm/u-boot.h> /* boot information for Linux kernel */
#include <asm/global_data.h>	/* global data used for startup functions */

#ifndef CONFIG_SERIAL_MULTI

#if defined(CONFIG_8xx_CONS_SMC1) || defined(CONFIG_8xx_CONS_SMC2) \
 || defined(CONFIG_8xx_CONS_SCC1) || defined(CONFIG_8xx_CONS_SCC2) \
 || defined(CONFIG_8xx_CONS_SCC3) || defined(CONFIG_8xx_CONS_SCC4)

#define CONFIG_SERIAL_MULTI	1

#endif

#endif /* CONFIG_SERIAL_MULTI */

/*
 * General Purpose Utilities
 */
#define min(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x > __y) ? __x : __y; })

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Function Prototypes
 */

void print_size(unsigned long long, const char *);
void print_buffer (ulong addr, void* data, uint width, uint count, uint linelen);

/* common/flash.c */
void flash_perror (int);

/* common/cmd_source.c */
int	source (ulong addr, const char *fit_uname);


/* common/cmd_doc.c */
void	doc_probe(unsigned long physadr);

#ifdef CONFIG_ARM
# include <asm/mach-types.h>
# include <asm/setup.h>
# include <asm/u-boot-arm.h>	/* ARM version to be fixed! */
#endif /* CONFIG_ARM */

void	pci_init      (void);
void	pci_init_board(void);
void	pciinfo	      (int, int);

#if defined(CONFIG_PCI) && (defined(CONFIG_4xx) && !defined(CONFIG_AP1000))
    int	   pci_pre_init	       (struct pci_controller *);
    int	   is_pci_host	       (struct pci_controller *);
#endif

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

/* $(BOARD)/eeprom.c */
void eeprom_init  (void);
#ifndef CONFIG_SPI
int  eeprom_probe (unsigned dev_addr, unsigned offset);
#endif
int  eeprom_read  (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt);
int  eeprom_write (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt);
#ifdef CONFIG_LWMON
extern uchar pic_read  (uchar reg);
extern void  pic_write (uchar reg, uchar val);
#endif

/* $(BOARD)/$(BOARD).c */
void board_reset(void);
void board_hang(void);
int board_eth_init(void);

int	icache_status (void);
void	icache_enable (void);
void	icache_disable(void);
int	dcache_status (void);
void	dcache_enable (void);
void	dcache_disable(void);

/* $(CPU)/cpu.c */
int	cpu_numcores  (void);
void	cpu_reset     (ulong addr);

/* $(CPU)/serial.c */
int	serial_init   (void);
void	serial_exit   (void);
void	serial_setbrg (void);
void	serial_putc   (const char);
void	serial_putc_raw(const char);
void	serial_puts   (const char *);
int	serial_getc   (void);
int	serial_tstc   (void);

/* $(CPU)/interrupts.c */
int	interrupt_init	   (void);
void	timer_interrupt	   (struct pt_regs *);
void	external_interrupt (struct pt_regs *);
void	irq_free_handler   (int);
void	reset_timer	   (void);
ulong	get_timer	   (ulong base);
void	set_timer	   (ulong t);
void	enable_interrupts  (void);
int	disable_interrupts (void);

#define BOOTCOUNT_MAGIC		0xB001C041

/* $(CPU)/.../<eth> */
void mii_init (void);

/* $(CPU)/.../lcd.c */
ulong	lcd_setmem (ulong);

/* $(CPU)/.../vfd.c */
ulong	vfd_setmem (ulong);

/* $(CPU)/.../video.c */
ulong	video_setmem (ulong);

/* arch/$(ARCH)/lib/cache.c */
void	cache_flush(void);
void	dcache_flush_range(unsigned long start, unsigned long stop);
void	dcache_invalidate_range(unsigned long start, unsigned long stop);

/* arch/$(ARCH)/lib/ticks.S */
unsigned long long get_ticks(void);
void	wait_ticks    (unsigned long);

/* arch/$(ARCH)/lib/time.c */
void	__udelay      (unsigned long);
ulong	usec2ticks    (unsigned long usec);
ulong	ticks2usec    (unsigned long ticks);
int	init_timebase (void);

/* lib/gunzip.c */
int gunzip(void *, int, unsigned char *, unsigned long *);
int zunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp,
						int stoponerr, int offset);
/* lib/vsprintf.c */
ulong	simple_strtoul(const char *cp,char **endp,unsigned int base);
unsigned long long	simple_strtoull(const char *cp,char **endp,unsigned int base);
long	simple_strtol(const char *cp,char **endp,unsigned int base);
void	panic(const char *fmt, ...)
		__attribute__ ((format (__printf__, 1, 2)));
int	sprintf(char * buf, const char *fmt, ...)
		__attribute__ ((format (__printf__, 2, 3)));
int	vsprintf(char *buf, const char *fmt, va_list args);

static inline char * strncpy_s(char * dest,const char *src,size_t count)
{
	char* c;
	if(count == 0) return NULL;
	c = strncpy(dest,src,count-1);
	dest[count-1] = '\0';
	return c;
}

/* common/cmd_nvedit2.c */
#include <environment.h>

/* lib/net_utils.c */
#include <net.h>
static inline IPaddr_t getenv_IPaddr (char *var)
{
	return (string_to_ip(getenv(var)));
}

/* lib/qsort.c */
void qsort(void *base, size_t nmemb, size_t size,
	   int(*compar)(const void *, const void *));

/* lib/time.c */
void	udelay        (unsigned long);

/* lib/strmhz.c */
char *	strmhz(char *buf, long hz);

/* lib/crc32.c */
#include <crc.h>

/* common/console.c */
int	ctrlc (void);
int	had_ctrlc (void);	/* have we had a Control-C since last clear? */
void	clear_ctrlc (void);	/* clear the Control-C condition */
int	disable_ctrlc (int);	/* 1 to disable, 0 to enable Control-C detect */

/*
 * STDIO based functions (can always be used)
 */

/* stdin */
int	getc(void);
int	tstc(void);

/* stdout */
void	putc(const char c);
void	nputc(const char *c, size_t n);
void	puts(const char *s);
int	printf(const char *fmt, ...)
		__attribute__ ((format (__printf__, 1, 2)));
int	vprintf(const char *fmt, va_list args);

#include <memregion.h>


#endif /* __ASSEMBLY__ */

/* Put only stuff here that the assembler can digest */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ROUND(a,b)		(((a) + (b)) & ~((b) - 1))
#define DIV_ROUND(n,d)		(((n) + ((d)/2)) / (d))
#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

#endif	/* __COMMON_H_ */
