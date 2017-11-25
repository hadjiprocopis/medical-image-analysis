struct _REGION {
	point		o; /* origin */
	dimensions	d; /* dimensions */

	supertype	*root;
};

region	*new_region(int /*x*/, int /*y*/, int /*w*/, int /*h*/);
region	*clone_region(region *);
char	*string_region(region *);
void	destroy_region(region *);
int	calculate_region_expression(region *, region *, operator *, supertype **/*result*/);
