#include "netscan.h"
#include "cJSON.h"

#define INTERFACENUM 8
struct interface_element{
	char *name;
	char mac[18];
	int port;
};

#define SUPPORT_STREAMBOOST
#ifdef SUPPORT_STREAMBOOST
#define SB_INFO_STATE_1 1
#define SB_INFO_STATE_2 2

typedef enum {
	TYPE_SOAP_OLD = 0, /* This type is for SOAP API */
	TYPE_AMAZON_KINDLE,
	TYPE_ANDROID_DEVICE,
	TYPE_ANDROID_PHONE,
	TYPE_ANDROID_TABLET,
	TYPE_APPLE_AIRPORT_EXPRESS,
	TYPE_BLU_RAY_PLAYER,
	TYPE_BRIDGE,
	TYPE_CABLE_STB,
	TYPE_CAMERA,
	TYPE_ROUTER,
	TYPE_DVR,
	TYPE_GAMING_CONSOLE,
	TYPE_IMAC,
	TYPE_IPAD,
	TYPE_IPAD_MINI,
	TYPE_IPONE_5_5S_5C,
	TYPE_IPHONE,
	TYPE_IPOD_TOUCH,
	TYPE_LINUX_PC,
	TYPE_MAC_MINI,
	TYPE_MAC_PRO,
	TYPE_MAC_BOOK,
	TYPE_MEDIA_DEVICE,
	TYPE_NETWORK_DEVICE,
	TYPE_OTHER_STB,
	TYPE_POWERLINE,
	TYPE_PRINTER,
	TYPE_REPEATER,
	TYPE_SATELLITE_STB,
	TYPE_SCANNER,
	TYPE_SLING_BOX,
	TYPE_SMART_PHONE,
	TYPE_STORAGE_NAS,
	TYPE_SWITCH,
	TYPE_TV,
	TYPE_TABLET,
	TYPE_UNIX_PC,
	TYPE_WINDOWS_PC,
	TYPE_SURFACE,
	TYPE_WIFI_EXTERNDER,
	TYPE_APPLE_TV,
	TYPE_AV_RECEIVER,
	TYPE_CHROMECAST,
	TYPE_GOOGLE_NEXUS_5,
	TYPE_GOOGLE_NEXUS_7,
	TYPE_GOOGLE_NEXUS_10,
	TYPE_OTHERS,
	TYPE_WN1000RP,
	TYPE_WN2500RP,
	TYPE_VOIP,
	TYPE_IPHONE_6_6S,
}netgear_devtype_t;

struct sb2netgear_devtype_mapping_table{
	char *sb_devtype;
	netgear_devtype_t devtype;
};

struct sb2netgear_priority_map_table{
	int  sb_priority;
	char *netgear_priority;
};

struct sb2netgear_priority_map_table priority_map_table[]={
	{10, "HIGHEST"},
	{20, "HIGH"   },
	{30, "MEDIUM" },
	{40, "LOW"    },

	{0 , NULL     }
};

struct device_list{
	char mac[18];
	char priority[8];
	netgear_devtype_t type;
	char host[MAX_HOSTNAME_LEN + 1];
	struct device_list *next;
};

