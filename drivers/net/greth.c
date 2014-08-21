/* Gaisler.com GRETH 10/100/1000 Ethernet MAC driver
 *
 * Driver use polling mode (no Interrupt)
 *
 * (C) Copyright 2007
 * Daniel Hellstrom, Gaisler Research, daniel@gaisler.com
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <netdev.h>
#include <malloc.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <ambapp.h>
#include <linux/ctype.h>

#include "greth.h"

#if defined(DEBUG) || defined(CONFIG_GRETH_DEBUG)
#define greth_debug(s, args...) printf("greth: " s , ## args)
#else
#define greth_debug(s, args...)
#endif

#if defined(CONFIG_GRETH_DEBUG_PACKET)
#define greth_debug_packet(s, args...) printf("greth: " s , ## args)
#define greth_debug_packet_buf(b,l)    print_buffer(0, (d), 4, (l), 0)
#else
#define greth_debug_packet(s, args...) do { } while(0)
#define greth_debug_packet_buf(b,l)    do { } while(0)
#endif

#define greth_error(s, args...) error("greth: " s, ##args)

/* ByPass Cache when reading regs */
#define GRETH_REGLOAD(addr)		(*(volatile unsigned int *)(addr))
/* Write-through cache ==> no bypassing needed on writes */
#define GRETH_REGSAVE(addr,data)	(*(volatile unsigned int *)(addr) = (data))
#define GRETH_REGORIN(addr,data) GRETH_REGSAVE(addr,GRETH_REGLOAD(addr)|data)
#define GRETH_REGANDIN(addr,data) GRETH_REGSAVE(addr,GRETH_REGLOAD(addr)&data)

#define GRETH_RXBD_CNT 4
#define GRETH_TXBD_CNT 1

#define GRETH_RXBUF_SIZE 1540
#define GRETH_BUF_ALIGN 4
#define GRETH_RXBUF_EFF_SIZE \
	( (GRETH_RXBUF_SIZE&~(GRETH_BUF_ALIGN-1))+GRETH_BUF_ALIGN )

#ifndef GRETH_HWADDR_0
#define GRETH_HWADDR_0            0x00
#define GRETH_HWADDR_1            0x00
#define GRETH_HWADDR_2            0x7a
#define GRETH_HWADDR_3            0xcc
#define GRETH_HWADDR_4            0x00
#define GRETH_HWADDR_5            0x12
#endif

typedef struct {
	greth_regs *regs;
	int irq;
	struct eth_device *dev;

	/* Hardware info */
	unsigned char phyaddr;
	int gbit_mac;

	/* Current operating Mode */
	int gb;			/* GigaBit */
	int fd;			/* Full Duplex */
	int sp;			/* 10/100Mbps speed (1=100,0=10) */
	int auto_neg;		/* Auto negotiate done */

	unsigned char hwaddr[6];	/* MAC Address */

	/* Descriptors */
	greth_bd *rxbd_base, *rxbd_max;
	greth_bd *txbd_base, *txbd_max;

	greth_bd *rxbd_curr;

	/* rx buffers in rx descriptors */
	void *rxbuf_base;	/* (GRETH_RXBUF_SIZE+ALIGNBYTES) * GRETH_RXBD_CNT */

	/* unused for gbit_mac, temp buffer for sending packets with unligned
	 * start.
	 * Pointer to packet allocated with malloc.
	 */
	void *txbuf;

	struct {
		/* rx status */
		unsigned int rx_packets,
		    rx_crc_errors, rx_frame_errors, rx_length_errors, rx_errors;

		/* tx stats */
		unsigned int tx_packets,
		    tx_latecol_errors,
		    tx_underrun_errors, tx_limit_errors, tx_errors;
	} stats;
} greth_priv;

static int greth_reset(greth_priv *greth);

static void greth_bd_write(volatile unsigned int *mem, unsigned int data)
{
#ifdef CONFIG_GRETH_SWAP_BD
	data = swab32(data);
#endif
	GRETH_REGSAVE(mem,data);
}

static unsigned int greth_bd_read(volatile unsigned int *mem)
{
	unsigned int data = GRETH_REGLOAD(mem);
#ifdef CONFIG_GRETH_SWAP_BD
	data = swab32(data);
#endif
	return data;
}

