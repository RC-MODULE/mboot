/*
 * (C) Copyright 2000
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>

void net_init_task_def(struct NetTask *task, proto_t p)
{
	memset(task, 0, sizeof(struct NetTask));
	task->proto = p;

	getenv_ul("loadaddr", &task->loadaddr, CONFIG_LOADADDR);
	getenv_s("bootfile", &task->bootfile[0], CONFIG_BOOTFILE);
}

void net_init_task_args(struct NetTask *task, proto_t p, int argc, char * const argv[])
{
	net_init_task_def(task, p);

	switch (argc) {
		case 2: {
			ulong addr;
			char* end;
			/* Only one arg accept two forms: Just load address, or just boot file
			 * name. */
			addr = simple_strtoul(argv[1], &end, 16);
			if (end == (argv[1] + strlen(argv[1]))) {
				task->loadaddr = addr;
			}
			else {
				strncpy_s(task->bootfile, argv[1], MAXPATH);
			}
			break;
		}

		case 3:
			task->loadaddr = simple_strtoul(argv[1], NULL, 16);
			strncpy_s(task->bootfile, argv[2], MAXPATH);
			break;

		default:
			break;
	}
}

extern int do_bootm (cmd_tbl_t *, int, int, char * const []);

static int netboot_common (struct NetTask *task, cmd_tbl_t *, int , char * const []);

int do_bootp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;
	net_init_task_args(&task, BOOTP, argc, argv);
	return netboot_common (&task, cmdtp, argc, argv);
}

