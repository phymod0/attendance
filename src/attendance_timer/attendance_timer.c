
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <ifaddrs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "arecord_list.h"
#include "attendance_client.h"

/* Configuration */
#define SERVER_IP	"192.168.52.1"
#define SERVER_PORT	5432

void output_filename(char *filename) {

	time_t nowtime;
	struct tm *nowtm;

	nowtime = time(NULL);
	nowtm = localtime(&nowtime);

	strftime(filename, 64, "%Y_%B_%d_attendance.txt", nowtm);

	return;

}

int get_own_ip(char *ip) {

	unsigned ref_ip = inet_addr(SERVER_IP);
	struct ifaddrs *ifap, *ifaptr;

	if ( getifaddrs(&ifap) < 0 )
		return -1;

	for ( ifaptr = ifap; ifaptr != NULL; ifaptr = ifaptr->ifa_next ) {

		struct sockaddr_in addr =
			*(struct sockaddr_in*)(ifaptr->ifa_addr);
		unsigned iface_ip = (unsigned)addr.sin_addr.s_addr;

		// Can't trust this line in big endian systems
		if ( ref_ip << 8 != iface_ip << 8 )
			continue;

		inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);

		printf("Using interface: %s (IP: %s)\n\n", ifaptr->ifa_name, ip);

		break;

	}

	freeifaddrs(ifap);

	if ( ifaptr == NULL )
		return -1;

	return 0;

}

void print_status_code(int code) {

	printf("Received status code (%d): ", code);

	switch (code) {
		case AS_PRESENT:
			printf("Student is marked present...\n");
			break;
		case AS_ABSENT:
			printf("Student is marked absent...\n");
			break;
		case AS_LATE:
			printf("Student is marked late...\n");
			break;
		case AS_CONN_FAILED:
			printf("Connection with the server failed...\n");
			break;
		case AS_AUTH_FAILED:
			printf("Authentication failure...\n");
			break;
		case CLIENT_FAILURE:
			printf("There was a problem on the client side...\n");
			break;
		case AS_REQ_DENIED:
			printf("Server denied the requested operation...\n");
			break;
		case AS_SUCCESS:
			printf("Operation successful...\n");
			break;
		default:
			printf("Unknown status code...\n");
	}

	return;

}

struct timeval get_remaining_tv(char *timestr) {

	long int h = 0, m = 0, s = 0;
	time_t nowtime;
	struct tm *nowtm;
	struct timeval tv;
	long int usecs1, usecs2, diff;

	sscanf(timestr, "%ld:%02ld", &h, &m);
	nowtime = time(NULL);
	nowtm = localtime(&nowtime);
	nowtm->tm_hour = h;
	nowtm->tm_min = m;
	nowtm->tm_sec = s;
	usecs2 = 1000000 * mktime(nowtm);

	gettimeofday(&tv, NULL);
	usecs1 = 1000000 * tv.tv_sec + tv.tv_usec;

	diff = usecs2 - usecs1;
	if ( diff < 0 )
		diff = 0;

	tv.tv_sec = diff / 1000000;
	tv.tv_usec = diff % 1000000;

	return tv;

}

int main(int argc, char **argv) {

	int status_code;
	struct timeval tv_limit;
	char output_file[64];
	struct arecord_list *list;
	int port = SERVER_PORT;
	char own_ip[INET_ADDRSTRLEN];
	char server_ip[INET_ADDRSTRLEN] = SERVER_IP;

	printf("\n");

	if ( argc != 2 ) {
		printf("Usage: %s <attendance closing time>\n\n", argv[0]);
		return -1;
	}

	if ( get_own_ip(own_ip) < 0 ) {
		printf("Couldn't find a valid interface."
			" Are you connected to the device?\n");
		return -1;
	}
	list = arecord_list_new();
	tv_limit = get_remaining_tv(argv[1]);

	select(0, NULL, NULL, NULL, &tv_limit);

	status_code = as_close_attendance(own_ip, server_ip, port, list);
	print_status_code(status_code);

	if ( status_code != AS_SUCCESS ) {
		printf("\nAborting...\n\n");
		arecord_list_free(list);
		return -1;
	}

	output_filename(output_file);

	printf("\nPrinting record list...\n");
	arecord_list_print(list);
	printf("\n");
	if ( arecord_list_fprint(list, output_file) < 0 )
		printf("Failed to write to %s\n", output_file);
	else
		printf("Written to %s\n", output_file);
	printf("\n");

	arecord_list_free(list);

	return 0;

}



