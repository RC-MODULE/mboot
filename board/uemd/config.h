/*
 * Copyright (C) 2010
 * Module RC
 * Sergey Mironov <ierton@gmail.com>
 * Configuation settings for the UEMD board.
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

#ifndef UEMD_CONFIG_H
#define UEMD_CONFIG_H

#include <config_defaults.h>

/*#define CONFIG_ARM_ARM1176        y*/
#define CONFIG_BOARD_UEMD         1
#define CONFIG_UEMD_MACH_TYPE     3281

/* Use HIGH-frequency mode */
#undef CONFIG_UEMD_LO_FREQ

#ifdef  CONFIG_UEMD_LO_FREQ
#define FREQ_54000000 40000000
#define FREQ_54       40
#else
#define FREQ_54000000 54000000
#define FREQ_54       54
#endif

/* Use only first 16M of RAM */
#define  CONFIG_UEMD_LO_MEM

#ifdef  CONFIG_UEMD_LO_MEM
#define _PHYS_EM0_SIZE 0x01000000
#else
#define _PHYS_EM0_SIZE 0x08000000
#endif

/*
 * Physical Memory Map
 */
#define CONFIG_SYS_TEXT_BASE      TEXT_BASE

#define CONFIG_SYS_UBOOT_BASE     CONFIG_SYS_TEXT_BASE

/* Internal memory*/
#define PHYS_IM0                  0x00100000
#define PHYS_IM0_SIZE             0x00040000
#define PHYS_IM1                  0x00140000
#define PHYS_IM1_SIZE             0x00040000

/* DDR memory */
#define PHYS_EM0                  0x40000000
#define PHYS_EM0_SIZE             _PHYS_EM0_SIZE
#define PHYS_EM1                  0xC0000000
#define PHYS_EM1_SIZE             0x08000000

#define CONFIG_SYS_MEMTEST_START  (PHYS_EM0)
#define CONFIG_SYS_MEMTEST_END    (PHYS_EM0 + PHYS_EM0_SIZE)

/* Environment data */
#define CONFIG_SYS_ENV_SIZE       0x400
#define CONFIG_SYS_ENV_ADDR       (PHYS_IM0 + PHYS_IM0_SIZE - CONFIG_SYS_ENV_SIZE)

/* Malloc data */
#define CONFIG_SYS_MALLOC_SIZE    0x8000
#define CONFIG_SYS_MALLOC_ADDR    (CONFIG_SYS_ENV_ADDR - CONFIG_SYS_MALLOC_SIZE)

/* Stack pointer address. Make sure it is 8-aligned. */
#define CONFIG_SYS_SP_ADDR        ((CONFIG_SYS_MALLOC_ADDR - 4) & (~7))

#define CONFIG_SYS_NO_ICACHE
#define CONFIG_SYS_NO_DCACHE


/*
 * System contoller registers
 */
/* One-time programmed memory */
#define CONFIG_OTP_SHADOW_BASE    0x20033000

/*
 * Timer
 *
 */
#define CONFIG_SYS_HZ             1000
/* Internal clock freq feeding the timer, in MHz */
#define CONFIG_SYS_TIMER_CLK_MHZ  FREQ_54
/* Timer base */
#define CONFIG_SYS_TIMERBASE      0x20024000	

/*
 * UART Configuration
 */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE -4
#define CONFIG_CONS_INDEX	  1
#define CONFIG_SYS_NS16550_CLK    FREQ_54000000
#define CONFIG_SYS_NS16550_COM1   0x2002b000
#define CONFIG_BAUDRATE           38400

/*
 * Ethernet configuration
 */
