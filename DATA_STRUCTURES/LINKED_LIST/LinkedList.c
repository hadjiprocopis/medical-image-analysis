#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common_IMMA.h"

#include "LinkedList.h"
#include "LinkedListItem.h"

/* private routines */
linkedlist_item	*_linkedlist_top(linkedlist */*a_list*/);
linkedlist_item	*_linkedlist_bottom(linkedlist */*a_list*/);
void		_linkedlist_remove_item(linkedlist_item	*/*an_item*/);
linkedlist_item	*_linkedlist_find_item(linkedlist */*a_list*/, void */*c*/);

/* method to add data into a linked list given just any item in the list and
   a pointer to the data to be appended.
   The new item will be placed at the end of the list.
   params:
	[] *name, the name of this list, just for reporting it
   returns:
	the list if successful, NULL otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist	*linkedlist_new(
		char *name
){
	linkedlist	*new_linkedlist;
	if( (new_linkedlist=(linkedlist *)malloc(sizeof(linkedlist))) == NULL ){
		fprintf(stderr, "linkedlist_new : could not allocate %zd bytes.\n", sizeof(linkedlist));
		return NULL;
	}
	new_linkedlist->name = name==NULL ? strdup("<a linked list>") : strdup(name);
	if( (new_linkedlist->root=linkedlist_item_new((void *)strdup("<root element>"), NULL, NULL)) == NULL ){
		fprintf(stderr, "linkedlist_new : could not create the root element.\n");
		free(new_linkedlist);
		return NULL;
	}
	return new_linkedlist;
}
/* method to add data into a linked list given just any item in the list and
   a pointer to the data to be appended.
   The new item will be placed at the end of the list.
   params:
	[] *an_item, one item in the list - it may not be the parent - we will traverse to the end to append this element
	[] *contents, a pointer (void *) to the data that the new item will contain
	[] char *toString(void *) is a pointer to the print function of the item - you can set this to null if you can't be bothered
	[] void destroy(void *) is a pointer to the destroy function of the item, just you &free if you can't be bothered implementing something, but you really should
   returns:
	true if successful, false otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
int	linkedlist_add_item(
	linkedlist	*a_list,
	void		*contents,	/* the contents of the new element in the list */
	char		*toString(void *),
	void		destroy(void *)
){
		
	linkedlist_item	*new_item;

	if( (new_item=(linkedlist_item *)malloc(sizeof(linkedlist_item))) == NULL ){
		fprintf(stderr, "linkedlist_add_item : could not allocate %zd bytes.\n", sizeof(linkedlist_item));
		return FALSE;
	}
	new_item->c = contents;
	new_item->toString = toString;
	new_item->destroy = destroy==NULL ? &free : destroy;

/*	add at the bottom, a bit slow */
/*	if( (p=_linkedlist_bottom(a_list)) == NULL ) p = a_list->root;
	p->n = new_item;
	new_item->n = NULL;
	new_item->p = p;

	return TRUE;
*/

/* add at the top */
	new_item->p = a_list->root;
	if( (new_item ->n=a_list->root->n) != NULL ) a_list->root->n->p = new_item;
	a_list->root->n = new_item;
	return TRUE;
}

