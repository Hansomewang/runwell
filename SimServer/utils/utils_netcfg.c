#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include "dbg_printf.h"
#include "utils_net.h"
#include "utils_file.h"
#include "utils_netcfg.h"

#define CONFIG_ONLINE_ONLY			1		// 只改变online网络配置,重启后失效
#define CONFIG_SCRIPT_ONLY			2		// 只改变ip.cfg配置文件内容（要重启才生效)
#define CONFIG_ONLINEANDSCRIPT	3		// 改变online设置和ip.cfg script，立刻生效而且以后重启也有效

#define NET_CONFIG_MODE	CONFIG_ONLINE_ONLY

extern NET_CONFIG_S  g_stNETConf;
#if NET_CONFIG_MODE==CONFIG_ONLINEANDSCRIPT
static int bConfigFileAndOnline=1;
#elif NET_CONFIG_MODE==CONFIG_SCRIPT_ONLY
static int bConfigFileAndOnline=0;
#endif
/////////////////////////////////////////////////////////////////////////////
#define FILE_NETSCRIP_OLD	"/etc/init.d/ip.cfg"
#define FILE_NETSCRIP_NEW	"/home/etc/ip.cfg"

static const char *file_netscript = NULL;
static const char *file_content = 
"ipaddr=192.168.1.135\n"
"netmask=255.255.255.0\n"
"gateway=192.168.1.1\n"
"ifaddr=06:16:bc:6f:a4:07\n\n"
"ifconfig eth0 down\n"
"ifconfig eth0 hw ether ${ifaddr}\n"
"ifconfig eth0 ${ipaddr} netmask ${netmask}\n"
"ifconfig eth0 up\n\n"
"route add default gw ${gateway}\n"
"# set broadcasting interface\n"
"route add -host 255.255.255.255 dev eth0\n"
"ifconfig lo 127.0.0.1\n";
		
static void probe_netscript()
{
	struct stat st;
	file_netscript = stat("/home/etc",&st)==0 ? FILE_NETSCRIP_NEW : FILE_NETSCRIP_OLD;
	if ( stat(file_netscript,&st)==-1 )
	{
		int fd;
		PRINTF1("NETCONFIG: 错误，找不到网络配置脚本文件, 生成默认配置文件: %s\n",file_netscript);
		fd = open(file_netscript, O_RDWR|O_CREAT, 0755);
		if ( fd != -1 )
		{
			write(fd, file_content, strlen(file_content));
			fchmod(fd, 0755);
			close(fd);
		}
	}
}
	
void factory_default_netconf()
{
	unsigned char mac[6];
	if ( !file_netscript ) probe_netscript();
	unlink(file_netscript);
	probe_netscript();
	// replace mac address in ip.cfg we just wrote.
	if ( get_mac_addr(NULL,mac)== 0 )
	{
		char chaddr[24];
		sprintf(chaddr, "%02x:%02x:%02x:%02x:%02x:%02x", 
			mac[0],mac[1],mac[2],	mac[3],mac[4],mac[5] );
		file_replace_str_after( file_netscript, "ifaddr=", chaddr, 1 );
	}
}

int config_ip( const char *ip_upd, int updonline )
{
	PRINTF1("config camera IP as %s...\n", ip_upd );
	if ( !file_netscript ) probe_netscript();
	// update ip in rcS
	if ( file_replace_str_after( file_netscript, "ipaddr=", ip_upd, 0 ) == -1 )
	{
		PRINTF1("\t cannot locate key string 'ioaddr=' in %s. --> failed.\n", file_netscript);
		return -1;
	}
	else
			PRINTF1("\t ip string modified in %s. OK.\n", file_netscript);
	if (updonline)
	{
		SOCKADDR_IN saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = (unsigned long)inet_addr(ip_upd);
		set_netaddr(NULL, &saddr);
	}
	return 0;
}
	
int config_netmask(const char *new_mask, int updonline )
{
	PRINTF1("config_netmask: set netmask to %s...\n", new_mask);
	if ( !file_netscript ) probe_netscript();
	file_replace_str_after(file_netscript, "netmask=", new_mask, 0 );
	if ( updonline )
		ifconfig(NULL,NULL,new_mask,NULL);			
	return 0;
}

int config_gateway(const char *gateway, int updonline )
{
	PRINTF1("config_gateway: replace gatway to=%s...\n", gateway );
	file_replace_str_after( file_netscript, "gateway=", gateway, 0 );
	if ( updonline )
	{		
		ifconfig(NULL,NULL,NULL,gateway);
	}	
	return 0;
}
	
