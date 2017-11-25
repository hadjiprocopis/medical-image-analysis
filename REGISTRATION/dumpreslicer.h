int	dumpNNreslicer_AIR308(
	int		nativeImageDimensions[],
	int		registeredImageDimensions[],
	double		**es, 
	int		**inputCoordinates, /* input coordinates in native space - origin is top-left  corner (e.g. dispunc) */
	int		**outputCoordinates,  /* output coordinates in registered space - origin the same as above (e.g. dispunc) */
	int		numCoordinates);	 /* the number of coordinates to convert */
int	dumpNNreslicer_AIR525(
	int		nativeImageDimensions[],
	int		registeredImageDimensions[],
	double		**es, 
	int		**inputCoordinates, /* input coordinates in native space - origin is top-left  corner (e.g. dispunc) */
	int		**outputCoordinates,  /* output coordinates in registered space - origin the same as above (e.g. dispunc) */
	int		numCoordinates);	 /* the number of coordinates to convert */

