


#ifndef CGI_COMMON_C
#define CGI_COMMON_C



#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <linux/sockios.h>

#include "cgi_common.h"



void server_print_error(const char *error) {

	printf(ETAG_START);
	printf("The server was unable to "
		"handle your request: %s", error);
	printf(ETAG_END);

	return;

}

void print_mac(unsigned char *mac) {

	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2],
		mac[3], mac[4], mac[5]
	);

	return;

}

int find_macaddr(char *ip_str, unsigned char *mac) {

	int sock_fd;
	struct arpreq areq;
	struct sockaddr_in *addr;

	memset(&areq, 0, sizeof(areq));

	addr = (struct sockaddr_in*)&areq.arp_pa;
	addr->sin_family = AF_INET;
	if ( inet_pton(AF_INET, ip_str, &(addr->sin_addr)) < 1 )
		return -1;

	addr = (struct sockaddr_in*)&areq.arp_ha;
	addr->sin_family = ARPHRD_ETHER;

	strcpy(areq.arp_dev, WIFI_IFACE);

	if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		return -1;

	if ( ioctl(sock_fd, SIOCGARP, (caddr_t)&areq) < 0 ) {
		close(sock_fd);
		return -1;
	}

	memcpy(mac, &(((struct sockaddr*)addr)->sa_data), 6);

	close(sock_fd);

	return 0;

}

void replace_occurrences(char *text1, char *text2, char *line) {

	long len1, len2, remainder;
	char *text_loc, *old_ptr, *new_ptr;
	char new_line[MAXLEN];

	if ( !strstr(line, text1) )
		return;

	len1 = strlen(text1);
	len2 = strlen(text2);

	old_ptr = line;
	new_ptr = new_line;

	while ( text_loc = strstr(old_ptr, text1) ) {

		long len = (long)(text_loc - old_ptr);

		memcpy(new_ptr, old_ptr, len);
		old_ptr += len;
		new_ptr += len;

		memcpy(new_ptr, text2, len2);
		old_ptr += len1;
		new_ptr += len2;

	}

	remainder = (long)(line + MAXLEN - old_ptr);
	memcpy(new_ptr, old_ptr, remainder);

	memcpy(line, new_line, MAXLEN);

	return;

}



#endif /* CGI_COMMON_C */



