#include <stdio.h>
#include <stdlib.h>

#include "Common_IMMA.h"

#include "contour.h"
#include "connected.h"

#define	NUM_OBJECTS_REALLOCATION_STEP	50

/* for a given pixel (i,j), the following offsets give the neighbouring
   pixels. e.g. (i-1, j+0) then (i+0, j-1) etc. */
const   int     _NEIGHBOURS_8_ARRAY[8][2] = {
	{-1,  0}, /* left */
	{0 , -1}, /* top */
	{1 ,  0}, /* right */
	{0 , +1}, /* bottom */
	{-1, -1}, /* top left */
	{+1, -1}, /* top right */
	{+1, +1}, /* bottom right */
	{-1, +1}  /* bottom left */
};
const   int     _NEIGHBOURS_4_ARRAY[4][2] = {
	{-1,  0}, /* left */
	{0 , -1}, /* top */
	{1 ,  0}, /* right */
	{0 , +1}  /* bottom */
};

/* private data structure */
typedef struct  _PIXEL_STRUCT_DUMMY {
	int		x, y, z;
	DATATYPE	v;
	int		id;
} _pixel;

/* private functions */
int	_find_connected_pixels2D_recursive_4(int x, int y, int w, int h, DATATYPE minP, DATATYPE maxP);
int	_find_connected_pixels2D_recursive_8(int x, int y, int w, int h, DATATYPE minP, DATATYPE maxP);
void	_find_connected_pixels3D_recursive(int x, int y, int z, int w, int h, int numSlices, DATATYPE minP, DATATYPE maxP);

/* private data */
static	_pixel	***pixels2D;
static	_pixel	****pixels3D;
static	int	entered, maxEntered; /* counting the recursion depth */

/* find_connected_pixels2D
	will take the 2D data of (w,h) width and height starting from (x,y)
	and find pixels that are connected to other neighbouring pixels
	of the same colour, plus or minus a threshold

	also, the criterion of what constitutes a neighbour (e.g. so far
	neighbours like a cross, or _NEIGHBOURS_8_ARRAY, full square around given pixel)
	is nType, an enum defined in 'contour.h'

	if you get a segmentation fault, it is likely that your stack size needs to be increased.
	at the unix csh shell do:
	limit stack 1000
	for a 1000 kbytes stack, try more and more if you get segmentation fault ...
*/

/* it will find all connected objects and then select the one which contains the seed point
   supplied - the connected object will be returned as well as the array 'result' will be set
   to 1 in the points belonging to the connected object */
connected_object	*find_connected_object2D(DATATYPE **data, int seedX, int seedY, int x, int y, int z, int w, int h, DATATYPE **result, neighboursType nType, DATATYPE minP, DATATYPE maxP, int *max_recursion_level){
	int			num_con_objs, i, j, ii, jj;
	connected_objects	*con_objs;
	connected_object	*ret_obj;

	/* first find all connected objects */
	if( find_connected_pixels2D(data, x, y, z, w, h, result, nType, minP, maxP,
					&num_con_objs, max_recursion_level, &con_objs) == FALSE ){
		fprintf(stderr, "find_connected_object2D : call to find_connected_pixels2D has failed.\n");
		return FALSE;
	}

	/* now go through them and find the one with the seed point in */
	for(i=0;i<num_con_objs;i++){
		for(j=0;j<con_objs->objects[i]->num_points;j++){
			if( (con_objs->objects[i]->x[j]==seedX) && (con_objs->objects[i]->y[j]==seedY) ){
				/* found it! */
				if( (ret_obj=connected_object_copy(con_objs->objects[i])) == NULL ){
					fprintf(stderr, "find_connected_object2D : call to connected_object_copy has failed.\n");
					connected_objects_destroy(con_objs);
					return NULL;
				}
				connected_objects_destroy(con_objs); /* we do not need all objects, only the one above */
				/* remove all other objects from the 'results' image array */
				for(ii=x;ii<x+w;ii++) for(jj=y;jj<y+h;jj++)
					if( result[ii][jj] == ret_obj->id ) result[ii][jj] = 1;
					else result[ii][jj] = 0;

				return ret_obj;
			}
		}
	}

	/* did not find the point requested */
	connected_objects_destroy(con_objs);
	return NULL;
}			
	
