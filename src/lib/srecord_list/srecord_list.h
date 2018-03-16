


/*!

	@file srecord_list.h
	@brief Header file for the srecord_list implementation.

*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*!

	@brief Node in the srecord_list structure.

*/
struct srecord {

	/// Next node in the list
	struct srecord *next;

	int roll_number;
	unsigned char mac_addr[6];
	char *name;

};

/*!

	@brief List of srecord nodes

	Add description here.

*/
struct srecord_list {

	/// Number of nodes in the list
	int n_srecords;

	/// List head
	struct srecord *head;

	/// List tail
	struct srecord *tail;

};

/*!

	@brief Callback function type for srecord_list_foreach.

	The function pointer passed to srecord_list_foreach will
	be casted to this type when the cb_data argument passed to the
	function is non-null.

*/
typedef int (*srecord_callback)(struct srecord*, void*);

/*!

	@brief Callback function type for srecord_list_foreach
	without a pointer to callback data.

	The function pointer passed to srecord_list_foreach will
	be casted to this type when the cb_data argument passed to the
	function is NULL. This removes the need to add a callback data
	argument in the callback functions where it is unnecessary.

*/
typedef int (*srecord_callback_nocbdata)(struct srecord*);



struct srecord *srecord_new(void);
struct srecord_list *srecord_list_new(void);
void srecord_list_push_front(struct srecord_list *list, struct srecord *srecord); // At head
void srecord_list_push_back(struct srecord_list *list, struct srecord *srecord); // At tail
void srecord_list_push(struct srecord_list *list, struct srecord *srecord); // At head
void srecord_list_insert(struct srecord_list *list, struct srecord *srecord); // At tail
struct srecord *srecord_list_front(struct srecord_list *list); // Head
struct srecord *srecord_list_back(struct srecord_list *list); // Tail
struct srecord *srecord_list_top(struct srecord_list *list); // Head
int srecord_list_pop_front(struct srecord_list *list); // Head
int srecord_list_pop(struct srecord_list *list); // Head
int srecord_list_foreach(struct srecord_list *list, void *srecord_cb, void *cb_data);
void srecord_list_empty(struct srecord_list *list);
void srecord_list_free(struct srecord_list *list);
void srecord_list_print(struct srecord_list *list);
struct srecord_list *srecord_list_load(const char *file_name);
void srecord_list_test(char *file_name);



