/*
 * roiedge.c
 * 
 * Form line segment table for an irregular region of interest
 * 
 * Last edit: 15 May 1997
 * 
 * +copyright 
 * -----------------------------------------------------------------------------
 * 
 * dispimage - Image display and analysis package
 * 
 * This module is part of the dispimage suite of routines.
 * 
 * Primary author:	Dave Plummer 
 *			Department of Medical Physics & Bio-engineering
 *  			University College London Hospitals 
 *			First Floor Shropshire House 
 *			11 - 20 Capper Street 
 *			London WC1E 6JA
 * 
 * 			Phone: +44 171 209 6271 
 *			Email: dlp@medphys.ucl.ac.uk
 * 
 * Contributing authors:
 * 
 * 
 * Acknowledgements: Once upon a time I saw a program by Brice Tebbs at UNC
 * called imagetool which was part of the /usr/image package. This inspired
 * this effort. Code fragments have come from sources too numerous to
 * mention but are included with grateful thanks to the contributors.
 * 
 * 
 * This software is the property of University College London (UCL). It is
 * available free of charge to non-profit organizations for research and
 * evaluation purposes. Transfer to a third party without the express
 * authorization of The Department of Medical Physics UCL is forbidden. No
 * warenty is given or implied as to the correctness of any part of this
 * software and neither the Authors nor the Department accept any liability.
 * 
 * 
 * 
 * ---------------------------------------------------------------------------- 
 * -copyright 
 *
 * 
 * This routine accepts a vector description of a region of interest where
 * each vertex is described by an integer coordinate pair. The region is 
 * closed by joining the last point back to the first. Coordinates must be 
 * positive and be bound by the specified y limit.
 * 
 * The output is a table containing, for each row of the image, an ordered 
 * list of start/end-point pairs defining a range of pixels to include in the
 * region. A pixel is included if more than half its area is enclosed by the
 * region outline.
 *
 * The table may then be used to generate ROI statistics or a bitmap.
 *
 * 
 * Original version		Rich Carson		UCLA 1981 
 * C addaptation		Dave Plummer		UCLH 1989 
 * Improved algorithm		Joanne Mathias		UCL  1996
 * 
 *
 * Usage: roiedge(ix, iy, npt, yp, np) 
 * Where: 
 *	ix, iy	- arrays of npt integer coordinates of region vertices
 *	yp	- array indexed by image row to receive pointers to integer
 *		  arrays of x intersections with edges of region.
 *		  The edges alternate: in, out, in, out....
 *	np	- array indexed by image row to receive integer count of 
 *		  intersections in that row. (This will always be even)
 *	ny	- number of rows in image, length of yp, np
 * 
 * Caller is responsible for deallocating crossing point arrays with free()
 * 
 */

// static char SCCSid[] = "@(#)roiedge.c	3.5\t05/15/97";

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for memcpy */
#include <math.h>


#define NOVERT	0
#define INCLUDE 1
#define EXCLUDE (-1)

/* The code was revised in March 1997
 * To facilitate comparability in ongoing studies, the following switch 
 * enables the new features which produce more accurate region areas.
 */
#define ROI Filling algorithms (These values are used in disproi.c )
#define FILL_1989	0
#define FILL_1997	1
/* AHP what is this for ??? */
/*int roi_fill_mode = FILL_1989;*/

/* Local functions */
static void addxpt( /* int, float, int, float *[], char *flag[], int[] */ );
static void floatmove( /* float*, float*, int */);
//static void intmove( /* int*, int*, int */);
static void charmove( /* char*, char*, int */);

static int region_clockwise(int ix[], int iy[], int npt);

