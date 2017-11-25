#ifndef _CONTOUR_HEADER

#define	_CONTOUR_MAX_POINTS	5000
typedef enum {NEIGHBOURS_8=0, NEIGHBOURS_4=1} neighboursType;

typedef	struct _CONTOUR {
	int			num_points;
	int			*x, *y;
} contour;

contour			*contour_new(int /*num_points*/);
void			contour_destroy(contour *);

/* startX and Y are optional, set to -1 if you want to automatically pick a starting point */
contour	*find_contour2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, int /*startX*/, int /*startY*/, int /*startLevel*/, int /*Threshold*/);

#include <connected.h>

#define _CONTOUR_HEADER
#endif

