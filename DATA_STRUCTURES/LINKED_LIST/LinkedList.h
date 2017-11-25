#ifndef LinkedList_GuardH
#define LinkedList_GuardH
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define	FIRST_ITEM(_l)	((-l)->root->n)

typedef struct _LINKED_LIST_ITEM	linkedlist_item;
typedef	struct _LINKED_LIST		linkedlist;
typedef	struct _LINKED_LIST_ITERATOR	linkedlist_iterator;

struct	_LINKED_LIST {
	char		*name;		/* the name of this list, it can be empty */

	linkedlist_item	*root;		/* root element, inaccessible to users */

	char	*(*toString)(void *);	/* function pointer to returning a description of this list */
	void	 (*destroy)(void *);	/* function pointer to destroying this list an int is a kind of parameter to be used in case you want destroy to be more recursive */
};
	
/* function prototypes */
linkedlist	*linkedlist_new(char */*listname*/);

int		linkedlist_add_item(linkedlist */*a_list*/, void */*contents*/, char *(void *)/*toString method*/, void (void *)/*destroy method*/);
int		linkedlist_remove_item(linkedlist */*a_list*/, void */*an_items_contents_pointer*/);
linkedlist	*linkedlist_merge_lists(linkedlist */*first_list*/, linkedlist */*second_list*/);
int		linkedlist_destroy(linkedlist */*a_list*/, int /*destroy_elements_flag*/);
void		linkedlist_print(linkedlist */*a_list*/);
char		*linkedlist_toString(linkedlist */*a_list*/);
int		linkedlist_count_items(linkedlist */*a_list*/);
void		**linkedlist_get_items(linkedlist */*a_list*/, int */*numItems*/);
#endif