/*
void roiedge(int ix[], int iy[], int npt, int *uyp[], int np[], int ny);
*/
void roiedge(ix, iy, npt, uyp, np, ny, fill_mode)
int       ix[], iy[], npt;
int      *uyp[], np[], ny;
int	  fill_mode;
{
    int       p0, p1, p2, p3, i, empty;
    int       y1, y2;
    float     x1, x2;
    float    *xp;
    float     newrnd;
    int       new_fill_mode;

    float  **yp;
    char   **flagp, *vp, *inc_flag;
    int      rdir;


/*    roi_fill_mode = fill_mode;*/

    /* Set the local ROI filling mode flags and parameters */
    if (fill_mode == FILL_1989) {
        new_fill_mode = 0;
	newrnd = 0.0;
	empty = 0;
    } else if (fill_mode == FILL_1997) {
	new_fill_mode = 1;
	newrnd = 0.5;
	empty = 1;
    } else {
	fprintf(stderr, "Unknown region filling mode\n");
        exit(1);
    }

    /* Determine the direction of the region outline */
    rdir = region_clockwise(ix, iy, npt);

    /* Zero intersection counts for all rows before allocating memory */
    for (i = 0; i < ny; i++) {
	np[i] = 0;
        uyp[i] = NULL;
    }

    /* Allocate working arrays */ 
    if ((yp = (float**) malloc(ny * sizeof(float*)))==NULL) {
	fprintf(stderr, "Malloc failed\n");
	return;
    }
    if ((flagp = (char**) malloc(ny * sizeof(char*)))==NULL) {
	free(yp);
	fprintf(stderr, "Malloc failed\n");
	return;
    }
    if ((inc_flag = (char*) malloc(npt * sizeof(char)))==NULL) {
	free(yp);
	free(flagp);
	fprintf(stderr, "Malloc failed\n");
	return;
    }

    /* Zero array pointers for all rows */
    for (i = 0; i < ny; i++) {
	yp[i] = NULL;
	flagp[i] = NULL;
    }

    /* Init vertex point flag array to include all vertices */
    for (p1=0; p1 < npt; p1++)
        inc_flag[p1] = INCLUDE;


    /* Examine vertex pixels, to decide which should be included.
     * If the internal angle is < 180, the pixel should excluded.
     * The array inc_flag determines the inclusion status of pixels at the
     * region vertex points. 
     *        inc_flag[p] = INCLUDE  -  Include point in region
     *        inc_flag[p] = EXCLUDE  -  Exclude point from region
     *
     */
    for (p0=npt-1,p1=0,p2=1; new_fill_mode && p1<npt; p0++,p1++,p2++) {
	double theta, dval;
	double dx0, dy0, dx2, dy2;

        if (p0 == npt)
	    p0 = 0;
	if (p2 == npt)
	    p2 = 0;

	/* Ignore duplicate points */
	if ( (ix[p1]==ix[p0] && iy[p1]==iy[p0]) || 
	     (ix[p1]==ix[p2] && iy[p1]==iy[p2])) {
	    continue;
	}

	/* Clear the empty flag if region has some extent */
	if (ix[p1] != ix[0] && iy[p1] != iy[p0])
	    empty = 0;

	/* Calculate internal angle at p1 as fraction of 360 degrees */
	dx0 = ix[p1] - ix[p0];
	dy0 = iy[p1] - iy[p0];
	dx2 = ix[p2] - ix[p1];
	dy2 = iy[p2] - iy[p1];

	/* This will calculate the internal angle for clockwise regions
 	 * and the external angle for anticlockwise regions */
	theta = modf(1.5 - (atan2(dy2, dx2) - atan2(dy0, dx0)) / (2*M_PI), &dval);

	/* If internal angle is < 180 exclude the vertex */
	if ((theta < 0.5 && rdir) || (theta >0.5 && !rdir) )
	    inc_flag[p1] = EXCLUDE;

     }


    /*
     * Mark the intersections of each edge with every row it crosses,
     * upto, but not including, the last row. 
     * Mark the intersection with the last row only if a change in direction 
     * is about to take place. 
     * Ignore all edges that are along rows (horizontal).
     */
    for (p1=0, p2=1; p1 < npt; p1++, p2++) {
	float x, g;
	int k;

	/* Wrap last point back to first */
	if (p2 == npt)
	    p2 = 0;

	/* Ignore duplicate points */
	if (new_fill_mode && ix[p1]==ix[p2] && iy[p1]==iy[p2]) {
	    continue;
	}

	/* Get current vector */
	x1 = (float) ix[p1];
	y1 = iy[p1];
	x2 = (float) ix[p2];
	y2 = iy[p2];

	/* Ignore horizontal segments */
	if (y1 == y2)
	    continue;

	/* Mark intersections, excluding the last row */
	g = (float) (x2 - x1) / (float) abs(y2 - y1);
	x = x1 + .5;

	if (y1 < y2) {
	    for (k = y1; k < y2; k++, x += g) {
		if (k == y1)
		    addxpt(k, x, inc_flag[p1], yp, flagp, np);
		else
		    addxpt(k, x, NOVERT, yp, flagp, np);
	    }
	} else {
	    for (k = y1; k > y2; k--, x += g) {
		if (k == y1)
		    addxpt(k, x, inc_flag[p1], yp, flagp, np);
		else
		    addxpt(k, x, NOVERT, yp, flagp, np);
	    }
	}

	/* Follow horizontal line to check for direction change */
	p3 = p2;
	do {
	    if (++p3 == npt)
		p3 = 0;
	} while (iy[p3] == y2 && p3 != p2);

	/* Add last point if p2 is a crest or a valley */
	if (((y1 < y2) && (y2 > iy[p3])) || ((y1 > y2) && (y2 < iy[p3])))
	    addxpt(y2, x2 + newrnd, inc_flag[p2], yp, flagp, np);

    }


    /* Now check that we have got all the horizontal segments */
    for (p1=0, p2=1; p1 < npt; p1++, p2++) {
	int k, n, iflag1, iflag2;

	if (p2 == npt)
	    p2 = 0;

	/* Ignore duplicate points */
	if (new_fill_mode && ix[p1]==ix[p2] && iy[p1]==iy[p2]) {
	    continue;
	}
  
	y1 = iy[p1];
	y2 = iy[p2];

	/* Continue if vector not horizontal */
	if (y1 != y2)
	    continue;

	/* Get forward vector and its end point flags */
	if (ix[p1] < ix[p2]) {
	    x1 = (float) ix[p1] + newrnd;
	    x2 = (float) ix[p2] + newrnd;
	    iflag1 = inc_flag[p1];
	    iflag2 = inc_flag[p2];
	} else {
	    x1 = (float) ix[p2] + newrnd;
	    x2 = (float) ix[p1] + newrnd;
	    iflag1 = inc_flag[p2];
	    iflag2 = inc_flag[p1];
	}

	/* Scan intersections for this row */
	n = np[y1];
	xp = yp[y1];
	vp = flagp[y1];
	for (k = 0; k < n; k+=2, xp+=2, vp+=2) {
	    float j1, j2;

	    /* Get start and end of this segment */
	    j1 = *(xp);
	    j2 = *(xp+1);

	    /* Continue if before our vector */
	    if (j2 <= x1 && !(new_fill_mode && j2 == x1))
		continue;

	    /* Add completely missing segment if required */
	    if (x2 < j1) {
		float xv1 = x1;
		float xv2 = x2;

		if (new_fill_mode) {
		    xv1 = ix[p1]+newrnd;
		    xv2 = ix[p2]+newrnd;
	        }
	    
		addxpt(y1, xv1, inc_flag[p1], yp, flagp, np);
		addxpt(y1, xv2, inc_flag[p2], yp, flagp, np);

		break;
	    }

	    /* Extend segment to left if required */
	    if (x1 < j1) {
		*xp = x1;
		*vp = iflag1;
	    }

	    /* Extend segment to right if required */
	    if (new_fill_mode && x2 > j2) {
		*(xp+1) = x2;
		*(vp+1) = iflag2;
	    }

	    /* Break if segment now covered */
	    if (x2 <= j2)
		break;

	    /* Adjust start of segment */
	    x1 = j2 + 1;
	}
    }


    /* Remove null, duplicate and overlapping segments from table */
    for (i = 0; i < ny; i++) {
	int l;

	xp = yp[i];
	vp = flagp[i];
	l = np[i];

	while (l>0) {

	    /* Remove adjacent points which are not explicitly included */
	    if (new_fill_mode &&
		(*xp == *(xp+1) && *vp != INCLUDE && *(vp+1) != INCLUDE) ) {
		np[i] -= 2;
		l -= 2;
		floatmove(xp, xp + 2, l);
		charmove(vp, vp + 2, l);
		continue;
	    }


	    /*
	     * Merge adjacent and overlapping segments. 
	     * If an adjacent segment terminates at a corner pixel only 
	     * merge if the flag has been set to INCLUDE.
	     */
	    if (xp > yp[i] && *(xp-1) >= *xp && *(vp-1) != EXCLUDE) {
		np[i] -= 2;
		floatmove(xp - 1, xp + 1, l);
		charmove(vp - 1, vp + 1, l);
		l -= 2;
		continue;
	    }

	    xp += 2;
	    vp += 2;
	    l -= 2;
	}
    }



    /*
     * Finally round the segment table to integral coordinates. 
     * Remaining corner pixels not to be included are moved inwards by 1 pixel. 
     * Offset of 0.49 ensures non-corner pixels are rounded correctly 
     * (ie included if the pixel area within the ROI is greater than 0.5).
     */
    for (i = 0; i < ny; i++) {
	int      *ixp;
	int	  j, n;

	if (empty)
	    np[i] = 0;

	n = np[i];
	xp = yp[i];
	vp = flagp[i];

	/* Allocate the segment array to return to user */
 	uyp[i] = ixp = (int*)malloc(n * sizeof(int));

	for (j = 0; j < n && !empty; j += 2) {

	    /* Exclude left end point of segment or round towards right */
	    if (*(vp++) == EXCLUDE)
		*(ixp++) = (int) (*(xp++) + 1);
	    else
		*(ixp++) = (int) (*(xp++) + 0.49);


	    /* Exclude right end point of segment or round towards left */
	    if (*(vp++) == EXCLUDE)
		*(ixp++) = (int) (*(xp++) - 1);
	    else
		*(ixp++) = (int) (*(xp++) - 0.49);
		
	}

	/* Free the temporary arrays for this row */
	free((char*)yp[i]);
	free((char*)flagp[i]);
    }

    /* Free the pointer arrays */
    free((char*)yp);
    free((char*)flagp);
    free((char*)inc_flag);

}