struct sb2netgear_devtype_mapping_table devtype_mapping_table[] = {
	{"Kindle Fire", TYPE_AMAZON_KINDLE},
	{"Kindle Fire HDX", TYPE_AMAZON_KINDLE},
	{"Kindle Fire HD", TYPE_AMAZON_KINDLE},
	{"Android", TYPE_ANDROID_DEVICE},
	{"Samsung Device", TYPE_ANDROID_DEVICE},
	{"HTC Android Device", TYPE_ANDROID_DEVICE},
	{"Droid Mini", TYPE_ANDROID_PHONE},
	{"HTC Amaze 4G", TYPE_ANDROID_PHONE},
	{"HTC Desire", TYPE_ANDROID_PHONE},
	{"HTC Droid DNA", TYPE_ANDROID_PHONE},
	{"HTC J", TYPE_ANDROID_PHONE},
	{"HTC One", TYPE_ANDROID_PHONE},
	{"HTC One S", TYPE_ANDROID_PHONE},
	{"HTC One M8", TYPE_ANDROID_PHONE},
	{"HTC One X", TYPE_ANDROID_PHONE},
	{"HTC Rezound", TYPE_ANDROID_PHONE},
	{"HTC Shift 4G", TYPE_ANDROID_PHONE},
	{"HTC ThunderBolt", TYPE_ANDROID_PHONE},
	{"HomeWizard", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy Nexus", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy Note", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy Note II", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy Note 3", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy S", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy S II", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy S III", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy S4", TYPE_ANDROID_PHONE},
	{"Samsung Galaxy S4 Mini", TYPE_ANDROID_PHONE},
	{"Xperia Z", TYPE_ANDROID_PHONE},
	{"Xperia Zl", TYPE_ANDROID_PHONE},
	{"Xperia Z2", TYPE_ANDROID_PHONE},
	{"Xperia go", TYPE_ANDROID_PHONE},
	{"Moto X", TYPE_ANDROID_PHONE},
	{"LG G2", TYPE_ANDROID_PHONE},
	{"LG G Pro 2", TYPE_ANDROID_PHONE},
	{"Moto G", TYPE_ANDROID_PHONE},
	{"Android Tablet", TYPE_ANDROID_TABLET},
	{"Asus MeMOPad", TYPE_ANDROID_TABLET},
	{"Samsung Galaxy Note Tablet", TYPE_ANDROID_TABLET},
	{"Samsung Galaxy Tab", TYPE_ANDROID_TABLET},
	{"Apple Airport Express", TYPE_APPLE_AIRPORT_EXPRESS},
	{"Sony BD Player", TYPE_BLU_RAY_PLAYER},
	{"Bridge", TYPE_BRIDGE},
	{"Cable STB", TYPE_CABLE_STB},
	{"Camera", TYPE_CAMERA},
	{"D-Link", TYPE_ROUTER},
	{"D-Link DSM-312", TYPE_ROUTER},
	{"Netgear", TYPE_ROUTER},
	{"DVR", TYPE_DVR},
	{"Swann DVR", TYPE_DVR},
	{"Nintendo", TYPE_GAMING_CONSOLE},
	{"Nintendo 3DS", TYPE_GAMING_CONSOLE},
	{"Nintendo Wii", TYPE_GAMING_CONSOLE},
	{"Nintendo Wii U", TYPE_GAMING_CONSOLE},
	{"NVIDIA SHIELD", TYPE_GAMING_CONSOLE},
	{"Playstation", TYPE_GAMING_CONSOLE},
	{"Playstation 3", TYPE_GAMING_CONSOLE},
	{"Playstation 4", TYPE_GAMING_CONSOLE},
	{"Playstation Vita", TYPE_GAMING_CONSOLE},
	{"Xbox One", TYPE_GAMING_CONSOLE},
	{"Xbox 360", TYPE_GAMING_CONSOLE},
	{"iMac", TYPE_IMAC},
	{"iPad", TYPE_IPAD},
	{"iPad 2", TYPE_IPAD},
	{"iPad Air", TYPE_IPAD},
	{"iPad Retina", TYPE_IPAD},
	{"iPad Mini Retina", TYPE_IPAD_MINI},
 	{"iPad Mini", TYPE_IPAD_MINI},
	{"iPhone 5", TYPE_IPONE_5_5S_5C},
	{"iPhone 5c", TYPE_IPONE_5_5S_5C},
	{"iPhone 5s", TYPE_IPONE_5_5S_5C},
	{"iPhone", TYPE_IPHONE},
	{"iPhone 4", TYPE_IPHONE},
	{"iPhone 4s", TYPE_IPHONE},
	{"iPod Touch", TYPE_IPOD_TOUCH},
	{"Linux", TYPE_LINUX_PC},
	{"Mac mini",TYPE_MAC_MINI},
	{"Mac Pro", TYPE_MAC_PRO},
	{"Mac OS X", TYPE_MAC_BOOK},
	{"Roku", TYPE_MEDIA_DEVICE},
	{"SunPower Device", TYPE_MEDIA_DEVICE},
	{"Tivo", TYPE_MEDIA_DEVICE},
	{"Slingbox", TYPE_MEDIA_DEVICE},
	{"Yamaha AVR", TYPE_MEDIA_DEVICE},
	{"Daemon Device", TYPE_MEDIA_DEVICE},
	{"Marantz Device", TYPE_MEDIA_DEVICE},
	{"Sirius XM Radio", TYPE_MEDIA_DEVICE},
	{"Sony Receiver", TYPE_MEDIA_DEVICE},
	{"Demon Device", TYPE_MEDIA_DEVICE},
	{"Cisco", TYPE_NETWORK_DEVICE},
	{"Logitech Media Server", TYPE_NETWORK_DEVICE},
	{"Magicjack", TYPE_NETWORK_DEVICE},
	{"Sonos", TYPE_NETWORK_DEVICE},
	{"Boxee", TYPE_OTHER_STB},
	{"D-Link DSM-312 MovieNite P1", TYPE_OTHER_STB},
	{"HDHomeRun", TYPE_OTHER_STB},
	{"Motorola TV Set-Top", TYPE_OTHER_STB},
	{"Powerline", TYPE_POWERLINE},
	{"HP InKjet", TYPE_PRINTER},
	{"Repeater", TYPE_REPEATER},
	{"Satellite STB", TYPE_SATELLITE_STB},
	{"Scanner", TYPE_SCANNER},
	{"Sling box", TYPE_SLING_BOX},
	{"BlackBerry", TYPE_SMART_PHONE},
	{"BlackBerry 9220", TYPE_SMART_PHONE},
	{"Nokia E72", TYPE_SMART_PHONE},
	{"Nokia Lumia", TYPE_SMART_PHONE},
	{"Nokia Lumia 520", TYPE_SMART_PHONE},
	{"Nokia Lumia 521", TYPE_SMART_PHONE},
	{"Nokia Lumia 822", TYPE_SMART_PHONE},
	{"Nokia Lumia 920", TYPE_SMART_PHONE},
	{"Nokia Lumia 928", TYPE_SMART_PHONE},
	{"Panasonic Smart Device", TYPE_SMART_PHONE},
	{"Symbian Phone", TYPE_SMART_PHONE},
	{"Windows Phone", TYPE_SMART_PHONE},
	{"Droid4", TYPE_SMART_PHONE},
	{"Droid Bionic", TYPE_SMART_PHONE},
	{"Droid M", TYPE_SMART_PHONE},
	{"Droid Razr", TYPE_SMART_PHONE},
	{"Droid Ultra", TYPE_SMART_PHONE},
	{"Storage (NAS)", TYPE_STORAGE_NAS},
	{"Seagate Device", TYPE_STORAGE_NAS},
	{"Synology Device", TYPE_STORAGE_NAS},
	{"Switch", TYPE_SWITCH},
	{"Amazon Fire TV", TYPE_TV},
	{"Google TV", TYPE_TV},
	{"Sharp Aquos LE830", TYPE_TV},
	{"Sharp Aquos TV", TYPE_TV},
	{"Sony KDL-55HX850", TYPE_TV},
	{"Sony TV", TYPE_TV},
	{"Vizio Smart Device", TYPE_TV},
	{"WD TV", TYPE_TV},
	{"DirectTV Device", TYPE_TV},
	{"Netgear NeoTV", TYPE_TV},
	{"Netgear NeoTV NTV300", TYPE_TV},
	{"Netgear NeoTV Prime GTV100", TYPE_TV},
	{"Samsung Smart TV", TYPE_TV},
	{"Sony - KDL-55W900A", TYPE_TV},
	{"Tablet", TYPE_TABLET},
	{"Nook", TYPE_TABLET},
	{"FreeBSD", TYPE_UNIX_PC},
	{"Windows", TYPE_WINDOWS_PC},
	{"Windows RT", TYPE_SURFACE},
	{"Wifi Extender", TYPE_WIFI_EXTERNDER},
	{"Apple TV", TYPE_APPLE_TV},
	{"AV Receiver", TYPE_AV_RECEIVER},
	{"Chromecast", TYPE_CHROMECAST},
	{"Nexus 4", TYPE_GOOGLE_NEXUS_5},
	{"Nexus 5", TYPE_GOOGLE_NEXUS_5},
	{"Nexus 7", TYPE_GOOGLE_NEXUS_7},
	{"Nexus 10", TYPE_GOOGLE_NEXUS_10},
	{"Apple", TYPE_OTHERS},
	{"Kodak Smart Device", TYPE_OTHERS},
	{"La Crosse Technology", TYPE_OTHERS},
	{"Linksys Phone Adapter", TYPE_OTHERS},
	{"Nest", TYPE_OTHERS},
	{"Obihai", TYPE_OTHERS},
	{"Obihai VoIP Adapter", TYPE_OTHERS},
	{"Philips Hue", TYPE_OTHERS},
	{"Unknown", TYPE_OTHERS},
	{"ADEMCO", TYPE_OTHERS},
	{"Chrome OS", TYPE_OTHERS},
	{"Frontier Silicon Radio", TYPE_OTHERS},
	{"HP Device", TYPE_OTHERS},
	{"Vonage Phone Adapter", TYPE_VOIP},
	{"Ooma Telo", TYPE_VOIP},
	{"iPhone 6", TYPE_IPHONE_6_6S},
	{"iPhone 6s", TYPE_IPHONE_6_6S},
	{"iPhone 6s Plus", TYPE_IPHONE_6_6S},
	{NULL, TYPE_OTHERS}
};

