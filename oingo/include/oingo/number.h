struct _NUMBER {
	char		*str;		/* original number string */
	int		i;		/* integer version of str, using rounding logic */
	float		f;		/* floating point version of str */

	supertype	*root;
};

number		*new_number(char *);
number		*new_number_number(float);
number		*clone_number(number *);
char		*string_number(number *);
char		*string_integer(number *);
char		*string_float(number *);
void		destroy_number(number *);
int		calculate_number_expression(number *, number *, operator *, supertype **/*result*/);