/* Add an intersection with row iy at position x with flag p
 * yp, flagp, np are the pointer and count arrays to update.
 * The intersection table is initialy allocated 4 slots. It subsequently
 * doubles in length each time it fills.
 */
static void
addxpt(iy, x, p, yp, flagp, np)
int iy;
float x;
int p;
float *yp[];
char *flagp[];
int np[];
{
    int       n, nalloc;
    float    *xp;
    char     *vp;

    /* Get table and number of points already in it */
    xp = yp[iy];
    vp = flagp[iy];
    n = np[iy];
    nalloc = 0;

    /* If nothing allocated, allocate something */
    if (n == 0)
	nalloc = 4;

    /* If power of two points allocated and n > 4 then extend */
    if (((n & (n - 1)) == 0) && n >= 4)
	nalloc = n * 2;

    if (nalloc) {
	if ((xp = (float *) malloc(nalloc * sizeof(float))) == NULL) {
	    fprintf(stderr, "Memory allocation failure.\n");
	    return;
	}
	if ((vp = (char *) malloc(nalloc * sizeof(char))) == NULL) {
	    fprintf(stderr, "Memory allocation failure.\n");
	    return;
	}

        if (n) {
	    memcpy(xp, yp[iy], n * sizeof(float));
	    memcpy(vp, flagp[iy], n * sizeof(char));
	    free((char *) yp[iy]);
	    free((char *) flagp[iy]);
 	}
	yp[iy] = xp;
	flagp[iy] = vp;
    }

    /* Look for place to insert edge and maybe push up list */
    for (; n>0; xp++, vp++, n--) {
	if (x < *xp) {
	    floatmove(xp+1, xp, n);
	    charmove(vp+1, vp, n);
	    break;
	}
    }

    *xp = x;
    *vp = p;
    np[iy]++;

}


