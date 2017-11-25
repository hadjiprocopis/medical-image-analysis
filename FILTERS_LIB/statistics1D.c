#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to calculate the mean of a 1D array's subwindow starting at offset
   inclusive, and for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
	   [] offset, where to start in the array
	   [] size, how many pixels to consider
   returns: the mean of the subwindow as double
   exceptions: if size is zero you will get division by zero,
   	       if offset exceeds the data size
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	mean1D(DATATYPE *data, int offset, int size){
	int		i;
	DATATYPE	*p = &(data[offset]);
	double		ret = 0.0;

	for(i=0;i<size;i++,p++) ret += *p;
	return ((double )ret) / ((double )size);
}
/* Function to calculate the standard deviation of a 1D array's subwindow starting at offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
	   [] offset, where to start,
	   [] size, how many pixels to consider
   returns: the standard deviation of the subwindow as double
   exceptions: if size is zero you will get division by zero
   note: this function has to call the mean1D function to get the average of the data first.
         if you want to have both mean and stdev, then use the *mean_stdev1D function
         which does this in 1 step.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	stdev1D(DATATYPE *data, int offset, int size){
	DATATYPE	*p = &(data[offset]);
	int		i;
	double		ret = 0.0, mean;

	mean = mean1D(data, offset, size);
	for(i=0;i<size;i++,p++) ret += SQR(*p - mean);
	return sqrt(ret / ((double )size));
}
/* Function to calculate the mean AND standard deviation of a 1D array's subwindow starting at offset
   inclusive, for size pixels
   This function is more efficient than calling mean1D and then stdev1D.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] addresses of mean and stdev to place the results
   returns: nothing
   exceptions: if size is zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	mean_stdev1D(DATATYPE *data, int offset, int size, double *mean, double *stdev){
	DATATYPE	*p = &(data[offset]);
	int		i;

	*mean = mean1D(data, offset, size);
	for(i=0,*stdev=0.0;i<size;i++,p++) *stdev += SQR(*p - *mean);
	*stdev = sqrt(*stdev / ((double )size));
}
/* Function to extract the minimum pixel of a region of a 1D data starting from offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   returns: the minimum pixel of the area as the same type as the data
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	minPixel1D(DATATYPE *data, int offset, int size){
	int		i;
	DATATYPE	*p = &(data[offset]), min = 1000000.0;

	for(i=0;i<size;i++,p++) min = MIN(min, *p);
	return i==0 ? 0 : min;
}
/* Function to extract the maximum pixel of a region of a 1D data starting from offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   returns: the maximum pixel of the area as the same type as the data
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	maxPixel1D(DATATYPE *data, int offset, int size){
	int		i;
	DATATYPE	*p = &(data[offset]), max = -100000.0;

	for(i=0;i<size;i++) max = MAX(max, *p);
	return i==0 ? 0 : max;
}
/* Function to extract the maximum pixel of a region of a 1D data starting from offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] the addresses of min and max pixels to place the results
   returns: nothing
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	min_maxPixel1D(DATATYPE *data, int offset, int size, DATATYPE *min, DATATYPE *max){
	int		i;
	DATATYPE	*p = &(data[offset]);

	*max = -1000000.0; *min = 1000000000.0;
	for(i=0;i<size;i++,p++){
		*max = MAX(*max, *p);
		*min = MIN(*min, *p);
	}
	if( i == 0 ){ *max = *min = 0; }
}
/* Function to extract mean,stdev,min,max from a region of a 1D data starting from offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
   	   [] the addresses of mean, stdev, min and max pixels to place the results
   returns: nothing
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	statistics1D(DATATYPE *data, int offset, int size, DATATYPE *min, DATATYPE *max, double *mean, double *stdev){
	DATATYPE	*p = &(data[offset]);
	int		i;

	*max = -1000000.0; *min = 1000000.0;
	for(i=0,*mean=0.0;i<size;i++,p++){
		*max = MAX(*max, *p);
		*min = MIN(*min, *p);
		*mean += *p;
	}
	if( i == 0 ){ *max = *min = 0; }

	p = &(data[offset]);
	*mean /= (double )size;
	for(i=0,*stdev=0.0;i<size;i++,p++) *stdev += SQR(*p - *mean);
	*stdev = sqrt(*stdev / ((double )size));
}
/* Function to calculate the mean frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the mean of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	frequency_mean1D(DATATYPE *data, int offset, int size, histogram hist){
	int	i;
	double	ret = 0.0;

	for(i=offset;i<size+offset;i++)
		ret += hist.bins[(int )(data[i]-hist.minPixel)];
	return ((double )ret) / ((double )size);
}
/* Function to calculate the stdev of the frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the stdev of frequency of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	frequency_stdev1D(DATATYPE *data, int offset, int size, histogram hist){
	int	i;
	double	ret = 0.0, mean;

	mean = frequency_mean1D(data, offset, size, hist);
	for(i=offset;i<size+offset;i++)
		ret += (hist.bins[(int )(data[i]-hist.minPixel)] - mean) * (hist.bins[(int )(data[i]-hist.minPixel)] - mean);
	return sqrt(ret / ((double )size));
}
/* Function to calculate the mean AND stdev of the frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *mean, *stdev, address to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency_mean_stdev1D(DATATYPE *data, int offset, int size, histogram hist, double *mean, double *stdev){
	int	i;

	*stdev = 0.0;
	*mean = frequency_mean1D(data, offset, size, hist);
	for(i=offset;i<size+offset;i++)
		*stdev += (hist.bins[(int )(data[i]-hist.minPixel)] - *mean) * (hist.bins[(int )(data[i]-hist.minPixel)] - *mean);
	*stdev = sqrt(*stdev / ((double )size));
}
/* Function to calculate the minimum frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the minimum frequency as an integer
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	frequency_min1D(DATATYPE *data, int offset, int size, histogram hist){
	int	i;
	int	min = hist.bins[(int )data[offset]-hist.minPixel];

	for(i=offset;i<size+offset;i++)
		min = MIN(min, hist.bins[(int )(data[i]-hist.minPixel)]);
	return min;
}
/* Function to calculate the maximum frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the maximum frequency as an integer
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	frequency_max1D(DATATYPE *data, int offset, int size, histogram hist){
	int	i;
	int	max = hist.bins[(int )(data[offset]-hist.minPixel)];

	for(i=offset;i<size+offset;i++)
		max = MAX(max, hist.bins[(int )(data[i]-hist.minPixel)]);
	return max;
}
/* Function to calculate the minimum AND maximum frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *min, *max, addresses to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency_min_max1D(DATATYPE *data, int offset, int size, histogram hist, int *min, int *max){
	int	i;
	*min = *max = hist.bins[(int )(data[offset]-hist.minPixel)];

	for(i=offset;i<size+offset;i++){
		*max = MAX(*max, hist.bins[(int )(data[i]-hist.minPixel)]);
		*min = MIN(*min, hist.bins[(int )(data[i]-hist.minPixel)]);
	}
}
/* Function to calculate the mean AND stdev AND minimum AND maximum frequency of pixel value occurence
   of a 1D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *mean, *stdev, *min, *max, addresses to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency1D(DATATYPE *data, int offset, int size, histogram hist, int *min, int *max, double *mean, double *stdev){
	int	i;

	*mean = *stdev = 0.0;
	*min = *max = hist.bins[(int )(data[offset]-hist.minPixel)];

	for(i=offset;i<size+offset;i++){
		*mean += hist.bins[(int )(data[i]-hist.minPixel)];
		*max = MAX(*max, hist.bins[(int )(data[i]-hist.minPixel)]);
		*min = MIN(*min, hist.bins[(int )(data[i]-hist.minPixel)]);
	}
	*mean /= (double )size;
	for(i=offset;i<size+offset;i++)
		*stdev = (hist.bins[(int )(data[i]-hist.minPixel)] - (*mean)) * (hist.bins[(int )(data[i]-hist.minPixel)] - (*mean));
	*stdev = sqrt((*stdev) / ((double )size));
}
