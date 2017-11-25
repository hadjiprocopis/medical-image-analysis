#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "contour.h"
#include "connected.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"
#include "threshold.h"
#include "erosion.h"

int	window_3x3_index[9][2]; /* these are defined in private_filters2D.h - it will be linked to it by the linker */
int	window_3x3_index_cross[4][2];
/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to do erosion
   params:
   	[] **data, the 2D array of pixels
   	[] x, y, w, h, the roi or just the whole image dimensions
   	[] **dataOut, the output image (eroded), we assume that dataOut is filled in with image and we will just have to remove pixels
   	[] threshold, the pixel value threshold
   	[] thresholdSpec.newValue, the pixel value to replace all eroded pixels
   returns: the number of pixels eroded
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	erode2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, spec thresholdSpec){
	int			i, j, ii, jj, k, obj, id,
				ret = 0,
				max_recursion_level = 0,
				num_connected_objects = 0;
	DATATYPE		**background;
	connected_objects	*myConnectedObjects;

	if( (background=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode2D : call to calloc2D has failed for %dx%d (2)\n", x+w, y+h);
		return FALSE;
	}

	/* now get all the connected objects - e.g. islands of background - we are
	   therefore interested for pixels between 0 and 1 */
	if( find_connected_pixels2D(data, x, y, 0, w, h, background,
				    NEIGHBOURS_4, 0, 1, /* <<< between 0 and 1 */
				    &num_connected_objects, &max_recursion_level, &myConnectedObjects) == FALSE ){
		fprintf(stderr, "erode2D : call to find_connected_pixels2D has failed.\n");
		freeDATATYPE2D(background, x+w);
		return FALSE;
	}

	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) dataOut[i][j] = data[i][j];

	/* now for each background island (a connected object) find its boundary pixels and *dilate* them
	   (e.g. erode its neighbouring pixels) */
	for(obj=0;obj<num_connected_objects;obj++){
		id = myConnectedObjects->objects[obj]->id;
		for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+h-1;j++) {
			if( background[i][j] == id ){ /* only 1 background at a time */
				for(k=0;k<4;k++){
					ii = i + window_3x3_index_cross[k][0]; /* this array contains offsets to drive you around the central pixel in a cross fashion - e.g. left pixel, top, right and bottom */
					jj = j + window_3x3_index_cross[k][1];
					if( IS_WITHIN(data[ii][jj], thresholdSpec.low, thresholdSpec.high) ){
						/* if it is neighbouring to a non-backg. pixel */
						/* remove that non-back. pixel */
						dataOut[ii][jj] = thresholdSpec.newValue;
						ret++;
					}
				}
			}
		}
	}

	freeDATATYPE2D(background, x+w);
	connected_objects_destroy(myConnectedObjects);
	return ret;
}
/* Function to do dilation (expand an outline)
   params:
   	[] **data, the 2D array of pixels
   	[] x, y, w, h, the roi or just the whole image dimensions
   	[] **dataOut, the output image (dilated)
   	[] threshold, the pixel value threshold
   	[] thresholdSpec.newValue, the pixel value that dilate pixels should have
   returns: the number of pixels dilated
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int     dilate2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, spec thresholdSpec){
	int			i, j, ii, jj, k, obj, id,
				ret = 0,
				max_recursion_level = 0,
				num_connected_objects = 0;
	DATATYPE		**background;
	connected_objects	*myConnectedObjects;

	if( (background=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode2D : call to calloc2D has failed for %dx%d (2)\n", x+w, y+h);
		return FALSE;
	}

	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) dataOut[i][j] = data[i][j];

	/* now get all the connected objects - e.g. islands of background - we are
	   therefore interested for pixels between 0 and 1 */
	if( find_connected_pixels2D(data, x, y, 0, w, h, background,
				    NEIGHBOURS_4, 0, 1, /* <<< between 0 and 1 */
				    &num_connected_objects, &max_recursion_level, &myConnectedObjects) == FALSE ){
		fprintf(stderr, "erode2D : call to find_connected_pixels2D has failed.\n");
		freeDATATYPE2D(background, x+w);
		return FALSE;
	}

	/* now for each background island (a connected object) find its boundary pixels and *erode* them
	   (e.g. dilate its neighbouring pixels) */
	for(obj=0;obj<num_connected_objects;obj++){
		id = myConnectedObjects->objects[obj]->id;
		for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+h-1;j++) {
			if( background[i][j] == id ){ /* only 1 background at a time */
				for(k=0;k<4;k++){
//					if( k == 5 ) continue;
					ii = i + window_3x3_index_cross[k][0];
					jj = j + window_3x3_index_cross[k][1];
					if( IS_WITHIN(data[ii][jj], thresholdSpec.low, thresholdSpec.high) ){
						/* if it is neighbouring to a non-backg. pixel */
						/* remove that non-back. pixel */
						dataOut[i][j] = thresholdSpec.newValue;
						ret++;
						break;
					}
				}
			}
		}
	}

	freeDATATYPE2D(background, x+w);
	connected_objects_destroy(myConnectedObjects);
	return ret;
}
/* Function to do erosion of the periphery of the image - e.g. the boundary between 0 and rest and rest and 0
   params:
   	[] **data, the 2D array of pixels
   	[] x, y, w, h, the roi or just the whole image dimensions
   	[] **dataOut, the output image (eroded), we assume that dataOut is filled in with image and we will just have to remove pixels
   	[] threshold, the pixel value threshold
   	[] thresholdSpec.newValue, the pixel value to replace all eroded pixels
   returns: the number of pixels eroded
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	erode_periphery2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, spec thresholdSpec){
	int			i, j, ii, jj, k,
				ret = 0, seedX = -1, seedY = -1,
				max_recursion_level = 0;
	DATATYPE		**scratchPad, **background;
	connected_object	*myConnectedObject;

	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
		if( data[i][j] == 0 ){
			seedX = i; seedY = j;
			i = x+w+1; break;
		}
	if( seedX < 0 ){
		fprintf(stderr, "erode_periphery2D : could not find any background pixels in the image supplied.\n");
		return 0;
	}

	if( (scratchPad=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to calloc2D has failed for %dx%d (1)\n", x+w, y+h);
		return FALSE;
	}
	if( (background=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to calloc2D has failed for %dx%d (2)\n", x+w, y+h);
		freeDATATYPE2D(scratchPad, x+w);
		return FALSE;
	}

	/* call the connected_object routine to find the one connected object which contains the
	   top-left pixel which we will assume belongs to the background. Firstly, the input
	   image will be reversed - all 0 pixels will become 1 and all non-zero will become zero */
	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
		if( data[i][j] > 0 ) scratchPad[i][j] = 0; else scratchPad[i][j] = 1;
		dataOut[i][j] = data[i][j];
	}

	if( (myConnectedObject=find_connected_object2D(scratchPad, seedX, seedY, x, y, 0, w, h, background, NEIGHBOURS_4, 1, 32768, &max_recursion_level)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to find_connected_object2D has failed for seed (%d, %d).\n", seedX, seedY);
		freeDATATYPE2D(scratchPad, x+w);
		return 0;
	}
	freeDATATYPE2D(scratchPad, x+w);

	/* right, now we have an array which contains 1 if the point is background and 0 otherwise */
	/* so now go and erode all the pixels which are at the boundary of back/non-back pixels */
	for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+h-1;j++) {
		if( background[i][j] == 1 ){
			for(k=0;k<9;k++){
				if( k == 5 ) continue;
				ii = i + window_3x3_index[k][0];
				jj = j + window_3x3_index[k][1];
				if( IS_WITHIN(data[ii][jj], thresholdSpec.low, thresholdSpec.high) ){
					/* if it is neighbouring to a non-backg. pixel */
					/* remove that non-back. pixel */
					dataOut[ii][jj] = thresholdSpec.newValue;
					ret++;
				}
			}
		}
	}

	freeDATATYPE2D(background, x+w);
	connected_object_destroy(myConnectedObject);

	return ret;
}