/* These routines copy memory efficiently and support overlapping blocks */

static void
floatmove(dest, src, n)
float *dest, *src;
int n;
{

    /* If destination overlays top of source, copy backwards */
    if (src < dest && src+n > dest) {
	src += n;
        dest += n;
	while (n-- > 0)
	    *(--dest) = *(--src);
    } else {
	while (n-- > 0)
	    *(dest++) = *(src++);
    }
}

#ifdef GDGDGDGDGDGGD176162626
static void
intmove(dest, src, n)
int *dest, *src;
int n;
{

  /* If destination overlays top of source, copy backwards*/
    if (src < dest && src+n > dest) {
	src += n;
        dest += n;
	while (n-- > 0)
	    *(--dest) = *(--src);
    } else {
	while (n-- > 0)
	    *(dest++) = *(src++);
    }

}
#endif

static void
charmove(dest, src, n)
char *dest, *src;
int n;
{

    /* If destination overlays top of source, copy backwards */
    if (src < dest && src+n > dest) {
	src += n;
        dest += n;
	while (n-- > 0)
	    *(--dest) = *(--src);
    } else {
	while (n-- > 0)
	    *(dest++) = *(src++);
    }

}

	
/* region_clockwise
 *
 * Return direction of a region. 
 *	Where 0,0 is taken as top left
 *	Returns TRUE for clockwise, else FALSE
 */
static int
region_clockwise(ix, iy, npt)
int ix[], iy[], npt;
{
    int i;
    int ixmin, ixmax, iymin, iymax;

    ixmin = ixmax = 0;
    iymin = iymax = 0;

    /* Find points on the region bounding box */
    for (i = 0; i < npt; i++) {
	if (ix[i] < ix[ixmin])
	    ixmin = i;
	if (ix[i] > ix[ixmax])
	    ixmax = i;
	if (iy[i] < iy[iymin])
	    iymin = i;
	if (iy[i] > iy[iymax])
	    iymax = i;

    }

    /* Are limits reached in order left->top->right->bottom->left */
    if ( (((iymin<=ixmax) && (ixmax<=iymax)) && ((ixmin<=iymin) || (iymax<=ixmin))) ||
         (((iymax<=ixmin) && (ixmin<=iymin)) && ((ixmax<=iymax) || (iymin<=ixmax))) )
	return(1);
    
    return(0);

}

/* End of roiedge.c */