/* Read MII register 'addr' from core 'regs' */
static int read_mii(int addr, int phyaddr, volatile greth_regs * regs)
{
	unsigned int start, timeout;
	timeout = CONFIG_GRETH_PHY_TIMEOUT_MS;

	start = get_timer(0);
	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
		if(get_timer(start) > timeout) {
			greth_error("read_mii: MII is not ready for operation\n");
			return -2;
		}
	}

	GRETH_REGSAVE(&regs->mdio, 
		((phyaddr & 0x1F) << 11) | 
		((addr & 0x1F) << 6) | 
		2);

	start = get_timer(0);
	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
		if(get_timer(start) > timeout) {
			greth_error("read_mii busy timeout\n");
			return -2;
		}
	}

	if (!(GRETH_REGLOAD(&regs->mdio) & GRETH_MII_NVALID)) {
		return (GRETH_REGLOAD(&regs->mdio) >> 16) & 0xFFFF;
	} else {
		return -1;
	}
}

static int write_mii(int addr, int data, int phyaddr, volatile greth_regs * regs)
{
	unsigned int regval;
	unsigned int start, timeout;
	timeout = CONFIG_GRETH_PHY_TIMEOUT_MS;

	start = get_timer(0);
	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
		if(get_timer(start) > timeout) {
			greth_error("write_mii: MII is not ready for operation\n");
			return -2;
		}
	}

	regval = 
		((data & 0xFFFF) << 16) | 
		((phyaddr & 0x1F) << 11) | 
		((addr & 0x1F) << 6) |
		1;

	GRETH_REGSAVE(&regs->mdio, regval);

	greth_debug("write_mii: 0x%08X < 0x%08X [p:%d a:%d d:0x%04X]\n", 
		(unsigned int)&regs->mdio, regval, phyaddr, addr, data);

	start = get_timer(0);
	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
		if(get_timer(start) > timeout) {
			greth_error("write_mii busy timeout\n");
			return -2;
		}
	}

	return 0;
}

/* init/start hardware and allocate descriptor buffers for rx side
 *
 */
static int greth_init(struct eth_device *dev)
{
	return 0;
}

static int greth_real_init(struct eth_device *dev)
{
	int i;

	greth_priv *greth = dev->priv;
	greth_regs *regs = greth->regs;
	greth_debug("greth_init\n");

	GRETH_REGSAVE(&regs->control, 0);

	if (!greth->rxbd_base) {

		/* allocate descriptors */
		greth->rxbd_base = (greth_bd *)
		    memalign(0x1000, GRETH_RXBD_CNT * sizeof(greth_bd));
		if(greth->rxbd_base == NULL) {
			error("Unable to allocate RxBD memory\n");
			return -1;
		}
		greth->txbd_base = (greth_bd *)
		    memalign(0x1000, GRETH_RXBD_CNT * sizeof(greth_bd));
		if(greth->txbd_base == NULL) {
			error("Unable to allocate TxBD memory\n");
			return -1;
		}

		/* allocate buffers to all descriptors  */
		greth->rxbuf_base =
		    malloc(GRETH_RXBUF_EFF_SIZE * GRETH_RXBD_CNT);
		if(greth->rxbuf_base == NULL) {
			error("Unable to allocate Rx buffer memory\n");
			return -1;
		}
	}

	memset(greth->rxbuf_base, 0, GRETH_RXBUF_EFF_SIZE * GRETH_RXBD_CNT);

	/* initate rx decriptors */
	for (i = 0; i < GRETH_RXBD_CNT; i++) {
		greth_bd_write(&greth->rxbd_base[i].addr, (unsigned int)
		    greth->rxbuf_base + (GRETH_RXBUF_EFF_SIZE * i));
		/* enable desciptor & set wrap bit if last descriptor */
		if (i >= (GRETH_RXBD_CNT - 1)) {
			greth_bd_write(&greth->rxbd_base[i].stat, GRETH_BD_EN | GRETH_BD_WR);
		} else {
			greth_bd_write(&greth->rxbd_base[i].stat, GRETH_BD_EN);
		}
	}

	/* initiate indexes */
	greth->rxbd_curr = greth->rxbd_base;
	greth->rxbd_max = greth->rxbd_base + (GRETH_RXBD_CNT - 1);
	greth->txbd_max = greth->txbd_base + (GRETH_TXBD_CNT - 1);
	/*
	 * greth->txbd_base->addr = 0;
	 * greth->txbd_base->stat = GRETH_BD_WR;
	 */

	/* initate tx decriptors */
	for (i = 0; i < GRETH_TXBD_CNT; i++) {
		greth_bd_write(&greth->txbd_base[i].addr, 0);
		/* enable desciptor & set wrap bit if last descriptor */
		if (i >= (GRETH_RXBD_CNT - 1)) {
			greth_bd_write(&greth->txbd_base[i].stat, GRETH_BD_WR);
		} else {
			greth_bd_write(&greth->txbd_base[i].stat, 0);
		}
	}

	/**** SET HARDWARE REGS ****/

	/* Set pointer to tx/rx descriptor areas */
	GRETH_REGSAVE(&regs->rx_desc_p, (unsigned int)&greth->rxbd_base[0]);
	GRETH_REGSAVE(&regs->tx_desc_p, (unsigned int)&greth->txbd_base[0]);

	/* Enable Transmitter, GRETH will now scan descriptors for packets
	 * to transmitt */
	greth_debug("greth_init: enabling receiver\n");
	GRETH_REGORIN(&regs->control, GRETH_RXEN);

	return 0;
}

