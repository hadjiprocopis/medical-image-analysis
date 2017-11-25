#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to calculate the mean of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
   returns: the mean of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	mean2D(DATATYPE **data, int x, int y, int w, int h){
	int	i, j;
	double	ret = 0.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			ret += data[i][j];
	return ((double )ret) / ((double )(w*h));
}
/* Function to calculate the standard deviation of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
   returns: the standard deviation of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   note: this function has to call the mean2D function to get the average of the data first.
         if you want to have both mean and stdev, then use the *mean_stdev2D function
         which does this in 1 step.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	stdev2D(DATATYPE **data, int x, int y, int w, int h){
	int	i, j;
	double	ret = 0.0, mean;

	mean = mean2D(data, x, y, w, h);
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			ret += (data[i][j] - mean) * (data[i][j] - mean);
	return sqrt(ret / ((double )(w*h)));
}
/* Function to calculate the mean AND standard deviation of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   This function is more efficient than calling mean2D and then stdev2D.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
   returns: a 2-element array of doubles. First element is mean, second element is stdev.
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	mean_stdev2D(DATATYPE **data, int x, int y, int w, int h, double *mean, double *stdev){
	int	i, j;
	*mean = mean2D(data, x, y, w, h);
	for(i=x,*stdev=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++)
			*stdev += SQR(data[i][j] - *mean);
	*stdev = sqrt(*stdev / ((double )(w*h)));
}
DATATYPE	minPixel2D(DATATYPE **data, int x, int y, int w, int h){
	int		i, j;
	DATATYPE	min = 10000000;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			min = MIN(min, data[i][j]);
	return min;
}
DATATYPE	maxPixel2D(DATATYPE **data, int x, int y, int w, int h){
	int		i, j;
	DATATYPE	max = -100000000.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			max = MAX(max, data[i][j]);
	return max;
}
void	min_maxPixel2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE *min, DATATYPE *max){
	int	i, j;
	*max = -100000000.0; *min = 100000000.0;
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++){
			*max = MAX(*max, data[i][j]);
			*min = MIN(*min, data[i][j]);
		}
}
void	statistics2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE *min, DATATYPE *max, double *mean, double *stdev){
	int	i, j;

	*max = -100000000.0; *min = 100000000.0;
	for(i=x,*mean=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++){
			*max = MAX(*max, data[i][j]);
			*min = MIN(*min, data[i][j]);
			*mean += data[i][j];
		}
	*mean /= (double )(w * h);
	for(i=x,*stdev=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++)
			*stdev += SQR(data[i][j] - *mean);
	*stdev = sqrt(*stdev / ((double )(w*h)));
}
/* Function to calculate the mean frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the mean of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	frequency_mean2D(DATATYPE **data, int x, int y, int w, int h, histogram hist){
	int	i, j;
	double	ret = 0.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			ret += hist.bins[(int )(data[i][j]-hist.minPixel)];

	return ((double )ret) / ((double )(w*h));
}
/* Function to calculate the stdev of the frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the stdev of frequency of the subwindow as double
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	frequency_stdev2D(DATATYPE **data, int x, int y, int w, int h, histogram hist){
	int	i, j;
	double	ret = 0.0, mean;

	mean = frequency_mean2D(data, x, y, w, h, hist);
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			ret += (hist.bins[(int )(data[i][j]-hist.minPixel)] - mean) * (hist.bins[(int )(data[i][j]-hist.minPixel)] - mean);
	return sqrt(ret / ((double )(w*h)));
}
/* Function to calculate the mean AND stdev of the frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *mean, *stdev, address to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency_mean_stdev2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, double *mean, double *stdev){
	int	i, j;

	*stdev = 0.0;
	*mean = frequency_mean2D(data, x, y, w, h, hist);
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			*stdev += (hist.bins[(int )(data[i][j]-hist.minPixel)] - *mean) * (hist.bins[(int )(data[i][j]-hist.minPixel)] - *mean);
	*stdev = sqrt(*stdev / ((double )(w*h)));
}
/* Function to calculate the minimum frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the minimum frequency as an integer
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	frequency_min2D(DATATYPE **data, int x, int y, int w, int h, histogram hist){
	int	i, j;
	int	min = hist.bins[(int )(data[x][y]-hist.minPixel)];

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			min = MIN(min, hist.bins[(int )(data[i][j]-hist.minPixel)]);
	return min;
}
/* Function to calculate the maximum frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
   returns: the maximum frequency as an integer
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	frequency_max2D(DATATYPE **data, int x, int y, int w, int h, histogram hist){
	int	i, j;
	int	max = hist.bins[(int )(data[x][y]-hist.minPixel)];

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			max = MAX(max, hist.bins[(int )(data[i][j]-hist.minPixel)]);
	return max;
}
/* Function to calculate the minimum AND maximum frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *min, *max, addresses to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency_min_max2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, int *min, int *max){
	int	i, j;
	*min = *max = hist.bins[(int )(data[x][y]-hist.minPixel)];

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++){
			*max = MAX(*max, hist.bins[(int )(data[i][j]-hist.minPixel)]);
			*min = MIN(*min, hist.bins[(int )(data[i][j]-hist.minPixel)]);
		}
}
/* Function to calculate the mean AND stdev AND minimum AND maximum frequency of pixel value occurence
   of a 2D array's subwindow starting at x, y
   inclusive, for w and h pixels in the X and Y direction.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] hist, a histogram struscture, the bins entry is an array of integers whose ith element tells us how many times the i+minPixel occurs.
	   [] *mean, *stdev, *min, *max, addresses to put the results
   returns: nothing, results are passed by address
   exceptions: if w or h or both are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	frequency2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, int *min, int *max, double *mean, double *stdev){
	int	i, j;

	*mean = *stdev = 0.0;
	*min = *max = hist.bins[(int )(data[x][y]-hist.minPixel)];

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++){
			*mean += hist.bins[(int )(data[i][j]-hist.minPixel)];
			*max = MAX(*max, hist.bins[(int )(data[i][j]-hist.minPixel)]);
			*min = MIN(*min, hist.bins[(int )(data[i][j]-hist.minPixel)]);
		}
	*mean /= (double )(w*h);
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			*stdev = (hist.bins[(int )(data[i][j]-hist.minPixel)] - (*mean)) * (hist.bins[(int )(data[i][j]-hist.minPixel)] - (*mean));
	*stdev = sqrt((*stdev) / ((double )(w*h)));
}
