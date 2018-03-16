


/*
 *
 * TODO:
 *	- Update client API's when done modifying in this directory
 *	- Built-in roster for stricter registration checks
 *	- Change hacky implementations
 *
 */



#include <stdio.h>
#include <stdlib.h>

#include "cgi_common.h"
#include "database_client.h"



void handle_new_student(void);
void handle_registered_student(int roll_number, char *name);



int main(void) {

	int status;
	char *client_ip;
	struct db_msg_data *data;

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

	/* Display result */
	if ( status == DB_NOT_FOUND ) {
		handle_new_student();
	} else if ( status == DB_FOUND ) {
		handle_registered_student(data->roll_number, data->name);
	} else {
		/* Bad status */
		printf(ETAG_START);
		printf("Database error: ");
		db_status_print(status);
		printf(ETAG_END);
	}

	db_msg_data_destroy(data);

	return 0;

}



void handle_new_student(void) {

	printf(
		"<"
		"META HTTP-EQUIV=Refresh CONTENT=\"0; "
		"URL=http://" WLAN_IP "/register_new.html\""
		">"
	);

	return;

}

void handle_registered_student(int roll_number, char *name) {

	FILE *fd;
	char roll[16];
	char line[MAXLEN];

	if ( !(fd = fopen("mark_attendance.html", "rb")) ) {
		server_print_error("Failed to load attendance page");
		return;
	}

	sprintf(roll, "%d", roll_number);

	while ( fgets(line, sizeof(line), fd) ) {
		replace_occurrences("__ROLL__", roll, line);
		replace_occurrences("__NAME__", name, line);
		printf("%s", line);
	}

	fclose(fd);

	return;

}