inline netgear_devtype_t get_netgear_devtype(char * typename)
{
	int i = 0;
	char *sb_devtype;

	for (sb_devtype = devtype_mapping_table[i].sb_devtype; (sb_devtype = devtype_mapping_table[i].sb_devtype) != NULL; ++i)
		if (strcasecmp(sb_devtype, typename) == 0)
			return devtype_mapping_table[i].devtype;

	return TYPE_OTHERS;
}

int sb_priority_2_netgear_prioriry(int sb_priority, char *netgear_priority)
{
	int i;

	for(i = 0; priority_map_table[i].netgear_priority != NULL; i++){
		if(sb_priority == priority_map_table[i].sb_priority){
			strcpy(netgear_priority, priority_map_table[i].netgear_priority);
			return 0;
		}
	}
	return -1;
}

#define DELETE_LAST_CHAR(str) str[strlen(str) - 1] = 0

static int priority_for_unknow_dev;

void update_streamboost_info(int state, uint8 *mac, struct in_addr ip, char *name, netgear_devtype_t type, double down, double up, long long epoch, int priority);

void get_streamboost_nodes_info(int state)
{
	FILE* fp;

	long long down, up, epoch;
	char mac_s[32];
	struct ether_addr mac;
	char ip_s[128]; /* maybe IPv6 IP address */
	struct in_addr ip;
	char name[512];
	char type_s[128];
	netgear_devtype_t type;
	char default_prio_s[32], priority_s[32];
	int priority = 30; /* MEDIUM = 30 */

	if (state == SB_INFO_STATE_2)
		priority_for_unknow_dev = 0;
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("REQUEST_URI", "/cgi-bin/ozker/api/nodes", 1);
	fp = popen("/usr/bin/cgi-fcgi -bind -connect 127.0.0.1:9000 | sed \'1,2d\' | jq \'.nodes[] | .Pipeline.mac_addr, .Pipeline.ip_addr, .Pipeline.name, .Pipeline.type, .Pipeline.default_prio, .Pipeline.down, .Pipeline.up, .Pipeline.epoch, .UI.priority\'", "r");
	if (fp) {
		while (1) {
			down = 0;
			up = 0;
			epoch = 0;
			memset(mac_s, 0, sizeof(mac_s));
			memset(&mac, 0, sizeof(mac));
			memset(name, 0, sizeof(name));
			memset(type_s, 0, sizeof(type_s));
			memset(priority_s, 0, sizeof(priority_s));
			if (fgets(mac_s, sizeof(mac_s), fp) == NULL)
				break;
			DELETE_LAST_CHAR(mac_s);
			if (ether_aton_r(mac_s, &mac) == NULL) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			if (fgets(ip_s, sizeof(ip_s), fp) == NULL)
				break;
			DELETE_LAST_CHAR(ip_s);
			if (inet_aton(ip_s, &ip) == 0) {
				/* maybe a IPv6 address, we should continue to get rest of node */
				ip.s_addr = 0;
			}
			if (fgets(name, sizeof(name), fp) == NULL) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			DELETE_LAST_CHAR(name);
			if (fgets(type_s, sizeof(type_s), fp) == NULL) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			DELETE_LAST_CHAR(type_s);
			type = get_netgear_devtype(type_s);
			if (fgets(default_prio_s, sizeof(default_prio_s), fp) == NULL) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			DELETE_LAST_CHAR(default_prio_s);
			if (fscanf(fp, "%lld\n", &down) != 1) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			if (fscanf(fp, "%lld\n", &up) != 1) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			if (fscanf(fp, "%lld\n", &epoch) != 1) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			if (fgets(priority_s, sizeof(priority_s), fp) == NULL) {
				printf("should not run into %s %d\n", __FILE__, __LINE__);
				break;
			}
			DELETE_LAST_CHAR(priority_s);
			if (strcmp("null", priority_s) == 0)
				priority = atoi(default_prio_s);
			else
				priority = atoi(priority_s);
			if (ip.s_addr != 0)
				update_streamboost_info(state, mac.ether_addr_octet, ip, name, type, (double)down, (double)up, epoch, priority);
		}
		pclose(fp);
	}
}
void read_device_list(struct device_list *head)
{
	int j;
	char name[16], *val, *mac, *priority, *type, *host;
	struct device_list *p1, *p2;

	head->next = NULL;
	p1 = head;
	for(j = 0; ; j++){
		sprintf(name, "%s%d", "device_list",j);
		val = config_get(name);
		if(*val == '\0')
			break;

		mac = strtok(val, " ");
		priority = strtok(NULL, " ");
		type = strtok(NULL, " ");
		/* Host name can be NULL(means clear the custom name and use the default name )
		 * or includes one or more SPACE characters
		 */
		host = type + strlen(type) + 1;
		if(mac == NULL || priority == NULL || type == NULL)
			continue;

		p2  = malloc(sizeof(struct device_list));
		if(!p2)
			break; /*or make head=NULL and return ?*/

		strncpy(p2->mac, mac, sizeof(p2->mac));
		strncpy(p2->priority, priority, sizeof(p2->priority));
		p2->type = atoi(type);
		strncpy(p2->host, host, sizeof(p2->host));
		p1->next = p2;
		p1       = p2;
		p2->next = NULL;
	}
}
void free_device_list(struct device_list *head)
{
	struct device_list *p1,*p2;

	p1 = head->next;
	while(p1){
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
}
#endif

struct arp_struct
{
	struct arp_struct *next;

	uint16 active;

	struct in_addr ip;
	uint8 mac[ETH_ALEN];
	ConnectionType ctype;
	unsigned int dtype;
	AttachedType atype;
	char model[MAX_MODEL_LEN + 1];
	char host[MAX_HOSTNAME_LEN + 1];

#ifdef SUPPORT_STREAMBOOST
	uint8 state;
	netgear_devtype_t type;
	double up;
	double down;
	long long epoch;
	int priority;
#endif
};

struct arp_struct *arp_tbl[NEIGH_HASHMASK + 1];

static struct arpmsg arpreq;

int init_arp_request(char *ifname)
{
	int s, i;
	struct ifreq ifr;
	struct arpmsg *arp;

	s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (s < 0)
		return 0;

	arp = &arpreq;
	memset(arp, 0, sizeof(struct arpmsg));

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	/*judge whether ARP_IFNAME has an IP,if not sleep 5s*/
	for (i=5; i >= 0; i--){
		if (i == 0)
			return 0;
		if (ioctl(s, SIOCGIFADDR, &ifr) != 0)
			sleep(5);
		else
			break;
	}
	memcpy(arp->ar_sip, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, 4);

	if (ioctl(s, SIOCGIFHWADDR, &ifr) != 0)
		return 0;
	memset(arp->h_dest, 0xFF, 6);
	memcpy(arp->h_source, ifr.ifr_hwaddr.sa_data, 6);
	arp->h_proto = htons(ETH_P_ARP);
	arp->ar_hrd = htons(ARPHRD_ETHER);
	arp->ar_pro = htons(ETH_P_IP);
	arp->ar_hln = 6;
	arp->ar_pln = 4;
	arp->ar_op = htons(ARPOP_REQUEST);
	memcpy(arp->ar_sha, ifr.ifr_hwaddr.sa_data, 6);

	close(s);
	return 1;
}

void acl_update_name(uint8 *mac, char *name)
{
	//char cmd[1024];
	char dev_mac[32];

	//snprintf(cmd, 1024, "/usr/sbin/acl_update_name %s %s", ether_etoa(mac, dev_mac), name);
	//system(cmd);
	dni_system(NULL, "/usr/sbin/acl_update_name", ether_etoa(mac, dev_mac), name, NULL);
}

cJSON *getJSONcode(char *filename)
{
	FILE *tfp;
	cJSON *root;
	int flen=0;
	char *json_buf;

	root=NULL;
	if((tfp = fopen(filename, "r")) == NULL)
		return NULL;

	fseek(tfp,0L,SEEK_END);
	flen=ftell(tfp);
	json_buf=(char *)malloc(flen+1);
	fseek(tfp,0L,SEEK_SET);
	fread(json_buf,flen,1,tfp);
	json_buf[flen]='\0';
	fclose(tfp);
	root = cJSON_Parse(json_buf);
	free(json_buf);
	json_buf = NULL;
	return root;
}

struct interface_element if_ele[INTERFACENUM] = {
	{"eth1", "00:00:00:00:00:00", 1},
	{"ath0", "00:00:00:00:00:00", 0},
	{"ath01", "00:00:00:00:00:00", 0},
	{"ath1", "00:00:00:00:00:00", 0},
	{"ath2", "00:00:00:00:00:00", 0},
	{"ath02", "00:00:00:00:00:00", 0},
	{"ath11", "00:00:00:00:00:00", 0},
	{"eth0", "00:00:00:00:00:00", 2},
	{NULL, "00:00:00:00:00:00", 0}
};

int get_interface_info () {
	FILE *fd;
	int i;
	char buff[512], *port, *bmac, *other;

	system("brctl showmacs br0 > /tmp/brctl_showmacs_br0");
	if((fd = fopen("/tmp/brctl_showmacs_br0", "r")) == NULL)
		return -1;

	//get mac addresses to compare
	for (i = 0; i < INTERFACENUM; i++){
		if(if_ele[i].name == NULL) break;
		get_mac(if_ele[i].name, if_ele[i].mac);
	}

	/*
	 * #brctl showmacs br0
	 * port no		mac addr			is local?		ageing time
	 * 1			20:14:07:11:2A:20	yes				1.01
	 * ......
	 */

	fgets(buff, sizeof(buff), fd);		//skip first line
	while(fgets(buff, sizeof(buff), fd)){
		port = strtok(buff, " \t\n");
		bmac = strtok(NULL, " \t\n");
		other = strtok(NULL, " \t\n");

		strupr(bmac);
		for (i = 0; i < INTERFACENUM; i++){
			if(if_ele[i].name == NULL) break;
			if(!strncmp(bmac, if_ele[i].mac, 17)){
				if_ele[i].port = atoi(port);
//				DEBUGP("Found %s interface port id %d\n", if_ele[i].name, if_ele[i].port);
				break;
			}
		}
	}

	//print interface infomation
//	DEBUGP("---Interface Table---\n");
	for (i = 0; i < INTERFACENUM; i++){
		if(if_ele[i].name == NULL) break;
//		DEBUGP("name:%s MAC:%s port:%d\n", if_ele[i].name, if_ele[i].mac, if_ele[i].port);
	}

	fclose(fd);
	return 0;
}

int get_attach_and_connection_type(struct arp_struct *u) {
	FILE *fd;
	int i, ret = -1;
	char buff[512], arpmac[32], *port, *bmac, *other;

	get_interface_info();

	if((fd = fopen("/tmp/brctl_showmacs_br0", "r")) == NULL) {
		DEBUGP("cannot open /tmp/brctl_showmacs_br0\n");
		return -1;
	}

	/*
	 * #brctl showmacs br0
	 * port no		mac addr			is local?		ageing time
	 * 1			20:14:07:11:2A:20	yes				1.01
	 * ......
	 */

	fgets(buff, sizeof(buff), fd);		//skip first line
	ether_etoa(u->mac, arpmac);
	while(fgets(buff, sizeof(buff), fd)){
		int portnum=0;

		port = strtok(buff, " \t\n");
		bmac = strtok(NULL, " \t\n");
		other = strtok(NULL, " \t\n");

		strupr(bmac);
		if(strncmp(arpmac, bmac, strlen(arpmac)))
			continue;

		portnum = atoi(port);
//		DEBUGP("ONE client[%s] from %d interface.\n", bmac, portnum);
		for (i = 0; i < INTERFACENUM; i++)
		{
			// For Orbi projects 2.4G is ath0, ath02. 5G is ath1,ath11
			if(if_ele[i].name == NULL) break;
			if (if_ele[i].port == portnum) {
				switch(i) {
					case ETH1:
					case ETH0:
						u->ctype = WIRED;
						u->atype = BASE_ATT;
						break;
					case ATH0:
					case ATH02:
						u->ctype = WIRELESS_2G;
						u->atype = BASE_ATT;
						break;
					case ATH1:
					case ATH11:
						u->ctype = WIRELESS_5G;
						u->atype = BASE_ATT;
						break;
					case ATH01:
					case ATH2:
						update_satellites_and_attachs(u);
						break;
				}
				ret = 0;
				break;
			}
		}
		break;
	}

	fclose(fd);
	return ret;
}

int update_arp_table(uint8 *mac, struct in_addr ip, int isrepl)
{
	uint32 i;
	char host[MAX_HOSTNAME_LEN + 1] = {0};
	struct arp_struct *u;

	/* check dhcp host */
	get_dhcp_host(host, ip, &isrepl);
	i = arp_hash(mac);
	for (u = arp_tbl[i]; u && memcmp(u->mac, mac, ETH_ALEN); u = u->next);
	if (u) {
		if (*host) {
			strncpy(u->host, host, MAX_HOSTNAME_LEN);
			acl_update_name(u->mac, host);
		}
		u->ip = ip;              /* The IP may be changed for DHCP      */
		u->active = 1;

		if (get_attach_and_connection_type(u) == -1)
			u->active = 0;
		return isrepl;	/* Do BIOS Name Query only ARP reply */
	}

	u = malloc(sizeof(struct arp_struct));
	if (u == 0)
		return 0;
	memset(u, 0, sizeof(struct arp_struct));
	u->ip = ip;
	u->active = 1;
	memcpy(u->mac, mac, ETH_ALEN);

	/* start catch HTTP packets when new mac is found*/
	system("/usr/sbin/UA_Parser");

	if (*host) {
		strncpy(u->host, host, MAX_HOSTNAME_LEN);
		acl_update_name(u->mac, host);
	}

	u->ctype = NONETYPE; //default value
	if (get_attach_and_connection_type(u) == -1)
			u->active = 0;
	else {
		if (u->atype == SATELLITE_ATT) {
			system("/etc/send_soap &");
		}
	}
	u->dtype = DEFAULT_DEVICE_TYPE;
	get_model_name(u->mac, u->model);
	
/*
 *	update local device table
 *	run UA_Parser
 */
#ifdef SUPPORT_STREAMBOOST
	u->type = TYPE_OTHERS;
#endif

	u->next = arp_tbl[i];
	arp_tbl[i] = u;

	return isrepl;
}

static void update_name(struct in_addr ip, char *host)
{
	int i;
	struct arp_struct *u;

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (u = arp_tbl[i]; u; u = u->next)
			if (u->ip.s_addr == ip.s_addr) {
				strncpy(u->host, host, MAX_HOSTNAME_LEN);
				acl_update_name(u->mac, host);
				return;
			}
	}
}

