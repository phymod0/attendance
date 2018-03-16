


#ifndef ATTENDANCE_SERVER_H
#define ATTENDANCE_SERVER_H

/* Configuration */
#define AUTH_KEY	0x13243546
#define DB_SERVER_IP	"127.0.0.1"
#define DB_SERVER_PORT	2345

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



/* Attendance server message structs */

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



#endif /* ATTENDANCE_SERVER_H */



