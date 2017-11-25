#ifndef	clunc_GUARDH
#define	clunc_GUARDH
struct	_CL_UNC {
	//DATATYPE	****data; /* [feature][slice][x][y] */
	int		w, h,	/* width and height of ROI */
			x, y,	/* x and y offsets of ROI */
			W, H;	/* actual image width and height */
	float		sliceSeparation; /* mm between adjacent slices */
	int		actualNumSlices, /* the absolutely actual number of slices in the unc file */
			numSlices;	/* and the number of slices we have read */
};
#endif
