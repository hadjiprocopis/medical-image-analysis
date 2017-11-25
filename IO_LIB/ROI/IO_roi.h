#ifndef	_IO_ROI_VISIT

typedef	struct	_ROI_DATA_STRUCT	roi;	/* forward declaration in order to use 'roi' below */

typedef	enum {
	UNKNOWN_ROI_REGION=0,
	RECTANGULAR_ROI_REGION=1,
	ELLIPTICAL_ROI_REGION=2,
	IRREGULAR_ROI_REGION=3,
	UNSPECIFIED_ROI_REGION=4
} roiRegionType;

typedef	struct	_ROI_REGION_RECTANGULAR_DATA_STRUCT {
	float		x0, y0,
			width, height;
	roi		*p;	/* my roi */
} roiRegionRectangular;

typedef	struct	_ROI_REGION_ELLIPTICAL_DATA_STRUCT {
	float		ex0, ey0,
			ea, eb,
			rot;
	roi		*p;	/* my roi */
} roiRegionElliptical;

typedef	struct	_ROI_POINT_IRREGULAR_POINT_DATA_STRUCT {
	int		X, Y, Z;	/* same as x, y and z but rounded and int
					   so that memory arrays can be accessed without typecast */
	float		x, y, z,	/* cartesian position when (0,0,0) is origin */
			r, theta,	/* polar position when centroid is origin */
			dx, dy,		/* deltas from x and y of previous point */
			slope,		/* dy / dx */
			dr, dtheta,	/* deltas from polar coordinates of previous point */
			v		/* pixel value if available */
			;
	roi		*p;		/* my roi */
} roiPoint;

typedef	struct	_ROI_REGION_IRREGULAR_DATA_STRUCT {
	int		num_points;	/* num of points describing this roi. This is *not* the num of points inside the roi - this is the number of points of the circumference of the roi area, see num_points_inside */
	roiPoint 	**points;
	roi		*p;		/* my roi */
} roiRegionIrregular;

struct	_ROI_DATA_STRUCT {
	int		slice;
	char		*name;
	char		*image;
	roiRegionType	type;
	void		*roi_region;

	float		centroid_x, centroid_y, /* the centroid of the roi */
			x0, y0,		/* imagine a rectangle enclosing the roi, then (x0, y0) is its top-left corner and width/height its dimensions */
			width, height,
			meanPixel, stdevPixel,	/* stats of the pixels inside the roi - only if 'roiPoint->v' above is available - use associate_with_unc_volume function */
			minPixel, maxPixel
			;
	roiPoint	**points_inside;   	/* and all the points - see structure above */
	int		num_points_inside; 	/* the number of points inside the roi */
};

/* this is in Grammar.y */
int	read_rois_from_file(char */*filename*/, roi ***/*ret_myRois*/, int */*ret_numRois*/);

/* these are in IO_roi.c */
char	*rois_toString(roi **/*a_rois*/, int /*numRois*/);
char	*roi_toString(roi */*a_roi*/);
void	roi_destroy(roi */*a_roi*/);
void	rois_destroy(roi **/*a_rois*/, int /*numRois*/);
roi	*roi_new(void);
roi	**rois_new(int /*numRois*/);
void	roi_region_destroy(void */*a_region*/, roiRegionType /*a_type*/);
void	*roi_region_new(roiRegionType /*a_type*/, roi */*p*/, int /*numPoints*//*only applicable for the irregular region*/);
void	roi_points_destroy(roiPoint **/*a_roi_points*/, int /*numPoints*/);
void	roi_point_destroy(roiPoint */*a_roi_point*/);
roiPoint **roi_points_new(int /*numPoints*/, roi */*parent roi use NULL if no parent*/);
roiPoint *roi_point_new(roi */*parent roi use NULL if no parent*/);

roi	**rois_copy(roi **/*a_rois*/, int /*numRois*/);
roi	*roi_copy(roi */*a_roi*/);
void	*roi_region_copy(void */*a_region*/, roiRegionType /*rType*/);
roiPoint *roi_point_copy(roiPoint */*a_point*/);
roiPoint **roi_points_copy(roiPoint **/*a_points*/, int /*numPoints*/);

int	write_rois_to_file(char */*filename*/, roi **/*myRois*/, int /*numRois*/, float /*pixel_size_x*/, float /*pixel_size_y*/);


#define	_IO_ROI_VISIT
#endif
