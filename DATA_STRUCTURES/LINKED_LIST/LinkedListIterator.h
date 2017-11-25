#ifndef LinkedListIterator_GUARDH
#define LinkedListIterator_GUARDH
struct	_LINKED_LIST_ITERATOR {
	linkedlist	*l;
	linkedlist_item	*i;
};

linkedlist_iterator     *linkedlist_iterator_init(linkedlist */*a_list*/);
int			linkedlist_iterator_has_more(linkedlist_iterator *);
void			*linkedlist_iterator_next(linkedlist_iterator *);
void			destroy_linkedlist_iterator(linkedlist_iterator *);
#endif
