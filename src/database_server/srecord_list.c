


/*!

	@file srecord_list.c

	@brief Singly linked list data structure for srecord information.

	Simple linked list implementation with stack/queue insertion/removal
	functions for recording srecord information.

	Callback implementations are provided to facilitate unduplicated
	insertion. Unduplicated insertion will be in O(n) but is favored
	over memory inefficiency.

*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "srecord_list.h"



/// Maximum line size to assume when loading srecord records from a file
#define MAX_LINESZ 1024



////////////////////////////////////////////////////////
///////////////////// USER-DEFINED /////////////////////
////////////////////////////////////////////////////////

/*!

	@brief Allocate memory an srecord list node and initialize
	with defaults.

	@return Pointer to created srecord node, or NULL on failure.

*/
struct srecord *srecord_new(void) {

	struct srecord *srecord =
		(struct srecord*)malloc(sizeof(struct srecord));

	if ( !srecord ) {
		printf("srecord_new: Memory allocation failure.\n");
		return NULL;
	}

	memset(srecord, 0, sizeof(struct srecord));

	return srecord;

}

/*!

	@brief Allocate memory for an srecord list and initialize from
	a file of srecord records.

	Each line in the record file must contain the following strings
	(in the same order) separated by the pipe character '|':
		- 12-digit (case-insensitive hex) colon-separated MAC address
		- Student roll number
		- Student name (first name only)
	None of these fields are optional.
	Example:
		- C0:BD:D1:24:26:D9|19100009|Awais

	@param file_name C string containing absolute or relative path
	to record file.

	@return Pointer to created srecord_list, or NULL on failure.

*/
struct srecord_list *srecord_list_load(const char *file_name) {

	FILE *fd;
	char line[MAX_LINESZ];
	struct srecord_list *list;

	fd = fopen(file_name, "rb");
	if (fd == NULL) {
		printf("Failed to open file \"%s\"\n", file_name);
		return NULL;
	}

	list = srecord_list_new();

	while (fgets(line, MAX_LINESZ, fd)) {

		struct srecord *srecord;
		unsigned mac_addr[6];

		srecord = srecord_new();

		srecord->name = (char*)malloc(64);
		/* Get rid of the newline */
		line[strlen(line) - 1] = '\0';
		sscanf(line, "%02x:%02x:%02x:%02x:%02x:%02x|%d|%s",
			&(mac_addr[0]), &(mac_addr[1]),
			&(mac_addr[2]), &(mac_addr[3]),
			&(mac_addr[4]), &(mac_addr[5]),
			&(srecord->roll_number), srecord->name
		);
		srecord->mac_addr[0] = (char)mac_addr[0];
		srecord->mac_addr[1] = (char)mac_addr[1];
		srecord->mac_addr[2] = (char)mac_addr[2];
		srecord->mac_addr[3] = (char)mac_addr[3];
		srecord->mac_addr[4] = (char)mac_addr[4];
		srecord->mac_addr[5] = (char)mac_addr[5];

		srecord_list_insert(list, srecord);

	}

	fclose(fd);

	return list;

}

/*!

	@brief Free memory allocated for an srecord_list node.

	Meant to be passed to srecord_list_foreach as a callback
	function in srecord_list_free and srecord_list_empty.

	@see srecord_list_foreach
	@see srecord_list_free
	@see srecord_list_empty

	Pointer members are assumed to point to allocated regions if
	not NULL, so avoid making modifications to these fields that
	can cause problems for deallocation.

	@param srecord Pointer to the srecord node in the list.

	@return 0 on success, or -1 on failure.

*/
static int free_srecord(struct srecord *srecord) {

	if (srecord->name)
		free(srecord->name);

	free(srecord);

	return 0;

}

/*!

	@brief Print an srecord node in the list.

	Meant to be passed to srecord_list_foreach as a callback
	function in srecord_list_print.

	@see srecord_list_foreach
	@see srecord_list_print

	@param srecord Pointer to the srecord node in the list.

	@return Just 0

*/
static int print_srecord(struct srecord *srecord) {

	printf("Student record:\n");

	printf("\tMAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		srecord->mac_addr[0], srecord->mac_addr[1],
		srecord->mac_addr[2], srecord->mac_addr[3],
		srecord->mac_addr[4], srecord->mac_addr[5]
	);
	if (srecord->roll_number)
		printf("\tRoll number: %d\n", srecord->roll_number);
	if (srecord->name)
		printf("\tName: %s\n", srecord->name);

	return 0;

}



///////////////////////////////////////////////////////
////////////////////// AUTOMATIC //////////////////////
///////////////////////////////////////////////////////

/*!

	@brief Allocate memory for an srecord_list and initialize
	with defaults.

	@return Pointer to the created list, or NULL on failure

*/
struct srecord_list *srecord_list_new(void) {

	struct srecord_list *list =
		(struct srecord_list*)malloc(sizeof(struct srecord_list));

	if ( list == NULL ) {
		printf("srecord_list_new: Allocation failure.\n");
		return NULL;
	}

	list->n_srecords = 0;

	list->head = NULL;
	list->tail = NULL;

	return list;

}

/*!

	@brief Insert an srecord struct pointer at the head
	of the list.

	@param list Pointer to an srecord_list struct
	@param srecord srecord struct pointer to insert

*/
void srecord_list_push_front(struct srecord_list *list, struct srecord *srecord) {

	if (list->n_srecords == 0) {

		srecord->next = NULL;
		list->head = list->tail = srecord;

	} else {

		srecord->next = list->head;
		list->head = srecord;

	}

	list->n_srecords += 1;

	return;

}

