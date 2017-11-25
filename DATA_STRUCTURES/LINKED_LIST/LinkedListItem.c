#include <stdio.h>
#include <stdlib.h>

#include "Common_IMMA.h"

#include "LinkedList.h"
#include "LinkedListItem.h"

/* method to create a new item for a linked list. These items are not
   for general use by users.
   params:
	[] *contents, a pointer (void *) to the data that the new item will contain
	[] char *toString(void *) is a pointer to the print function of the item - you can set this to null if you can't be bothered
	[] void destroy(void *) is a pointer to the destroy function of the item, just you &free if you can't be bothered implementing something, but you really should
   returns:
	the item if successful, NULL otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist_item	*linkedlist_item_new(
	void		*contents,	/* the contents of the new element in the list */
	char		*toString(void *),
	void		destroy(void *)
){		
	linkedlist_item	*new_item;

	if( (new_item=(linkedlist_item *)malloc(sizeof(linkedlist_item))) == NULL ){
		fprintf(stderr, "linkedlist_item_new : could not allocate %zd bytes.\n", sizeof(linkedlist_item));
		return NULL;
	}
	new_item->c = contents;
	new_item->toString = toString;
	new_item->destroy = destroy==NULL ? &free : destroy;

	new_item->n = new_item->p = NULL;

	return new_item;
}
