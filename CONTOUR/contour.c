#include <stdio.h>
#include <stdlib.h>

#include "Common_IMMA.h"

#include "contour.h"

static int npts = 0;
static int alevel;
static contour *my_contour;

static float rmulfac = 1.;
static int maxdim;
static int xdim, ydim;
static int width, height;
static int maxx, maxy;
static int ix0, iy0;
static DATATYPE **img;
static char *mapbuf;

static int contour_level = 100;
static int auto_contour_level = TRUE;   /* Contour following control */
int	edge_level;

/*
 *				3
 * Define directions:	     2  +  0,4
 *				1,5
 */
static int idirx[6] = {1, 0, -1, 0, 1, 0};
static int idiry[6] = {0, 1, 0, -1, 0, 1};

char     *map();
static DATATYPE *image_element();
static DATATYPE image_value();
void draw(int, int, int, int);
void order(void);
void reduce(int);
void store(float, float);
int edge_strength(int, int, int);
void do_contour(int is, int js, int level, int threshold, int *npt, int maxvec);
void	find_starting_point(int *startX, int *startY, int *startLevel);

contour	*find_contour2D(DATATYPE **data, int x0, int y0, int w, int h, int sX, int sY, int sL, int Threshold){

	int	startX, startY, startLevel;
	int	num_points; /* the final number of points in the contour */
	int	i, j;	
	contour	*a_contour;
	DATATYPE p;

	/* initialise */
	xdim = w; /* xdim and ydim are search space for contour */
	ydim = h;
	width = w;
	height = h;
	maxx = width - 1;
	maxy = height - 1;
	ix0 = x0;
	iy0 = y0;
	img = data;
	edge_level = 100;
	num_points =0;

	if( (my_contour=contour_new(_CONTOUR_MAX_POINTS)) == NULL ){
		fprintf(stderr, "find_contour2D : call to contour_new has failed for %d initial contour points (change the constant _CONTOUR_MAX_POINTS in contour.h).\n", _CONTOUR_MAX_POINTS);
		return NULL;
	}

	if( (sX<0) && (sY<0) ){
		startX = -1; startY = -1; startLevel = 32767;
		for(i=x0;i<width;i++) for(j=y0;j<height;j++){
			if( (p=image_value(i, j)) <= 0 ) continue;
			if( p < startLevel ){
				startLevel = p;
				startX = i; startY = j;
			}
		}
	} else { startX = sX; startY = sY; startLevel = sL; }
	
	if( Threshold < 0 ) Threshold = 0; /* don't know what this is */

	find_starting_point(&startX, &startY, &startLevel);
	do_contour(startX, startY, startLevel, Threshold, &num_points, 2000);

	if( num_points == 0 ){
		int	myI = 0, myJ = 0;

		/* usually this is just only 1 point, the algorithm makes no roi around this point */
		for(i=x0;i<width;i++) for(j=y0;j<height;j++)
			if( (p=image_value(i, j)) > 0 ){
				num_points++; myI = i; myJ = j;
			}
		if( num_points == 1 ){
			if( (a_contour=contour_new(4)) == NULL ){
				fprintf(stderr, "find_contour2D : call to contour_new has failed for %d points (final).\n", num_points);
				contour_destroy(my_contour);
				return NULL;
			}
			a_contour->x[0] = myI - 1; a_contour->y[0] = myJ; /* left */
			a_contour->x[1] = myI; a_contour->y[1] = myJ - 1; /* top  */
			a_contour->x[2] = myI + 1; a_contour->y[2] = myJ; /* right*/
			a_contour->x[3] = myI; a_contour->y[3] = myJ + 1; /* bottom */
		} else {
			fprintf(stderr, "find_contour2D : take me back to andreas to fix me (%d).\n", num_points);
			contour_destroy(my_contour);
			return NULL;
		}
	} else {
		if( (a_contour=contour_new(num_points)) == NULL ){
			fprintf(stderr, "find_contour2D : call to contour_new has failed for %d points (final).\n", num_points);
			contour_destroy(my_contour);
			return NULL;
		}
		for(i=0;i<num_points;i++){
			a_contour->x[i] = my_contour->x[i];
			a_contour->y[i] = my_contour->y[i];
		}
	}
	contour_destroy(my_contour);
	return a_contour;
}
	