/* minP and maxP are threshold ranges for what pixels to count,
   result, is a [][] DATATYPE array for storing the result,
   each connected object will be colored with a different color.
   optional,
   	*num_connected_objects : to store the number of connected objects found
   	*max_recursion_level   : to store the maximum number of recursion calls.
   	**con_obj	       : to store a list of all connected objects with all the points of each (see connected.h for the structure info)
   leave NULL in order to ignore,
   if you want to use con_obj, then do not forget to free it at the end of its use by:
   	connected_objects_destroy(con_obj);
    */

int	find_connected_pixels2D(DATATYPE **data, int x, int y, int z, int w, int h, DATATYPE **result, neighboursType nType, DATATYPE minP, DATATYPE maxP, int *num_connected_objects, int *max_recursion_level, connected_objects **con_obj){
	/* con_obj is optional, if it is not null, then a list of connected objects, (see contour.h), will be allocated and returned */

	int		numUniqueIDs,
			*levels, dummy;
	int		*numPixelsPerObject;
	float		scale;
	int		i, j, ii, jj, k, l, xmax, ymax;

	if( (pixels2D=(_pixel ***)malloc(w * sizeof(_pixel **))) == NULL ){
		fprintf(stderr, "find_connected_pixels2D : could not allocate %zd bytes for pixels2D.\n", w * sizeof(_pixel **));
		return FALSE;
	}
			
	for(i=x,ii=0;i<x+w;i++,ii++){
		if( (pixels2D[ii]=(_pixel **)malloc(h * sizeof(_pixel *))) == NULL ){
			fprintf(stderr, "find_connected_pixels2D : could not allocate %zd bytes for pixels2D[%d].\n", h * sizeof(_pixel *), ii);
			for(j=0;j<i;j++) free(pixels2D[j]);
			free(pixels2D);
			return FALSE;
		}
		for(j=y,jj=0;j<y+h;j++,jj++){
			if( (pixels2D[ii][j]=(_pixel *)malloc(sizeof(_pixel))) == NULL ){
				fprintf(stderr, "find_connected_pixels2D : could not allocate %zd bytes for pixels2D[%d][%d].\n", sizeof(_pixel), ii, jj);
				for(j=0;j<i;j++) free(pixels2D[j]);
				free(pixels2D);
				return FALSE;
			}
			/* initial values */
			pixels2D[ii][jj]->x = i;
			pixels2D[ii][jj]->y = j;
			pixels2D[ii][jj]->z = z;
			pixels2D[ii][jj]->v = data[i][j];
			pixels2D[ii][jj]->id = -1;
		}
	}

	if( (numPixelsPerObject=(int *)malloc(NUM_OBJECTS_REALLOCATION_STEP*sizeof(int))) == NULL ){
		fprintf(stderr, "find_connected_pixels2D : could not allocate %zd bytes for numPixelsPerObject (first time).\n", NUM_OBJECTS_REALLOCATION_STEP*sizeof(int));
		for(i=0;i<w;i++) free(pixels2D[i]); free(pixels2D);
		return FALSE;
	}

	numUniqueIDs = 1; entered = 0; maxEntered = -1;
	switch( nType ){
		case NEIGHBOURS_8:
			for(i=1;i<w-1;i++) for(j=1;j<h-1;j++)
				if( IS_WITHIN(pixels2D[i][j]->v, minP, maxP) && (pixels2D[i][j]->id==-1) ){
					if( ((numUniqueIDs-1)%NUM_OBJECTS_REALLOCATION_STEP) == 0 )
						if( (numPixelsPerObject=(int *)realloc(numPixelsPerObject, (numUniqueIDs+NUM_OBJECTS_REALLOCATION_STEP)*sizeof(int))) == NULL ){
							fprintf(stderr, "find_connected_pixels2D : could not reallocate %zd bytes for numPixelsPerObject (%d time).\n", (numUniqueIDs+NUM_OBJECTS_REALLOCATION_STEP)*sizeof(int), numUniqueIDs/NUM_OBJECTS_REALLOCATION_STEP);
							for(ii=0;ii<w;ii++) free(pixels2D[ii]); free(pixels2D);
							return FALSE;
						}
					pixels2D[i][j]->id = numUniqueIDs;
					numPixelsPerObject[numUniqueIDs-1] = _find_connected_pixels2D_recursive_8(i, j, w, h, minP, maxP);
					numUniqueIDs++;
				}
			break;
		case NEIGHBOURS_4:
			for(i=1;i<w-1;i++) for(j=1;j<h-1;j++)
				if( IS_WITHIN(pixels2D[i][j]->v, minP, maxP) && (pixels2D[i][j]->id==-1) ){
					if( ((numUniqueIDs-1)%NUM_OBJECTS_REALLOCATION_STEP) == 0 ){
						if( (numPixelsPerObject=(int *)realloc(numPixelsPerObject, (numUniqueIDs+NUM_OBJECTS_REALLOCATION_STEP)*sizeof(int))) == NULL ){
							fprintf(stderr, "find_connected_pixels2D : could not reallocate %zd bytes for numPixelsPerObject (%d time).\n", (numUniqueIDs+NUM_OBJECTS_REALLOCATION_STEP)*sizeof(int), numUniqueIDs/NUM_OBJECTS_REALLOCATION_STEP);
							for(ii=0;ii<w;ii++) free(pixels2D[ii]); free(pixels2D);
							return FALSE;
						}
					}
					pixels2D[i][j]->id = numUniqueIDs;
					numPixelsPerObject[numUniqueIDs-1] = _find_connected_pixels2D_recursive_4(i, j, w, h, minP, maxP);
					numUniqueIDs++;
				}
			break;
		default:
			fprintf(stderr, "find_connected_pixels2D : neighbours type '%d' is not implemented.\n", nType);
			for(i=0;i<w;i++){
				for(j=0;j<h;j++) free(pixels2D[i][j]);
				free(pixels2D[i]);
			}
			free(pixels2D);
			return FALSE;
	}
	numUniqueIDs--;
	scale = 32767.0 / (numUniqueIDs+1);

	if( (levels=(int *)malloc(numUniqueIDs*sizeof(int))) == NULL ){
		fprintf(stderr, "find_connected_pixels2D : could not allocate %zd bytes for levels.\n", numUniqueIDs*sizeof(int));
		for(i=0;i<w;i++) free(pixels2D[i]); free(pixels2D);
		return FALSE;
	}
		
	/* create the output pixel levels (labels) for each connected object */
	for(i=0;i<numUniqueIDs;i++) levels[i] = (int )( ((float )(i+1)) * scale );

	/* shuffle the levels, 100 times it is enough */
	/* we do this so that each connected object is randomly assigned a color */
	for(i=0;i<100*numUniqueIDs;i++){
		j = lrand48() % numUniqueIDs;
		k = lrand48() % numUniqueIDs;
		dummy = levels[j];
		levels[j] = levels[k];
		levels[k] = dummy;
	}

	if( con_obj != NULL ){
		/* for each connected object, make a list of all its points inside */
		if( (*con_obj=connected_objects_new(numUniqueIDs, numPixelsPerObject)) == NULL ){
			fprintf(stderr, "find_connected_pixels2D : call to connected_objects_new has failed for %d objects.\n", numUniqueIDs);
			for(i=0;i<w;i++) free(pixels2D[i]); free(pixels2D);
			free(numPixelsPerObject);
			return FALSE;
		}
		for(i=0;i<numUniqueIDs;i++) (*con_obj)->objects[i]->num_points = 0;
		for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
			if( IS_WITHIN(pixels2D[ii][jj]->v, minP, maxP) && (pixels2D[ii][jj]->id >= 0) ){
				k = pixels2D[ii][jj]->id-1;
				l = (*con_obj)->objects[k]->num_points;

				result[i][j] = levels[k];
				(*con_obj)->objects[k]->x[l] = i;
				(*con_obj)->objects[k]->y[l] = j;
				(*con_obj)->objects[k]->z[l] = z; /* slice number */
				(*con_obj)->objects[k]->v[l] = pixels2D[ii][jj]->v;

				(*con_obj)->objects[k]->num_points++;
				(*con_obj)->objects[k]->id = levels[k];
			}
			else result[i][j] = 0;

		/* now find bounding box for each connected object */
		for(i=0;i<numUniqueIDs;i++){
			xmax = (*con_obj)->objects[i]->x0 = (*con_obj)->objects[i]->x[0];
			ymax = (*con_obj)->objects[i]->y0 = (*con_obj)->objects[i]->y[0];
			for(j=1;j<(*con_obj)->objects[i]->num_points;j++){
				if( (*con_obj)->objects[i]->x[j] < (*con_obj)->objects[i]->x0 ) (*con_obj)->objects[i]->x0 = (*con_obj)->objects[i]->x[j];
				if( (*con_obj)->objects[i]->y[j] < (*con_obj)->objects[i]->y0 ) (*con_obj)->objects[i]->y0 = (*con_obj)->objects[i]->y[j];
				if( (*con_obj)->objects[i]->x[j] > xmax ) xmax = (*con_obj)->objects[i]->x[j];
				if( (*con_obj)->objects[i]->y[j] > ymax ) ymax = (*con_obj)->objects[i]->y[j];
			}
			(*con_obj)->objects[i]->w = xmax - (*con_obj)->objects[i]->x0 + 1;
			(*con_obj)->objects[i]->h = ymax - (*con_obj)->objects[i]->y0 + 1;
		}
	} else {
		for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
			if( (pixels2D[ii][jj]->v > 0) && (pixels2D[ii][jj]->id >= 0) )
				result[i][j] = levels[pixels2D[ii][jj]->id-1];
			else result[i][j] = 0;
	}
	free(levels);

	for(i=0;i<w;i++){
		for(j=0;j<h;j++) free(pixels2D[i][j]);
		free(pixels2D[i]);
	}
	free(pixels2D);
	free(numPixelsPerObject);

	if( max_recursion_level != NULL ) *max_recursion_level = maxEntered;
	if( num_connected_objects != NULL ) *num_connected_objects = numUniqueIDs;

	return TRUE;
}