/*!

	@brief Insert an srecord struct pointer at the tail
	of the list.

	@param list Pointer to an srecord_list struct
	@param srecord srecord struct pointer to insert

*/
void srecord_list_push_back(struct srecord_list *list, struct srecord *srecord) {

	if (list->n_srecords == 0) {

		srecord->next = NULL;
		list->head = list->tail = srecord;

	} else {

		srecord->next = NULL;
		list->tail->next = srecord;
		list->tail = srecord;

	}

	list->n_srecords += 1;

	return;

}

/*!

	@brief Insert an srecord struct pointer at the head
	of the list.

	@param list Pointer to an srecord_list struct
	@param srecord srecord struct pointer to insert

*/
void srecord_list_push(struct srecord_list *list, struct srecord *srecord) {

	srecord_list_push_front(list, srecord);

	return;

}

/*!

	@brief Insert an srecord struct pointer at the tail
	of the list.

	@param list Pointer to an srecord_list struct
	@param srecord srecord struct pointer to insert

*/
void srecord_list_insert(struct srecord_list *list, struct srecord *srecord) {

	srecord_list_push_back(list, srecord);

	return;

}

/*!

	@brief Retrieve a pointer to the head of the list.

	@param list Pointer to an srecord_list struct

	@return Pointer to the head of the list

*/
struct srecord *srecord_list_front(struct srecord_list *list) {

	return list->head;

}

/*!

	@brief Retrieve a pointer to the tail of the list.

	@param list Pointer to an srecord_list struct

	@return Pointer to the tail of the list

*/
struct srecord *srecord_list_back(struct srecord_list *list) {

	return list->tail;

}

/*!

	@brief Retrieve a pointer to the head of the list.

	@param list Pointer to an srecord_list struct

	@return Pointer to the head of the list

*/
struct srecord *srecord_list_top(struct srecord_list *list) {

	return list->head;

}

/*!

	@brief Remove the head of the list.

	@param list Pointer to an srecord_list struct

	@return 0 on success, -1 if there was nothing to remove

*/
int srecord_list_pop_front(struct srecord_list *list) {

	struct srecord *next;

	if (list->n_srecords == 0)
		return -1;

	if (list->n_srecords == 1) {

		free_srecord(list->head);
		list->head = list->tail = NULL;
		list->n_srecords = 0;

		return 0;

	}

	next = list->head->next;
	free_srecord(list->head);
	list->head = next;
	list->n_srecords -= 1;

	return 0;

}

/*!

	@brief Remove the head of the list.

	@param list Pointer to an srecord_list struct

	@return 0 on success, -1 if there was nothing to remove

*/
int srecord_list_pop(struct srecord_list *list) {

	return srecord_list_pop_front(list);

}

/*!

	@brief Invoke a callback function on all srecord pointers
	in the list.

	The callback function must take a pointer to a srecord
	struct, optionally a pointer to callback data, and return an
	integer result. The return value of this function will be the
	sum of results returned by each invocation of the callback
	function. This is intended to be useful where the callback
	would be an indicator function, and we want the number of
	successes after iterating over all list nodes.

	srecord_cb is taken as a void pointer to a function and
	the actual function type is decided at runtime as either a
	function taking an srecord struct pointer or a function
	taking both an srecord struct pointer and callback data.
	If cb_data is NULL then srecord_cb is assumed to be of
	the former type, otherwise it is assumed to be of the latter
	type and cb_data will be passed to the function as a second
	argument.

	@param list Pointer to an srecord_list struct
	@param srecord_cb Function pointer to callback function
	@param cb_data Pointer to callback data

	@see srecord_callback
	@see srecord_callback_nocbdata

	@return Integer sum of invocation results

*/
int srecord_list_foreach(struct srecord_list *list, void *srecord_cb, void *cb_data) {

	int sum = 0;

	struct srecord *iter = list->head;

	if ( cb_data )

		while (iter) {
			struct srecord *next = iter->next;
			sum += ((srecord_callback)srecord_cb)(iter, cb_data);
			iter = next;
		}

	else

		while (iter) {
			struct srecord *next = iter->next;
			sum += ((srecord_callback_nocbdata)srecord_cb)(iter);
			iter = next;
		}

	return sum;

}

/*!

	@brief Destroy all nodes and empty the list.

	@param list Pointer to an srecord_list struct

*/
void srecord_list_empty(struct srecord_list *list) {

	srecord_list_foreach(list, free_srecord, NULL);

	list->n_srecords = 0;

	list->head = NULL;
	list->tail = NULL;

	return;

}

/*!

	@brief Destroy all nodes along with the list.

	@param list Pointer to an srecord_list struct

*/
void srecord_list_free(struct srecord_list *list) {

	srecord_list_foreach(list, free_srecord, NULL);

	free(list);

	return;

}

/*!

	@brief Print all nodes in the list.

	Printing will be done as specified in print_srecord.

	@see print_srecord

	@param list Pointer to an srecord_list struct

*/
void srecord_list_print(struct srecord_list *list) {

	srecord_list_foreach(list, print_srecord, NULL);

	return;

}

/*!

	@brief Test implementations.

	Loads a list from a file, displays it and destroys the list.

	@param file_name Relative or absolute path to the record file.

*/
void srecord_list_test(char *file_name) {

	struct srecord_list *list = srecord_list_load(file_name);

	srecord_list_print(list);

	srecord_list_free(list);

	return;

}




