#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <Common_IMMA.h>
#include <filters.h>
#include <Alloc.h>

#include "ring.h"

/* given a set of connected pixels in data, it will produce a ring of width 'width' pixels
   around and at distance 'distance' from the periphery of the pixels - distance can be negative
   which means that the ring falls inside the connected objects.

   w, h, x and y define the rectangle in data where the operation is to be made

   W and H are the real dimensions of data and dataOut

   returns the number of pixels in the ring on success and -1 on failure
*/
int	ring(DATATYPE **data, int width, int distance, int x, int y, int w, int h, int W, int H, int pixelOut, DATATYPE **dataOut){
	/* if we want to expand the objects we call 'dilate' else 'erode' */
	DATATYPE	***scratchPad;
	spec		pixelSpec;
	register int	i, j, k, ret;

	pixelSpec.low = 1;
	pixelSpec.high = 32768;

	if( (scratchPad=callocDATATYPE3D(2, W, H)) == NULL ){
		fprintf(stderr, "ring : call to callocDATATYPE2D has failed for scratchPad (2x%dx%d).\n", W, H);
		return -1;
	}

	/* this is the object, we will colour it with a different color than output so we can remove it later */
	k = pixelOut + 2;
	for(i=0;i<W;i++) for(j=0;j<H;j++) scratchPad[0][i][j] = (data[i][j] > 0) ? k : 0;

 	if( distance < 0 ){
		pixelSpec.newValue = pixelOut + 1;
		/* move inside the object */
		for(k=0;k<-distance;k++){
			erode2D(scratchPad[0], x, y, w, h, scratchPad[1], pixelSpec);
			for(i=0;i<W;i++) for(j=0;j<H;j++)
				if( scratchPad[1][i][j] == pixelSpec.newValue ) scratchPad[0][i][j] = 0;
		}
	} else {
		pixelSpec.newValue = pixelOut + 1;
		/* move outside the object */
		for(k=0;k<distance;k++){
			dilate2D(scratchPad[0], x, y, w, h, scratchPad[1], pixelSpec);
			for(i=0;i<W;i++) for(j=0;j<H;j++)
				if( scratchPad[1][i][j] == pixelSpec.newValue ) scratchPad[0][i][j] = pixelSpec.newValue;
		}
	}
	/* now we are positioned and going to work on the actual body of the ring */
	pixelSpec.newValue = pixelOut;
	for(k=0;k<width;k++){
		dilate2D(scratchPad[0], x, y, w, h, scratchPad[1], pixelSpec);
		for(i=0;i<W;i++) for(j=0;j<H;j++)
			if( scratchPad[1][i][j] == pixelSpec.newValue ) scratchPad[0][i][j] = pixelSpec.newValue;
	}

	/* ok, now remove pixels we do not need and get the pixels we need */
	for(i=0,ret=0;i<W;i++) for(j=0;j<H;j++){
		if( scratchPad[0][i][j] == pixelOut ){
			dataOut[i][j] = pixelOut;
			ret++;
		} else dataOut[i][j] = 0;
	}

	freeDATATYPE3D(scratchPad, 2, W);
	return ret;
}