void update_bios_name(uint8 *mac, char *host, struct in_addr ip)
{
	uint32 i;
	struct arp_struct *u;

	i = arp_hash(mac);
	for (u = arp_tbl[i]; u && memcmp(u->mac, mac, ETH_ALEN); u = u->next);

	if (u == 0) {
		update_name(ip, host); /* try it by IP address */
		return;
	}

	strncpy(u->host, host, MAX_HOSTNAME_LEN);
	acl_update_name(u->mac, host);
}

#ifdef SUPPORT_STREAMBOOST
void update_streamboost_info(int state, uint8 *mac, struct in_addr ip, char *name, netgear_devtype_t type, double down, double up, long long epoch, int priority)
{
	uint32 i;
	struct arp_struct *u;

	i = arp_hash(mac);
	for (u = arp_tbl[i]; u && memcmp(u->mac, mac, ETH_ALEN); u = u->next); /* or find the node according ip ? */

	if (state == SB_INFO_STATE_2 && u && u->active == 1 && priority > priority_for_unknow_dev)
		priority_for_unknow_dev = priority;

	if (!u) {
		u = malloc(sizeof(struct arp_struct));
		if (!u)
			return;
		memset(u,  0, sizeof(struct arp_struct));
		u->ip = ip;
		memcpy(u->mac, mac, ETH_ALEN);
		u->next = arp_tbl[i];
		arp_tbl[i] = u;
	}

	if (strcmp(name, "null") != 0) {
		strncpy(u->host, name, MAX_HOSTNAME_LEN);
		acl_update_name(u->mac, name); /* should update acl name here ? */
	}
	u->type = type;
	u->priority = priority;

	if (state == SB_INFO_STATE_1) {
		u->down = down;
		u->up = up;
		u->epoch = epoch;
		u->state = state;
	}
	else if (state == SB_INFO_STATE_2 && u->state == SB_INFO_STATE_1) {
		long long divisor;
		divisor = (epoch - u->epoch) * 1000000 / 8;
		u->down = (down - u->down) / divisor;
		u->up = (up - u->up) / divisor;
		if (u->down < 0.0)
			u->down = 0.0;
		if (u->up < 0.0)
			u->up = 0.0;
		u->state = SB_INFO_STATE_2;
	}
}
#endif

