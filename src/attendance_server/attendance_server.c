


#ifndef ATTENDANCE_SERVER_C
#define ATTENDANCE_SERVER_C



#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <endian.h>
#include <sys/time.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "arecord_list.h"
#include "database_client.h"

#include "attendance_server.h"



/* List to hold attendance records */
static struct arecord_list *arecords;

/* Flag indicating if attendances are still being recorded */
static int server_open = 1;

/* Networking functions */
static int server_socket_new(char *port);
static void htonmsg(struct as_msg *msg);
static void ntohmsg(struct as_msg *msg);
static int recv_data(int sock_fd, char *recv_buf, int recv_len);
static int send_data(int sock_fd, char *send_buf, int send_len);
static int recv_msg(int sock_fd, struct as_msg *msg);
static int send_msg(int sock_fd, struct as_msg *msg);

/* Record list callbacks */
static int match_record(struct arecord *record, void *cb_data);
static int send_record(struct arecord *record, void *cb_data);

/* Attendance server helpers */
static unsigned gen_random_u32(void);
static unsigned compute_resp(unsigned challenge);
static int register_mark(struct as_msg_data *data);

/* Attendance server functions */
static int handle_mark(struct as_msg_data *data, int sock_fd);
static int handle_close(int sock_fd);

/* Attendance message handler */
static int attendance_server_handle_msg(struct as_msg *msg, int sock_fd);



int main(int argc, char **argv) {

	struct as_msg msg;
	int server_sockfd, conn_sockfd;

	if ( argc != 2 ) {
		printf("Usage: %s <PORT>\n", argv[0]);
		return -1;
	}

	if ( !(arecords = arecord_list_new()) )
		return -1;

	if ( (server_sockfd = server_socket_new(argv[1])) < 0 )
		return -1;

	while ( 1 ) {

		struct sockaddr_in peer_addr;
		socklen_t peer_addrlen;

		peer_addrlen = sizeof(peer_addr);
		conn_sockfd = accept(server_sockfd, (struct sockaddr*)&peer_addr, &peer_addrlen);

		if ( conn_sockfd < 0 ) {
			perror("accept() failed");
			if ( errno == EBADF || errno == EINVAL || errno == ENOTSOCK ) break;
			close(conn_sockfd);
			continue;
		}

		if ( recv_msg(conn_sockfd, &msg) < 0 ) {
			close(conn_sockfd);
			continue;
		}

		/* Only local processes should be trusted for attendance requests */
		if ( peer_addr.sin_addr.s_addr != inet_addr("127.0.0.1") && msg.request.op_code == OP_MARK ) {
			close(conn_sockfd);
			continue;
		}

		if ( attendance_server_handle_msg(&msg, conn_sockfd) < 0 ) {
			close(conn_sockfd);
			continue;
		}

		close(conn_sockfd);

	}

	/* Shouldn't get here unless we're handling interrupts */

	close(conn_sockfd);
	close(server_sockfd);

	arecord_list_free(arecords);

	return 0;

}



static int match_record(struct arecord *record, void *cb_data) {

	int roll_number = *(int*)cb_data;

	if ( roll_number == record->roll_number )
		return 1;

	return 0;

}

static int register_mark(struct as_msg_data *data) {

	int roll_number;
	int n_records_matched;
	struct arecord *new_record;

	roll_number = data->roll_number;

	n_records_matched =
		arecord_list_foreach(arecords, match_record, &roll_number);

	if ( n_records_matched > 0 )
		// Already recorded
		return PRESENT;

	if ( !server_open )
		return ABSENT;

	if ( !(new_record = arecord_new()) )
		return DENY;

	new_record->tv_sec = data->tv_sec;
	new_record->tv_usec = data->tv_usec;
	new_record->roll_number = data->roll_number;

	arecord_list_insert(arecords, new_record);

	return PRESENT;

}

static int handle_mark(struct as_msg_data *data, int sock_fd) {

	int status;
	struct as_msg msg;

	status = register_mark(data);

	memset(&msg, 0, sizeof(msg));
	msg.reply.status = status;

	if ( send_msg(sock_fd, &msg) < 0 )
		return -1;

	return 0;

}

static unsigned gen_random_u32(void) {

	srand(time(NULL));

	return (unsigned)rand();

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

static int send_record(struct arecord *record, void *cb_data) {

	struct as_msg msg;
	int sock_fd = *(int*)cb_data;

	memset(&msg, 0, sizeof(msg));
	msg.reply.status = DATA;
	msg.tv_sec = record->tv_sec;
	msg.tv_usec = record->tv_usec;
	msg.roll_number = record->roll_number;

	if ( send_msg(sock_fd, &msg) < 0 )
		return 0;

	return 1;

}

static int handle_close(int sock_fd) {

	struct as_msg msg;
	struct timeval tv;
	unsigned chal, resp;
	int n_records_sent, db_resp;

	chal = gen_random_u32();
	resp = compute_resp(chal);

	memset(&msg, 0, sizeof(msg));
	msg.reply.status = CHALLENGE;
	msg.reply.chal = chal;

	if ( send_msg(sock_fd, &msg) < 0 )
		return -1;

	if ( recv_msg(sock_fd, &msg) < 0 )
		return -1;

	if ( msg.request.op_code != OP_AUTH )
		return -1;

	if ( msg.request.resp != resp ) {

		memset(&msg, 0, sizeof(msg));
		msg.reply.status = DENY;

		if ( send_msg(sock_fd, &msg) < 0 )
			return -1;

		return 0;

	}

	/* Authentication succeeded at this point */

	gettimeofday(&tv, NULL);

	memset(&msg, 0, sizeof(msg));
	msg.reply.status = TIME;
	msg.tv_sec = tv.tv_sec;
	msg.tv_usec = tv.tv_usec;

	if ( send_msg(sock_fd, &msg) < 0 )
		return -1;

	n_records_sent =
		arecord_list_foreach(arecords, send_record, &sock_fd);

	if ( n_records_sent < arecords->n_arecords )
		return -1;

	memset(&msg, 0, sizeof(msg));
	msg.reply.status = END_DATA;

	if ( send_msg(sock_fd, &msg) < 0 )
		return -1;

	// Commit the database transactions and mark the server closed

	server_open = 0;

	if ( (db_resp = db_commit_all(DB_SERVER_IP, DB_SERVER_PORT)) != DB_OP_SUCCESS )
		printf("Warning, database commit failed with exit code %d.\n", db_resp);

	return 0;

}

static int attendance_server_handle_msg(struct as_msg *msg, int sock_fd) {

	int status = 0;

	struct as_msg_data *data =
		(struct as_msg_data*)(((char*)msg) + sizeof(struct as_msg_hdr));

	switch ( msg->request.op_code ) {

		case OP_MARK:
			status = handle_mark(data, sock_fd);
			break;

		case OP_CLOSE:
			status = handle_close(sock_fd);
			break;

		default:
			/* Bad operation, ignore */
			status = -1;
			break;

	}

	return status;

}

static int server_socket_new(char *port) {

	int server_sockfd;
	struct sockaddr_in server_addr;

	server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( server_sockfd < 0 ) {
		perror("Socket initialization failed");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if ( bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("bind() failed");
		return -1;
	}

	if ( listen(server_sockfd, SOMAXCONN) < 0 ) {
		perror("listen() failed");
		return -1;
	}

	return server_sockfd;

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



#endif /* ATTENDANCE_SERVER_C */



