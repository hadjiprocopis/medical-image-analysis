struct	_EXPRESSION {
	supertype	*a, /* left of the operator */
			*b; /* right of the operator */
	operator	*op;
	supertype	*result;

	supertype	*root;
};

expression	*new_expression(supertype */*a*/, supertype */*b*/, operator *);
expression	*clone_expression(expression *);
void		destroy_expression(expression *);
char		*string_expression(expression *);
int		evaluate_expression(expression *);
int		calculate_expression(expression *);
