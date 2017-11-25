#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "histogram.h"
#include "statistics.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* Function to calculate the mean of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   returns: the mean of the subwindow as double
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   exceptions: if w, h or d are zero you will get division by zero
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
double	mean3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d){
	int	i, j, k;
	double	ret = 0.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				ret += data[k][i][j];
	return ((double )ret) / ((double )(w*h*d));
}

/* Function to calculate the standard deviation of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   returns: the standard deviation of the subwindow as double
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   note: this function has to call the mean3D function to get the average of the data first.
         if you want to have both mean and stdev, then use the mean_stdev3D function
         which does this in 1 step (or use the statistics3D function to also get min and max
         pixel information in one step.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
double	stdev3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d){
	int	i, j, k;
	double	ret = 0.0, mean;

	mean = mean3D(data, x, y, z, w, h, d);
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				ret += SQR(data[k][i][j] - mean);
	return sqrt(ret / ((double )(w*h*d)));
}

/* Function to calculate the mean AND standard deviation of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   	   [] *mean, *stdev, address of variables to place the results for mean and stdev respectively
   returns: nothing, you will get the results if you pass them by address
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   note: this function is faster than calling mean3D and stdev3D one after the other.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
void	mean_stdev3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, double *mean, double *stdev){
	int	i, j, k;
	*mean = mean3D(data, x, y, z, w, h, d);
	for(i=x,*stdev=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				*stdev += SQR(data[k][i][j] - *mean);
	*stdev = sqrt(*stdev / ((double )(w*h*d)));
}

/* Function to return the minimum pixel intensity of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   returns: the minimum pixel intensity of the 3D array subwindow specified
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
DATATYPE	minPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d){
	int		i, j, k;
	DATATYPE	min = 10000000.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				min = MIN(min, data[i][j][k]);
	return min;
}

/* Function to return the maximum pixel intensity of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   returns: the maximum pixel intensity of the 3D array subwindow specified
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
DATATYPE	maxPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d){
	int		i, j, k;
	DATATYPE	max = -100000000.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				max = MAX(max, data[k][i][j]);
	return max;
}

/* Function to return the minimum AND maximum pixel intensity of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   	   [] *min, *max, address of variables to place the results for min and max pixel intensities respectively
   returns: nothing, you get the results by passing them by address
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   note: this function is faster than calling minPixel3D and maxPixel3D one after the other.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
void	min_maxPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE *min, DATATYPE *max){
	int	i, j, k;
	*max = -10000000.0; *min = 10000000.0;

	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++){
				*max = MAX(*max, data[k][i][j]);
				*min = MIN(*min, data[k][i][j]);
			}
}

/* Function to return the minimum, maximum pixel intensity, mean and standard deviation
   of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
   	   [] *min, *max, *mean, *stdev, address to place the results of min, max, mean and stdev
   returns: nothing, you get the results by passing them by address
   note: z represents the slice number, where x and y the x and y coordinates of each slice
   note: this function is faster than calling minPixel3D and maxPixel3D one after the other.
   author: Andreas Hadjiprocopis, NMR, ION, 2001
   exceptions: if w, h or d are zero you will get division by zero
*/
void	statistics3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE *min, DATATYPE *max, double *mean, double *stdev){
	int	i, j, k;

	*max = -10000000.0; *min = 10000000.0;
	for(i=x,*mean=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++){
				*max = MAX(*max, data[k][i][j]);
				*min = MIN(*min, data[k][i][j]);
				*mean += data[k][i][j];
			}
	*mean /= (double )(w * h * d);
	for(i=x,*stdev=0.0;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				*stdev += SQR(data[k][i][j] - *mean);
	*stdev = sqrt(*stdev / ((double )(w*h*d)));
}
