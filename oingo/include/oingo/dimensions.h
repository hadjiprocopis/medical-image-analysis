struct _DIMENSIONS {
	int		w, h;

	supertype	*root;
};

dimensions	*new_dimensions(int /*w*/, int /*h*/);
dimensions	*clone_dimensions(dimensions *);
char	*string_dimensions(dimensions *);
void	destroy_dimensions(dimensions *);
int	calculate_dimensions_expression(dimensions *, dimensions *, operator *, supertype **/*result*/);

