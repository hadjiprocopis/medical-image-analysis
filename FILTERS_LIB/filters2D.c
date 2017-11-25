#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "filters.h"
#include "filters2D.h"
#include "private_filters2D.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to do sharpen on a 2D image
   params:
   	[] **data, the 2D array of pixels
   	[] x, y, w, h, the roi or just the whole image dimensions
   	[] **dataOut, the output image (eroded), we assume that dataOut is filled in with image and we will just have to remove pixels
   	[] level, sharpen level
   returns: the number of pixels eroded
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	sharpen2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0,sum=0.0;k<9;k++)
				sum += _SHARPEN_KERNEL[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
			dataOut[i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
		}
}

/* laplacian or edge detect */
void	laplacian2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0,sum=0.0;k<9;k++)
				sum += _LAPLACIAN_KERNEL[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
			dataOut[i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
		}
}

/* sobel or edge detect */
void	sobel2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	float		sumX, sumY;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0,sumX=0.0,sumY=0.0;k<9;k++){
				sumX += _SOBEL_KERNEL_X[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
				sumY += _SOBEL_KERNEL_Y[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
			}
			dataOut[i][j] = MIN( 32767, MAX(0, ROUND(level * sqrt(SQR(sumX)+SQR(sumY)) )) );
		}
}

/* prewitt or edge detect */
void	prewitt2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	float		sumX, sumY;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0,sumX=0.0,sumY=0.0;k<9;k++){
				sumX += _PREWITT_KERNEL_X[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
				sumY += _PREWITT_KERNEL_Y[k] * data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
			}
			dataOut[i][j] = MIN( 32767, MAX(0, ROUND(level * sqrt(SQR(sumX)+SQR(sumY)) )) );
		}
}

/* average */
void	average2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0,sum=0.0;k<9;k++)
				sum += _AVERAGE_KERNEL[k] * (float )(data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]]);
			dataOut[i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
		}
}

/* median */
void	median2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;
	DATATYPE	windowData[9];

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(k=0;k<9;k++)
				windowData[k] = data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]];
			qsort((void *)windowData, 9, sizeof(DATATYPE), _DATATYPE_comparator_ascending);
			dataOut[i][j] = windowData[4];
		}
}
/* max */
void	max2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			dataOut[i][j] = data[i+window_3x3_index[0][0]][j+window_3x3_index[0][1]];
			for(k=1;k<9;k++)
				dataOut[i][j] = MAX(data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]], dataOut[i][j]);
		}
}
/* min */
void	min2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE **dataOut, float level){
	int		i, j, k;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			dataOut[i][j] = data[i+window_3x3_index[0][0]][j+window_3x3_index[0][1]];
			for(k=1;k<9;k++)
				dataOut[i][j] = MIN(data[i+window_3x3_index[k][0]][j+window_3x3_index[k][1]], dataOut[i][j]);
		}
}

static	int	_DATATYPE_comparator_ascending(const void *a, const void *b){
	if( *((int *)a) > *((int *)b) ) return 1;
	if( *((int *)a) < *((int *)b) ) return -1;
	return 0;
}
/*
static	int	_DATATYPE_comparator_descending(const void *a, const void *b){
	if( *((int *)a) > *((int *)b) ) return -1;
	if( *((int *)a) < *((int *)b) ) return 1;
	return 0;
}

*/