int config_macaddr(const char *mac_new, int updonline )
{
	PRINTF1("config_macaddr: replace gatway to=%s...\n", mac_new );
	if ( !file_netscript ) probe_netscript();
		
	if ( file_replace_str_after( file_netscript, "ifaddr=", mac_new, 1 )==-1 )
	{
		unsigned char mac_addr[6];
		char mac_now[24];
		
		get_mac_addr( NULL, mac_addr );
		sprintf(mac_now, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5] );

		PRINTF1("config_macaddr - change mac addr from %s to %s\n", mac_now, mac_new);
		file_replace_str( file_netscript, mac_now, mac_new, 1 );
	}
	if ( updonline )
	{ 
		unsigned char hwaddr[6];
		unsigned int mac[6];
		int i;
		
		sscanf(mac_new, "%02x:%02x:%02x:%02x:%02x:%02x", mac, mac+1, mac+2, mac+3, mac+4, mac+5 );
		for(i=0; i<6; i++)
			hwaddr[i] = (unsigned char)mac[i];
		set_mac_addr(NULL, hwaddr ); 
	}
	return 0;
}

void get_network_config(NET_CONFIG_S *pconf)
{
	SOCKADDR_IN addr;
	// initial network config data
	get_netaddr(NULL, &addr);
	pconf->ip = SOCKADDR_IN_ADDR(addr);
	get_netmask(NULL,&addr);
	pconf->netmask = SOCKADDR_IN_ADDR(addr);
	pconf->gw = get_gatewayaddr(NULL,NULL);	
	get_mac_addr(NULL,pconf->mac);
}

void set_network_config(NET_CONFIG_S *pnewconf, NET_CONFIG_S *poldconf)
{
	const char *ifname = get_ifadapter_name();
	if ( poldconf==NULL || pnewconf->ip != poldconf->ip )
	{
#if NET_CONFIG_MODE==CONFIG_ONLINE_ONLY		
		SOCKADDR_IN saddr;
		saddr.sin_addr.s_addr = pnewconf->ip;
		set_netaddr(ifname,&saddr);
#else
		config_ip(INET_NTOA(pnewconf->ip),bConfigFileAndOnline);
#endif		
		PRINTF1("NETCONF - set ip address as: %s\n", INET_NTOA(pnewconf->ip));
	}
	
	if ( poldconf==NULL || pnewconf->netmask != poldconf->netmask )
	{		
#if NET_CONFIG_MODE==CONFIG_ONLINE_ONLY		
		SOCKADDR_IN saddr;
		saddr.sin_addr.s_addr = pnewconf->netmask;
		set_netmask(ifname,&saddr);
#else	
		config_netmask(INET_NTOA(pnewconf->mask),bConfigFileAndOnline);
#endif		
		PRINTF1("NETCONF - set netmask address as: %s\n", INET_NTOA(pnewconf->netmask));
	}
	
	if ( poldconf==NULL || pnewconf->gw != poldconf->gw )
	{		
#if NET_CONFIG_MODE==CONFIG_ONLINE_ONLY		
		SOCKADDR_IN gwaddr;
		gwaddr.sin_addr.s_addr = INADDR_ANY;
		del_default_gateway(ifname, &gwaddr);
		gwaddr.sin_addr.s_addr = pnewconf->gw;
		add_default_gateway(ifname, &gwaddr);
#else 		
		config_gateway(INET_NTOA(pnewconf->gw), bConfigFileAndOnline);
#endif		
		PRINTF1("NETCONF - set default gateway address as: %s\n", INET_NTOA(pnewconf->gw));
	}
	
	if ( poldconf==NULL || memcmp(pnewconf->mac, poldconf->mac, 6) != 0 )
	{
#if NET_CONFIG_MODE==CONFIG_ONLINE_ONLY		
		set_mac_addr(ifname, pnewconf->mac);
#else	
		char chaddr[24];
		strcpy(chaddr, mac2string(pnewconf->mac));
		config_macaddr(chaddr,bConfigFileAndOnline); 			
#endif				
		PRINTF1("NETCONF - set mac address as: %s\n", mac2string(pnewconf->mac));
	}
}

int bind_extra_ip(const char *ifname, int index, unsigned long ip)
{
	char dev_name[32];
	SOCKADDR_IN  saddr;
	
	if ( ifname==NULL )
		ifname = get_ifadapter_name();
	sprintf(dev_name, "%s:%d", ifname, index);
	saddr.sin_addr.s_addr = ip;
	return set_netaddr(dev_name, &saddr);
}