void recv_bios_pack(char *buf, int len, struct in_addr from)
{
#define HDR_SIZE		sizeof(struct nb_response_header)
	uint16 num;
	uint8 *p, *e;
	struct nb_response_header *resp;

	if (len < HDR_SIZE)
		return;

	resp = (struct nb_response_header *)buf;
	num = resp->num_names;
	p = (uint8*)&buf[HDR_SIZE];
	e = p + (num * 18);
	/* unique name, workstation service - this is computer name */
	for (; p < e; p += 18)
		if ((p[15] == 0 || p[15] == 0x20) && (p[16] & 0x80) == 0)
			break;
	if (p == e)
		return;
	if (p[15] == 0x20)
		p[15] = 0;
	update_bios_name(e, (char *)p, from);
}

int open_arp_socket(struct sockaddr *me)
{
	int s;
	int buffersize = 200 * 1024;

	s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if (s < 0)
		return -1;

	/* We're trying to override buffer size  to set a bigger buffer. */
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize)))
		DEBUGP("setsocketopt error!\n");

	me->sa_family = PF_PACKET;
	strncpy(me->sa_data, ARP_IFNAME, 14);
	if (bind(s, me, sizeof(*me)) < 0)
		return -1;
	if (init_arp_request(ARP_IFNAME) == 0)
		return -1;

	return s;
}

