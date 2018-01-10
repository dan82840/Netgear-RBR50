#include "netscan.h"

#define INTERFACENUM 10

struct interface_element{
	char *name;
	char mac[18];
	int port;
};

/* modified from "linux-2.4.18/net/ipv4/arp.c" */
uint32 arp_hash(uint8 *pkey)
{
#define GET_UINT32(p)	((p[0]) |(p[1] << 8) |(p[2] << 16) |(p[3] << 24))
	uint32 hash_val;

	hash_val = GET_UINT32(pkey);
	hash_val ^= hash_val >> 16;
	hash_val ^= hash_val >> 8;
	hash_val ^= hash_val >> 3;

	return hash_val & NEIGH_HASHMASK;
}

void get_dhcp_host(char host[], struct in_addr ip, int *isrepl)
{
	FILE *tfp;
	char *ipaddr;
	char *hostname;
	char *ipstr;
	char buff[512];

	host[0] = '\0';
	ipstr = inet_ntoa(ip);
	if ((tfp = fopen(DHCP_LIST_FILE,"r")) == 0)
		return;

	while (fgets(buff, sizeof(buff), tfp)) {
		ipaddr = strtok(buff, " \t\n");
		hostname = strtok(NULL, "\t\n");
		if (ipaddr == NULL || hostname == NULL)
			continue;

		if (strcmp(ipaddr, ipstr) == 0) {
			strncpy(host, hostname, MAX_HOSTNAME_LEN);
			*isrepl = 0;
			break;
		}
	}

	fclose(tfp);
}

//void remove_disconn_dhcp(struct in_addr ip)
//{
//	int i, k, result;
//	int target = 0;
//	int target_num = 0;
//	FILE *fp;
//	fpos_t pos_w,pos_r,pos;
//	char ipaddr[32];
//	char line[512];
//	char list_str[512];
//
//	if ( !(fp = fopen (DHCP_LIST_FILE,"r")))
//		return;
//	
//	while(fgets(line, sizeof(line), fp) != NULL) {
//		result = sscanf(line, "%s%s", ipaddr,list_str);
//		if (result == 2){
//			if(memcmp(inet_ntoa(ip), ipaddr, strlen(ipaddr)) == 0) {
//				target = 1;
//				break;
//			}
//		}
//		target_num ++;
//	}
//	fclose(fp);
//
//	if (target != 1)
//		return;
//
//	if ( !(fp = fopen (DHCP_LIST_FILE,"r+")))
//		return;
//	for (i = 0; i < target_num; i++)
//		fgets(line,sizeof(line),fp);
//	
//	/* save the file pointer position */
//	fgetpos (fp,&pos_w);
//	/* position the delete line */
//	fgets(line,sizeof(line),fp);
//	fgetpos (fp,&pos_r);
//	pos = pos_r;
//
//	while (1)
//	{
//		/* set a new file position */ 
//		fsetpos (fp,&pos);
//		if (fgets(line,sizeof(line),fp) ==NULL) 
//			break;
//		fgetpos (fp,&pos_r);
//		pos = pos_w;
//		fsetpos (fp,&pos);
//		fprintf(fp,"%s",line);
//		fgetpos (fp,&pos_w);
//		pos = pos_r;
//	}
//	pos = pos_w;
//	fsetpos (fp,&pos);
//	k = strlen(line);
//	for (i=0;i<k;i++) 
//		fputc(0x20,fp);
//	
//	fclose(fp);
//}

char *ether_etoa(uint8 *e, char *a)
{
	static char hexbuf[] = "0123456789ABCDEF";

	int i, k;

	for (k = 0, i = 0; i < 6; i++) {
		a[k++] = hexbuf[(e[i] >> 4) & 0xF];
		a[k++] = hexbuf[(e[i]) & 0xF];
		a[k++]=':';
	}

	a[--k] = 0;

	return a;
}

/*
 * xss Protection 
 * < -> &lt;
 * > -> &gt;
 * ( -> &#40;
 * ) -> &#41;
 * " -> &#34;
 * ' -> &#39;
 * # -> &#35;
 * & -> &#38;
 */
char *host_stod(char *s)
{//change special character to ordinary string
	static char str[MAX_HOSTNAME_LEN*5 + 1 ];
	char c, *p;

	p = str;
        while((c = *s++) != '\0') {
                if(c == '"'){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '4'; *p++ = ';';
                } else if( c == '(' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '4'; *p++ = '0'; *p++ = ';';
                } else if( c == ')' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '4'; *p++ = '1'; *p++ = ';';
                } else if( c == '#' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '5'; *p++ = ';';
                } else if( c == '&' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '8'; *p++ = ';';
                } else if( c == '<' ){
                        *p++ = '&'; *p++ = 'l'; *p++ = 't'; *p++ = ';';
                } else if( c == '>' ){
                        *p++ = '&'; *p++ = 'g'; *p++ = 't'; *p++ = ';';
                } else if (c == '\'') {
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '9'; *p++ = ';';
                }
                else {
                        *p++ = c;
                }
        }
        *p = '\0';

	return str;
}

void strupr(char *str)
{
	for(;*str != '\0'; str++)
	{
		if(*str >= 97 && *str <= 122)
			*str = (*str)-32;
	}
}

int check_sta_format(char *info)
{
	int i,len;

	len = strlen(info);

	if(len < 26 || len > 30)
		return 0;

	for(i=0; i<len; i++)
	{
		if((i==2 || i==5 || i==8 || i==11 || i==14) && *(info+i)!= ':')
			return 0;

		if(*(info+17) > 32) // ignore those are not space or Tab
			return 0;
	}

	return 1;
}

