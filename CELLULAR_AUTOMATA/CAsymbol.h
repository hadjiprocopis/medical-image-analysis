#ifndef	_CA_SYMBOL_VISITED

struct	_CA_SYMBOL {
	int	id;	/* the id of the symbol */
	char	*name;	/* the name of the symbol - user specific,
			   have to freed it will be struduped -
			   not important apart for checking */
};


symbol	*ca_new_symbol(int /*id*/, char */*name*/);
void	ca_destroy_symbol(symbol */*a_symbol*/);
char	*ca_toString_symbol(symbol */*a_symbol*/);
void	ca_print_symbol(FILE */*stream*/, symbol */*a_symbol*/);
#define	_CA_SYMBOL_VISITED
#endif