int recv_arp_pack(struct arpmsg *arpkt, struct in_addr *send_ip)
{
	static uint8 zero[6] = { 0, 0, 0, 0, 0, 0 };

	struct in_addr src_ip;

	if (arpkt->ar_op != htons(ARPOP_REQUEST) && arpkt->ar_op != htons(ARPOP_REPLY))
		return 0;
	if (arpkt->ar_hrd != htons(ARPHRD_ETHER) ||arpkt->ar_pro != htons(ETH_P_IP))
		return 0;
	if (arpkt->ar_pln != 4 ||arpkt->ar_hln != ETH_ALEN)
		return 0;

	/*
	  * If It is Gratuitous ARP message, ignore it for Home Router passing Xbox test,
	  * else we need change the `udhcpd` code about `checking IP used` too much
	  * to pass `XBox DHCP Lease Test`. The normal ARP message ==MAY BE== all
	  * right for Attached Devices function.... &_&.
	  */
	if (memcmp(arpkt->ar_sip, arpkt->ar_tip, 4) == 0)
		return 0;

	memcpy(&src_ip, arpkt->ar_sip, 4);
	if (src_ip.s_addr == 0 ||memcmp(arpkt->ar_sha, zero, 6) == 0)
		return 0;

	*send_ip = src_ip;
	return update_arp_table(arpkt->ar_sha, src_ip, arpkt->ar_op == htons(ARPOP_REPLY));
}

void update_satellite_name(struct arp_struct *u) {
	int scount, i;
	cJSON *dataArray, *Item, *macItem, *modelItem, *deviceItem;
	char mac[32];

	system("/usr/sbin/satelliteinfo device > /tmp/netscan/current_satellite_list");
	ether_etoa(u->mac, mac);

	if((dataArray = getJSONcode("/tmp/netscan/current_satellite_list")) != NULL) {
		scount = cJSON_GetArraySize(dataArray);

		for(i=0; i<scount; i++) {
			Item  = cJSON_GetArrayItem(dataArray, i);
			if (Item == NULL) goto err;
			macItem = cJSON_GetObjectItem(Item, "mac address");
			modelItem = cJSON_GetObjectItem(Item, "module name");
			deviceItem =  cJSON_GetObjectItem(Item, "device name");
			if (macItem == NULL || modelItem == NULL || deviceItem == NULL) goto err;
			if (strcmp(macItem->valuestring, mac) == 0) {
				strncpy(u->host, deviceItem->valuestring, MAX_HOSTNAME_LEN);
				DEBUGP("Get satellite[MAC:%s] name:%s\n", mac, u->host);
				cJSON_Delete(dataArray);
				return;
			}
		}

	}

	DEBUGP("[net-scan]name of satellite[MAC:%s] is not found\n", mac);
	cJSON_Delete(dataArray);
	return;
err:
	DEBUGP("[net-scan]Get satelliteinfo device error\n");
	cJSON_Delete(dataArray);
	return;
}

void update_satellite_attaches(struct arp_struct *u) {
	int scount, i, dcount, j;
	cJSON *dataArray, *Item, *macItem, *typeItem, *deviceItem, *satellite_dev, *sItem;
	char mac[32], type[8], *p0, *p1, *sname;

	ether_etoa(u->mac, mac);

	system("/usr/sbin/satelliteinfo attached > /tmp/netscan/satellite_attached_dev");

	if((dataArray = getJSONcode("/tmp/netscan/satellite_attached_dev")) != NULL) {
		scount = cJSON_GetArraySize(dataArray);

		for(i=0; i<scount; i++) {
			satellite_dev = cJSON_GetArrayItem(dataArray, i);
			p0 = cJSON_PrintUnformatted(satellite_dev);
			p1 = strtok(p0, "\"");
			sname = strtok(NULL, "\"");
			sItem = cJSON_GetObjectItem(satellite_dev, sname);
			if (sItem == NULL)
				goto err;

			dcount = cJSON_GetArraySize(sItem);
			for(j=0; j<dcount; j++) {
				Item  = cJSON_GetArrayItem(sItem, j);
				macItem = cJSON_GetObjectItem(Item, "attached device mac address");
				typeItem = cJSON_GetObjectItem(Item, "attached connection type");
				deviceItem =  cJSON_GetObjectItem(Item, "attached device name");

				if (macItem == NULL || typeItem == NULL || deviceItem == NULL)
					goto err;

				if (strcmp(macItem->valuestring, mac) == 0) {
					if (u->host[0] == '\0')
						strncpy(u->host, deviceItem->valuestring, MAX_HOSTNAME_LEN);
					strncpy(type, typeItem->valuestring, 8);
					if (strcmp(type, "2.4G") == 0)
						u->ctype = WIRELESS_2G;
					else if (strcmp(type, "5G") == 0)
						u->ctype = WIRELESS_5G;
					else if (strcmp(type, "wired") == 0)
						u->ctype = WIRED;
					if(p0)
					{
						free(p0);
						p0 = NULL;
					}
					cJSON_Delete(dataArray);
					return;
				}
			}
			if(p0)
			{
				free(p0);
				p0 = NULL;
			}
		}

	}

	DEBUGP("[net-scan]satellite attaches info[MAC:%s] is not found\n", mac);
	if(p0)
	{
		free(p0);
		p0 = NULL;
	}
	cJSON_Delete(dataArray);
	return;
err:
	DEBUGP("[net-scan]Get satelliteinfo attached device error\n");
	if(p0)
	{
		free(p0);
		p0 = NULL;
	}
	cJSON_Delete(dataArray);
	return;

}