/* connectivity for a pixel (i, j) can be defined as either:
	a) if it has the same pixel colour as all FOUR neighbours,
	   e.g. top, bottom, left and right, (_NEIGHBOURS_4_ARRAY) like a cross,
	   or,
	b) (a) above plus 4 more neighbours diagonally top-left, top-right,
	   bottom-left and bottom-right, (NEIGBOURS_8)

	two more functions below are doing just that,

	they return the number of pixels making up each connected object
*/

int	_find_connected_pixels2D_recursive_8(int x, int y, int w, int h, DATATYPE minP, DATATYPE maxP){
	int		ii, i, j, n = 0;

	if( !IS_WITHIN(pixels2D[x][y]->v, minP, maxP) ) return n;

	n = 1;
	for(ii=0;ii<8;ii++){
		i = _NEIGHBOURS_8_ARRAY[ii][0] + x; j = _NEIGHBOURS_8_ARRAY[ii][1] + y;
		if( (i<0) || (i>=w) || (j<0) || (j>=h) || (pixels2D[i][j]->id > -1) ) continue;
		if( IS_WITHIN(pixels2D[i][j]->v, minP, maxP) ){
			if( entered++ > maxEntered ) maxEntered = entered;
			pixels2D[i][j]->id = pixels2D[x][y]->id;
			n += _find_connected_pixels2D_recursive_8(i, j, w, h, minP, maxP);
		}
	}
	entered--;
	return n;
}
int	_find_connected_pixels2D_recursive_4(int x, int y, int w, int h, DATATYPE minP, DATATYPE maxP){
	int		ii, i, j, n = 0;

	if( !IS_WITHIN(pixels2D[x][y]->v, minP, maxP) ) return n;

	n = 1;
	for(ii=0;ii<4;ii++){
		i = _NEIGHBOURS_4_ARRAY[ii][0] + x; j = _NEIGHBOURS_4_ARRAY[ii][1] + y;
		if( (i<0) || (i>=w) || (j<0) || (j>=h) || (pixels2D[i][j]->id > -1) ) continue;
		if( IS_WITHIN(pixels2D[i][j]->v, minP, maxP) ){
			if( entered++ > maxEntered ) maxEntered = entered;
			pixels2D[i][j]->id = pixels2D[x][y]->id;
			n += _find_connected_pixels2D_recursive_4(i, j, w, h, minP, maxP);
		}
	}
	entered--;
	return n;
}

