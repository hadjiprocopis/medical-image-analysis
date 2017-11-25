struct	_SUPERTYPE {
	type		t;
	char		*stringType;
	union {
		number		*n;
		string		*s;
		boolean		*b;
		parameter	*p;
		function	*f;
		designator	*d;
		expression	*e;
		operator	*o;
		image		*i;
		dimensions	*dim;
		region		*reg;
		point		*poi;
	} c;
};

void		*clone(type, void *);
void		destroy(type, void *);
supertype	*new_supertype(type, void */*contents*/);
supertype	*clone_supertype(supertype *);
void		destroy_supertype(supertype *);
char		*string_supertype(supertype *);
char		*string_type(type);
int		evaluate_supertype(supertype *);

#define	new(_n_t, _n_c) (((void *)new_supertype((_n_t),(_n_c))))
