


#ifndef ATTENDANCE_CLIENT_C
#define ATTENDANCE_CLIENT_C

/* Configuration */
#define AUTH_KEY	0x13243546

/* Operation codes */
#define OP_MARK		0x00 /* Mark someone's attendance */
#define OP_CLOSE	0x01 /* Close the attendance server */
#define OP_AUTH		0x02 /* Verify the challenge response for authentication */

/* Response codes */
#define PRESENT		0x00 /* Marked present */
#define ABSENT		0x01 /* Marked absent */
#define LATE		0x02 /* Marked late */
#define CHALLENGE	0x03 /* Sent a challenge for verification */
#define DENY		0x04 /* The requested operation was denied */
#define TIME		0x05 /* Sent the current time for synchronization */
#define DATA		0x06 /* Payload contains data */
#define END_DATA	0x07 /* End of data stream - no data in the payload */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "arecord_list.h"
#include "attendance_client.h"



struct as_msg_hdr {

	union {

		struct {
			int op_code;
			unsigned resp;
		} request __attribute__((packed));

		struct {
			int status;
			unsigned chal;
		} reply __attribute__((packed));

	};

} __attribute__((packed));

struct as_msg_data {

	long long tv_sec;
	long long tv_usec;

	int roll_number;

} __attribute__((packed));

struct as_msg {

	union {

		struct {
			int op_code;
			unsigned resp;
		} request __attribute__((packed));

		struct {
			int status;
			unsigned chal;
		} reply __attribute__((packed));

	};

	long long tv_sec;
	long long tv_usec;

	int roll_number;

} __attribute__((packed));



static unsigned compute_resp(unsigned challenge);
static struct timeval correct_tv(struct timeval tv, struct timeval bad_tv, struct timeval good_tv);
static int ipport2addr(char *ip, int port, struct sockaddr_in *addr);
static int client_socket_new(const char *ip);
static void htonmsg(struct as_msg *msg);
static void ntohmsg(struct as_msg *msg);
static int recv_data(int sock_fd, char *recv_buf, int recv_len);
static int send_data(int sock_fd, char *send_buf, int send_len);
static int recv_msg(int sock_fd, struct as_msg *msg);
static int send_msg(int sock_fd, struct as_msg *msg);