void	_find_connected_pixels3D_recursive(int x, int y, int z, int w, int h, int numSlices, DATATYPE minP, DATATYPE maxP){
	int		i, j, k;

	if( !IS_WITHIN(pixels3D[z][x][y]->v, minP, maxP) ) return;

	for(i=x-1;i<=x+1;i++)
		for(j=y-1;j<=y+1;j++){
			for(k=z-1;k<=z+1;k++){
				if( (i==x) && (j==y) && (k==z) ) continue;
				if( (i<0) || (i>=w) || (j<0) || (j>=h) || (k<0) || (k>=numSlices) || (pixels3D[k][i][j]->id > -1) ) continue;
				if( IS_WITHIN(pixels3D[k][i][j]->v, minP, maxP) ){
					if( entered++ > maxEntered ) maxEntered = entered;
					pixels3D[k][i][j]->id = pixels3D[z][x][y]->id;
					_find_connected_pixels3D_recursive(i, j, k, w, h, numSlices, minP, maxP);
				}
			}
		}
	entered--;
}

connected_object *connected_object_new(int num_points){
	connected_object *a_c;

	if( (a_c=(connected_object *)malloc(sizeof(connected_object))) == NULL ){
		fprintf(stderr, "connected_object_new : could not allocate %zd bytes for a_c.\n", sizeof(connected_object));
		return NULL;
	}
	if( (a_c->x=(int *)malloc(num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "connected_object_new : could not allocate %zd bytes for a_c->x.\n", num_points*sizeof(int));
		free(a_c);
		return NULL;
	}
	if( (a_c->y=(int *)malloc(num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "connected_object_new : could not allocate %zd bytes for a_c->y.\n", num_points*sizeof(int));
		free(a_c->x); free(a_c);
		return NULL;
	}
	if( (a_c->z=(int *)malloc(num_points*sizeof(int))) == NULL ){
		fprintf(stderr, "connected_object_new : could not allocate %zd bytes for a_c->z.\n", num_points*sizeof(int));
		free(a_c->y); free(a_c->x); free(a_c);
		return NULL;
	}
	if( (a_c->v=(DATATYPE *)malloc(num_points*sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "connected_object_new : could not allocate %zd bytes for a_c->v.\n", num_points*sizeof(DATATYPE));
		free(a_c->z); free(a_c->y); free(a_c->x); free(a_c);
		return NULL;
	}
	a_c->num_points = num_points;
	a_c->id = -1;
	a_c->x0 = a_c->y0 = a_c->w = a_c->h = -1;

	return a_c;
}
connected_object *connected_object_copy(connected_object *src){
	connected_object *a_c;
	int		i;

	if( (a_c=connected_object_new(src->num_points)) == NULL ){
		fprintf(stderr, "connected_object_copy : call to connected_object_new has failed for %d points.\n", src->num_points);
		return NULL;
	}
	for(i=0;i<src->num_points;i++){
		a_c->x[i] = src->x[i];
		a_c->y[i] = src->y[i];
		a_c->z[i] = src->z[i];
		a_c->v[i] = src->v[i];
	}
	a_c->num_points = src->num_points;
	a_c->id = src->id;
	a_c->x0 = src->x0;
	a_c->y0 = src->y0;
	a_c->w  = src->w;
	a_c->h  = src->h;

	return a_c;
}
void    connected_object_destroy(connected_object *a_c){
	free(a_c->x);
	free(a_c->y);
	free(a_c->z);
	free(a_c->v);
	free(a_c);
}
connected_objects *connected_objects_new(int num_connected_objects, int *num_points){
	/* num_connected_objects and num_points ARE OPTIONAL.
	   num_points is a list of number of points for each object, leave null if this is not known etc,
	   num_connected_objects is the number of objects leave 0 if not known */

	connected_objects 	*a_c;
	int			i, j;

	if( (a_c=(connected_objects *)malloc(sizeof(connected_objects))) == NULL ){
		fprintf(stderr, "connected_objects_new : could not allocate %zd bytes for a_c.\n", sizeof(connected_objects));
		return NULL;
	}

	if( num_connected_objects > 0 ){
		if( (a_c->objects=(connected_object **)malloc(num_connected_objects*sizeof(connected_object *))) == NULL ){
			fprintf(stderr, "connected_objects_new : could not allocate %zd bytes for objects.\n", num_connected_objects*sizeof(connected_object *));
			free(a_c);
			return NULL;
		}
		if( num_points != NULL ){
			for(i=0;i<num_connected_objects;i++)
				if( (a_c->objects[i]=connected_object_new(num_points[i])) == NULL ){
					fprintf(stderr, "connected_objects_new : call to connected_object_new has failed for %d object.\n", i);
					for(j=0;j<i;j++) connected_object_destroy(a_c->objects[j]);
					free(a_c);
					return NULL;
				}
		} else for(i=0;i<num_connected_objects;i++) a_c->objects[i] = NULL;
		a_c->num_connected_objects = num_connected_objects;
	} else {
		a_c->objects = NULL;
		a_c->num_connected_objects = 0;
	}

	return a_c;
}
void    connected_objects_destroy(connected_objects *a_c){
	int	i;
	if( a_c->objects != NULL )
		for(i=0;i<a_c->num_connected_objects;i++) connected_object_destroy(a_c->objects[i]);
	free(a_c);
}

