struct	_SYMBOL {
	char		*name;			/* name of symbol */
	supertype	*c;			/* contents */
	symbol		*next, *previous;	/* link list previous and next nodes */
};

symbol	*new_symbol(supertype */*item to add*/, char */* UNIQUE name of symbol you want to give it*/);
symbol	*clone_symbol(symbol */*symbol to clone*/);
char	*string_symbol(symbol */*symb*/);
void	destroy_symbol(symbol */*symb*/);
int	evaluate_symbol(symbol */*symb*/);
