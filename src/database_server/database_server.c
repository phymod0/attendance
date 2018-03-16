


#ifndef DATABASE_SERVER_C
#define DATABASE_SERVER_C



#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "srecord_list.h"

#include "database_server.h"



/* Records loaded from the database file */
static struct srecord_list *loaded_records;

/* New records to be stored to the database file */
static struct srecord_list *new_records;

/* Networking functions */
static int db_socket_new(char *ip, char *port);
static void htonmsg(struct db_message *msg);
static void ntohmsg(struct db_message *msg);
static int recv_data(int sock_fd, char *recv_buf, int recv_len);
static int send_data(int sock_fd, char *send_buf, int send_len);
static int recv_msg(int sock_fd, struct db_message *msg);
static int send_msg(int sock_fd, struct db_message *msg);

/* srecord_list callback functions */
static int db_find_record(struct srecord *record, void *cb_data);
static int db_commit_record(struct srecord *record, void *cb_data);

/* Database functions */
static int retrieve_record(struct msg_data *data);
static int store_record(struct msg_data *data);
static int commit_records(void);

/* Database message handler */
static int database_server_handle_msg(struct db_message *msg, int sock_fd);



int main(int argc, char **argv) {

	struct db_message *msg;
	unsigned char recv_buf[MSG_LEN];
	int database_sockfd, conn_sockfd;

	if ( argc != 3 ) {
		printf("Usage: %s <IP> <PORT>\n", argv[0]);
		return -1;
	}

	if ( (database_sockfd = db_socket_new(argv[1], argv[2])) < 0 )
		return -1;

	if ( !(loaded_records = srecord_list_load(DB_FILE)) ) {
		close(database_sockfd);
		return -1;
	}
	if ( !(new_records = srecord_list_new()) ) {
		srecord_list_free(loaded_records);
		close(database_sockfd);
		return -1;
	}

	msg = (struct db_message*)recv_buf;

	while ( 1 ) {

		struct sockaddr_in peer_addr;
		socklen_t peer_addrlen;

		peer_addrlen = sizeof(peer_addr);
		conn_sockfd = accept(database_sockfd, (struct sockaddr*)&peer_addr, &peer_addrlen);

		if ( conn_sockfd < 0 ) {
			perror("accept() failed");
			if ( errno == EBADF || errno == EINVAL || errno == ENOTSOCK ) break;
			close(conn_sockfd);
			continue;
		}

		if ( recv_msg(conn_sockfd, msg) < 0 ) {
			close(conn_sockfd);
			continue;
		}

		if ( msg->operation == OP_EXIT )
			break;

		if ( database_server_handle_msg(msg, conn_sockfd) < 0 ) {
			close(conn_sockfd);
			continue;
		}

		close(conn_sockfd);

	}

	commit_records();

	close(conn_sockfd);

	srecord_list_free(new_records);
	srecord_list_free(loaded_records);

	close(database_sockfd);

	return 0;

}



/*
 *
 * XXX: Cheap workaround: assume that we either set the roll
 * number to 0 expecting to equate the MAC address or set it
 * to non-zero expecting to equate the roll number.
 *
 * Re-implementation: the equality decision must be taken
 * from a separate field in the msg_data struct.
 *
 */
static int db_find_record(struct srecord *record, void *cb_data) {

	struct msg_data *data = (struct msg_data*)cb_data;

	if ( data->roll_number ) {
		/* Use the roll number to match a record */
		if ( data->roll_number != record->roll_number )
			return 0;
		memcpy(data->mac_addr, record->mac_addr, 6);
		strcpy(data->name, record->name);
	} else {
		/* Use the MAC address to match a record */
		if ( strncmp(data->mac_addr, record->mac_addr, 6) != 0 )
			return 0;
		data->roll_number = record->roll_number;
		strcpy(data->name, record->name);
	}

	return 1;

}

