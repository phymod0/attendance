


/*!

	@file arecord_list.h
	@brief Header file for the arecord_list implementation.

*/



#ifndef ARECORD_LIST_H
#define ARECORD_LIST_H



#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*!

	@brief Node in the arecord_list structure.

*/
struct arecord {

	/// Next node in the list
	struct arecord *next;

	long long tv_sec;
	long long tv_usec;
	int roll_number;

};

/*!

	@brief List of arecord nodes

	Add description here.

*/
struct arecord_list {

	/// Number of nodes in the list
	int n_arecords;

	/// List head
	struct arecord *head;

	/// List tail
	struct arecord *tail;

};

/*!

	@brief Callback function type for arecord_list_foreach.

	The function pointer passed to arecord_list_foreach will
	be casted to this type when the cb_data argument passed to the
	function is non-null.

*/
typedef int (*arecord_callback)(struct arecord*, void*);

/*!

	@brief Callback function type for arecord_list_foreach
	without a pointer to callback data.

	The function pointer passed to arecord_list_foreach will
	be casted to this type when the cb_data argument passed to the
	function is NULL. This removes the need to add a callback data
	argument in the callback functions where it is unnecessary.

*/
typedef int (*arecord_callback_nocbdata)(struct arecord*);



struct arecord *arecord_new(void);
struct arecord_list *arecord_list_new(void);
void arecord_list_push_front(struct arecord_list *list, struct arecord *arecord); // At head
void arecord_list_push_back(struct arecord_list *list, struct arecord *arecord); // At tail
void arecord_list_push(struct arecord_list *list, struct arecord *arecord); // At head
void arecord_list_insert(struct arecord_list *list, struct arecord *arecord); // At tail
struct arecord *arecord_list_front(struct arecord_list *list); // Head
struct arecord *arecord_list_back(struct arecord_list *list); // Tail
struct arecord *arecord_list_top(struct arecord_list *list); // Head
int arecord_list_pop_front(struct arecord_list *list); // Head
int arecord_list_pop(struct arecord_list *list); // Head
int arecord_list_foreach(struct arecord_list *list, void *arecord_cb, void *cb_data);
void arecord_list_empty(struct arecord_list *list);
void arecord_list_free(struct arecord_list *list);
void arecord_list_print(struct arecord_list *list);
int arecord_list_fprint(struct arecord_list *list, const char *file_name);
struct arecord_list *arecord_list_load(const char *file_name);
void arecord_list_test(char *file_name);



#endif /* ARECORD_LIST_H */