/* method to find a linked list item, give its contents,
   it basically compares the pointers
   params:
	[] *a_list, the list to search in for the item
	[] *c, a void * pointer to the contents of the data that an item allegedly holds
   returns:
	the linkedlist item if found or NULL otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist_item	*_linkedlist_find_item(
	linkedlist	*a_list,
	void		*c
){
	linkedlist_item *an_item = a_list->root;

	while( (an_item=NEXT_ITEM(an_item)) != NULL )
		if( an_item->c == c ) return an_item;
	return NULL;
}

/* method to remove an item from the list, given its contents and trying
   to match it with items in the list with the same contents pointer
   params:
	[] *a_list, the list to remove the item from
	[] *c, the contents
   returns:
	TRUE if the the item was found and removed, FALSE otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
int	linkedlist_remove_item(
	linkedlist	*a_list,
	void	*c
){
	linkedlist_item	*an_item;
	if( (an_item=_linkedlist_find_item(a_list, c)) == NULL ){
		fprintf(stderr, "linkedlist_remove_item : could not find item with contents at %p\n", c);
		return FALSE;
	}
	_linkedlist_remove_item(an_item);
	free(an_item);
	return TRUE;
}

/* private method to remove an item from the list. This method WILL NOT find
   an item and then remove it but just ONLY change its next and previous items.
   This method WILL NOT destroy the item - you should use the DESTROY_ITEM(linkedlist_item *)
   macro as defined in LinkedList.h
   params:
	[] *an_item, one item in the list that should be removed
   returns:
	nothing
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
void	_linkedlist_remove_item(
	linkedlist_item	*an_item
){
		
	if( an_item == NULL ) return;

	an_item->p->n = an_item->n;
	if( an_item->n != NULL ) an_item->n->p = an_item->p;

	an_item->n = an_item->p = NULL;
}

/* method to merge two lists together. The parent of the first list (first parameter)
   will be the parent of the merged result. The last element of the second list
   will be the last element of the merged result.
   No new items will be created, data will not be duplicated.
   The merged result will be just a relocation of the two lists, so do not
   destroy them afterwards.
   params:
	[] *first_list, *second_list, the two lists to merge with as many elements
	   as you want.
   returns:
	true or false but the merged result will be held in the first list.
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist	*linkedlist_merge_lists(
	linkedlist	*first_list,
	linkedlist	*second_list
){
	linkedlist_item	*last;
	linkedlist	*merged;
	char		*name,
			*n1 = (first_list==NULL)||(first_list->name==NULL) ? strdup("<null>") : first_list->name,
			*n2 = (second_list==NULL)||(second_list->name==NULL) ? strdup("<null>") : second_list->name;

	if( (name=(char *)malloc(strlen(n1)+strlen(n2)+15)) == NULL ){
		fprintf(stderr, "linkedlist_merge_lists : could not allocate %zd bytes.\n", strlen(first_list->name)+strlen(second_list->name)+12);
		return NULL;
	}

	if( (merged=linkedlist_new(name)) == NULL ){
		fprintf(stderr, "linkedlist_merge_lists : call to linkedlist_new has failed for the 2 lists '%s' and '%s'.\n", n1, n2);
		free(name);
		return NULL;
	}

	if( (first_list!=NULL) && (first_list->root->n!=NULL) ){
		merged->root->n = first_list->root->n;
		merged->root->n->p = merged->root;
	 	if( (second_list!=NULL) && (second_list->root->n!=NULL) ){
			if( (last=_linkedlist_bottom(first_list)) == NULL ){
				fprintf(stderr, "linkedlist_merge_lists : call to _linklist_bottom has failed for the 2 lists '%s' and '%s'.\n", n1, n2);
				free(name); linkedlist_destroy(merged, FALSE);
				return NULL;
			}
			last->n = second_list->root->n;
			last->n->p = last;
		}
	} else {
	 	if( (second_list!=NULL) && (second_list->root->n!=NULL) ){
			merged->root->n = second_list->root->n;
			merged->root->n->p = merged->root;
		}
	}
	/* destroy the lists but not their elements */
	if( first_list != NULL ) linkedlist_destroy(first_list, FALSE);
	if( second_list != NULL ) linkedlist_destroy(second_list, FALSE);
	return merged;
}

