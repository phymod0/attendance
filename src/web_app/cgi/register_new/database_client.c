


#ifndef DATABASE_CLIENT_C
#define DATABASE_CLIENT_C



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "database_client.h"



/* Operation codes */
#define OP_GET		0x00
#define OP_PUT		0x01
#define OP_COMMIT	0x02
#define OP_EXIT		0x03

/* Configuration */
#define MAXNAMESZ	64
#define MSG_LEN		256



struct db_msg_hdr {

	union {
		unsigned char operation;
		unsigned char status;
	};

} __attribute__((packed));

struct db_message {

	union {
		unsigned char operation;
		unsigned char status;
	};

	int roll_number;
	unsigned char mac_addr[6];
	char name[0];

} __attribute__((packed));



static int ipport2addr(char *ip, int port, struct sockaddr_in *addr);
static int client_socket_new(const char *ip);
static void ntohmsg(struct db_message *msg);
static void htonmsg(struct db_message *msg);
static int send_data(int sock_fd, char *send_buf, int send_len);
static int recv_data(int sock_fd, char *recv_buf, int recv_len);
static int send_msg(int sock_fd, struct db_message *msg);
static int recv_msg(int sock_fd, struct db_message *msg);



struct db_msg_data *db_msg_data_new(void) {

	char *msg_buf;

	if ( !(msg_buf = (char*)malloc(MSG_LEN)) ) {
		printf("db_msg_data_new(): Failed allocation.\n");
		return NULL;
	}

	memset(msg_buf, 0, MSG_LEN);

	return (struct db_msg_data*)(msg_buf + sizeof(struct db_msg_hdr));

}

int db_get_record(char *server_ip, int server_port, struct db_msg_data *query) {

	int conn_sockfd;
	char recv_buf[MSG_LEN];
	struct sockaddr_in server_addr;
	struct db_msg_data *response_data;
	struct db_message *msg, *response;

	response = (struct db_message*)recv_buf;
	response_data = (struct db_msg_data*)(recv_buf + sizeof(struct db_msg_hdr));

	msg = (struct db_message*)(((char*)query) - sizeof(struct db_msg_hdr));
	msg->operation = OP_GET;

	if ( (conn_sockfd = client_socket_new("127.0.0.1")) < 0 )
		return DB_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return DB_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return DB_CONN_FAILED;
	}

	if ( send_msg(conn_sockfd, msg) < 0 )
		return DB_CONN_FAILED;

	if ( recv_msg(conn_sockfd, response) < 0 )
		return DB_CONN_FAILED;

	if ( response->status == DB_FOUND )
		memcpy(query, response_data, MSG_LEN - sizeof(struct db_msg_hdr));

	return response->status;

}

int db_put_record(char *server_ip, int server_port, struct db_msg_data *record) {

	int conn_sockfd;
	char recv_buf[MSG_LEN];
	struct sockaddr_in server_addr;
	struct db_message *msg, *response;

	response = (struct db_message*)recv_buf;

	msg = (struct db_message*)(((char*)record) - sizeof(struct db_msg_hdr));
	msg->operation = OP_PUT;

	if ( (conn_sockfd = client_socket_new("127.0.0.1")) < 0 )
		return DB_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return DB_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return DB_CONN_FAILED;
	}

	if ( send_msg(conn_sockfd, msg) < 0 )
		return DB_CONN_FAILED;

	if ( recv_msg(conn_sockfd, response) < 0 )
		return DB_CONN_FAILED;

	return response->status;

}

int db_commit_all(char *server_ip, int server_port) {

	int conn_sockfd;
	char send_buf[MSG_LEN];
	char recv_buf[MSG_LEN];
	struct sockaddr_in server_addr;
	struct db_message *msg, *response;

	msg = (struct db_message*)send_buf;
	response = (struct db_message*)recv_buf;

	msg->operation = OP_COMMIT;

	if ( (conn_sockfd = client_socket_new("127.0.0.1")) < 0 )
		return DB_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return DB_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return DB_CONN_FAILED;
	}

	if ( send_msg(conn_sockfd, msg) < 0 )
		return DB_CONN_FAILED;

	if ( recv_msg(conn_sockfd, response) < 0 )
		return DB_CONN_FAILED;

	return response->status;

}

int db_send_exit(char *server_ip, int server_port) {

	int conn_sockfd;
	char send_buf[MSG_LEN];
	struct sockaddr_in server_addr;
	struct db_message *msg;

	msg = (struct db_message*)send_buf;
	msg->operation = OP_EXIT;

	if ( (conn_sockfd = client_socket_new("127.0.0.1")) < 0 )
		return DB_CONN_FAILED;

	if ( ipport2addr(server_ip, server_port, &server_addr) < 0 )
		return DB_CONN_FAILED;

	if ( connect(conn_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("connect() failed");
		return DB_CONN_FAILED;
	}

	if ( send_msg(conn_sockfd, msg) < 0 )
		return DB_CONN_FAILED;

	return DB_OP_SUCCESS;

}

void db_msg_data_destroy(struct db_msg_data *data) {

	char *msg_buf = (char*)data;

	free(msg_buf - sizeof(struct db_msg_hdr));

	return;

}

void db_status_print(int status) {

	switch ( status ) {
		case DB_OP_SUCCESS:
			printf("The requested operation was successful.\n");
			break;
		case DB_OP_FAILED:
			printf("The requested operation failed.\n");
			break;
		case DB_OP_PARTIAL:
			printf("Some store operations failed to commit.\n");
			break;
		case DB_FOUND:
			printf("Results were found for the query.\n");
			break;
		case DB_NOT_FOUND:
			printf("No results were found for the query.\n");
			break;
		case DB_BAD_QUERY:
			printf("Ignored bad query.\n");
			break;
		case DB_CONN_FAILED:
			printf("Failed to connect to the database server.\n");
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

static void htonmsg(struct db_message *msg) {

	msg->operation = htonl(msg->operation);
	msg->roll_number = htonl(msg->roll_number);

	return;

}

static void ntohmsg(struct db_message *msg) {

	msg->operation = ntohl(msg->operation);
	msg->roll_number = ntohl(msg->roll_number);

	return;

}

static int recv_msg(int sock_fd, struct db_message *msg) {

	int ret;

	if ( recv_data(sock_fd, (char*)msg, MSG_LEN) < 0 )
		return -1;

	ntohmsg(msg);

	return 0;

}

static int send_msg(int sock_fd, struct db_message *msg) {

	int ret;

	htonmsg(msg);

	if ( send_data(sock_fd, (char*)msg, MSG_LEN) < 0 )
		return -1;

	return 0;

}



#endif /* DATABASE_CLIENT_C */



