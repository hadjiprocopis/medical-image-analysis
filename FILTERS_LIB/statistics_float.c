#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"

/* Function to calculate the mean of a Pixel1D float array's subwindow starting at offset
   inclusive, and for size items
   params: *data, Pixel1D array of float
	   [] offset, where to start in the array
	   [] size, how many pixels to consider
   returns: the mean of the subwindow as float
   exceptions: if size is zero you will get division by zero,
   	       if offset exceeds the data size
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float	meanPixel1D_float(float *data, int offset, int size){
	int		i;
	float		*p = &(data[offset]);
	float		ret = 0.0;

	for(i=0;i<size;i++,p++) ret += *p;
	return ((float )ret) / ((float )size);
}
/* Function to calculate the standard deviation of a Pixel1D array's subwindow starting at offset
   inclusive, for size pixels
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
	   [] offset, where to start,
	   [] size, how many pixels to consider
   returns: the standard deviation of the subwindow as float
   exceptions: if size is zero you will get division by zero
   note: this function has to call the meanPixel1D_float function to get the average of the data first.
         if you want to have both mean and stdev, then use the *mean_stdevPixel1D_float function
         which does this in 1 step.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float	stdevPixel1D_float(float *data, int offset, int size){
	float	*p = &(data[offset]);
	int		i;
	float		ret = 0.0, mean;

	mean = meanPixel1D_float(data, offset, size);
	for(i=0;i<size;i++,p++) ret += SQR(*p - mean);
	return sqrt(ret / ((float )size));
}
/* Function to calculate the mean AND standard deviation of a Pixel1D array's subwindow starting at offset
   inclusive, for size pixels
   This function is more efficient than calling meanPixel1D_float and then stdevPixel1D_float.
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] addresses of mean and stdev to place the results
   returns: nothing
   exceptions: if size is zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	mean_stdevPixel1D_float(float *data, int offset, int size, float *mean, float *stdev){
	float	*p = &(data[offset]);
	int		i;

	*mean = meanPixel1D_float(data, offset, size);
	for(i=0,*stdev=0.0;i<size;i++,p++) *stdev += SQR(*p - *mean);
	*stdev = sqrt(*stdev / ((float )size));
}
/* Function to extract the minimum pixel of a region of a Pixel1D data starting from offset
   inclusive, for size pixels
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   returns: the minimum pixel of the area as the same type as the data
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float	minPixel1D_float(float *data, int offset, int size){
	int		i;
	float	*p = &(data[offset]), min = 10000000000.0;

	for(i=0;i<size;i++,p++) min = MIN(min, *p);
	return i==0 ? 0 : min;
}
/* Function to extract the maximum pixel of a region of a Pixel1D data starting from offset
   inclusive, for size pixels
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   returns: the maximum pixel of the area as the same type as the data
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
float	maxPixel1D_float(float *data, int offset, int size){
	int		i;
	float	*p = &(data[offset]), max = -10000000000.0;

	for(i=0;i<size;i++,p++) max = MAX(max, *p);
	return i==0 ? 0 : max;
}
/* Function to extract the maximum pixel of a region of a Pixel1D data starting from offset
   inclusive, for size pixels
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] the addresses of min and max pixels to place the results
   returns: nothing
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	min_maxPixel1D_float(float *data, int offset, int size, float *min, float *max){
	int		i;
	float	*p = &(data[offset]);
	*max = -100000000000.0; *min = 10000000000.0;
	for(i=0;i<size;i++,p++){
		*max = MAX(*max, *p);
		*min = MIN(*min, *p);
	}
	if( i == 0 ){ *max = *min = 0; }
}
/* Function to extract mean,stdev,min,max from a region of a Pixel1D data starting from offset
   inclusive, for size pixels
   params: *data, Pixel1D_float array of float (this can be defined to whatever - say int or float or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] the addresses of mean, stdev, min and max pixels to place the results
   returns: nothing
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	statistics1D_float(float *data, int offset, int size, float *min, float *max, float *mean, float *stdev){
	float	*p = &(data[offset]);
	int		i;

	*max = -10000000000.0; *min = 1000000000.0;
	for(i=0,*mean=0.0;i<size;i++,p++){
		*max = MAX(*max, *p);
		*min = MIN(*min, *p);
		*mean += *p;
	}
	if( i == 0 ){ *max = *min = 0; }

	p = &(data[offset]);
	*mean /= (float )size;
	for(i=0,*stdev=0.0;i<size;i++,p++)
		*stdev += SQR(*p - *mean);
	*stdev = sqrt(*stdev / ((float )size));
}