char *get_mac(char *ifname, char *eabuf)
{
	int s;
	struct ifreq ifr;

	eabuf[0] = '\0';
	s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (s < 0)
		return eabuf;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0)
		ether_etoa((uint8 *)ifr.ifr_hwaddr.sa_data, eabuf);
	close(s);

	return eabuf;
}

struct interface_element if_ele[INTERFACENUM] = {
	{"eth1", "00:00:00:00:00:00", 1},
	{"ath0", "00:00:00:00:00:00", 0},
	{"ath01", "00:00:00:00:00:00", 0},
	{"ath1", "00:00:00:00:00:00", 0},
	{"ath2", "00:00:00:00:00:00", 0},
	{"ath02", "00:00:00:00:00:00", 0},	//backhaul2 2.4G
	{"ath21", "00:00:00:00:00:00", 0},	//backhaul2 5G
	{"ath03", "00:00:00:00:00:00", 0},	//guest 2.4G
	{"ath11", "00:00:00:00:00:00", 0},	//guest 5G
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
				DEBUGP("Found %s interface port id %d\n", if_ele[i].name, if_ele[i].port);
				break;
			}
		}
	}

	//print interface infomation
	DEBUGP("---Interface Table---\n");
	for (i = 0; i < INTERFACENUM; i++){
		if(if_ele[i].name == NULL) break;
		DEBUGP("name:%s MAC:%s port:%d\n", if_ele[i].name, if_ele[i].mac, if_ele[i].port);
	}

	fclose(fd);
	return 0;
}

int get_device_type(uint8 *mac) {
	FILE *fd;
	int found = NONETYPE, i, repacd_mode = 0;
	char buff[512], arpmac[32], *port, *bmac, *other;

	get_interface_info();
	repacd_mode = atoi(config_get("repacd_Daisy_Chain_Enable"));
	if((fd = fopen("/tmp/brctl_showmacs_br0", "r")) == NULL)
		return NONETYPE;

	/*
	 * #brctl showmacs br0
	 * port no		mac addr			is local?		ageing time
	 * 1			20:14:07:11:2A:20	yes				1.01
	 * ......
	 */

	fgets(buff, sizeof(buff), fd);		//skip first line
	ether_etoa(mac, arpmac);
	while(fgets(buff, sizeof(buff), fd)){
		int portnum=0;

		port = strtok(buff, " \t\n");
		bmac = strtok(NULL, " \t\n");
		other = strtok(NULL, " \t\n");

		strupr(bmac);
		if(strncmp(arpmac, bmac, strlen(arpmac)))
			continue;

		portnum = atoi(port);
		DEBUGP("ONE client[mac:%s] from %d interface.\n", bmac, portnum);
		for (i = 0; i < INTERFACENUM; i++)
		{
			// For Orbi projects 2.4G is ath0, ath02. 5G is ath1,ath11
			if(if_ele[i].name == NULL) break;
			if (if_ele[i].port == portnum && repacd_mode) {
				switch(i) {
					case ETH1:
					case ETH0:
						found = WIRED;
						goto ret;
					case ATH0:
					case ATH03:
						found = WIRELESS_2G;
						goto ret;
					case ATH1:
					case ATH11:
						found = WIRELESS_5G;
						goto ret;
					case ATH21:
					case ATH01:
					case ATH2:
					case ATH02:
						found = BACKHAUL; 
						goto ret;
				}
			}
			else if (if_ele[i].port == portnum){
				switch(i) {
					case ETH1:
					case ETH0:
						found = WIRED;
						goto ret;
					case ATH0:
					case ATH02:
						found = WIRELESS_2G;
						goto ret;
					case ATH1:
					case ATH11:
						found = WIRELESS_5G;
						goto ret;
					case ATH01:
					case ATH2:
						found = BACKHAUL; 
						goto ret;
				}
			}
		}
	}

ret:
	fclose(fd);
	return found;
}

int rmsemicolon(char *mac, char *newmac) {
	int i, j;
	for (i = 0, j = 0; i < 32 && mac[i] != '\0'; i++)
		if ((mac[i] >= '0' && mac[i] <= '9') || (mac[i] >= 'A' && mac[i] <= 'F'))
			newmac[j++] = mac[i];
		else if (mac[i] == ':')
			continue;
		else
			return -1;
	newmac[j]='\0';
	return 0;
}

void get_vendor(char *mac, char *file, int len, char *model) {
	FILE *fp;
	char line[256], *newline, *oui, *name, newmac[32];

	fp = fopen(file, "r");
	if (fp == NULL) {
		DEBUGP("Cannot open file %s\n", file);
		return NULL;
	}

	if (rmsemicolon(mac, newmac) < 0) return;

	while (fgets(line, sizeof(line), fp)) {
		newline = strtok(line, "\n");
		oui = strtok(newline, ",");
		name = strtok(NULL, ",");
		if (strncmp(newmac, oui, len) == 0) {
			strncpy(model, name, MAX_MODEL_LEN);
			DEBUGP("find vendor name:%s oui:%s mac:%s\n", name, oui, mac);
			break;
		}
	}
	fclose(fp);
	return;
}

void get_model_name_from_oui(char *mac, char *model) {
	char *name = NULL;
	get_vendor(mac, MAC_OUI_FILE, 6, model);
	if (model[0] != '\0') return;
	get_vendor(mac, MAC_MAM_FILE, 7, model);
	if (model[0] != '\0') return;
	get_vendor(mac, MAC_OUI36_FILE, 9, model);
	if (model[0] != '\0') return;
}

void get_model_name(uint8 *macint, char *model) {
	char mac[32], name;

	ether_etoa(macint, mac);

	get_model_name_from_oui(mac, model);
	return;
}