/* method to traverse a linked list and return the last element in it.
   params:
	[] *a_list, a list to get its last element
   returns:
	the last element in the list or null if the input parameter is null
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist_item	*_linkedlist_bottom(
	linkedlist	*a_list
){
	linkedlist_item *last = a_list->root->n, *next;

	if( last == NULL ) return NULL;

	while( (next=NEXT_ITEM(last)) != NULL ) last = next;

	return last;
}

/* method to traverse a linked list and return the first element in it.
   params:
	[] *a_list to get its first element (after the root)
   returns:
	the first element in the list or null if the input parameter is null
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
linkedlist_item	*_linkedlist_top(
	linkedlist	*a_list
){
	return a_list->root->n;
}

/* method to destroy a list and all its elements
   params:
	[] *a_list to be destroyed
	[] destroy_elements_flag, if TRUE the elements of the list will be destroyed
   returns:
	TRUE if successful, FALSE otherwise
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
int	linkedlist_destroy(
	linkedlist	*a_list,
	int		destroy_elements_flag
){
	linkedlist_item *last, *previous;

	if( (last=_linkedlist_bottom(a_list)) != NULL ){
		/* the list has elements, destroy them (including root) */
		if( destroy_elements_flag )
			while( (previous=PREVIOUS_ITEM(last)) != NULL ){
				DESTROY_ITEM(last); /* << this will call the destroy function for the item */
				free(last);
				last = previous;
			}
		else
			while( (previous=PREVIOUS_ITEM(last)) != NULL ){
				free(last);
				last = previous;
			}
	}
	free(a_list->name);
	free(a_list);

	return TRUE;
}
	
/* method to print to stdout a list and all its elements
   params:
	[] *a_list to be printed WITH all its elements
   returns:
	nothing
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
void	linkedlist_print(
	linkedlist	*a_list
){
	linkedlist_item *first = a_list->root;

	printf("list '%s':\n", a_list->name);
	while( (first=NEXT_ITEM(first)) != NULL ){
		printf("%s\n", PRINT_ITEM(first));
	}
}

/* method to return a string of a printout of the list and all its elements
   params:
	[] *a_list to be printed WITH all its elements
   returns:
	the string of the contents
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
char	*linkedlist_toString(
	linkedlist	*a_list
){
	linkedlist_item *first = a_list->root;
	char		**s = NULL, *ret, *p;
	int		i, l, count;

	if( (count=linkedlist_count_items(a_list)) > 0 ){
		if( (s=(char **)malloc(count * sizeof(char *))) == NULL ){
			fprintf(stderr, "linkedlist_toString : could not allocate %zd bytes (s).\n", count * sizeof(char *));
			return strdup("<failed>");
		}
		for(i=0,l=0;i<count;i++){
			first = NEXT_ITEM(first);
			s[i] = PRINT_ITEM(first);
			l += strlen(s[i]) + 3;
		}
		if( (ret=(char *)malloc(l)) == NULL ){
			fprintf(stderr, "inkedlist_toString : could not allocate %d bytes (ret).\n", l);
			for(i=0,l=0;i<count;i++) free(s[i]);
			free(s);
			return strdup("<failed>");
		}
		p = &(ret[0]);
		for(i=0;i<count;i++){ sprintf(p, "%s\n", s[i]); p = &(ret[strlen(ret)]); free(s[i]); }
		free(s);
		return ret;
	}
	return "<empty>";
}

/* method to count the number of items in a list
   params:
	[] *a_list
   returns:
	the number of items in the list (excluding the private root element)
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
int	linkedlist_count_items(
	linkedlist	*a_list
){
	int	count = 0;
	linkedlist_item *first = a_list->root;

	while( (first=NEXT_ITEM(first)) != NULL ) count++;

	return count;
}

/* method to return an array of the elements of the linked list
   params:
	[] *a_list
	[] *numItems, it will be set to the number of items in the list
   returns:
	an array of void *, which are the elements in the list
   author:
   	Andreas Hadjiprocopis, NMR, ION, 2001
*/   
void	**linkedlist_get_items(
	linkedlist	*a_list,
	int		*numItems
){
	int		i = 0;
	void		**ret;
	linkedlist_item *an_item = a_list->root;

	if( (*numItems=linkedlist_count_items(a_list)) == 0 ) return NULL;
	if( (ret=(void **)malloc(*numItems*sizeof(void *))) == NULL ){
		fprintf(stderr, "linkedlist_get_items : could not allocate %zd bytes.\n", *numItems*sizeof(void *));
		*numItems = 0;
		return NULL;
	}

	while( (an_item=NEXT_ITEM(an_item)) != NULL ) ret[i++] = an_item->c;
	return ret;
}