void update_satellites_and_attachs(struct arp_struct *u) {
	FILE *fp_orbi;
	char buff[64], name[64], cfg_change[128];
	char *val, *sep;
	char *status_t, *mac_t, *type_t, *order_t, *host_t, *log_t;
	int i;
	char mac[32], wlan_line[256], *tmp, *orbimac, *orbiip;
	ether_etoa(u->mac, mac);

	sep = " ";

	fp_orbi = fopen("/tmp/hyt_result", "r");
	if (fp_orbi != NULL) {
		while(fgets(wlan_line, sizeof(wlan_line), fp_orbi)) {
			tmp = wlan_line;
			orbimac = strtok(tmp, ",");
			orbiip = strtok(NULL, ",");
			if (strcmp(orbimac, mac) == 0) {
				if (config_match("ap_mode", "1"))
					update_satellite_name(u);
				inet_aton(orbiip, &u->ip);
				u->atype = BASE_ATT_S;
				u->ctype = SATELLITE;
				strcpy(u->model, "Netgear");

				for ( i = 1; ; i++)
				{
					sprintf(name, "%s%d", "access_control", i);
					val = config_get(name);

					if (*val == '\0')
						break;

					status_t = strtok(val, sep);
					mac_t = strtok(NULL, sep);
					type_t = strtok(NULL, sep);
					order_t = strtok(NULL, sep);
					host_t = strtok(NULL, sep);
					log_t = strtok(NULL, sep);
					if(strcasecmp(mac_t, mac) == 0 && strcmp(type_t, "3") != 0)
					{
						sprintf(cfg_change, "%s %s %s %s %s %s", status_t, mac_t, "3", order_t, host_t, log_t);
						config_set(name, cfg_change);
						break;
					}
				}
				fclose(fp_orbi);
				return;
			}
		}
	}

	u->atype = SATELLITE_ATT;

	if(fp_orbi)
		fclose(fp_orbi);
	return;
}

void update_devices_info(struct arp_struct *u) {
	FILE *fp;
	char line[256], *newline, *mac, *type, *model, *name, *flag, arpmac[32];

	fp = fopen(LOCAL_DEVICE_FILE, "r");
	if (fp == NULL) return;

	ether_etoa(u->mac, arpmac);
	while (fgets(line, sizeof(line), fp)) {
		newline = strtok(line, "\n");
		mac = strtok(newline, ",");
		type = strtok(NULL, ",");
		model = strtok(NULL, ",");
		name = strtok(NULL, ",");
		flag = strtok(NULL, ",");

		if (strcmp(mac, arpmac) == 0) {
			if (strcmp(flag, "1") == 0) {	//user customized device
				u->dtype = (unsigned int)atoi(type);
				strncpy(u->model, model, MAX_MODEL_LEN);
				strncpy(u->host, name, MAX_HOSTNAME_LEN);
			} else {
				u->dtype = (unsigned int)atoi(type);
				strncpy(u->model, model, MAX_MODEL_LEN);
			}
			break;
		}
	}

	fclose(fp);
	return;
}

