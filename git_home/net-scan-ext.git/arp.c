#include "netscan.h"

struct arp_struct
{
	struct arp_struct *next;

	uint16 active;

	struct in_addr ip;
	uint8 mac[ETH_ALEN];
	DeviceType ctype;
	char model[MAX_MODEL_LEN + 1];
	char host[MAX_HOSTNAME_LEN + 1];
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
	char dev_mac[32];

	dni_system(NULL, "/usr/sbin/acl_update_name", ether_etoa(mac, dev_mac), name, NULL);
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
		return isrepl;	/* Do BIOS Name Query only ARP reply */
	}

	u = malloc(sizeof(struct arp_struct));
	if (u == 0)
		return 0;
	memset(u, 0, sizeof(struct arp_struct));
	u->ip = ip;
	u->active = 1;
	memcpy(u->mac, mac, ETH_ALEN);

	if (*host) {
		strncpy(u->host, host, MAX_HOSTNAME_LEN);
		acl_update_name(u->mac, host);
	}

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

void show_arp_table(void)
{
	int i, device_num = 1;
	char mac[32];
	struct arp_struct *u;
	struct arp_struct **pprev;
	FILE *fp;

	fp = fopen(ARP_FILE, "w");
	if (fp == NULL) {
		DEBUGP("[net-scan]open %s failed\n", ARP_FILE);
		return;
	}

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (pprev = &arp_tbl[i], u = *pprev; u; ) {
			char typechar[16];

			u->ctype = get_device_type(u->mac);

			if (u->active == 0 || u->ctype == NONETYPE) {
				*pprev = u->next;
				free(u);
				u = *pprev;
				continue;
			}

			/* for GUI dealing easily:  &lt;unknown&gt;   <----> <unknown>*/
			if (u->ctype == WIRELESS_2G || u->ctype == WIRELESS_5G || u->ctype == WIRED) {
				switch (u->ctype) {
					case WIRELESS_2G:
						strcpy(typechar, "2.4G");
						break;
					case WIRELESS_5G:
						strcpy(typechar, "5G");
						break;
					case WIRED:
						strcpy(typechar, "wired");
						break;
				}
				DEBUGP("-----device:%d-----\n", device_num);
				DEBUGP("%s\n", inet_ntoa(u->ip));
				DEBUGP("%s\n", ether_etoa(u->mac, mac));
				DEBUGP("%d\n", u->ctype);
				DEBUGP("0\n");	//attched type
				DEBUGP("0\n"); //device type
				DEBUGP("%s\n", u->model[0] == '\0' ? "Unknown" : u->model);
				DEBUGP("%s\n", u->host[0] == '\0' ? "Unknown" : host_stod(u->host));

				fprintf(fp, "-----device:%d-----\n", device_num++);
				fprintf(fp, "%s\n", inet_ntoa(u->ip));
				fprintf(fp, "%s\n", ether_etoa(u->mac, mac));
				fprintf(fp, "%s\n", typechar);
				fprintf(fp, "0\n");	//attached type
				fprintf(fp, "0\n"); //device type
				fprintf(fp, "%s\n", u->model[0] == '\0' ? "Unknown" : u->model);
				fprintf(fp, "%s\n", u->host[0] == '\0' ? "Unknown" : host_stod(u->host));
			}

			pprev = &u->next;
			u = *pprev;
		}
	}
	fclose(fp);
}

/* To fix bug 22146, add function reset_arp_table, it can set active status of all nodes in the arp_tbl to 0 */
void reset_arp_table()
{
	int i;
	struct arp_struct *u;

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (u = arp_tbl[i]; u; u = u->next) {
			u->active = 0;
		}
	}
}

void scan_arp_table(int sock, struct sockaddr *me)
{
	int i;
	int count = 0;
	struct arpmsg *req;
	struct arp_struct *u;
	char *ipaddr;
	char buffer[512];
	struct in_addr addr;
	FILE *fp;

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
		if(count < 3)
			usleep(500000);
	}
}
