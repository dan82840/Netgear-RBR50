#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/time.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include "netscan.h"

#define TIMERINTERVAL	60
#define CHECKTIME		15
#define PIDFILE	"/var/run/UA_Parser.pid"
#define UAFILE	"/tmp/device_tables/UA.csv"
//#define LOGFILE	"/tmp/UA.log"
#define NETLINK_XHTTP 22
#define BUFLEN 4096
#define UH_LIMIT_HEADERS	64
#define UH_HTTP_MSG_GET		0
#define UH_HTTP_MSG_HEAD	1
#define UH_HTTP_MSG_POST	2
#define UH_HTTP_MSG_PUT		3
#define UH_HTTP_MSG_DELETE	4

#define array_size(x) \
	(sizeof(x) / sizeof(x[0]))

#define foreach_header(i, h) \
	for( i = 0; (i + 1) < (sizeof(h) / sizeof(h[0])) && h[i]; i += 2 )

typedef struct nl_packet_msg {
	size_t data_len;
	size_t http_len;
	unsigned char payload[0];
} nl_packet_msg_t;

struct http_request {
	int	method;
	float version;
	int redirect_status;
	char *url;
	char *headers[UH_LIMIT_HEADERS];
	struct auth_realm *realm;
};

char *strfind(char *haystack, int hslen, const char *needle, int ndlen)
{
	int match = 0;
	int i, j;

	for( i = 0; i < hslen; i++ )
	{
		if( haystack[i] == needle[0] )
		{
			match = ((ndlen == 1) || ((i + ndlen) <= hslen));

			for( j = 1; (j < ndlen) && ((i + j) < hslen); j++ )
			{
				if( haystack[i+j] != needle[j] )
				{
					match = 0;
					break;
				}
			}

			if( match )
				return &haystack[i];
		}
	}

	return NULL;
}

static struct http_request * uh_http_header_parse(char *buffer, int buflen)
{
	char *method  = &buffer[0];
	char *path    = NULL;
	char *version = NULL;

	char *headers = NULL;
	char *hdrname = NULL;
	char *hdrdata = NULL;

	int i;
	int hdrcount = 0;

	static struct http_request req;

	memset(&req, 0, sizeof(req));

	/* terminate initial header line */
	if( (headers = strfind(buffer, buflen, "\r\n", 2)) != NULL )
	{
		buffer[buflen-1] = 0;

		*headers++ = 0;
		*headers++ = 0;

		/* find request path */
		if( (path = strchr(buffer, ' ')) != NULL )
			*path++ = 0;

		/* find http version */
		if( (path != NULL) && ((version = strchr(path, ' ')) != NULL) )
			*version++ = 0;

		/* check method */
		if (method && !strcmp(method, "GET"))
			req.method = UH_HTTP_MSG_GET;
		else if (method && !strcmp(method, "POST"))
			req.method = UH_HTTP_MSG_POST;
		else if (method && !strcmp(method, "HEAD"))
			req.method = UH_HTTP_MSG_HEAD;
		else if (method && !strcmp(method, "PUT"))
			req.method = UH_HTTP_MSG_PUT;
		else if (method && !strcmp(method, "DELETE"))
			req.method = UH_HTTP_MSG_DELETE;
		else
			return NULL;

		/* check path */
		if( !path || !strlen(path) )
			return NULL;
		else
			req.url = path;

		/* check version */
		if( (version == NULL) || (strcmp(version, "HTTP/0.9") &&
		    strcmp(version, "HTTP/1.0") && strcmp(version, "HTTP/1.1")) )
			return NULL;
		else
			req.version = strtof(&version[5], NULL);


		/* process header fields */
		for( i = (int)(headers - buffer); i < buflen; i++ )
		{
			/* found eol and have name + value, push out header tuple */
			if( hdrname && hdrdata && (buffer[i] == '\r' || buffer[i] == '\n') )
			{
				buffer[i] = 0;

				/* store */
				if( (hdrcount + 1) < array_size(req.headers) )
				{
					req.headers[hdrcount++] = hdrname;
					req.headers[hdrcount++] = hdrdata;

					hdrname = hdrdata = NULL;
				}

				/* too large */
				else
					return NULL;
			}

			/* have name but no value and found a colon, start of value */
			else if( hdrname && !hdrdata &&
			    ((i+1) < buflen) && (buffer[i] == ':')
			) {
				buffer[i] = 0;
				hdrdata = &buffer[i+1];

				while ((hdrdata + 1) < (buffer + buflen) && *hdrdata == ' ')
					hdrdata++;
			}

			/* have no name and found [A-Za-z], start of name */
			else if( !hdrname && isalpha(buffer[i]) )
			{
				hdrname = &buffer[i];
			}
		}

		/* valid enough */
		req.redirect_status = 200;
		return &req;
	}

