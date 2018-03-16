


/*!

	@file arecord_list.c

	@brief Singly linked list data structure for arecord information.

	Simple linked list implementation with stack/queue insertion/removal
	functions for recording arecord information.

	Callback implementations are provided to facilitate unduplicated
	insertion. Unduplicated insertion will be in O(n) but is favored
	over memory inefficiency.

*/



#ifndef ARECORD_LIST_C
#define ARECORD_LIST_C



#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/time.h>

#include "arecord_list.h"



/// Maximum line size to assume when loading arecord records from a file
#define MAX_LINESZ 1024



////////////////////////////////////////////////////////
///////////////////// USER-DEFINED /////////////////////
////////////////////////////////////////////////////////

/*!

	@brief Allocate memory an arecord list node and initialize
	with defaults.

	@return Pointer to created arecord node, or NULL on failure.

*/
struct arecord *arecord_new(void) {

	struct arecord *arecord =
		(struct arecord*)malloc(sizeof(struct arecord));

	if ( !arecord ) {
		printf("arecord_new: Memory allocation failure.\n");
		return NULL;
	}

	memset(arecord, 0, sizeof(struct arecord));

	return arecord;

}

/*!

	@brief Allocate memory an arecord list and initialize from
	a file of arecord records.

	Each line in the record file must contain the following strings
	(in the same order) separated by the pipe character '|':
		- Roll number
		- seconds.microseconds since epoch to 6 decimal figures
	Example:
		- 19100009|2110453.100584

	@param file_name C string containing absolute or relative path
	to record file.

	@return Pointer to created arecord_list, or NULL on failure.

*/
struct arecord_list *arecord_list_load(const char *file_name) {

	FILE *fd;
	char line[MAX_LINESZ];
	struct arecord_list *list;

	fd = fopen(file_name, "rb");
	if (fd == NULL) {
		printf("Failed to open file \"%s\"\n", file_name);
		return NULL;
	}

	list = arecord_list_new();

	while (fgets(line, MAX_LINESZ, fd)) {

		struct arecord *arecord;

		arecord = arecord_new();

		sscanf(line, "%d|%lld.%06lld\n", &(arecord->roll_number),
			&(arecord->tv_sec), &(arecord->tv_usec));

		arecord_list_insert(list, arecord);

	}

	fclose(fd);

	return list;

}

/*!

	@brief Free memory allocated for an arecord_list node.

	Meant to be passed to arecord_list_foreach as a callback
	function in arecord_list_free and arecord_list_empty.

	@see arecord_list_foreach
	@see arecord_list_free
	@see arecord_list_empty

	Pointer members are assumed to point to allocated regions if
	not NULL, so avoid making modifications to these fields that
	can cause problems for deallocation.

	@param arecord Pointer to the arecord node in the list.

	@return 0 on success, or -1 on failure.

*/
static int free_arecord(struct arecord *arecord) {

	free(arecord);

	return 0;

}

/*!

	@brief Write an arecord node in the list to a file.

	Meant to be passed to arecord_list_foreach as a callback
	function in arecord_list_fprint.x

	@see arecord_list_foreach
	@see arecord_list_fprint

	@param arecord Pointer to the arecord node in the list.
	@param cb_data Pointer to the file descriptor casted to
	void.

	@return Just 0

*/
static int fprint_arecord(struct arecord *arecord, void *cb_data) {

	FILE *fd;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];

	fd = *(FILE**)cb_data;

	nowtime = (time_t)(arecord->tv_sec);
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof(tmbuf), "%I:%M:%S %p", nowtm);

	fprintf(fd, "Student %d was present at %s.\n",
		arecord->roll_number, tmbuf);

	return 0;

}



///////////////////////////////////////////////////////
////////////////////// AUTOMATIC //////////////////////
///////////////////////////////////////////////////////

/*!

	@brief Allocate memory for an arecord_list and initialize
	with defaults.

	@return Pointer to the created list, or NULL on failure

*/
struct arecord_list *arecord_list_new(void) {

	struct arecord_list *list =
		(struct arecord_list*)malloc(sizeof(struct arecord_list));

	if ( list == NULL ) {
		printf("arecord_list_new: Allocation failure.\n");
		return NULL;
	}

	list->n_arecords = 0;

	list->head = NULL;
	list->tail = NULL;

	return list;

}

/*!

	@brief Insert an arecord struct pointer at the head
	of the list.

	@param list Pointer to an arecord_list struct
	@param arecord arecord struct pointer to insert

*/
void arecord_list_push_front(struct arecord_list *list, struct arecord *arecord) {

	if (list->n_arecords == 0) {

		arecord->next = NULL;
		list->head = list->tail = arecord;

	} else {

		arecord->next = list->head;
		list->head = arecord;

	}

	list->n_arecords += 1;

	return;

}

/*!

	@brief Insert an arecord struct pointer at the tail
	of the list.

	@param list Pointer to an arecord_list struct
	@param arecord arecord struct pointer to insert

*/
void arecord_list_push_back(struct arecord_list *list, struct arecord *arecord) {

	if (list->n_arecords == 0) {

		arecord->next = NULL;
		list->head = list->tail = arecord;

	} else {

		arecord->next = NULL;
		list->tail->next = arecord;
		list->tail = arecord;

	}

	list->n_arecords += 1;

	return;

}

