struct	_BOOLEAN {
	char		v; /* value, TRUE or FALSE */

	supertype	*root;
};

boolean	*new_boolean(char v);
boolean	*clone_boolean(boolean *);
void	destroy_boolean(boolean *);
char	*string_boolean(boolean *);
int	calculate_boolean_expression(boolean *, boolean *, operator *, supertype **/*result*/);
