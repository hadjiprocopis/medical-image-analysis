struct _POINT {
	int		x, y;

	supertype	*root;
};

point	*new_point(int /*x*/, int /*y*/);
point	*clone_point(point *);
char	*string_point(point *);
void	destroy_point(point *);
int	calculate_point_expression(point *, point *, operator *, supertype **/*result*/);

