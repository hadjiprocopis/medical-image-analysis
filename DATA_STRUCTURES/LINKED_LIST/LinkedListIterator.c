#include <stdio.h>
#include <stdlib.h>

#include "Common_IMMA.h"

#include "LinkedList.h"
#include "LinkedListItem.h"
#include "LinkedListIterator.h"

/* example for use
	linkedlist		list;
	linkedlist_iterator	*iter;
	mytype			*an_item;

	list = ... blah blah initialise and fill linked list with items of type mytype...
	
	for(iter=linkedlist_iterator_init(list);linkedlist_iterator_has_more(iter);){
		an_item = (mytype *)linkedlist_iterator_next(iter);
		...
	}

	// you can use the iterator again, even if its linkedlist has been modified
	for(iter=linkedlist_iterator_init(list);linkedlist_iterator_has_more(iter);){
		an_item = (mytype *)linkedlist_iterator_next(iter);
		...
	}

	// do not forget to destroy the iterator when finished with it
	destroy_linkedlist_iterator(iter);
*/

/* function to return an iterator object which will take you through the
   specified linked list */
linkedlist_iterator     *linkedlist_iterator_init(linkedlist *a_list){
	linkedlist_iterator	*ret;

	if( (ret=(linkedlist_iterator *)malloc(sizeof(linkedlist_iterator))) == NULL ){
		fprintf(stderr, "linkedlist_iterator_init : could not allocate %zd bytes.\n", sizeof(linkedlist_iterator));
		return NULL;
	}
	ret->l = a_list;
	ret->i = ret->l->root;
	return	ret;
}

/* function to check whether the iterator can supply with more elements.
   When this function returns false, you should not use the linkedlist_iterator_next function, below.
   Returns TRUE if there are more items or FALSE otherwise
*/
int	linkedlist_iterator_has_more(linkedlist_iterator *an_iterator){
	return an_iterator->i->n != NULL;
}

/* function to return the next item from the iterator.
   Use this function only after a call to linkedlist_iterator_next returned TRUE */
void	*linkedlist_iterator_next(linkedlist_iterator *an_iterator){
	an_iterator->i = an_iterator->i->n;
	return an_iterator->i->c;
}

/* function to destroy the iterator when done */
void	destroy_linkedlist_iterator(linkedlist_iterator *iter){
	free(iter);
}