int as_mark_attendance(char *client_ip, char *server_ip, int server_port, int roll_number) {

	int conn_sockfd;
	struct timeval tv;
	struct as_msg msg;
	struct sockaddr_in server_addr;

	gettimeofday(&tv, NULL);

	memset(&msg, 0, sizeof(msg));
	msg.request.op_code = OP_MARK;
	msg.tv_sec = tv.tv_sec;
	msg.tv_usec = tv.tv_usec;
	msg.roll_number = roll_number;

	if ( (conn_sockfd = client_socket_new(client_ip)) < 0 )
		return AS_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return AS_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return AS_CONN_FAILED;
	}

	if ( send_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	if ( recv_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	close(conn_sockfd);

	switch ( msg.reply.status ) {

		case PRESENT:
			return AS_PRESENT;

		case ABSENT:
			return AS_ABSENT;

		case LATE:
			return AS_LATE;

		case DENY:
			return AS_REQ_DENIED;

		default:
			/*
			 * If the status code was not understood
			 * assume this was a connection fault...
			 */
			return AS_CONN_FAILED;

	}

}

int as_close_attendance(char *client_ip, char *server_ip, int server_port, struct arecord_list *list) {

	int conn_sockfd;
	struct as_msg msg;
	unsigned chal, resp;
	struct timeval ref_tv, as_tv;
	struct sockaddr_in server_addr;

	arecord_list_empty(list);

	if ( (conn_sockfd = client_socket_new(client_ip)) < 0 )
		return AS_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return AS_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return AS_CONN_FAILED;
	}

	memset(&msg, 0, sizeof(msg));
	msg.request.op_code = OP_CLOSE;

	if ( send_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	if ( recv_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	if ( msg.reply.status != CHALLENGE )
		return AS_CONN_FAILED;

	chal = msg.reply.chal;
	resp = compute_resp(chal);

	printf("Received challenge: 0x%08x\n", chal);
	printf("Computed response:  0x%08x\n", resp);

	memset(&msg, 0, sizeof(msg));
	msg.request.op_code = OP_AUTH;
	msg.request.resp = resp;

	if ( send_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	if ( recv_msg(conn_sockfd, &msg) < 0 )
		return AS_CONN_FAILED;

	gettimeofday(&ref_tv, NULL);

	if ( msg.reply.status == DENY )
		return AS_AUTH_FAILED;

	if ( msg.reply.status != TIME )
		return AS_AUTH_FAILED;

	as_tv.tv_sec = msg.tv_sec;
	as_tv.tv_usec = msg.tv_usec;

	while ( 1 ) {

		struct arecord *new_record;
		struct timeval tv, corrected_tv;

		if ( recv_msg(conn_sockfd, &msg) < 0 )
			return AS_CONN_FAILED;

		if ( msg.reply.status == END_DATA )
			break;

		if ( !(new_record = arecord_new()) )
			return CLIENT_FAILURE;

		tv.tv_sec = msg.tv_sec;
		tv.tv_usec = msg.tv_usec;
		corrected_tv = correct_tv(tv, as_tv, ref_tv);

		new_record->tv_sec = corrected_tv.tv_sec;
		new_record->tv_usec = corrected_tv.tv_usec;
		new_record->roll_number = msg.roll_number;

		arecord_list_insert(list, new_record);

	};

	close(conn_sockfd);

	return AS_SUCCESS;

}

void as_status_print(int status) {

	switch ( status ) {
		case AS_PRESENT:
			printf("The server marked the entry present.\n");
			break;
		case AS_ABSENT:
			printf("The server marked the entry absent.\n");
			break;
		case AS_LATE:
			printf("The server marked the entry late.\n");
			break;
		case AS_CONN_FAILED:
			printf("Connection with the server failed.\n");
			break;
		case AS_AUTH_FAILED:
			printf("Authentication failed.\n");
			break;
		case CLIENT_FAILURE:
			printf("An error occurred at the client side.\n");
			break;
		case AS_REQ_DENIED:
			printf("The server denied the request.\n");
			break;
		case AS_SUCCESS:
			printf("The operation was successful.\n");
			break;
		default:
			printf("Unrecognized server response.\n");
			break;
	}

	return;

}



static int ipport2addr(char *ip, int port, struct sockaddr_in *addr) {

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	if ( (addr->sin_addr.s_addr = inet_addr(ip)) < 0 )
		return -1;

	return 0;

}

static int client_socket_new(const char *ip) {

	int sock_fd;
	struct sockaddr_in client_addr;

	if ( (sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		perror("Socket initialization failed");
		return -1;
	}

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(ip);

	if ( bind(sock_fd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0 ) {
		perror("bind() failed");
		return -1;
	}

	return sock_fd;

}

static unsigned compute_resp(unsigned challenge) {

	unsigned i, result = challenge;

	for (i = 0; i < 256; i++) {

		unsigned result_low;
		unsigned result_high;

		result ^= AUTH_KEY;

		result_low = result & 0xFFFF;
		result_high = result >> 16;

		result = (result_low << 16) | result_high;

		result += AUTH_KEY;

	}

	return result;

}

static struct timeval correct_tv(struct timeval tv, struct timeval bad_tv, struct timeval good_tv) {

	struct timeval correct_timeval;
	long int usecs, bad_usecs, good_usecs, correct_usecs;

	usecs =  1000000 * tv.tv_sec + tv.tv_usec;
	bad_usecs = 1000000 * bad_tv.tv_sec + bad_tv.tv_usec;
	good_usecs = 1000000 * good_tv.tv_sec + good_tv.tv_usec;

	correct_usecs = usecs + good_usecs - bad_usecs;

	correct_timeval.tv_sec = correct_usecs / 1000000;
	correct_timeval.tv_usec = correct_usecs % 1000000;

	return correct_timeval;

}

static int recv_data(int sock_fd, char *recv_buf, int recv_len) {

	while ( recv_len ) {

		int n_received;

		n_received = recv(sock_fd, recv_buf, recv_len, 0);

		if ( n_received < 0 ) {
			perror("recv() failed");
			return -1;
		}

		recv_buf += n_received;
		recv_len -= n_received;

	}

	return 0;

}

static int send_data(int sock_fd, char *send_buf, int send_len) {

	while ( send_len ) {

		int n_sent;

		n_sent = send(sock_fd, send_buf, send_len, 0);

		if ( n_sent < 0 ) {
			perror("send() failed");
			return -1;
		}

		send_buf += n_sent;
		send_len -= n_sent;

	}

	return 0;

}

static void htonmsg(struct as_msg *msg) {

	msg->request.op_code = htonl(msg->request.op_code);
	msg->request.resp = htonl(msg->request.resp);

	msg->tv_sec = htobe64(msg->tv_sec);
	msg->tv_usec = htobe64(msg->tv_usec);
	msg->roll_number = htonl(msg->roll_number);

	return;

}

static void ntohmsg(struct as_msg *msg) {

	msg->request.op_code = ntohl(msg->request.op_code);
	msg->request.resp = ntohl(msg->request.resp);

	msg->tv_sec = be64toh(msg->tv_sec);
	msg->tv_usec = be64toh(msg->tv_usec);
	msg->roll_number = ntohl(msg->roll_number);

	return;

}

static int recv_msg(int sock_fd, struct as_msg *msg) {

	int ret;

	if ( recv_data(sock_fd, (char*)msg, sizeof(struct as_msg)) < 0 )
		return -1;

	ntohmsg(msg);

	return 0;

}

static int send_msg(int sock_fd, struct as_msg *msg) {

	int ret;

	htonmsg(msg);

	if ( send_data(sock_fd, (char*)msg, sizeof(struct as_msg)) < 0 )
		return -1;

	return 0;

}



#endif /* ATTENDANCE_CLIENT_C */



