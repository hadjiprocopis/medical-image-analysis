#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "filters3D.h"
#include "private_filters3D.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to do sharpen on a 3D image
   params:
   	[] ***data, the 3D array of pixels
	[] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
	[] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   	[] ***dataOut, the output image (eroded), we assume that dataOut is filled in with image and we will just have to remove pixels
   	[] level, sharpen level
   returns: the number of pixels eroded
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	sharpen3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0,sum=0.0;k<27;k++)
					sum += _SHARPEN_KERNEL_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
				dataOut[l][i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
			}
		}
}

/* laplacian or edge detect */
void	laplacian3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0,sum=0.0;k<27;k++)
					sum += _LAPLACIAN_KERNEL_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
				dataOut[l][i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
			}
		}
}

/* sobel or edge detect */
void	sobel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	float		sumX, sumY;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0,sumX=sumY=0.0;k<27;k++){
					sumX += _SOBEL_KERNEL_X_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
					sumY += _SOBEL_KERNEL_Y_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
				}
				dataOut[l][i][j] = MIN( 32767, MAX(0, ROUND(level * sqrt(SQR(sumX)+SQR(sumY)) )) );
			}
		}
}

/* prewitt or edge detect */
void	prewitt3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	float		sumX, sumY;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0,sumX=sumY=0.0;k<27;k++){
					sumX += _PREWITT_KERNEL_X_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
					sumY += _PREWITT_KERNEL_Y_3D[k] * data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
				}
				dataOut[l][i][j] = MIN( 32767, MAX(0, ROUND(level * sqrt(SQR(sumX)+SQR(sumY)) )) );
			}
		}
}

/* average */
void	average3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	float		sum;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0,sum=0.0;k<27;k++)
					sum += _AVERAGE_KERNEL_3D[k] * (float )(data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]]);
				dataOut[l][i][j] = MIN(32767, MAX(0, ROUND(level * sum)));
			}
		}
}

/* median */
void	median3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;
	DATATYPE	windowData[27];

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++){
			for(l=z+1;l<z+d-1;l++){
				for(k=0;k<27;k++)
					windowData[k] = data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]];
				qsort((void *)windowData, 27, sizeof(DATATYPE), _DATATYPE_comparator_ascending);
				dataOut[l][i][j] = windowData[14];
			}
		}
}
/* max */
void	max3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++)
			for(l=z+1;l<z+d-1;l++){
				dataOut[l][i][j] = data[l+window_3x3x3_index[0][2]][i+window_3x3x3_index[0][0]][j+window_3x3x3_index[0][1]];
				for(k=1;k<27;k++)
					dataOut[l][i][j] = MAX(data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]], dataOut[l][i][j]);
			}
}
/* min */
void	min3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE ***dataOut, float level){
	int		i, j, l, k;

	for(i=x+1;i<x+w-1;i++)
		for(j=y+1;j<y+h-1;j++)
			for(l=z+1;l<z+d-1;l++){
				dataOut[l][i][j] = data[l+window_3x3x3_index[0][2]][i+window_3x3x3_index[0][0]][j+window_3x3x3_index[0][1]];
				for(k=0;k<27;k++)
					dataOut[l][i][j] = MIN(data[l+window_3x3x3_index[k][2]][i+window_3x3x3_index[k][0]][j+window_3x3x3_index[k][1]], dataOut[l][i][j]);
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