static int db_commit_record(struct srecord *record, void *cb_data) {

	FILE *db_fd;
	struct srecord *record_copy;

	// If someone is stupid enough to log in twice
	record_copy = srecord_new();
	if ( !record_copy )
		return 0;
	record_copy->roll_number = record->roll_number;
	strncpy(record_copy->mac_addr, record->mac_addr, 6);
	if ( !(record_copy->name = strdup(record->name)) ) {
		free(record_copy);
		return 0;
	}
	srecord_list_insert(loaded_records, record_copy);

	// Write to database file
	db_fd = *(FILE**)(cb_data);
	fprintf(db_fd, "%02x:%02x:%02x:%02x:%02x:%02x|%d|%s\n",
		record->mac_addr[0], record->mac_addr[1],
		record->mac_addr[2], record->mac_addr[3],
		record->mac_addr[4], record->mac_addr[5],
		record->roll_number, record->name
	);

	return 1;

}

// Search for a record given a MAC address
static int retrieve_record(struct msg_data *data) {

	int n_records_found;

	n_records_found =
		srecord_list_foreach(loaded_records, db_find_record, (void*)data);
	if ( n_records_found > 0 )
		return DB_FOUND;

	n_records_found =
		srecord_list_foreach(new_records, db_find_record, (void*)data);
	if ( n_records_found > 0 )
		return DB_FOUND;

	return DB_NOT_FOUND;

}

// Store a record in the list of new records
static int store_record(struct msg_data *data) {

	int n_records_found;
	struct srecord *new_record;

	n_records_found =
		srecord_list_foreach(loaded_records, db_find_record, (void*)data);
	if ( n_records_found > 0 )
		// Record already exists!
		return DB_BAD_QUERY;

	n_records_found =
		srecord_list_foreach(new_records, db_find_record, (void*)data);
	if ( n_records_found > 0 )
		// Harmless, but still shouldn't happen
		return DB_BAD_QUERY;

	new_record = srecord_new();
	if ( !new_record )
		return DB_OP_FAILED;
	new_record->roll_number = data->roll_number;
	strncpy(new_record->mac_addr, data->mac_addr, 6);
	if ( !(new_record->name = strdup(data->name)) ) {
		free(new_record);
		return DB_OP_FAILED;
	}

	srecord_list_insert(new_records, new_record);

	return DB_OP_SUCCESS;

}

// Save the new records to the database file
static int commit_records(void) {

	FILE *db_fd;
	int n_records_committed, ret;

	/*
	 *
	 * Do not unnecessarily disturb
	 * a possibly write-sensitive
	 * filesystem...
	 *
	 */
	if ( new_records->n_srecords == 0 )
		return DB_OP_SUCCESS;

	ret = DB_OP_SUCCESS;

	db_fd = fopen(DB_FILE, "a");
	if ( !db_fd ) {
		perror("Error appending to database file");
		return DB_OP_FAILED;
	}

	n_records_committed =
		srecord_list_foreach(new_records, db_commit_record, &db_fd);

	if ( n_records_committed < new_records->n_srecords ) {
		printf("Commit warning: Not all temporary records were committed.\n");
		ret = DB_OP_PARTIAL;
	}

	/*
	 * XXX: Emptying all records even if
	 * some were not committed.
	 */
	srecord_list_empty(new_records);

	fclose(db_fd);

	return ret;

}

static int database_server_handle_msg(struct db_message *msg, int sock_fd) {

	int status;

	struct msg_data *data =
		(struct msg_data*)(((char*)msg) + sizeof(struct msg_hdr));

	switch ( msg->operation ) {

		case OP_GET:
			status = retrieve_record(data);
			break;

		case OP_PUT:
			status = store_record(data);
			break;

		case OP_COMMIT:
			status = commit_records();
			break;

		default:
			/* Bad operation, ignore */
			break;

	}

	/*
	 * Bounce the message back replacing the
	 * operation code with the operation status
	 * code.
	 */
	msg->status = status;
	if ( send_msg(sock_fd, msg) < 0 )
		return -1;

	return 0;

}

static int db_socket_new(char *ip, char *port) {

	int database_sockfd;
	struct sockaddr_in server_addr;

	database_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( database_sockfd < 0 ) {
		perror("Socket initialization failed");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = inet_addr(ip);

	if ( bind(database_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		perror("bind() failed");
		return -1;
	}

	if ( listen(database_sockfd, SOMAXCONN) < 0 ) {
		perror("listen() failed");
		return -1;
	}

	return database_sockfd;

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



#endif /* DATABASE_SERVER_C */



