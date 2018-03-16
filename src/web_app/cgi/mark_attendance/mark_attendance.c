


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cgi_common.h"
#include "database_client.h"
#include "attendance_client.h"



int main(void) {

	struct db_msg_data *data;
	int status, content_len, roll_number;
	char *client_ip, *len_str, line[MAXLEN];

	printf("Content-Type:text/html\n\n");

	/* New database message */
	if ( !(data = db_msg_data_new()) ) {
		server_print_error("Failed allocation");
		return -1;
	}

	/* Determine client IP and load MAC address */
	if ( !(client_ip = getenv("REMOTE_ADDR")) ) {
		server_print_error("Failed to determine user IP");
		db_msg_data_destroy(data);
		return -1;
	}
	if ( find_macaddr(client_ip, data->mac_addr) < 0 ) {
		server_print_error("Failed to determine user MAC");
		db_msg_data_destroy(data);
		return -1;
	}

	/* Get the roll number and name from the MAC */
	status = db_get_record(DB_SERVER_IP, DB_SERVER_PORT, data);
	if ( status != DB_FOUND ) {
		/* Bad status */
		printf(ETAG_START);
		printf("Database error: ");
		db_status_print(status);
		printf(ETAG_END);
		db_msg_data_destroy(data);
		return -1;
	}

	/* Determine content length */
	if ( !(len_str = getenv("CONTENT_LENGTH")) ) {
		server_print_error("Request was invalid (1)");
		db_msg_data_destroy(data);
		return -1;
	}
	content_len = atoi(len_str);

	/* Fetch roll number from form input */
	roll_number = 0;
	while ( fgets(line, sizeof(line), stdin) ) {
		if ( memcmp(line, "roll_number=", 12) == 0 ) {
			sscanf(line+12, "%d", &roll_number);
			break;
		}
	}
	if ( !roll_number ) {
		server_print_error("Request was invalid (2)");
		db_msg_data_destroy(data);
		return -1;
	}

	/* Verify roll number from form input */
	if ( roll_number != data->roll_number ) {
		server_print_error("Identity mismatch");
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



