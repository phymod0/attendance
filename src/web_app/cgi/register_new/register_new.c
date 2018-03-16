


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cgi_common.h"
#include "database_client.h"
#include "attendance_client.h"



/* XXX: Update the number of admins on every change to the list! */
#define N_ADMINS 2
unsigned char admin_mac_list[N_ADMINS][6] = {
	"\xAC\x5F\x3E\x5C\x72\x44", /* Umair */
	"\x14\xD1\x1F\x56\xA5\x6F", /* Khadija */
};



int read_identity(char *name, int *roll_number);
int is_admin_mac(char *mac);



/*
 *
 * Outline:
 *
 *    - If the MAC address is of an admin then mark
 *    attendance and leave
 *
 *    - Check if MAC address is already registered
 *
 *    - Check if roll number is already registered
 *
 *    - Add new data to database
 *
 *    - Mark attendance
 *
 * XXX: Admins should be redirected to a separate
 * page from the index page. Don't mix up this
 * registration page with the admin interface.
 *
 */
int main(void) {

	int status, roll_number;
	struct db_msg_data *data;
	unsigned char mac_addr[6];
	char *client_ip, name[64];

	printf("Content-Type:text/html\n\n");

	/* Parse name and roll number from form input */
	if ( read_identity(name, &roll_number) < 0 ) {
		server_print_error("Request was invalid");
		return -1;
	}

	/* Determine client IP and MAC address */
	if ( !(client_ip = getenv("REMOTE_ADDR")) ) {
		server_print_error("Failed to determine user IP");
		return -1;
	}
	if ( find_macaddr(client_ip, mac_addr) < 0 ) {
		server_print_error("Failed to determine user MAC");
		return -1;
	}

	/* If this is an admin MAC then just mark the attendance and leave */
	if ( is_admin_mac(mac_addr) ) {
		status = as_mark_attendance(LOCALHOST_IP,
			AS_SERVER_IP, AS_SERVER_PORT, roll_number);
		if ( status == AS_PRESENT ) {
			printf(STAG_START);
			printf("%d was marked present", roll_number);
			printf(STAG_END);
		} else if ( status == AS_ABSENT ) {
			printf(ETAG_START);
			printf("%d was marked absent", roll_number);
			printf(ETAG_END);
		} else if ( status == AS_LATE ) {
			printf(MTAG_START);
			printf("%d was marked late", roll_number);
			printf(MTAG_END);
		} else {
			printf(ETAG_START);
			printf("Attendance server error: ");
			as_status_print(status);
			printf(ETAG_END);
			return -1;
		}
		return 0;
	}

	/* New database message */
	if ( !(data = db_msg_data_new()) ) {
		server_print_error("Failed allocation");
		return -1;
	}

	/* Determine if this MAC is already registered */
	memcpy(data->mac_addr, mac_addr, 6);
	status = db_get_record(DB_SERVER_IP, DB_SERVER_PORT, data);
	if ( status == DB_FOUND ) {
		printf(ETAG_START);
		printf("Request denied: Cannot register "
			"device more than once");
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	} else if ( status != DB_NOT_FOUND ) {
		printf(ETAG_START);
		printf("Database error: ");
		db_status_print(status);
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	}

	/* Determine if this roll number is already registered */
	data->roll_number = roll_number;
	status = db_get_record(DB_SERVER_IP, DB_SERVER_PORT, data);
	if ( status == DB_FOUND ) {
		printf(ETAG_START);
		printf("Request denied: Cannot register "
			"student more than once");
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	} else if ( status != DB_NOT_FOUND ) {
		printf(ETAG_START);
		printf("Database error: ");
		db_status_print(status);
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	}

	/* Add record to database server */
	data->roll_number = roll_number;
	memcpy(data->mac_addr, mac_addr, 6);
	strncpy(data->name, name, sizeof(name));
	status = db_put_record(DB_SERVER_IP, DB_SERVER_PORT, data);
	if ( status != DB_OP_SUCCESS ) {
		printf(ETAG_START);
		printf("Database error: ");
		db_status_print(status);
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	}

	/* Mark attendance */
	status = as_mark_attendance(LOCALHOST_IP,
		AS_SERVER_IP, AS_SERVER_PORT, roll_number);
	if ( status == AS_PRESENT ) {
		printf(STAG_START);
		printf("You were marked present");
		printf(STAG_END);
	} else if ( status == AS_ABSENT ) {
		printf(ETAG_START);
		printf("You were marked absent");
		printf(ETAG_END);
	} else if ( status == AS_LATE ) {
		printf(MTAG_START);
		printf("You were marked late");
		printf(MTAG_END);
	} else {
		printf(ETAG_START);
		printf("Attendance server error: ");
		as_status_print(status);
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	}

	db_msg_data_destroy(data);

	return 0;

}



int is_admin_mac(char *mac) {

	int i;

	for ( i = 0; i < N_ADMINS; i++ )
		if ( memcmp(mac, admin_mac_list[i], 6) == 0 )
			return 1;

	return 0;

}

int read_identity(char *name, int *roll_number) {

	char line[MAXLEN];

	/* Check if we even have form input */
	if ( !getenv("CONTENT_LENGTH") )
		return -1;

	/* Fetch roll number and name from form input */
	*roll_number = 0;
	while ( fgets(line, sizeof(line), stdin) ) {
		if ( memcmp(line, "identity=", 9) == 0 ) {
			sscanf(line+9, "%d%*3s%s", roll_number, name);
			break;
		}
	}
	if ( !*roll_number )
		return -1;

	return 0;

}



