#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"
#include "threshold.h"
#include "maps.h"
 
/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */
 
/* Function to do the following:
	fill in a changeMap depending whether the discrepancy between a pixel value OR the mean pixel
	value of a small subwindow and the mean pixel value of the whole image OR the
	mean pixel value of a larger sub-window is within lo-hi limits. For each image pixel, the
	result will be a changemap_entry (e.g. changed - to change the pixel value of the image, or
	unchanged - to leave the pixel unchanged because it does not fulfill the criteria)
   if a logical operation is defined, it will combine the contents of the changeMap with the changes
   found depending on the logical operation specified.

   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
	   [] **changeMap, 2D array containing enums for changed/unchanged. This represents the result
	      of the previous map application, if any, and the results of this map application
	      will overwrite the old contents of changeMap but depending on the logical operation specified.
	   [] **dataOut, 2D array of DATATYPE into which the final result will reside.
	      if you are only interested on the changeMap, then set this to NULL,
	   [] minOutputColor, maxOutputColor, the dataOut pixel values can be scaled to be within
	      these 2 numbers. If you set them to zero, then no scaling will be done.
	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
              *** beware,  segmentation fault will occur when x+w > image width ...
	   [] sw, sh, mean should be calculated for a small subwindow around each pixel of size 2*sw+1, 2*sh+1
	   [] SW, SH, mean of the whole image is replaced by the mean of this larger subwindow (compared to sw,sh)
	      if the discrepancies of the two means lie between lo-hi then it is success.
	   [] logop, a logical operation AND/OR/NOT or NOP if you do not wish to combine the results
	      of this map application with any other map results.
	   [] mapop, a map operation:
	       1) compare single pixel's property (e.g. pixel value or pixel value frequency) to whole image
	       2) compare single pixel's property (e.g. pixel value or pixel value frequency) to small window's
		  respective property, centered around the pixel
	       3) compare a small window's property (pixel value mean, stdev, pixel value frequency mean, stdev)
		  centered around pixel to whole image
	       4) compare small window's property to larger window's property - both windows centered around pixel in question
	   [] wholeImageMean/Stdev, the mean/stdev of the whole image, in the cases where no larger subwindow is
	      present. When a larger subwindow is present this entry is ignored and can be set to don'tcare
	   [] histogram, in the frequency maps, we need a histogram to have previously been calculated on
	      the area of interest/whole image

	      *** IF you set wholeImageMean/Stdev to zero, you are effectively asking the question whether the pixel value
	      of each pixel is between lo-hi.
   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_mean2D(DATATYPE **data, char ** changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMean){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowMean, largeSubwindowMean;

#ifdef DEBUG
	fprintf(stderr, "maps, map_mean2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), global=%f\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, wholeImageMean);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(data[i][j], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMean, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMean), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMean), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMean-wholeImageMean), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMean = mean2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowMean-smallSubwindowMean), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_mean2D : unknown map operation %d.\n", mapop);
				return FALSE;
		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_mean2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_mean2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(data[i][j], lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? data[i][j] : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMean, lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? smallSubwindowMean : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-wholeImageMean) : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-smallSubwindowMean) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMean-wholeImageMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowMean-wholeImageMean) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowMean = mean2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMean = mean2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMean-largeSubwindowMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowMean-largeSubwindowMean) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_mean2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_mean2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_mean2D : exiting TRUE\n");
#endif
	return TRUE;
}		

/* Function to do the same as map_mean2D but working on the stdev of a small subwindow.

   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_stdev2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageStdev){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowStdev, largeSubwindowStdev;

#ifdef DEBUG
	fprintf(stderr, "maps, map_stdev2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), global=%f\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, wholeImageStdev);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(data[i][j], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowStdev, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageStdev), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowStdev), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowStdev-wholeImageStdev), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowStdev = stdev2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowStdev-smallSubwindowStdev), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_stdev2D : unknown map operation %d.\n", mapop);
				return FALSE;
		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_stdev2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_stdev2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				fprintf(stderr, "maps, map_stdev2D : SINGLE_PIXEL and stdev of pixel value is a meaningless combination.\n");
				return FALSE;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowStdev, lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? smallSubwindowStdev : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-wholeImageStdev) : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-smallSubwindowStdev) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowStdev-wholeImageStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? ABS(smallSubwindowStdev-wholeImageStdev) : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowStdev = stdev2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowStdev = stdev2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowStdev-largeSubwindowStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowStdev-largeSubwindowStdev) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_stdev2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_stdev2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_stdev2D : exiting TRUE\n");
#endif
	return TRUE;
}		

/* Function to do the same as map_mean2D but working on the frequency of occurence of a pixel value
   rather than the pixel value itself.
   For this to happen, you need to specify a histogram of the image area of interest
   (e.g. an array of integers whose index is the pixel value and its contents the
    frequency of occurence of the index)
   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_frequency_mean2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowFrequencyMean, largeSubwindowFrequencyMean;

#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_mean2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), histogram # bins=%d\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, hist->numBins);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMean, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMean), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMean-hist->meanFrequency), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMean = frequency_mean2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowFrequencyMean-smallSubwindowFrequencyMean), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_frequency_mean2D : unknown map operation %d.\n", mapop);

		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_frequency_mean2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_mean2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))] : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMean, lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? smallSubwindowFrequencyMean : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency) : 0;
					tempData[ii][jj] = ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMean) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMean-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMean-hist->meanFrequency) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowFrequencyMean = frequency_mean2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMean = frequency_mean2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMean-largeSubwindowFrequencyMean), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMean-largeSubwindowFrequencyMean) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_frequency_mean2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_mean2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_mean2D : exiting TRUE\n");
#endif
	return TRUE;
}

/* Function to do the same as map_frequency_mean2D but working with the stdev of the frequency of occurence
   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_frequency_stdev2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowFrequencyStdev, largeSubwindowFrequencyStdev;

#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_stdev2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), histogram # bins=%d\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, hist->numBins);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				fprintf(stderr, "maps, map_frequency_stdev2D : SINGLE_PIXEL and stdev of frequency of occurence is a meaningless combination.\n");
				return FALSE;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyStdev, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->stdevFrequency), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyStdev), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyStdev-hist->stdevFrequency), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyStdev = frequency_stdev2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowFrequencyStdev-smallSubwindowFrequencyStdev), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_frequency_stdev2D : unknown map operation %d.\n", mapop);
				return FALSE;
		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_frequency_stdev2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_stdev2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				fprintf(stderr, "maps, map_frequency_stdev2D : SINGLE_PIXEL and stdev of frequency of occurence is a meaningless combination.\n");
				return FALSE;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyStdev, lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? smallSubwindowFrequencyStdev : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->stdevFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->stdevFrequency) : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyStdev) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyStdev-hist->stdevFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyStdev-hist->stdevFrequency) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowFrequencyStdev = frequency_stdev2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyStdev = frequency_stdev2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyStdev-largeSubwindowFrequencyStdev), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyStdev-largeSubwindowFrequencyStdev) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_frequency_stdev2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_stdev2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_stdev2D : exiting TRUE\n");
#endif
	return TRUE;
}


/* xxx */
/* Function to do the same as map_mean2D but working on the min pixel values of a small subwindow.

   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_minPixel2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMinPixel){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowMinPixel, largeSubwindowMinPixel;

#ifdef DEBUG
	fprintf(stderr, "maps, map_minPixel2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), global=%f\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, wholeImageMinPixel);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(data[i][j], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMinPixel, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMinPixel), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMinPixel), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMinPixel-wholeImageMinPixel), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMinPixel = minPixel2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowMinPixel-smallSubwindowMinPixel), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_minPixel2D : unknown map operation %d.\n", mapop);
				return FALSE;
		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_minPixel2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_minPixel2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				fprintf(stderr, "maps, map_minPixel2D : SINGLE_PIXEL and min of pixel value is a meaningless combination.\n");
				return FALSE;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMinPixel, lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? smallSubwindowMinPixel : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMinPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-wholeImageMinPixel) : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMinPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-smallSubwindowMinPixel) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMinPixel-wholeImageMinPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? ABS(smallSubwindowMinPixel-wholeImageMinPixel) : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowMinPixel = minPixel2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMinPixel = minPixel2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMinPixel-largeSubwindowMinPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowMinPixel-largeSubwindowMinPixel) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_minPixel2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_minPixel2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_minPixel2D : exiting TRUE\n");
#endif
	return TRUE;
}		

/* Function to do the same as map_mean2D but working on the min pixel values of a small subwindow.

   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_maxPixel2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMaxPixel){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowMaxPixel, largeSubwindowMaxPixel;

#ifdef DEBUG
	fprintf(stderr, "maps, map_maxPixel2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), global=%f\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, wholeImageMaxPixel);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(data[i][j], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMaxPixel, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMaxPixel), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMaxPixel), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMaxPixel-wholeImageMaxPixel), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMaxPixel = maxPixel2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowMaxPixel-smallSubwindowMaxPixel), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_maxPixel2D : unknown map operation %d.\n", mapop);
				return FALSE;
		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_maxPixel2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_maxPixel2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				fprintf(stderr, "maps, map_maxPixel2D : SINGLE_PIXEL and min of pixel value is a meaningless combination.\n");
				return FALSE;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowMaxPixel, lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? smallSubwindowMaxPixel : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-wholeImageMaxPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-wholeImageMaxPixel) : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(data[i][j]-smallSubwindowMaxPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(data[i][j]-smallSubwindowMaxPixel) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMaxPixel-wholeImageMaxPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = (float )(changeMap[i][j] == changed ? ABS(smallSubwindowMaxPixel-wholeImageMaxPixel) : 0.0);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowMaxPixel = maxPixel2D(data, i-sw, j-sh, SW, SH);
					largeSubwindowMaxPixel = maxPixel2D(data, i-lw, j-lh, LW, LH);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowMaxPixel-largeSubwindowMaxPixel), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowMaxPixel-largeSubwindowMaxPixel) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_maxPixel2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_maxPixel2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_maxPixel2D : exiting TRUE\n");
#endif
	return TRUE;
}		


/* ooo */
/* Function to do the same as map_mean2D but working on the frequency of occurence of a pixel value
   rather than the pixel value itself.
   For this to happen, you need to specify a histogram of the image area of interest
   (e.g. an array of integers whose index is the pixel value and its contents the
    frequency of occurence of the index)
   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_frequency_min2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowFrequencyMin, largeSubwindowFrequencyMin;

#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_min2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), histogram # bins=%d\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, hist->numBins);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMin, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMin), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMin-hist->meanFrequency), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMin = frequency_min2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowFrequencyMin-smallSubwindowFrequencyMin), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_frequency_min2D : unknown map operation %d.\n", mapop);

		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_frequency_min2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_min2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))] : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMin, lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? smallSubwindowFrequencyMin : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency) : 0;
					tempData[ii][jj] = ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMin), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMin) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMin-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMin-hist->meanFrequency) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowFrequencyMin = frequency_min2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMin = frequency_min2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMin-largeSubwindowFrequencyMin), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMin-largeSubwindowFrequencyMin) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_frequency_min2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_min2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_min2D : exiting TRUE\n");
#endif
	return TRUE;
}

/* Function to do the same as map_mean2D but working on the frequency of occurence of a pixel value
   rather than the pixel value itself.
   For this to happen, you need to specify a histogram of the image area of interest
   (e.g. an array of integers whose index is the pixel value and its contents the
    frequency of occurence of the index)
   returns: true on success, false on failure - meaning an error in sw, sh, lw, lh numbers
   note: sw and sh must be specified
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	map_frequency_max2D(DATATYPE **data, char **changeMap, DATATYPE **dataOut, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist){
	int	i, j;
	int	SW = 2 * sw + 1, SH = 2 * sh + 1, LW = 2 * lw + 1, LH = 2 * lh + 1;
	double	smallSubwindowFrequencyMax, largeSubwindowFrequencyMax;

#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_max2D : entering, ROI(%d,%d,%d,%d), smallWindow(%d,%d) largeWindow(%d,%d), LoHi(%f,%f), mapOp=%d, logOp=%d, outColorRange=(%d,%d), histogram # bins=%d\n", x, y, w, h, sw, sh, lw, lh, lo, hi, mapop, logop, minOutputColor, maxOutputColor, hist->numBins);
#endif
	if( dataOut == NULL ){
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
				break;
			case SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMax, lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMax), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw;i<x+w-sw;i++) for(j=y+sh;j<y+h-sh;j++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMax-hist->meanFrequency), lo, hi)?changed:unchanged];
				}
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw;i<x+w-lw;i++) for(j=y+lh;j<y+h-lh;j++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMax = frequency_max2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(largeSubwindowFrequencyMax-smallSubwindowFrequencyMax), lo, hi)?changed:unchanged];
				}
				break;
			default:
				fprintf(stderr, "maps, map_frequency_max2D : unknown map operation %d.\n", mapop);

		} /* switch(mapop) */
	} else { /* dataOut is NOT null, we need to write the results to it */
		float	**tempData, Xmin = 1000000, Xmax = -10000000;
		int	ii, jj;
		if( (tempData=(float **)calloc2D(w, h, sizeof(float))) == NULL ){
			fprintf(stderr, "maps, map_frequency_max2D: could not allocate %d x %d floats.\n", w, h);
			return FALSE;
		}
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_max2D : allocated tempdata : %d x %d floats.\n", w, h);
#endif
		switch( mapop ){
			case SINGLE_PIXEL:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))], lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))] : 0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(smallSubwindowFrequencyMax, lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? smallSubwindowFrequencyMax : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE:
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++){
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency) : 0;
					tempData[ii][jj] = ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-hist->meanFrequency);
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMax), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(hist->bins[MAX(0, (int )(data[i][j]-hist->minPixel))]-smallSubwindowFrequencyMax) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE:
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMax-hist->meanFrequency), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMax-hist->meanFrequency) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+sw,ii=0;i<x+w-sw;i++,ii++) for(j=y+sh,jj=0;j<y+h-sh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			case COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW:
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++){
					smallSubwindowFrequencyMax = frequency_max2D(data, i-sw, j-sh, SW, SH, *hist);
					largeSubwindowFrequencyMax = frequency_max2D(data, i-lw, j-lh, LW, LH, *hist);
					changeMap[i][j] = thresholdTruthTable[logop][(changemap_entry )changeMap[i][j]][IS_WITHIN(ABS(smallSubwindowFrequencyMax-largeSubwindowFrequencyMax), lo, hi)?changed:unchanged];
					tempData[ii][jj] = changeMap[i][j] == changed ? ABS(smallSubwindowFrequencyMax-largeSubwindowFrequencyMax) : 0.0;
					Xmin = MIN(Xmin, tempData[ii][jj]); Xmax = MAX(Xmax, tempData[ii][jj]);
				}
				for(i=x+lw,ii=0;i<x+w-lw;i++,ii++) for(j=y+lh,jj=0;j<y+h-lh;j++,jj++)
					dataOut[i][j] = ROUND(SCALE_OUTPUT(tempData[ii][jj], (float )minOutputColor, (float )maxOutputColor, Xmin, Xmax));
				break;
			default:
				fprintf(stderr, "maps, map_frequency_max2D : unknown map operation %d.\n", mapop);
				free2D((void **)tempData, w);
				return FALSE;
		} /* switch(mapop) */
		free2D((void **)tempData, w);
#ifdef DEBUG
		fprintf(stderr, "maps, map_frequency_max2D : freed tempdata\n");
#endif
	} /* dataOut */
#ifdef DEBUG
	fprintf(stderr, "maps, map_frequency_max2D : exiting TRUE\n");
#endif
	return TRUE;
}