/*!

	@brief Insert an arecord struct pointer at the head
	of the list.

	@param list Pointer to an arecord_list struct
	@param arecord arecord struct pointer to insert

*/
void arecord_list_push(struct arecord_list *list, struct arecord *arecord) {

	arecord_list_push_front(list, arecord);

	return;

}

/*!

	@brief Insert an arecord struct pointer at the tail
	of the list.

	@param list Pointer to an arecord_list struct
	@param arecord arecord struct pointer to insert

*/
void arecord_list_insert(struct arecord_list *list, struct arecord *arecord) {

	arecord_list_push_back(list, arecord);

	return;

}

/*!

	@brief Retrieve a pointer to the head of the list.

	@param list Pointer to an arecord_list struct

	@return Pointer to the head of the list

*/
struct arecord *arecord_list_front(struct arecord_list *list) {

	return list->head;

}

/*!

	@brief Retrieve a pointer to the tail of the list.

	@param list Pointer to an arecord_list struct

	@return Pointer to the tail of the list

*/
struct arecord *arecord_list_back(struct arecord_list *list) {

	return list->tail;

}

/*!

	@brief Retrieve a pointer to the head of the list.

	@param list Pointer to an arecord_list struct

	@return Pointer to the head of the list

*/
struct arecord *arecord_list_top(struct arecord_list *list) {

	return list->head;

}

/*!

	@brief Remove the head of the list.

	@param list Pointer to an arecord_list struct

	@return 0 on success, -1 if there was nothing to remove

*/
int arecord_list_pop_front(struct arecord_list *list) {

	struct arecord *next;

	if (list->n_arecords == 0)
		return -1;

	if (list->n_arecords == 1) {

		free_arecord(list->head);
		list->head = list->tail = NULL;
		list->n_arecords = 0;

		return 0;

	}

	next = list->head->next;
	free_arecord(list->head);
	list->head = next;
	list->n_arecords -= 1;

	return 0;

}

/*!

	@brief Remove the head of the list.

	@param list Pointer to an arecord_list struct

	@return 0 on success, -1 if there was nothing to remove

*/
int arecord_list_pop(struct arecord_list *list) {

	return arecord_list_pop_front(list);

}

/*!

	@brief Invoke a callback function on all arecord pointers
	in the list.

	The callback function must take a pointer to a arecord
	struct, optionally a pointer to callback data, and return an
	integer result. The return value of this function will be the
	sum of results returned by each invocation of the callback
	function. This is intended to be useful where the callback
	would be an indicator function, and we want the number of
	successes after iterating over all list nodes.

	arecord_cb is taken as a void pointer to a function and
	the actual function type is decided at runtime as either a
	function taking an arecord struct pointer or a function
	taking both an arecord struct pointer and callback data.
	If cb_data is NULL then arecord_cb is assumed to be of
	the former type, otherwise it is assumed to be of the latter
	type and cb_data will be passed to the function as a second
	argument.

	@param list Pointer to an arecord_list struct
	@param arecord_cb Function pointer to callback function
	@param cb_data Pointer to callback data

	@see arecord_callback
	@see arecord_callback_nocbdata

	@return Integer sum of invocation results

*/
int arecord_list_foreach(struct arecord_list *list, void *arecord_cb, void *cb_data) {

	int sum = 0;

	struct arecord *iter = list->head;

	if ( cb_data )

		while (iter) {
			struct arecord *next = iter->next;
			sum += ((arecord_callback)arecord_cb)(iter, cb_data);
			iter = next;
		}

	else

		while (iter) {
			struct arecord *next = iter->next;
			sum += ((arecord_callback_nocbdata)arecord_cb)(iter);
			iter = next;
		}

	return sum;

}

/*!

	@brief Destroy all nodes and empty the list.

	@param list Pointer to an arecord_list struct

*/
void arecord_list_empty(struct arecord_list *list) {

	arecord_list_foreach(list, free_arecord, NULL);

	list->n_arecords = 0;

	list->head = NULL;
	list->tail = NULL;

	return;

}

/*!

	@brief Destroy all nodes along with the list.

	@param list Pointer to an arecord_list struct

*/
void arecord_list_free(struct arecord_list *list) {

	arecord_list_foreach(list, free_arecord, NULL);

	free(list);

	return;

}

/*!

	@brief Print all nodes in the list.

	Printing will be done as specified in fprint_arecord.

	@see fprint_arecord

	@param list Pointer to an arecord_list struct

*/
void arecord_list_print(struct arecord_list *list) {

	arecord_list_foreach(list, fprint_arecord, &stdout);

	return;

}

/*!

	@brief Write all nodes in the list to a file.

	Writing will be done as specified in fprint_arecord.

	@see fprint_arecord

	@param list Pointer to an arecord_list struct
	@param file_name Path to the output file

	@return 0 on success, -1 on failure

*/
int arecord_list_fprint(struct arecord_list *list, const char *file_name) {

	FILE *fd;

	if ( !(fd = fopen(file_name, "wb")) ) {
		printf("arecord_list_fprint(): Failed to open file \"%s\".\n", file_name);
		return -1;
	}

	arecord_list_foreach(list, fprint_arecord, &fd);

	fclose(fd);

	return 0;

}

/*!

	@brief Test implementations.

	Loads a list from a file, displays it and destroys the list.

	@param file_name Relative or absolute path to the record file.

*/
void arecord_list_test(char *file_name) {

	struct arecord_list *list = arecord_list_load(file_name);

	arecord_list_print(list);

	arecord_list_free(list);

	return;

}



#endif /* ARECORD_LIST_C */



