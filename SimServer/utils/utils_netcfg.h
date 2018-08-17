#ifndef UTILS_NETCFG_INCLUDED
#define UTILS_NETCFG_INCLUDED

typedef struct {
	// network
	unsigned long	ip;
	unsigned long	netmask;
	unsigned long	gw;
	unsigned char mac[8];		// we only use first 6 bytes.
} NET_CONFIG_S;	

extern int config_ip( const char *ip_upd, int bSetOnline );
extern int config_netmask(const char *new_mask, int bSetOnline );
extern int config_gateway(const char *gateway, int bSetOnline );
extern int config_macaddr(const char *mac_new, int bSetOnline );

// get current network configuration (what you see by ifconfig command)
extern void get_network_config(NET_CONFIG_S *pconf);

// config network ip, netmask, gw, mac addr according to members in 'pnewconf'.
// if 'poldconf' is not null, only those members are changed will be processed and changed.
extern void set_network_config(NET_CONFIG_S *pnewconf, NET_CONFIG_S *poldconf);
// index shall be 0 ~ N (N is maximum number of extra IP address bind to same 'ifname'
extern int bind_extra_ip( const char *ifname, int index, unsigned long ip);

// 将网络设置恢复为出厂设置 (按控制器“RESET”键5秒后复位一些配置文件，包括网络设置）
extern void factory_default_netconf();
#endif