/* Initiate PHY to a relevant speed
 * return:
 *  0 = success
 *  -1 = fail
 *  -2 = timeout/fail
 */
static int greth_init_phy(greth_priv * dev)
{
	greth_regs *regs = dev->regs;
	int tmp=0, tmp1, tmp2, cfg, i;
	unsigned int start, timeout;
	int ret;

	/* X msecs to ticks */
	timeout = CONFIG_GRETH_PHY_TIMEOUT_MS;

	/* get phy control register default values */
	start = get_timer(0);
	do {
		cfg = read_mii(0, dev->phyaddr, regs);
		if(cfg < 0) {
			greth_error("read_mii error (%d)\n", cfg);
			return cfg;
		}
		if (get_timer(start) > timeout) {
			greth_error("Looks like PHY is in permanent reset state\n");
			return -2;
		}
	} while(cfg & 0x8000);

	greth_debug("Resetting the PHY\n");

	/* reset PHY */
	ret = write_mii(0, 0x8000 | cfg, dev->phyaddr, regs);
	if(ret < 0) {
		greth_error("Unable to reset the PHY (%d)\n", ret);
		return ret;
	}

	/* Wait for PHY reset completion */
	start = get_timer(0);
	do {
		cfg = read_mii(0, dev->phyaddr, regs);
		if(cfg < 0) {
			greth_error("read_mii error (%d)\n", cfg);
			return cfg;
		}
		if (get_timer(start) > timeout) {
			greth_error("PHY reset timeout\n");
			return -2;	/* Fail */
		}
	} while(cfg & 0x8000);

	/* 
	 * Check if PHY is autoneg capable and then determine operating
	 * mode, otherwise force it to 10 Mbit halfduplex
	 */
	dev->gb = 0;
	dev->fd = 0;
	dev->sp = 0;
	dev->auto_neg = 0;
	if (!((cfg >> 12) & 1)) {
		ret = write_mii(0, 0, dev->phyaddr, regs);
		if(ret < 0) {
			greth_error("Unable write default vals to PHY (%d)\n", ret);
			return ret;
		}
	} else {
		/* wait for auto negotiation to complete and then check operating mode */
		dev->auto_neg = 1;
		i = 0;
		while (!(((tmp = read_mii(1, dev->phyaddr, regs)) >> 5) & 1)) {
			if (get_timer(start) > timeout) {
				printf("greth: Auto negotiation timed out. "
				       "Selecting default config\n");
				dev->gb = ((cfg >> 6) & 1)
				    && !((cfg >> 13) & 1);
				dev->sp = !((cfg >> 6) & 1)
				    && ((cfg >> 13) & 1);
				dev->fd = (cfg >> 8) & 1;
				goto auto_neg_done;
			}
		}
		if ((tmp >> 8) & 1) {
			tmp1 = read_mii(9, dev->phyaddr, regs);
			tmp2 = read_mii(10, dev->phyaddr, regs);
			if ((tmp1 & GRETH_MII_EXTADV_1000FD) &&
			    (tmp2 & GRETH_MII_EXTPRT_1000FD)) {
				dev->gb = 1;
				dev->fd = 1;
			}
			if ((tmp1 & GRETH_MII_EXTADV_1000HD) &&
			    (tmp2 & GRETH_MII_EXTPRT_1000HD)) {
				dev->gb = 1;
				dev->fd = 0;
			}
		}
		if ((dev->gb == 0) || ((dev->gb == 1) && (dev->gbit_mac == 0))) {
			tmp1 = read_mii(4, dev->phyaddr, regs);
			tmp2 = read_mii(5, dev->phyaddr, regs);
			if ((tmp1 & GRETH_MII_100TXFD) &&
			    (tmp2 & GRETH_MII_100TXFD)) {
				dev->sp = 1;
				dev->fd = 1;
			}
			if ((tmp1 & GRETH_MII_100TXHD) &&
			    (tmp2 & GRETH_MII_100TXHD)) {
				dev->sp = 1;
				dev->fd = 0;
			}
			if ((tmp1 & GRETH_MII_10FD) && (tmp2 & GRETH_MII_10FD)) {
				dev->fd = 1;
			}
			if ((dev->gb == 1) && (dev->gbit_mac == 0)) {
				dev->gb = 0;
				dev->fd = 0;
				write_mii(0, dev->sp << 13, dev->phyaddr, regs);
			}
		}
	}
auto_neg_done:
	greth_debug("%s GRETH Ethermac at [0x%x] irq %d. Running "
		"%d Mbps %s duplex\n", dev->gbit_mac ? "10/100/1000" : "10/100", 
		(unsigned int)(regs), 
		(unsigned int)(dev->irq), dev->gb ? 1000 : (dev->sp ? 100 : 10), 
		dev->fd ? "full" : "half");
	/* Read out PHY info if extended registers are available */
	if (tmp & 1) {
		tmp1 = read_mii(2, dev->phyaddr, regs);
		tmp2 = read_mii(3, dev->phyaddr, regs);
		tmp1 = (tmp1 << 6) | ((tmp2 >> 10) & 0x3F);
		tmp = tmp2 & 0xF;

		tmp2 = (tmp2 >> 4) & 0x3F;
		greth_debug("PHY: Vendor %x   Device %x    Revision %d\n", tmp1,
		       tmp2, tmp);
	} else {
		printf("PHY info not available\n");
	}

	/* set speed and duplex bits in control register */
	GRETH_REGORIN(&regs->control,
		      (dev->gb << 8) | (dev->sp << 7) | (dev->fd << 4));

	return 0;
}