contour	*contour_new(int num_points){
	contour	*a_c;

	if( (a_c=(contour *)malloc(sizeof(contour))) == NULL ){
		fprintf(stderr, "contour_new : could not allocate %zd bytes for a_c.\n", sizeof(contour));
		return NULL;
	}
	if( (a_c->x=(int *)malloc(num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "contour_new : could not allocate %zd bytes for a_c->x.\n", num_points*sizeof(int));
		free(a_c);
		return NULL;
	}
	if( (a_c->y=(int *)malloc(num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "contour_new : could not allocate %zd bytesc for a_c->x.\n", num_points*sizeof(int));
		free(a_c->x); free(a_c);
		return NULL;
	}
	a_c->num_points = num_points;

	return a_c;
}
void	contour_destroy(contour *a_c){
	free(a_c->x);
	free(a_c->y);
	free(a_c);
}
	
void	find_starting_point(int *startX, int *startY, int *startLevel)
{
    int       i, j, t, iy = *startY, ix = *startX;
    int       amax;
    int       d = 5;            /* Size of start point search area this value
                                 * ought to be odd */
                                 
	iy = MAX(d/2+2, iy);
	ix = MAX(d/2+2, ix);

    /* Now find the point with the largest gradient  */
    amax = 0;
    for (j = iy - d / 2; j <= iy + d / 2; j++) {
        for (i = ix - d / 2; i <= ix + d / 2; i++) {
            t = edge_strength(i, j, d);
            if (t > amax) {
                *startX = i;
                *startY = j;
                amax = t;
                *startLevel = edge_level;
            }
        }
    }
}    

void do_contour(int is, int js, int level, int threshold, int *npt, int maxvec)
    /* is, js, level;	 Start point and value */
    /* threshold;	 Threshold/nearest-to-level */
    /* *npt;	 	 Location to receive count */
    /*  maxvec	 Maximum vectors in outline, contour points will be reduced to this number if more */
{
    int       i, j, aa, ab;
    int       d, d1, dir, dx, dy, iv, iv1;

    /* Translate the start point into zoomed coordinates */
//    is -= x0;
//    js -= y0;

    /* Init output buffer pointers */
    maxdim = my_contour->num_points;
    *npt = 0;

    alevel = level;

    if ((mapbuf = (char *) malloc(width * height)) == NULL)
	return;

    /* Initialize the map array to zero */
    for (j = 0; j < height; j++)
	for (i = 0; i < width; i++)
	    *map(i, j) = 0;

    /* Find strongest direction from is, js */
    d = 0;
    dir = 0;
    iv = *image_element(is, js);
    for (i = 0; i < 4; i++) {
	iv1 = *image_element(is + idirx[i], js + idiry[i]);
	d1 = abs(iv - iv1);
	if ((d1 > d) && ((iv < level && iv1 > level) || (iv > level && iv1 < level))) {
	    d = d1;
	    dir = i;
	}
    }

    /* Find all points on appropriate gradients and mark them in map */
    dx = idirx[dir];
    dy = idiry[dir];
    for (j = 0; j < maxy; j++) {
	for (i = 0; i < maxx; i++) {
	    aa = *image_element(i, j) - alevel;
	    ab = *image_element(i + dx, j + dy) - alevel;
	    if ((aa >= 0 && ab < 0) || (aa < 0 && ab >= 0)) {
		*map(i, j) = 1;
	    }
	}
    }

    /* Follow the contour from the start point */
    draw(is, js, dir, threshold);

    /* Put region in standard order */
    order();

    /* Reduce number of points to whatever may be required */
    reduce(maxvec);

    *npt = npts;
    free(mapbuf);
    return;
}

/*
 * Follow a contour starting from point: iz, jz in direction dir.
 * Threshold controls the following mode: nearest or threshold
 */
void draw(iz, jz, dir, threshold)
    int       iz, jz, dir, threshold;
{
    int       i, j, idir, ipen, idx, idy, iddx, iddy;
    int       d1, d2, d3, d4, s1, s3, s4, fa;
    float     factor, px, py, xs = 0, ys = 0;

    i = iz;
    j = jz;
    idir = dir;
    ipen = 0;

    npts = 0;

/*
 * Consider the  points:
 *				4  1
 *				3  2
 * Rotated according to idir which defines the vector point 1 -> point 2.
 * Points 1 and 2 are known to straddle the contour.
 *
 * Differences from threshold:	 	d4  d1
 *					d3  d2
 * Sign of difference (below=true):	s4  s1
 *					s3		(s2 must be -s1)
*/


    while (1) {

	/* i and j are current pixel coordinate of point 1 */

	/* Get value of point 1 wrt contour level */
	d1 = *image_element(i, j) - alevel;
	s1 = d1 < 0;

	/* Get offsets to point 2 */
	idx = idirx[idir];
	idy = idiry[idir];

	/* Get value of point 2 */
	d2 = *image_element(i + idx, j + idy) - alevel;

/*
		printf("I,J=%3d,%3d   IDIR=%d    AA,AB=%6d,%6d\n",
				i+1,j+1, idir+1, d1,d2);

*/
	/*
	 * Calculate real coordinates of intersection with contour. In
	 * threshold mode we just pick the nearer pixel.
	 */
	if (d1 == d2)
	    factor = 0.;
	else if (threshold) {
	    if (d1 >= 0)
		factor = 0.;
	    else
		factor = 1.;
	} else
	    factor = (float) d1 / (float) (d1 - d2);

	px = (float) i + factor * (float) idx;
	py = (float) j + factor * (float) idy;

	/* Store previous point (if any) and remember px, py */
	if (ipen)
	    store(xs, ys);

	xs = px;
	ys = py;
	ipen++;

	/* If same |direction| as start, we may have finished */
	if ((idir & 1) == (dir & 1)) {
	    if (idir == dir) {	/* Exactly the same */
		iddx = i;
		iddy = j;
	    } else {		/* Opposite */
		iddx = i + idirx[idir];
		iddy = j + idiry[idir];
	    }

	    /* Done if target is not marked */
	    if (*map(iddx, iddy) == 0)
		return;

	    /* Else unmark and keep following */
	    *map(iddx, iddy) = 0;
	}
	/* Get position of point 4 */
	iddx = i + idirx[idir + 1];
	iddy = j + idiry[idir + 1];

	/* Get difference and sign for point 3 */
	d3 = *image_element(iddx + idx, iddy + idy) - alevel;
	s3 = d3 < 0;

	/* And for point 4 */
	d4 = *image_element(iddx, iddy) - alevel;
	s4 = d4 < 0;

	if (s3 && s4) {
	    if (s1) {
		/*
		 * -  - -  +
		 * 
		 * Continue anticlockwise from point 3
		 */

		i = iddx + idx;
		j = iddy + idy;
		goto anticlockwise;
	    } else {
		/*
		 * -  + -  -
		 * 
		 * Continue clockwise from current point
		 */

		goto clockwise;
	    }

	}
	if (!(s3 || s4)) {
	    if (s1) {
		/*
		 * +  - +  +
		 * 
		 * Continue clockwise from current point
		 */

		goto clockwise;
	    } else {
		/*
		 * +  + +  -
		 * 
		 * Continue anticlockwise from point 3
		 */

		i = idx + iddx;
		j = idy + iddy;

		goto anticlockwise;
	    }
	}
	if ((s1 || !s4) && (!s1 || s4)) {
	    /*
	     * +  +	or	-  - x  x		x  x
	     * 
	     * Continue in same direction from point 4
	     */
	    i = iddx;
	    j = iddy;
	    continue;
	}
	fa = d1 * d3 - d2 * d4;
	if (fa == 0) {
	    /* Continue in same direction from point 4 */
	    i = iddx;
	    j = iddy;
	    continue;
	} else if (fa > 0) {
	    /* Continue anticlockwise from point 3 */
	    i = idx + iddx;
	    j = idy + iddy;

	    goto anticlockwise;
	} else
	    goto clockwise;

	/* Change search direction and continue following */

clockwise:
	if (++idir >= 4)
	    idir = 0;
	continue;

anticlockwise:
	if (--idir < 0)
	    idir = 3;
	continue;
    }
}

/*
 * Store a point in the region array
 */
void store(float xs, float ys)
{
    int       ixs, iys;

    ixs = (int) (xs * rmulfac + 0.5) + ix0;
    iys = (int) (ys * rmulfac + 0.5) + iy0;

    /* Ignore duplicate points */
    if (npts) {
	if (my_contour->x[npts - 1] == ixs && my_contour->y[npts - 1] == iys)
	    return;
    }
    /* Reduce straight lines */
    if (npts > 1) {
/*
		if (my_contour->x[npts-1]-my_contour->x[npts-2] == ixs-my_contour->x[npts-1] &&
				my_contour->y[npts-1]-my_contour->y[npts-2] == iys-my_contour->y[npts-1])
			npts--;
*/
	if (ixs == my_contour->x[npts - 1] && ixs == my_contour->x[npts - 2])
	    npts--;

	if (iys == my_contour->y[npts - 1] && iys == my_contour->y[npts - 2])
	    npts--;
    }
    if (npts >= maxdim)
	fprintf(stderr, "contour.c, store : Too many points in contour %d > %d (change the constant _CONTOUR_MAX_POINTS in contour.h).\n", npts, maxdim);
    else {
/*
		if (npts)
			drawline(my_contour->x[npts-1], my_contour->y[npts-1], ixs, iys);
*/

	my_contour->x[npts] = ixs;
	my_contour->y[npts] = iys;
	npts++;
    }
}

/* Change region to clockwise starting in top left */
void order()
{
    int       i, j, t;

    /* First find top left point */
    j = 0;
    for (i = 0; i < npts; i++)
	if (my_contour->y[i] < my_contour->y[j] || (my_contour->y[i] == my_contour->y[j] && my_contour->x[i] <= my_contour->x[j]))
	    j = i;

    /* Now reorder from top left */
    if (j != 0) {
	int      *xpt = (int *) malloc(npts * sizeof(int));
	int      *ypt = (int *) malloc(npts * sizeof(int));

	if (xpt == NULL || ypt == NULL) {
	    fprintf(stderr, "malloc failed\n");
	    if (xpt)
		free(xpt);
	    return;
	}
	for (i = 0; j < npts; i++, j++) {
	    xpt[i] = my_contour->x[j];
	    ypt[i] = my_contour->y[j];
	}
	for (j = 0; i < npts; i++, j++) {
	    xpt[i] = my_contour->x[j];
	    ypt[i] = my_contour->y[j];
	}
	for(i=0;i<npts;i++){
		my_contour->x[i] = xpt[i];
		my_contour->y[i] = ypt[i];
	}
	free(xpt);
	free(ypt);
    }
    /* Return if region is clockwise */
    for (i = 1, j = npts - 1; i < j; i++, j--) {
	if (my_contour->y[i] < my_contour->y[j])
	    return;
	if (my_contour->y[i] > my_contour->y[j])
	    break;
    }

    /* Reverse region direction */
    for (i = 1, j = npts - 1; i < j; i++, j--) {
	t = my_contour->y[i];
	my_contour->y[i] = my_contour->y[j];
	my_contour->y[j] = t;
	t = my_contour->x[i];
	my_contour->x[i] = my_contour->x[j];
	my_contour->x[j] = t;
    }

}


/* A nasty cludge to reduce the number of points in a contour
 * This routins exists purely for compatibility with UCH GIP
 */
void reduce(maxpts)
    int       maxpts;
{
    int       i, j, nskip;

    if (npts < maxpts)
	return;

    nskip = (npts + maxpts - 1) / maxpts;
    for (i = 2, j = 0; i < npts; i += nskip, j++) {
	my_contour->x[j] = my_contour->x[i];
	my_contour->y[j] = my_contour->y[i];
    }

    npts = j;
}

/* Value for points on boundry of search area */
static DATATYPE boundry = -32768;

static DATATYPE *
image_element(i, j)
    register int i, j;
{

    if (i <= 0 || i >= maxx || j <= 0 || j >= maxy)
	return (&boundry);
    else
	return &(img[i][j]);
}

static DATATYPE
image_value(int i, int j)
{
	return img[i][j];
}

char     *
map(i, j)
    int       i, j;
{

    return ((mapbuf + width * j + i));
}

int edge_strength(ix, iy, d)
    int       ix, iy, d;
{   
    int       i;
    int       th1, th2;
    int       tv1, tv2;
    int       hs, vs;
            
    /* Number of points on each side of ix,iy to examine */
    d = d / 2;
        
    th1 = th2 = 0;   
    for (i = 1; i <= d; i++) {
        th1 += image_value(ix + i, iy);
        th2 += image_value(ix - i, iy);
    }
    hs = abs(th1 - th2);
        
    tv1 = tv2 = 0;
    for (i = 1; i <= d; i++) {
        tv1 += image_value(ix, iy + i);
        tv2 += image_value(ix, iy - i);
    }   
    vs = abs(tv1 - tv2);

    /* If contour level is fixed, edge must straddle it */
    if (!auto_contour_level) {
        if ((th1 > contour_level && th2 > contour_level) ||
            (th1 < contour_level && th2 < contour_level))
            hs = 0;
    
        if ((tv1 > contour_level && tv2 > contour_level) ||
            (tv1 < contour_level && tv2 < contour_level))  
            vs = 0;
    }    
    /* Return stronger edge and set level */
    if (hs > vs) {
        edge_level = (th1 + th2) / (2 * d);
        return (hs);
    } else {
        edge_level = (tv1 + tv2) / (2 * d);
        return (vs);
    }

}

