struct	_STRING {
	char		*s;

	supertype	*root;
};
string		*new_string(char *);
string		*clone_string(string *);
void		destroy_string(string *);
char		*string_string(string *);
int		calculate_string_expression(string *, string *, operator *, supertype **/*result*/);
