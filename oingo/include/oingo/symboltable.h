struct	_SYMBOLTABLE {
	char		*name;
	type		t;		/* what kind of symbols are we storing ? */
	symbol		*root;		/* the parent of the link-list of symbols this is a dummy symbol - private use */
	int		n;		/* number of elements */
};

/* methods to create a symbol table etc */
symboltable	*new_symboltable(char */*name*/, type /*type*/);
char	*string_symboltable(symboltable */*table*/);
void	destroy_symboltable(symboltable */*table*/);

/* public methods for users to do symboltable manipulation */
/* an item is a supertype, a symbol is a symbol type */
int	add_item_to_symboltable(symboltable */*symt*/, supertype */*item*/, char */* UNIQUE name for this symbol*/);
int	remove_named_symbol_from_symboltable(symboltable */*symt*/, char */*name*/);
symbol	*get_named_symbol_from_symboltable(symboltable */*symt*/, char */*name*/);
symbol	**enumerate_symbols_of_symboltable(symboltable */*symt*/);
int	evaluate_symboltable(symboltable */*symt*/);