#define CONFIG_GRETH              1
#define CONFIG_GRETH_BASE         0x20034000
#define CONFIG_GRETH_IRQ          (-1)
#define CONFIG_GRETH_PHY_ADDR     0x1F
#define CONFIG_GRETH_PHY_TIMEOUT_MS   3000
#define CONFIG_GRETH_SWAP_BD
#define CONFIG_GRETH_DEBUG
//#define CONFIG_GRETH_DEBUG_PACKET
/* General network settings */
#define CONFIG_NETMASK            "255.255.255.0"
#define CONFIG_GATEWAYIP          "10.0.0.1"
#define CONFIG_SERVERIP           "10.0.0.1"
#define CONFIG_IPADDR             "10.0.0.103"
#define CONFIG_ETHADDR            "00:02:F7:00:27:0F"
#define CONFIG_GRETH_SET_HWADDR
/* Enables new U-Boot networking stack */
#define CONFIG_NET_MULTI          1
/* Sets the MTU value for tftp. Larger values may 
 * cause greth to hung up. */
#define CONFIG_TFTP_BLOCKSIZE     600

/*
 * MNAND specific defines
 */
#define CONFIG_MTD
#define CONFIG_CMD_MTD
#define CONFIG_CMD_MTDBOOT
#define CONFIG_MTD_PARTITIONS
// Name of a boot partition
#define CONFIG_MTD_BOOTNAME "kernel"
#define CONFIG_MTD_MNAND
#define CONFIG_MNAND_BASE       0x2003f000
/*Uncomment to use slower timings*/
//#define CONFIG_MTD_MNAND_SLOW
#define CONFIG_MNAND_TIMEOUT_MS   5000
//#define CONFIG_MNAND_DEBUG_HIST
//#define CONFIG_CMD_MNANDCTL

/*
 * Booting
 */
#define CONFIG_BOOTDIR    "/tftpboot/smironov-boot"
#define CONFIG_BOOTFILE    CONFIG_BOOTDIR "/uImage"
#define CONFIG_LOADADDR    0x40100000
#define CONFIG_BOOTCOMMAND "tftp;bootm"
#define CONFIG_BOOTDELAY   10

/*
 * Commands configurationn
 */
//#define CONFIG_CMD_BDI
//#define CONFIG_CMD_ENV
//#define CONFIG_CMD_FLASH
//#define CONFIG_CMD_SAVEENV
//#define CONFIG_CMD_IMI
#define CONFIG_CMD_MEMORY
//#define CONFIG_CMD_MTEST_XDMAC
//#define CONFIG_CMD_MTEST_COMPRESS
//#define CONFIG_BZIP2
//#define BZ_DEBUG
//#define CONFIG_CMD_DHCP
//#define CONFIG_CMD_ELF
#define CONFIG_CMD_RUN
//#define CONFIG_CMD_ASKENV
//#define CONFIG_CMD_EDITENV
#define CONFIG_CMD_MISC
#define CONFIG_CMD_FWUPGRADE
#define CONFIG_CMD_FWUPGRADE_BUFFER_SIZE 0x20000
#define CONFIG_CMD_FWUPGRADE_BUFFER_ADDR CONFIG_LOADADDR
#define CONFIG_CMD_GO
//#define CONFIG_CMD_UEMD_DRAM
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
//#define CONFIG_CMD_MD5SUM
//#define CONFIG_CMD_MNAND

/*
 * Miscellaneous configurable options
 */
/* call misc_init_r during start up */
/* Long help messages */
#define CONFIG_SYS_LONGHELP
/* Console I/O Buffer Size */
#define CONFIG_SYS_CBSIZE    256
/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT    "UEMD # "
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE    (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
/* Max number of command args */
#define CONFIG_SYS_MAXARGS   16
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE  CONFIG_SYS_CBSIZE	
/* Everything, incl board info, in Hz */
#undef	CONFIG_SYS_CLKS_IN_HZ
/* Default image load address (used by bootm)
 * 32K from the top of RAM, as arm/Booting says */
/*#define CONFIG_SYS_LOAD_ADDR  (PHYS_EM0 + 0x00008000)*/
/* Default address of kernel options */
#define CONFIG_SYS_PARAM_ADDR (PHYS_EM0 + 0x00000100)
/* Prevent compiling flash-specific coed */
#define CONFIG_SYS_NO_FLASH 1
/* Number of timeouts before giving up */
#define CONFIG_NET_RETRY_COUNT    3

#endif