void show_arp_table(void)
{
	int i, device_num = 1, satellite_num = 0, flag;
	FILE *fp, *hyt_fp;
	char mac[32], hyt_line[256], *tmp, *hytmac;
	struct arp_struct *u;
	struct arp_struct **pprev;
#ifdef SUPPORT_STREAMBOOST
	FILE *sb_fp, *name_type_fp;
	struct device_list head_node, *sb_device_list, *sb_dlp;
	char  netgear_priority[32];

	get_streamboost_nodes_info(SB_INFO_STATE_2);
	sb_device_list = &head_node;
	read_device_list(sb_device_list);
	sb_fp = fopen("/tmp/netscan/attach_device_streamboost", "w");
	name_type_fp = fopen("/tmp/netscan/default_name_type", "w");
#endif

	fp = fopen(ARP_FILE, "w");
	hyt_fp = fopen("/tmp/soapclient/allconfig_result", "r");
	if (fp == NULL) {
		DEBUGP("[net-scan]open %s failed\n", ARP_FILE);
	}

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		char typechar[16];
		for (pprev = &arp_tbl[i], u = *pprev; u; ) {
			if (u->active == 0) {
				*pprev = u->next;
				free(u);
				u = *pprev;
				continue;
			}

			if (u->atype == SATELLITE_ATT) {
				update_satellite_attaches(u);
				if (u->ctype == NONETYPE) {
					system("/etc/send_soap &");
				}
			}
			flag = 0;
			if (hyt_fp != NULL && u->ctype == SATELLITE) {
				fseek(hyt_fp, 0L, SEEK_SET);
				while(fgets(hyt_line, sizeof(hyt_line), hyt_fp)) {
					tmp = hyt_line;
					hytmac = strtok(tmp, " ");
					flag = atoi(strtok(NULL, "\n"));
					if (strcasecmp(ether_etoa(u->mac, mac), hytmac) == 0)
						break;
				}
			}
			if (flag){
				pprev = &u->next;
				u = *pprev;
				continue;
			}

			switch (u->ctype) {
				case WIRELESS_2G:
					strcpy(typechar, "2.4G");
					break;
				case WIRELESS_5G:
					strcpy(typechar, "5G");
					break;
				case SATELLITE:
					strcpy(typechar, "wireless");
					update_satellites_and_attachs(u);
					satellite_num++;
					u->dtype = 0;
					break;
				case WIRED:
					strcpy(typechar, "wired");
					break;
				default:
					strcpy(typechar, "unknown");
					break;
			}

			update_devices_info(u);
			/* for GUI dealing easily:  &lt;unknown&gt;   <----> <unknown>*/
			DEBUGP("-----device:%d-----\n", device_num);
			DEBUGP("%s\n", inet_ntoa(u->ip));
			DEBUGP("%s\n", ether_etoa(u->mac, mac));
			DEBUGP("%d\n", u->ctype);
			DEBUGP("%d\n", u->atype);
			DEBUGP("%d\n", u->dtype);
			DEBUGP("%s\n", u->model[0] == '\0' ? "Unknown" : u->model);
			DEBUGP("%s\n", u->host[0] == '\0' ? "Unknown" : host_stod(u->host));

			fprintf(fp, "-----device:%d-----\n", device_num++);
			fprintf(fp, "%s\n", inet_ntoa(u->ip));
			fprintf(fp, "%s\n", ether_etoa(u->mac, mac));
			fprintf(fp, "%s\n", typechar);
			fprintf(fp, "%d\n", u->atype);
			fprintf(fp, "%d\n", u->dtype);
			fprintf(fp, "%s\n", u->model[0] == '\0' ? "Unknown" : u->model);
			fprintf(fp, "%s\n", u->host[0] == '\0' ? "Unknown" : host_stod(u->host));

#ifdef SUPPORT_STREAMBOOST
			ether_etoa(u->mac, mac);
			if (sb_fp){
				for(sb_dlp = sb_device_list->next; sb_dlp != NULL && strcmp(sb_dlp ->mac, mac) != 0; sb_dlp = sb_dlp->next);
				if(sb_priority_2_netgear_prioriry(u->priority, netgear_priority) == -1){
					/*
					 * Sometimes if no traffic is sent to wan, the default priority of device
					 * set to 255. Once traffic send to wan, the default priority of device
					 * set to 10/20/30/40. We hide this situation for end user and show Low
					 * in GUI for now.
					 */
					strcpy(netgear_priority, "LOW");
				}
				if(sb_dlp){
					/* This attached device was edited by user*/
					/* IP, MAC, Icon type, priority, upload speed, download speed, Device name */
					fprintf(sb_fp, "%s %s %d %s %.2f %.2f %s @#$&*!\n",
					inet_ntoa(u->ip),  mac, sb_dlp->type == TYPE_SOAP_OLD ? u->type : sb_dlp->type, netgear_priority,
					u->state == SB_INFO_STATE_2? u->down : 0.0,
					u->state == SB_INFO_STATE_2? u->up : 0.0,
					sb_dlp->host[0] == '\0' ? (u->host[0] == '\0' ? "&lt;unknown&gt;" : host_stod(u->host)) : host_stod(sb_dlp->host));
				}
				else{
					/* This attached device was NOT edited by user*/
					fprintf(sb_fp, "%s %s %d %s %.2f %.2f %s @#$&*!\n",
					inet_ntoa(u->ip), mac, u->type, netgear_priority,
					u->state == SB_INFO_STATE_2? u->down : 0.0,
					u->state == SB_INFO_STATE_2? u->up : 0.0,
					u->host[0] == '\0' ? "&lt;unknown&gt;" : host_stod(u->host));
				}
			}
			if (name_type_fp){
				if(u->host[0] != '\0')
					fprintf(name_type_fp, "%s %d 0 %s\n", mac, u->type, u->host);
				else
					fprintf(name_type_fp, "%s %d 1 Unknown\n", mac, u->type);
			}
#endif

			pprev = &u->next;
			u = *pprev;
		}
	}
	if (fp)
		fclose(fp);
	if (hyt_fp)
		fclose(hyt_fp);

	if(fp = fopen(SATELLITE_NUM_FILE, "w")){
		fprintf(fp, "%d", satellite_num);
		fclose(fp);
	}

	if(fp = fopen(ATTACHED_DEVICE_NUM_FILE, "w")){
		fprintf(fp, "%d", --device_num - satellite_num);
		fclose(fp);
	}

#ifdef SUPPORT_STREAMBOOST
	if (sb_fp)
		fclose(sb_fp);
	if(name_type_fp)
		fclose(name_type_fp);
	free_device_list(sb_device_list);
#endif
}

/* To fix bug 22146, add function reset_arp_table, it can set active status of all nodes in the arp_tbl to 0 */
void reset_arp_table()
{
	int i;
	struct arp_struct *u;

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (u = arp_tbl[i]; u; u = u->next) {
			u->active = 0;
#ifdef SUPPORT_STREAMBOOST
			u->state = 0;
#endif
		}
	}
#ifdef SUPPORT_STREAMBOOST
	get_streamboost_nodes_info(SB_INFO_STATE_1);
#endif
}

void scan_arp_table(int sock, struct sockaddr *me, int force_soap_flag)
{
	int i;
	int count = 0;
	struct arpmsg *req;
	struct arp_struct *u;
	char *ipaddr;
	char buffer[512];
	struct in_addr addr;
	FILE *fp;

	if(force_soap_flag) {
	        config_set("soap_setting", "AttachDevice");
	        system("/usr/bin/killall -SIGUSR1 soap_agent");
	} else {
		system("/etc/send_soap &");
	}


	while (count != 3) {
		count++;
		req = &arpreq;
		for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
			for (u = arp_tbl[i]; u; u = u->next) {
				memcpy(req->ar_tip, &u->ip, 4);
				memcpy(req->ar_tha, &u->mac, 6);
				memcpy(req->h_dest, &u->mac, 6);
				sendto(sock, req, sizeof(struct arpmsg), 0, me, sizeof(struct sockaddr));
			}
		}
		/**
		 * For beta issue: TD-23
		 * If use Ixia with some virtual DHCP clients to test "Attached Device" feature,
		 * Ixia could not send arp packet actively, we need request all IPs that DHCP server
		 * assigned while user refresh "Attached Device" table.
		 * We just request all IPs in "/tmp/dhcpd_hostlist" that were not recorded in 'arp_tbl'.
		 */
		if (fp = fopen(DHCP_LIST_FILE,"r")) {
			while (fgets(buffer, sizeof(buffer), fp)) {
				ipaddr = strtok(buffer, " \t\n");
				if (ipaddr && inet_aton(ipaddr, &addr) != 0) {
					for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
						for (u = arp_tbl[i]; u && memcmp(&u->ip, &addr, sizeof(addr)); u = u->next);
						if (u) break;
					}
					if (u) continue;
					memcpy(req->ar_tip, &addr, 4);
					sendto(sock, req, sizeof(struct arpmsg), 0, me, sizeof(struct sockaddr));
				}
			}
			fclose(fp);
		}
		if(count < 3)
			usleep(500000);
	}

	sleep(1);	// sleep one more second to wait soap response
}
