


#ifndef CGI_COMMON_H
#define CGI_COMMON_H



#define DB_SERVER_IP	"127.0.0.1"
#define DB_SERVER_PORT	2345

#define AS_SERVER_IP	"127.0.0.1"
#define AS_SERVER_PORT	5432

#define MAXLEN		1024

#define WIFI_IFACE	"br-lan"
#define WLAN_IP		"192.168.52.1"
#define LOCALHOST_IP	"127.0.0.1"

/* Color specifiers */
#define BG_RED		"<body style=\"background-color:#FFDADA;\">"
#define BG_YELLOW	"<body style=\"background-color:#FFFFDA;\">"
#define BG_GREEN	"<body style=\"background-color:#DAFFDA;\">"
#define BG_BLUE		"<body style=\"background-color:#DADAFF;\">"
#define BG_END		"</body>"
/* Position specifiers */
#define POS_CENTER	"<div align=\"center\"><br><br><h align=\"center\" style=\"font-size:3em; text-align:center;\">"
#define POS_END		"</h></div>"

/* Display tags */
#define ETAG_START	BG_RED POS_CENTER
#define ETAG_END	POS_END BG_END
#define MTAG_START	BG_YELLOW POS_CENTER
#define MTAG_END	POS_END BG_END
#define STAG_START	BG_GREEN POS_CENTER
#define STAG_END	POS_END BG_END
#define ATAG_START	BG_BLUE POS_CENTER
#define ATAG_END	POS_END BG_END



void server_print_error(const char *error);
void print_mac(unsigned char *mac);
int find_macaddr(char *ip_str, unsigned char *mac);
void replace_occurrences(char *text1, char *text2, char *line);



#endif /* CGI_COMMON_H */



