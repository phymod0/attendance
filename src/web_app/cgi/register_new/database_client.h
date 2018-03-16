


#ifndef DATABASE_CLIENT_H
#define DATABASE_CLIENT_H

/* Status codes */
#define DB_OP_SUCCESS	0x00
#define DB_OP_FAILED	0x01
#define DB_OP_PARTIAL	0x02
#define DB_FOUND	0x03
#define DB_NOT_FOUND	0x04
#define DB_BAD_QUERY	0x05
#define DB_CONN_FAILED	0x06



struct db_msg_data {

	int roll_number;
	unsigned char mac_addr[6];
	char name[0];

} __attribute__((packed));



struct db_msg_data *db_msg_data_new(void);
int db_get_record(char *server_ip, int server_port, struct db_msg_data *query);
int db_put_record(char *server_ip, int server_port, struct db_msg_data *record);
int db_commit_all(char *server_ip, int server_port);
int db_send_exit(char *server_ip, int server_port);
void db_msg_data_destroy(struct db_msg_data *data);
void db_status_print(int status);



#endif /* DATABASE_CLIENT_H */



