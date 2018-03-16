


#ifndef ATTENDANCE_CLIENT_H
#define ATTENDANCE_CLIENT_H

/* Status codes */
#define AS_PRESENT	0x00 /* Attendance server marked you present */
#define AS_ABSENT	0x01 /* Attendance server marked you absent */
#define AS_LATE		0x02 /* Attendance server marked you late */
#define AS_CONN_FAILED	0x03 /* Failed to contact the attendance server */
#define AS_AUTH_FAILED	0x04 /* Failed to authenticate with the attendance server */
#define CLIENT_FAILURE	0x05 /* The operation failed due to a problem with the client */
#define AS_REQ_DENIED	0x06 /* The server denied the request */
#define AS_SUCCESS	0x07 /* The operation succeeded */



#include "arecord_list.h"



int as_mark_attendance(char *client_ip, char *server_ip, int server_port, int roll_number);
int as_close_attendance(char *client_ip, char *server_ip, int server_port, struct arecord_list *list);
void as_status_print(int status);



#endif /* ATTENDANCE_CLIENT_H */



