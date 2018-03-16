


#ifndef DATABASE_SERVER_H
#define DATABASE_SERVER_H

#define DB_FILE		"/root/attendance-tools-servers/student_records.db"

#define OP_GET		0x00
#define OP_PUT		0x01
#define OP_COMMIT	0x02
#define OP_EXIT		0x03

#define DB_OP_SUCCESS	0x00
#define DB_OP_FAILED	0x01
#define DB_OP_PARTIAL	0x02
#define DB_FOUND	0x03
#define DB_NOT_FOUND	0x04
#define DB_BAD_QUERY	0x05

#define MAXNAMESZ	64
#define MSG_LEN		256



struct msg_hdr {

	union {
		unsigned char operation;
		unsigned char status;
	};

} __attribute__((packed));

struct msg_data {

	int roll_number;
	unsigned char mac_addr[6];
	char name[0];

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



#endif /* DATABASE_SERVER_H */




