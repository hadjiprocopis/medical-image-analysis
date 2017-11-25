#ifndef LinkedListItem_GUARDH
#define LinkedListItem_GUARDH
struct	_LINKED_LIST_ITEM {
	void		*c;		/* contents */

	linkedlist_item	*n,		/* next element */
			*p;		/* previous element */

	char	*(*toString)(void *);	/* function pointer to returning a description of this item */
	void	 (*destroy)(void *);	/* function pointer to destroying this item */
};
	
/* some convenience methods for item accession, printing etc. */
/* these methods check first to see if param is NULL */
#define	NEXT_ITEM(__an_item__)		(((__an_item__)==NULL)?(__an_item__):(__an_item__)->n)
#define PREVIOUS_ITEM(__an_item__)	(((__an_item__)==NULL)?(__an_item__):(__an_item__)->p)
#define	PRINT_ITEM(__an_item__)		(((__an_item__)==NULL)?"<null>":(((__an_item__)->toString==NULL)?"<toString: not implemented>":((__an_item__)->toString((__an_item__)->c))))
#define	DESTROY_ITEM(__an_item__)	((((__an_item__)!=NULL)&&((__an_item__)->destroy!=NULL))?(__an_item__)->destroy((__an_item__)->c):NULL)

/* function prototypes */
linkedlist_item	*linkedlist_item_new(void */*contents*/, char *(void *)/*toString method*/, void (void *)/*destroy method*/);
#endif
