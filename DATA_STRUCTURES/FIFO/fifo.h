typedef	struct _FIFO	fifo;

struct	_FIFO {
	char		*name;
	linkedlist	*c;	/* linked list with items, first item always goes at the bottom of the list */
	int		nC;	/* number of items */
};