	/* Malformed request */
	return NULL;
}

typedef enum dev_info_item {
	DEVINFO_NONE,
	DEVINFO_IP,
	DEVINFO_MAC,
	DEVINFO_TYPE,
	DEVINFO_ATT,
	DEVINFO_DEVTYPE,
	DEVINFO_MODEL,
	DEVINFO_HOST
}DevInfoItem;

void get_mac_from_ip(char *mac, char *ip) {
	#define DEVICE_LINE			8
	FILE *fp;
	int row, item;
	char line[356], *newline, ip2[32], mac2[32];
	system("killall -SIGUSR1 net-scan; sleep 1");
	fp = fopen("/tmp/netscan/attach_device", "r");
	if (fp == NULL) return;

	for (row = 0; fgets(line, sizeof(line), fp); row++) {
		newline = strtok(line, "\n");
		item = row%DEVICE_LINE;
		if (item == DEVINFO_IP)
			strncpy(ip2, newline, sizeof(ip2));
		if (item == DEVINFO_MAC)
			strncpy(mac2, newline, sizeof(mac2));
		if (item == DEVINFO_HOST) {
			if (strcmp(ip, ip2) == 0) {
				strncpy(mac, mac2, sizeof(mac2));
				return;
			}
		}
	}

	fclose(fp);
	return;
}

void get_model_name_from_UA(char *ua, char *ip) {
	FILE *fp;
	char line[256], *newline, *sku, *model, *type, mac[32], cmd[512];

	fp = fopen(UAFILE, "r");
	if (!fp) return;

	memset(mac, 0, 32);
	while (fgets(line, sizeof(line), fp)) {
		newline = strtok(line, "\n");
		sku = strtok(newline, ",");
		model = strtok(NULL, ",");
		type = strtok(NULL, ",");
		if (strcasestr(ua, sku)) {
			DEBUGP("[%s][%d]find model:%s sku:%s type:%s ip:%s\n", __FILE__, __LINE__, model, sku, type, ip);
			get_mac_from_ip(mac, ip);
			if (mac[0] != '\0') {
				DEBUGP("[%s][%d]find model:%s sku:%s type:%s mac:%s\n", __FILE__, __LINE__, model, sku, type, mac);
				snprintf(cmd, sizeof(cmd), "devices_info update \"%s\" \"%s\" \"%s\"", mac, type, model);
				system(cmd);
			}
			break;
		}
	}

	fclose(fp);
	return;
}

void get_ip_string(unsigned int hexip, char *ip) {
	char *org;
	org = &hexip;
	snprintf(ip, 32, "%u.%u.%u.%u", org[3], org[2], org[1], org[0]);
}

void XHTTP_rule(int flag) {
	if (flag == 0) {
		system("iptables -t mangle -D PREROUTING -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 80 -j XHTTP");
		system("iptables -t mangle -D PREROUTING -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 8080 -j XHTTP");
	} else if (flag == 1) {
		system("iptables -t mangle -I PREROUTING 1 -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 80 -j XHTTP");
		system("iptables -t mangle -I PREROUTING 1 -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 8080 -j XHTTP");
	} else {
		system("[ \"x$(iptables -t mangle -nvL |grep XHTTP)\" = \"x\" ] && iptables -t mangle -I PREROUTING 1 -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 80 -j XHTTP && iptables -t mangle -I PREROUTING 1 -i br0 -p tcp --tcp-flags ALL PSH,ACK --dport 8080 -j XHTTP");
	}
}

int check_count;

void signal_handler(int sig) {
	if (sig == SIGALRM) {
		check_count++;
		if (check_count != CHECKTIME) {
			XHTTP_rule(2);
			//reset timer
			struct itimerval tv;
			tv.it_value.tv_sec = TIMERINTERVAL;
			tv.it_value.tv_usec = 0;
			tv.it_interval.tv_sec = 0;
			tv.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &tv, 0);
		}
	}

	if (check_count == CHECKTIME || sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
		XHTTP_rule(0);
		if (sig == SIGALRM)
			DEBUGP("[%s][%d]time out\n", __FILE__, __LINE__);
		unlink(PIDFILE);
		exit(0);
	}
}