/* Function to do dilation of the periphery of the image - e.g. the boundary between 0 and rest and rest and 0
   params:
   	[] **data, the 2D array of pixels
   	[] x, y, w, h, the roi or just the whole image dimensions
   	[] **dataOut, the output image (dilated), we assume that dataOut is filled in with image and we will just have to remove pixels
   	[] threshold, the pixel value threshold
   	[] thresholdSpec.newValue, the pixel value to replace all dilated pixels
   returns: the number of pixels dilated
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	dilate_periphery2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, spec thresholdSpec){
	int			i, j, ii, jj, k,
				ret = 0, seedX = -1, seedY = -1,
				max_recursion_level = 0;
	DATATYPE		**scratchPad, **background;
	connected_object	*myConnectedObject;

	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
		if( data[i][j] == 0 ){
			seedX = i; seedY = j;
			i = x+w+1; break;
		}
	if( seedX < 0 ){
		fprintf(stderr, "erode_periphery2D : could not find any background pixels in the image supplied.\n");
		return 0;
	}

	if( (scratchPad=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to calloc2D has failed for %dx%d (1)\n", x+w, y+h);
		return FALSE;
	}
	if( (background=callocDATATYPE2D(x+w, y+h)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to calloc2D has failed for %dx%d (2)\n", x+w, y+h);
		freeDATATYPE2D(scratchPad, x+w);
		return FALSE;
	}

	/* call the connected_object routine to find the one connected object which contains the
	   top-left pixel which we will assume belongs to the background. Firstly, the input
	   image will be reversed - all 0 pixels will become 1 and all non-zero will become zero */
	for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
		if( data[i][j] > 0 ) scratchPad[i][j] = 0; else scratchPad[i][j] = 1;
		dataOut[i][j] = data[i][j];
	}

	if( (myConnectedObject=find_connected_object2D(scratchPad, seedX, seedY, x, y, 0, w, h, background, NEIGHBOURS_4, 1, 32768, &max_recursion_level)) == NULL ){
		fprintf(stderr, "erode_periphery2D : call to find_connected_object2D has failed for seed (%d, %d).\n", seedX, seedY);
		freeDATATYPE2D(scratchPad, x+w);
		return 0;
	}
	freeDATATYPE2D(scratchPad, x+w);

	/* right, now we have an array which contains 1 if the point is background and 0 otherwise */
	/* so now go and erode all the pixels which are at the boundary of back/non-back pixels */
	for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+h-1;j++) {
		if( background[i][j] == 1 ){
			for(k=0;k<9;k++){
				if( k == 5 ) continue;
				ii = i + window_3x3_index[k][0];
				jj = j + window_3x3_index[k][1];
				if( IS_WITHIN(data[ii][jj], thresholdSpec.low, thresholdSpec.high) ){
					/* if it is neighbouring to a non-backg. pixel */
					dataOut[i][j] = thresholdSpec.newValue;
					ret++;
					break;
				}
			}
		}
	}
	freeDATATYPE2D(background, x+w);
	connected_object_destroy(myConnectedObject);

	return ret;
}