static void greth_halt(struct eth_device *dev)
{
}

#if 0
static void greth_halt(struct eth_device *dev)
{
	greth_priv *greth;
	greth_regs *regs;
	int i;
	greth_debug("greth_halt\n");
	if (!dev || !dev->priv)
		return;

	greth = dev->priv;
	regs = greth->regs;

	if (!regs)
		return;

	/* disable receiver/transmitter by clearing the enable bits */
	GRETH_REGANDIN(&regs->control, ~(GRETH_RXEN | GRETH_TXEN));

	/* reset rx/tx descriptors */
	if (greth->rxbd_base) {
		for (i = 0; i < GRETH_RXBD_CNT; i++) {
			greth_bd_write(&greth->rxbd_base[i].stat, 
				(i >= (GRETH_RXBD_CNT - 1)) ? GRETH_BD_WR : 0);
		}
	}

	if (greth->txbd_base) {
		for (i = 0; i < GRETH_TXBD_CNT; i++) {
			greth_bd_write(&greth->txbd_base[i].stat, 
			    (i >= (GRETH_TXBD_CNT - 1)) ? GRETH_BD_WR : 0);
		}
	}
}
#endif 

static int greth_send(struct eth_device *dev, volatile void *eth_data, int data_length)
{
	greth_priv *greth = dev->priv;
	greth_regs *regs = greth->regs;
	greth_bd *txbd;
	void *txbuf;
	unsigned int status;
	int bad;

	greth_debug_packet("greth_send\n");

	/* send data, wait for data to be sent, then return */
	if (((unsigned int)eth_data & (GRETH_BUF_ALIGN - 1))
		&& !greth->gbit_mac) {
		/* data not aligned as needed by GRETH 10/100, solve this by allocating 4 byte aligned buffer
		 * and copy data to before giving it to GRETH.
		 */
		if (!greth->txbuf) {
			greth->txbuf = malloc(GRETH_RXBUF_SIZE);
			if(greth->txbuf == NULL) {
				error("greth: failed to allocate Tx buffer\n");
				return -1;
			}
			greth_debug("GRETH: allocated aligned tx-buf\n");
		}

		txbuf = greth->txbuf;

		/* copy data info buffer */
		memcpy((char *)txbuf, (char *)eth_data, data_length);

		/* keep buffer to next time */
	} else {
		txbuf = (void *)eth_data;
	}
	/* get descriptor to use, only 1 supported... hehe easy */
	txbd = greth->txbd_base;

	/* setup descriptor to wrap around to it self */
	greth_bd_write(&txbd->addr, (unsigned int)txbuf);
	greth_bd_write(&txbd->stat, GRETH_BD_EN | GRETH_BD_WR | data_length);

	/* Remind Core which descriptor to use when sending */
	GRETH_REGSAVE(&regs->tx_desc_p, (unsigned int)txbd);

	/* initate send by enabling transmitter */
	GRETH_REGORIN(&regs->control, GRETH_TXEN);

	/* Wait for data to be sent */
	while ((status = greth_bd_read(&txbd->stat)) & GRETH_BD_EN) {
		;
	}

	bad = 0;

	/* was the packet transmitted succesfully? */
	if (status & GRETH_TXBD_ERR_AL) {
		greth->stats.tx_limit_errors++;
		bad = 1;
	}

	if (status & GRETH_TXBD_ERR_UE) {
		greth->stats.tx_underrun_errors++;
		bad = 1;
	}

	if (status & GRETH_TXBD_ERR_LC) {
		greth->stats.tx_latecol_errors++;
		bad = 1;
	}

	if (bad) {
		/* any error */
		greth->stats.tx_errors++;
		printf
			("greth_send: Bad packet (li:%d, un:%d, lc:%d, s:0x%08x, np:%d)\n",
			 greth->stats.tx_limit_errors,
			 greth->stats.tx_underrun_errors,
			 greth->stats.tx_latecol_errors, status,
			 greth->stats.tx_packets);
		return -1;
	}

	/* bump tx packet counter */
	greth->stats.tx_packets++;

	/* return succefully */
	return 0;
}