int main(int argc, char ** argv)
{
	int sock_fd, data_len, fd, pid;
	struct sockaddr_nl loc_addr, peer_addr;
	static unsigned char *buf;
	struct nlmsghdr *nlh;
	unsigned int addrlen;
//	struct stat fstat;
//	char old_file[132] = {0};
	struct sigaction sa;
	struct itimerval tv;
	FILE *fp;
	char s[32];

//	if (stat(LOGFILE, &fstat) == 0 && fstat.st_size > 5242880) {
//		sprintf(old_file, "%s.old", LOGFILE);
//		rename(LOGFILE, old_file);
//	}
//	fd = open(LOGFILE, O_CREAT | O_APPEND | O_RDWR);
//	if (fd == -1) {
//		fprintf(stderr, "[%s][%d]open error:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
//		return -1;
//	}
//	dup2(fd, 1);

	daemon(1, 1);

	pid = getpid();
	if (access(PIDFILE, F_OK) == 0) {
		fp = fopen(PIDFILE, "r");
		if(fp == NULL)
			return -1;
		fgets(s, sizeof(s), fp);
		if (atoi(s) != pid){
			fclose(fp);
			DEBUGP("UA_Parser already open\n");
			return;
		}
		fclose(fp);
	} else {
		fp = fopen(PIDFILE, "w");
		if(fp == NULL)
			return -1;
		fprintf(fp, "%d", pid);
		fclose(fp);
	}

	system("[ \"x$(lsmod | grep 'ipt_xhttp' >/dev/null 2>&1)\" != \"x0\" ] && rmmod /lib/modules/$(uname -r)/ipt_xhttp.ko >/dev/null");
	system("[ \"x$(lsmod | grep 'ipt_xhttp' >/dev/null 2>&1)\" = \"x\" ] && insmod /lib/modules/$(uname -r)/ipt_xhttp.ko >/dev/null");

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = signal_handler;
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	check_count=0;
	tv.it_value.tv_sec = TIMERINTERVAL;
	tv.it_value.tv_usec = 0;
	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &tv, 0);

	sock_fd = socket(PF_NETLINK, SOCK_DGRAM, 7);
	memset(&loc_addr, 0, sizeof(struct sockaddr_nl));
	loc_addr.nl_family = AF_NETLINK;
	loc_addr.nl_pid = getpid();
	loc_addr.nl_groups = 1;
	if ( bind(sock_fd, (struct sockaddr_nl *)&loc_addr, sizeof(loc_addr)) == -1 ) {
		DEBUGP("[%s][%d]bind error:%d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
		goto err;
	}

	memset(&peer_addr, 0, sizeof(struct sockaddr_nl));
	peer_addr.nl_family = AF_NETLINK;
	peer_addr.nl_pid = 0;
	peer_addr.nl_groups = 1;

	addrlen = sizeof(peer_addr);
	buf = (unsigned char *)malloc(BUFLEN);

	XHTTP_rule(1);
	while(1) {
		nl_packet_msg_t *upkt;
		struct iphdr *iph;
		struct http_request *req;
		int i = 0;
		char sip[32], dip[32];

		ssize_t retlen = recvfrom(sock_fd, buf, BUFLEN, 0, (struct sockaddr_nl *)&peer_addr, &addrlen);

		if ( retlen <= 0 || addrlen != sizeof(peer_addr) || peer_addr.nl_pid != 0 ) {
			DEBUGP("[%s][%d]recvfrom error, retlen:%d, %d, %s\n", __FILE__, __LINE__, retlen, errno, strerror(errno));
			continue;
		}

		nlh = (struct nlmsghdr *)buf;

		if (nlh->nlmsg_len > retlen) {
			DEBUGP("[%s][%d]nlmsg error, %d, %s\n", __FILE__, __LINE__, errno, strerror(errno));
			continue;
		}

		upkt = NLMSG_DATA((struct nlmsghdr *)(buf));
		iph = (struct iphdr *) upkt->payload;

		get_ip_string(ntohl(iph->saddr), sip);
		get_ip_string(ntohl(iph->daddr), dip);
		DEBUGP("[%s][%d]src addr:%s, dest addr:%s, len:%d, ip protocol:%d, data_len:%d, http_len:%d\n", __FILE__, __LINE__, sip, dip, ntohs(iph->tot_len),iph->protocol, upkt->data_len, upkt->http_len);


		if (req) {
			DEBUGP("[%s][%d]Get request\n",  __FILE__, __LINE__);
			foreach_header(i, req->headers) {
				if( ! strcasecmp(req->headers[i], "User-Agent") ) {
					DEBUGP("[%s][%d]Get User-Agent:%s\n",  __FILE__, __LINE__, req->headers[i+1]);
					get_model_name_from_UA(req->headers[i+1], sip);
					break;
				}
			}
		}
	}

err:
	close(sock_fd);
	unlink(PIDFILE);
	return 1;
}