U_BOOT_CMD(
	bootp,	3,	1,	do_bootp,
	"boot image via network using BOOTP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);

int do_tftpb (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;
	net_init_task_args(&task, TFTP, argc, argv);
	task.u.tftp.verbose = 1;
	return netboot_common (&task, cmdtp, argc, argv);
}

U_BOOT_CMD(
	tftpboot,	3,	1,	do_tftpb,
	"boot image via network using TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);

#ifdef CONFIG_CMD_RARP
int do_rarpb (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;
	net_init_task_args(&task, RARP, argc, argv);
	return netboot_common (&task, cmdtp, argc, argv);
}

U_BOOT_CMD(
	rarpboot,	3,	1,	do_rarpb,
	"boot image via network using RARP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

#if defined(CONFIG_CMD_DHCP)
int do_dhcp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;
	net_init_task_args(&task, DHCP, argc, argv);
	return netboot_common (&task, cmdtp, argc, argv);
}

U_BOOT_CMD(
	dhcp,	3,	1,	do_dhcp,
	"boot image via network using DHCP/TFTP protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

#if defined(CONFIG_CMD_NFS)
int do_nfs (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;
	net_init_task_args(&task, NFS, argc, argv);
	return netboot_common (&task, cmdtp, argc, argv);
}

U_BOOT_CMD(
	nfs,	3,	1,	do_nfs,
	"boot image via network using NFS protocol",
	"[loadAddress] [[hostIPaddr:]bootfilename]"
);
#endif

static void netboot_update_env (struct NetTask *task)
{
	char tmp[22];

	setenv_ul("loadaddr", "0x%08lX", task->loadaddr);
	setenv("bootfile", task->bootfile);

	if (NetOurGatewayIP) {
		ip_to_string (NetOurGatewayIP, tmp);
		setenv ("gatewayip", tmp);
	}

	if (NetOurSubnetMask) {
		ip_to_string (NetOurSubnetMask, tmp);
		setenv ("netmask", tmp);
	}

	if (NetOurHostName[0])
		setenv ("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv ("rootpath", NetOurRootPath);

	if (NetOurIP) {
		ip_to_string (NetOurIP, tmp);
		setenv ("ipaddr", tmp);
	}

	if (NetServerIP) {
		ip_to_string (NetServerIP, tmp);
		setenv ("serverip", tmp);
	}

	if (NetOurDNSIP) {
		ip_to_string (NetOurDNSIP, tmp);
		setenv ("dnsip", tmp);
	}
#if defined(CONFIG_BOOTP_DNS2)
	if (NetOurDNS2IP) {
		ip_to_string (NetOurDNS2IP, tmp);
		setenv ("dnsip2", tmp);
	}
#endif
	if (NetOurNISDomain[0])
		setenv ("domain", NetOurNISDomain);

#if defined(CONFIG_CMD_SNTP) \
    && defined(CONFIG_BOOTP_TIMEOFFSET)
	if (NetTimeOffset) {
		sprintf (tmp, "%d", NetTimeOffset);
		setenv ("timeoffset", tmp);
	}
#endif
#if defined(CONFIG_CMD_SNTP) \
    && defined(CONFIG_BOOTP_NTPSERVER)
	if (NetNtpServerIP) {
		ip_to_string (NetNtpServerIP, tmp);
		setenv ("ntpserverip", tmp);
	}
#endif
}

static ulong load_addr;

static int
netboot_common (struct NetTask *task, cmd_tbl_t *cmdtp, int argc, char * const argv[])
{
	const char *s;
	int size;
	int ret;

	ret = NetLoop(task);
	if (ret < 0) {
		printf("NET transfer failed: ret %d\n", ret);
		return ret;
	}
	size = ret;

	netboot_update_env(task);

	if (size == 0) {
		printf("NET warning: zero-length file received\n");
		return 0;
	}

	/* flush cache */
	flush_cache(task->loadaddr, size);

	/* Loading ok, check if we should attempt an auto-start */
	if (((s = getenv("autostart")) != NULL) && (strcmp(s,"yes") == 0)) {
		char *local_args[2];

		local_args[0] = argv[0];
		local_args[1] = NULL;

		printf ("NET automatic boot: addr 0x%08lX subcmd %s\n",
			load_addr, argv[0]);
		ret = do_bootm (cmdtp, 0, 1, local_args);
	}

	return ret;
}

#if defined(CONFIG_CMD_PING)
int do_ping (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct NetTask task;

	if (argc < 2)
		return -1;

	NetPingIP = string_to_ip(argv[1]);
	if (NetPingIP == 0)
		return cmd_usage(cmdtp);

	net_init_task_args(&task, PING, argc, argv);
	if (NetLoop(&task) < 0) {
		printf("ping failed; host %s is not alive\n", argv[1]);
		return 1;
	}

	printf("ping host %s is alive\n", argv[1]);

	return 0;
}

U_BOOT_CMD(
	ping,	2,	1,	do_ping,
	"send ICMP ECHO_REQUEST to network host",
	"pingAddress"
);
#endif

#if defined(CONFIG_CMD_CDP)
static void cdp_update_env(void)
{
	char tmp[16];

	if (CDPApplianceVLAN != htons(-1)) {
		printf("CDP offered appliance VLAN %d\n", ntohs(CDPApplianceVLAN));
		VLAN_to_string(CDPApplianceVLAN, tmp);
		setenv("vlan", tmp);
		NetOurVLAN = CDPApplianceVLAN;
	}

	if (CDPNativeVLAN != htons(-1)) {
		printf("CDP offered native VLAN %d\n", ntohs(CDPNativeVLAN));
		VLAN_to_string(CDPNativeVLAN, tmp);
		setenv("nvlan", tmp);
		NetOurNativeVLAN = CDPNativeVLAN;
	}

}

int do_cdp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int r;

	r = NetLoop(CDP);
	if (r < 0) {
		printf("cdp failed; perhaps not a CISCO switch?\n");
		return 1;
	}

	cdp_update_env();

	return 0;
}

U_BOOT_CMD(
	cdp,	1,	1,	do_cdp,
	"Perform CDP network configuration",
);
#endif

#if defined(CONFIG_CMD_SNTP)
int do_sntp (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *toff;

	if (argc < 2) {
		NetNtpServerIP = getenv_IPaddr ("ntpserverip");
		if (NetNtpServerIP == 0) {
			printf ("ntpserverip not set\n");
			return (1);
		}
	} else {
		NetNtpServerIP = string_to_ip(argv[1]);
		if (NetNtpServerIP == 0) {
			printf ("Bad NTP server IP address\n");
			return (1);
		}
	}

	toff = getenv ("timeoffset");
	if (toff == NULL) NetTimeOffset = 0;
	else NetTimeOffset = simple_strtol (toff, NULL, 10);

	if (NetLoop(SNTP) < 0) {
		printf("SNTP failed: host %s not responding\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	sntp,	2,	1,	do_sntp,
	"synchronize RTC via network",
	"[NTP server IP]\n"
);
#endif

#if defined(CONFIG_CMD_DNS)
int do_dns(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 1)
		return cmd_usage(cmdtp);

	/*
	 * We should check for a valid hostname:
	 * - Each label must be between 1 and 63 characters long
	 * - the entire hostname has a maximum of 255 characters
	 * - only the ASCII letters 'a' through 'z' (case-insensitive),
	 *   the digits '0' through '9', and the hyphen
	 * - cannot begin or end with a hyphen
	 * - no other symbols, punctuation characters, or blank spaces are
	 *   permitted
	 * but hey - this is a minimalist implmentation, so only check length
	 * and let the name server deal with things.
	 */
	if (strlen(argv[1]) >= 255) {
		printf("dns error: hostname too long\n");
		return 1;
	}

	NetDNSResolve = argv[1];

	if (argc == 3)
		NetDNSenvvar = argv[2];
	else
		NetDNSenvvar = NULL;

	if (NetLoop(DNS) < 0) {
		printf("dns lookup of %s failed, check setup\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	dns,	3,	1,	do_dns,
	"lookup the IP of a hostname",
	"hostname [envvar]"
);

#endif	/* CONFIG_CMD_DNS */
