/*
 * (C) Copyright 2010
 * RC Module
 * Sergey Mironov <ierton@gmail.com>
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

/*
 * ARM Dual timer-specific code,
 * see AMBA Design Kit Technical Reference Manual (pdf)
 *
 * Those timers can also be found on ARM's RealView Emulation Board.
 */

#include <common.h>

#define TIMER_TIMER1LOAD				0x000
#define TIMER_TIMER1VALUE				0x004
#define TIMER_TIMER1CONTROL				0x008
#define TIMER_TIMER1CLEAR				0x00C
#define TIMER_TIMER1RIS				 	0x010
#define TIMER_TIMER1MIS				 	0x014
#define TIMER_TIMER1BGLOAD				0x018

// Timer control register, TimerXControl

#define TIMER_ENABLE					0x80// Enable bit:
											// 0 = Timer disabled (default) 
											// 1 = Timer enabled.
#define TIMER_PERIOD_MODE				0x40// Mode bit:
											// 0 = Timer is in free-running mode (default)
											// 1 = Timer is in periodic mode.
#define TIMER_INTERRUPT_ENABLE			0x20// Interrupt Enable bit:
											// 0 = TimerInterrupt disabled
											// 1 = TimerInterrupt enabled (default)
											//(0x10) bit 4 - RESERVED Reserved bit, 
											//       do not modify, and ignore on read

#define TIMER_PRE1						0x00// 00 = 0 stages of prescale, clock is 
											//        divided by 1 (default)
#define TIMER_PRE16						0x04// 01 = 4 stages of prescale, clock is divided by 16
#define TIMER_PRE256					0x08// 10 = 8 stages of prescale, clock is divided by 256
											// Prescale bits:
											// 00 = 0 stages of prescale, 
											//        clock is divided by 1 (default)
											// 01 = 4 stages of prescale, clock is 
											//        divided by 16 10 = 8 stages of prescale, 
											//        clock is divided by 256 
											// 11 = Undefined, do not use.
#define TIMER_SIZE32					0x02// Selects 16/32 bit counter operation: 
											// 1 = 32-bit counter,
											// 0 = 16-bit counter (default))
#define TIMER_ONESHOT					1	// Selects one-shot or wrapping counter mode:
											// 0 = wrapping mode (default) 
											// 1 = one-shot mode/*}}}*/

#define TIMER_RELOAD   0xFFFFFFFFUL

inline unsigned long timer_readl(unsigned long addr)
{
	return (*(volatile unsigned long *)(CONFIG_SYS_TIMERBASE+addr));
}

inline void timer_writel(unsigned long val, unsigned long addr)
{
	(*(volatile unsigned long *)(CONFIG_SYS_TIMERBASE+addr)) = val;
}

inline ulong tics_to_usec(ulong tics)
{
	return tics / CONFIG_SYS_TIMER_CLK_MHZ;
}

inline ulong tics_to_msec(ulong tics)
{
	return tics_to_usec(tics) / 1000;
}

/* Time in ms */
static ulong g_time_ms;
/* Last seen value of timer, in tics */
static ulong g_last_tics;

void reset_timer_masked(void)
{
	g_last_tics = timer_readl(TIMER_TIMER1VALUE);
	g_time_ms = 0;	       	    
}

void reset_timer(void)
{
	reset_timer_masked();
}

void uemd_timer_init(void)
{
	timer_writel(TIMER_RELOAD, TIMER_TIMER1LOAD);	
	timer_writel(TIMER_ENABLE | TIMER_SIZE32, TIMER_TIMER1CONTROL);
	reset_timer_masked();
}

void update_global_time(ulong curr_tics)
{
	ulong delta_time_ms;

	if (g_last_tics >= curr_tics) {
		delta_time_ms = tics_to_msec(g_last_tics - curr_tics);
	} 
	else {			
		delta_time_ms = tics_to_msec(g_last_tics + (TIMER_RELOAD - curr_tics));
	}

	if(delta_time_ms > 0) {
		g_last_tics = curr_tics;
		g_time_ms += delta_time_ms;
	}
}

/*
 * Returns current time, in ms. 
 *
 * Function should be called once every (1/CONFIG_SYS_TIMER_CLK_MHZ) to
 * keep it precise.
 */
ulong get_timer_masked(void)
{
	update_global_time( timer_readl(TIMER_TIMER1VALUE) );
	return g_time_ms;
}

/*
 * Returns current time (in ms), relative to base. 
 *
 * See get_timer_masked() description
 */
ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

/* 
 * Sleeps for usec useconds. Safe to call in timeout loop.
 */
void __udelay (unsigned long usec)
{
	ulong tics, last_tics, delta;
	ulong time_usec = 0;

	last_tics = timer_readl(TIMER_TIMER1VALUE);
	while ( time_usec <  usec ) {

		tics = timer_readl(TIMER_TIMER1VALUE);

		if (last_tics >= tics) {
			delta = tics_to_usec(last_tics - tics);
		} 
		else {			
			delta = tics_to_usec(last_tics + TIMER_RELOAD - tics);
		}

		update_global_time( tics );

		if(delta > 0) {
			time_usec += delta;
			last_tics = tics;
		}
	}
}