static int greth_recv(struct eth_device *dev)
{
	greth_priv *greth = dev->priv;
	greth_regs *regs = greth->regs;
	greth_bd *rxbd;
	unsigned int status, len = 0, bad;
	unsigned char *d;
	int enable = 0;
	int i;
	/* Receive One packet only, but clear as many error packets as there are
	 * available.
	 */
	{
		/* current receive descriptor */
		rxbd = greth->rxbd_curr;

		/* get status of next received packet */
		status = greth_bd_read(&rxbd->stat);

		bad = 0;

		/* stop if no more packets received */
		if (status & GRETH_BD_EN) {
			goto done;
		}
		greth_debug_packet("greth_recv: packet 0x%lx, 0x%lx, len: %d\n",
		       (long unsigned int)rxbd, (long unsigned int)status, status & GRETH_BD_LEN);

		/* Check status for errors.
		 */
		if (status & GRETH_RXBD_ERR_FT) {
			greth->stats.rx_length_errors++;
			bad = 1;
		}
		if (status & (GRETH_RXBD_ERR_AE | GRETH_RXBD_ERR_OE)) {
			greth->stats.rx_frame_errors++;
			bad = 1;
		}
		if (status & GRETH_RXBD_ERR_CRC) {
			greth->stats.rx_crc_errors++;
			bad = 1;
		}
		if (bad) {
			greth->stats.rx_errors++;
			printf
			    ("greth_recv: Bad packet (%d, %d, %d, 0x%08x, %d)\n",
			     greth->stats.rx_length_errors,
			     greth->stats.rx_frame_errors,
			     greth->stats.rx_crc_errors, status,
			     greth->stats.rx_packets);
			/* print all rx descriptors */
			for (i = 0; i < GRETH_RXBD_CNT; i++) {
				printf("[%d]: Stat=0x%x, Addr=0x%x\n", i,
				       GRETH_REGLOAD(&greth->rxbd_base[i].stat),
				       GRETH_REGLOAD(&greth->rxbd_base[i].
						     addr));
			}
		} else {
			/* flush all data cache to make sure we're not reading old packet data */
			cache_flush();
			/* Process the incoming packet. */
			len = status & GRETH_BD_LEN;
			d = (unsigned char *)greth_bd_read(&rxbd->addr);

			greth_debug_packet_buf(d, 16);

			/* pass packet on to network subsystem */
			NetReceive((void *)d, len);

			/* bump stats counters */
			greth->stats.rx_packets++;

			/* bad is now 0 ==> will stop loop */
		}

		/* reenable descriptor to receive more packet with this descriptor, wrap around if needed */
		greth_bd_write(&rxbd->stat, 
		    GRETH_BD_EN |
		    (((unsigned int)greth->rxbd_curr >=
		      (unsigned int)greth->rxbd_max) ? GRETH_BD_WR : 0));
		enable = 1;

		/* increase index */
		greth->rxbd_curr =
		    ((unsigned int)greth->rxbd_curr >=
		     (unsigned int)greth->rxbd_max) ? greth->
		    rxbd_base : (greth->rxbd_curr + 1);
	};

	if (enable) {
		GRETH_REGORIN(&regs->control, GRETH_RXEN);
	}
done:
	/* return positive length of packet or 0 if non recieved */
	return len;
}

