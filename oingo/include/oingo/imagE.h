struct _IMAGE {
	DATATYPE	***data;	/* data can be accessed by [slice][x][y] */
	char		***changeMap;
	dimensions	d; 		/* image dimensions */
	region		roi;		/* region of interest - if any */
	int		numSlices;	/* how many slices ? */
	int		depth, format;	/* color depth and pixel format */
	supertype	*root;
};

image	*new_image(DATATYPE ***/*data*/, char ***/*changeMap*/, int /*w*/, int /*h*/, int /*numSlices*/, int /*depth*/, int /*format*/);
image	*clone_image(image *);
char	*string_image(image *);
void	destroy_image(image *);
int	calculate_image_expression(image *, image *, operator *, supertype **/*result*/);
