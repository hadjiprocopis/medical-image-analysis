#ifndef	_PRIVATE_FILTERS_2D_VISITED

/* private - comparators for qsort */
static  int     _DATATYPE_comparator_ascending(const void *, const void *);
//static  int     _DATATYPE_comparator_descending(const void *, const void *);


/* the x and y offsets in order to get the element of a 2D array window centered in the middle */
/* e.g.
	for(i=0+1;i<128-1;i++)
		for(j=0+1;j<128-1;j++){
			sum = 0;
			for(k=0;k<9;k++)
				sum += data[i+ window_3x3_index[k][0]][j + window_3x3_index[k][1]];
		}
*/
static	int	window_3x3_index[9][2] = {
	{-1, -1}, {-1, 0}, {-1, 1},
	{0, -1} , {0, 0} , {0,  1},
	{1, -1} , {1, 0} , {1,  1}
};

/*static	int	window_3x3_index_cross[4][2] = {
	         {-1, 0},
	{0, -1} ,        {0,  1},
		 {1, 0}
};
*/
static	float	_AVERAGE_KERNEL[9] = {
		1.0/9.0,	1.0/9.0,	1.0/9.0,
		1.0/9.0,	1.0/9.0,	1.0/9.0,
		1.0/9.0,	1.0/9.0,	1.0/9.0
};
static	float	_SHARPEN_KERNEL[9] = {
/*		-1.0/9.0,	-1.0/9.0,	-1.0/9.0,
		-1.0/9.0,	8.0/9.0,	-1.0/9.0,
		-1.0/9.0,	-1.0/9.0,	-1.0/9.0*/
		0.0,	-1.0,	0.0,
		-1.0,	5.0,	-1.0,
		0.0,	-1.0,	0.0
};
static	float	_SOBEL_KERNEL_X[9] = {
		-1.0,		0.0,		1.0,
		-2.0,		0.0,		2.0,
		-1.0,		0.0,		1.0
};
static	float	_SOBEL_KERNEL_Y[9] = {
		1.0,		2.0,		1.0,
		0.0,		0.0,		0.0,
		-1.0,		-2.0,		-1.0
};
static	float	_LAPLACIAN_KERNEL[9] = {
		1.0/9.0,	4.0/9.0,	1.0/9.0,
		4.0/9.0,	-20.0/9.0,	4.0/9.0,
		1.0/9.0,	4.0/9.0,	1.0/9.0
};
static	float	_PREWITT_KERNEL_X[9] = {
		-1.0,		0.0,		1.0,
		-1.0,		0.0,		1.0,
		-1.0,		0.0,		1.0
};
static	float	_PREWITT_KERNEL_Y[9] = {
		1.0,		1.0,		1.0,
		0.0,		0.0,		0.0,
		-1.0,		-1.0,		-1.0
};

#define	_PRIVATE_FILTERS_2D_VISITED
#endif