static void greth_set_hwaddr(greth_priv * greth, unsigned char *mac)
{
	/* save new MAC address */
	greth->hwaddr[0] = mac[0];
	greth->hwaddr[1] = mac[1];
	greth->hwaddr[2] = mac[2];
	greth->hwaddr[3] = mac[3];
	greth->hwaddr[4] = mac[4];
	greth->hwaddr[5] = mac[5];
	greth->regs->esa_msb = (mac[0] << 8) | mac[1];
	greth->regs->esa_lsb =
	    (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
	greth_debug("GRETH: New MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int greth_set_hwaddr_from_netdev(struct eth_device *dev)
{
	greth_priv *greth = (greth_priv*) dev->priv;
	if(!is_valid_ether_addr(dev->enetaddr))
		return -1;
	greth_set_hwaddr(greth, dev->enetaddr);
	return 0;
}

static const char __greth_phys[] = CONFIG_GRETH_PHY_ADDRS;

static int greth_reset(greth_priv *greth)
{
	struct eth_device *dev;
	dev = greth->dev;
	unsigned int start, timeout;
	int ret;
	int pos=0;

retry:
	greth_debug("Resetting GRETH\n");

	/* X msecs to ticks */
	timeout = CONFIG_GRETH_PHY_TIMEOUT_MS;

	/* Reset Core */
	GRETH_REGSAVE(&greth->regs->control, GRETH_RESET);

	start = get_timer(0);

	/* Wait for core to finish reset cycle */
	while (GRETH_REGLOAD(&greth->regs->control) & GRETH_RESET) {
		if(get_timer(start) > timeout) {
			greth_error("Timeout while waiting for reset.\n");
			return -2;
		}
	};

	char *p_addr = getenv("phyaddr");
	if (p_addr) {
		greth->phyaddr = (unsigned char) simple_strtoul(p_addr, NULL, 16);
		greth_debug("greth: using PHY addr from env: %x\n", greth->phyaddr);
	} else {
		greth_debug("greth: 'phyaddr' not set, fall back to built-in table\n");
		greth->phyaddr = __greth_phys [pos];
		greth_debug("greth: using preset PHY addr: %x\n", greth->phyaddr);
	}
	/* Get the phy address which assumed to have been set
	   correctly with the reset value in hardware */
//	greth->phyaddr = (GRETH_REGLOAD(&greth->regs->mdio) >> 11) & 0x1F;
//	greth_debug("greth: PHY addr detected: %d\n", greth->phyaddr);

	/* Check if mac is gigabit capable */
	greth->gbit_mac = (GRETH_REGLOAD(&greth->regs->control) >> 27) & 1;

	/* Make descriptor string */
	if (greth->gbit_mac) {
		sprintf(dev->name, "GRETH_10/100/GB");
	} else {
		sprintf(dev->name, "GRETH_10/100");
	}

	/* initiate PHY, select speed/duplex depending on connected PHY */
	ret = greth_init_phy(greth);
	if (ret != 0) {
		/* Failed to init PHY (timedout) */
		
		pos++;
		if ((__greth_phys[pos])) {
	        greth_error("Failed to init PHY (%d) - trying a different PHY addr\n", ret);
			goto retry;
                }
		greth_error("Giving up, phy looks dead\n");	
		return ret;
	}

#if 0
	ret = dev->write_hwaddr(dev);
	if(ret < 0) {
		/* HW Address not found in environment, Set default HW address */
		dev->enetaddr[0] = GRETH_HWADDR_0;	/* MSB */
		dev->enetaddr[1] = GRETH_HWADDR_1;
		dev->enetaddr[2] = GRETH_HWADDR_2;
		dev->enetaddr[3] = GRETH_HWADDR_3;
		dev->enetaddr[4] = GRETH_HWADDR_4;
		dev->enetaddr[5] = GRETH_HWADDR_5;	/* LSB */
		ret = dev->write_hwaddr(dev);
		if(ret < 0) {
			greth_error("Failed to set default MAC\n");
			return ret;
		}
	}
#endif

	return 0;
}

/* Uses env, uses console, uses malloc */
int greth_initialize()
{
	greth_priv *greth;
	ambapp_apbdev apbdev;
	struct eth_device *dev;

#ifdef CONFIG_GRETH_BASE
	greth_debug("Setting GRETH base addr to 0x%08X\n", CONFIG_GRETH_BASE);
	apbdev.address = CONFIG_GRETH_BASE;
	apbdev.irq = CONFIG_GRETH_IRQ;
#else
	greth_debug("Scanning for GRETH\n");
	/* Find Device & IRQ via AMBA Plug&Play information */
	if (ambapp_apb_first(VENDOR_GAISLER, GAISLER_ETHMAC, &apbdev) != 1) {
		error("greth: Scanning AMBA bus failed\n");
		return -1;	/* GRETH not found */
	}
#endif

	greth = (greth_priv *) malloc(sizeof(greth_priv));
	if ( greth == NULL ) {
		error("greth: Unable to malloc greth_priv\n");
		return -1;
	}
	dev = (struct eth_device *)malloc(sizeof(struct eth_device));
	if (dev == NULL) {
		error("greth: Unable to malloc eth_device\n");
		return -1;
	}
	memset(dev, 0, sizeof(struct eth_device));
	memset(greth, 0, sizeof(greth_priv));

	greth->regs = (greth_regs *) apbdev.address;
	greth->irq = apbdev.irq;
	greth_debug("Found GRETH at 0x%lx, irq %ld\n", (ulong) greth->regs, (ulong) greth->irq);
	dev->priv = (void *)greth;
	dev->iobase = (unsigned int)greth->regs;
	dev->init = greth_init;
	dev->halt = greth_halt;
	dev->send = greth_send;
	dev->recv = greth_recv;
	dev->write_hwaddr = greth_set_hwaddr_from_netdev;
	greth->dev = dev;

	/* Register Device to EtherNet subsystem  */

	while(1) {
		int ret = greth_reset(greth);
		if(ret == 0) 
			break;
		error("greth: Failed to initialize (%d). Retrying.\n\n", ret);
	};

	eth_register(dev);
	return greth_real_init(dev);
}


