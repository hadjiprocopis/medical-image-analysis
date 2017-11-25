#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"

/* DATATYPE is defined in filters.h to be anythng you like (int or short int) */

/* private (comparators for qsort) */
typedef	struct	_HISTOGRAM_SORT_DATA {
	float		value;
	DATATYPE	pixel;
} _histogram_sort_data;

static  int     _histogram_comparator_ascending(const void *, const void *);
static  int     _histogram_comparator_descending(const void *, const void *);

/* Function to produce a histogram of pixel intensities of a region of a 1D data starting from offset
   inclusive, for size pixels
   params: *data, 1D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] offset, where to start
   	   [] size, how many pixels to consider
	   [] binSize, the size of each bin
   returns: a histogram structure as described in <histogram.h>
   	    NOTE that although bins start from zero, pixel ranges for each
   	    bin start from minPixel. So if your pixel values range from 100 to 200
   	    and you have 2 bins, then first bin represents pixel values from 100 to 150
   	    and second bin, pixel values from 150 to 200.
   note : DO NOT FORGET TO USE destroy_histogram(histogram *) to finish with the histogram when not needed any more
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
histogram *histogram1D(DATATYPE *data, int offset, int size, int binSize){
	int		i;
	histogram	*ret;

	if( (ret=(histogram *)malloc(sizeof(histogram))) == NULL ){
		fprintf(stderr, "histogram1D : could not allocate %zd bytes for histogram.\n", sizeof(histogram));
		return NULL;
	}
	ret->smooth = ret->normalised = NULL; ret->is_smooth = ret->is_normalised = FALSE; 
	ret->smooth_window_size = -1;

	ret->binSize = binSize;
	min_maxPixel1D(data, offset, size, &(ret->minPixel), &(ret->maxPixel));
	ret->numBins = (int )((ret->maxPixel - ret->minPixel) / ret->binSize) + 1;
	if( (ret->bins=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram1D : could not allocate %d integers (min pixel = %d, max pixel = %d) for bins.\n", ret->numBins, ret->minPixel, ret->maxPixel);
		free(ret);
		return NULL;
	}
	if( (ret->cumulative_frequency=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram1D : could not allocate %d integers(2) (min pixel = %d, max pixel = %d) for cumulative_frequency.\n", ret->numBins, ret->minPixel, ret->maxPixel);
		free(ret->bins); free(ret);
		return NULL;
	}
	for(i=0;i<ret->numBins;i++) ret->bins[i] = 0;
	/* 0 to binSize-1 goes to first bin, binSize to 2*binSize-1 goes to 2nd bin etc. */
	/* if minPixel not 0 then replace 0 by minPixel, so :
	   minPixel to minPixel+binSize-1 goes to 1st bin etc. */
	for(i=offset;i<size+offset;i++)
		ret->bins[(int )((data[i] - ret->minPixel)/ret->binSize)]++;

	ret->cumulative_frequency[0] = ret->bins[0];
	for(i=1;i<ret->numBins;i++)
		ret->cumulative_frequency[i] = ret->cumulative_frequency[i-1] + ret->bins[i];

	if( calculate_histogram(ret) == FALSE ){
		fprintf(stderr, "histogram1D : call to calculate_histogram has failed.\n");
		free(ret->bins); free(ret->cumulative_frequency); free(ret);
		return NULL;
	}
	return ret;
}

/* Function to free the data occupied by a histogram structure
	[] *h, the histogram to destroy and free
   returns: nothing
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
void	destroy_histogram(histogram *h){
	if( h->bins != NULL ) free(h->bins);
	if( h->smooth != NULL ) free(h->smooth);
	if( h->normalised != NULL ) free(h->normalised);
	free(h);
}

/* private : 2 routines for qsort to sort an array of integers in ascending and descending order */
static	int	_histogram_comparator_descending(const void *a, const void *b){
	float	_a = ((_histogram_sort_data *)a)->value,
		_b = ((_histogram_sort_data *)b)->value;

	if( _a > _b ) return -1;
	if( _a < _b ) return 1;
	return 0;
}
static	int	_histogram_comparator_ascending(const void *a, const void *b){
	float	_a = ((_histogram_sort_data *)a)->value,
		_b = ((_histogram_sort_data *)b)->value;

	if( _a > _b ) return 1;
	if( _a < _b ) return -1;
	return 0;
}

/* Function to sort bin or normalised or smoothed entries of a histogram
	(e.g. the frequencies of occurence)
	[] *h, the histogram
	[] sort_type, HISTOGRAM_SORT_ASCENDING, HISTOGRAM_SORT_BINS, etc. (see histogram.h)
   returns: an array of DATATYPE which represents the pixels ordered according to spec.
	    i.e. it returns an array of indices to the bins sorted.
	    For example (ret is the returned array)
	    	histo->bins[ret[0]-histo->minPixel] is the frequency of
	    	occurence of the most popular pixel (note the -minPixel).
	    
	** do not forget to free the returned array when not needed any more.

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
DATATYPE	*sort_histogram(histogram *h, int sort_type){
	int			i;
	DATATYPE		*ret;
	_histogram_sort_data	*dummy;

	if( (ret=(DATATYPE *)malloc(h->numBins*sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "sort_histogram : could not allocate %zd bytes for ret.\n", h->numBins*sizeof(DATATYPE));
		return NULL;
	}
	if( (dummy=(_histogram_sort_data *)malloc(h->numBins*sizeof(_histogram_sort_data))) == NULL ){
		fprintf(stderr, "sort_histogram : could not allocate %zd bytes for dummy.\n", h->numBins*sizeof(_histogram_sort_data));
		free(ret);
		return NULL;
	}
	if( sort_type & HISTOGRAM_SORT_SMOOTH ){
		if( h->is_smooth == FALSE ){
			fprintf(stderr, "sort_histogram : can not sort histogram wrt smoothed frequency data because it is not smoothed.\n");
			free(ret); free(dummy);
			return NULL;
		}
		for(i=0;i<h->numBins;i++){ dummy[i].pixel = i+h->minPixel; dummy[i].value = h->smooth[i]; }
	} else if( sort_type & HISTOGRAM_SORT_NORMALISED ){
		if( h->is_normalised == FALSE ){
			fprintf(stderr, "sort_histogram : can not sort histogram wrt normalised frequency data because it is not normalised.\n");
			free(ret); free(dummy);
			return NULL;
		}
		for(i=0;i<h->numBins;i++){ dummy[i].pixel = i+h->minPixel; dummy[i].value = h->normalised[i]; }
	} else /* just from the bins */
		for(i=0;i<h->numBins;i++){ dummy[i].pixel = i+h->minPixel; dummy[i].value = (float )(h->bins[i]); }

	if( sort_type & HISTOGRAM_SORT_DESCENDING )
		qsort((void *)dummy, h->numBins, sizeof(_histogram_sort_data), _histogram_comparator_descending);
	else /* ascending (the default) */
		qsort((void *)dummy, h->numBins, sizeof(_histogram_sort_data), _histogram_comparator_ascending);

	/* donot forget, it returns pixel values which are
	   index (pixel-minPixel) to the bins or smooth arrays etc. */
	for(i=0;i<h->numBins;i++) ret[i] = dummy[i].pixel;

	free(dummy);
	return ret;
}

/* Function to smooth a histogram. Histogram is the input parameter
   
   Given a window size W, the ith element of the input array will
   take the new value of the sum of: W elements before to W elements after
   including the ith element itself (in the middle)
   and divided by (2*W+1). (Sum_(i-W)^(i+W) b[i]) / (2*W+1)

   params:
   	[] the histogram structure pointer
	[] smoothWindowSize, **HALF** the size of the smoothing window.
	[] use_normalised_values, a flag which, if TRUE, indicates that
	   the data for the smoothing operation should be supplied from
	   the normalised array rather than the bins. FALSE means data
	   is taken from the bins.
   returns: TRUE if success or false otherwise.

   The results will be placed in the *smooth array of the histogram,
   and so will be the window size. The array will be of the same size
   as that of bins, however
   its first and last W/2 elements will be the same as that of
   the bins (because smoothing can not be done at the edges)

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	histogram_smooth(histogram *h, int smoothWindowSize, char use_normalised_values){
	int	i, j;
	float	sum, divi = 2.0 * ((float )smoothWindowSize) + 1.0;

	if( (h->smooth_window_size=smoothWindowSize) <= 0 ){
		fprintf(stderr, "histogram_smooth : window size for smoothing must be a positive integer, it was %d!\n", smoothWindowSize);
		return FALSE;
	}
	if( (use_normalised_values==TRUE) && (h->is_normalised==FALSE) ){
		fprintf(stderr, "histogram_smooth : normalised values were requested but no normalisation has taken place for this histogram.\n");
		return FALSE;
	}

	if( h->smooth == NULL )
		if( (h->smooth=(float *)malloc(h->numBins*sizeof(float))) == NULL ){
			fprintf(stderr, "histogram_smooth : could not allocate %zd bytes for smooth array.\n", h->numBins*sizeof(float));
			return FALSE;
		}
	if( use_normalised_values ){
		for(i=0;i<h->numBins;i++) h->smooth[i] = h->normalised[i];
		for(i=smoothWindowSize;i<h->numBins-smoothWindowSize;i++){
			for(j=i-smoothWindowSize,sum=0.0;j<i+smoothWindowSize;j++)
				sum += h->normalised[j];
			h->smooth[i] = sum / divi;
		}
	} else {
		for(i=0;i<h->numBins;i++) h->smooth[i] = (float )(h->bins[i]);
		for(i=smoothWindowSize;i<h->numBins-smoothWindowSize;i++){
			for(j=i-smoothWindowSize,sum=0.0;j<i+smoothWindowSize;j++)
				sum += (float )(h->bins[j]);
			h->smooth[i] = sum / divi;
		}
	}

	h->is_smooth = TRUE;

	return TRUE;
}
/* Function to normalise a histogram. 
   
   Normalisation consists of dividing each element of the input array by
   the sum of all elements of the input array (or those within 'offset' and 'size')

   params:
	[] h, the input histogram to normalise.
	[] use_smooth_values, a flag which, if TRUE, indicates that
	   the data for the normalising operation should be supplied from
	   the smooth array rather than the bins. FALSE means data
	   is taken from the bins.
   returns: TRUE if success, or FALSE if malloc errors or if smooth array is empty but use_smooth_values was true

   The results will be placed into the *normalised array of the histogram structure

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
int	histogram_normalise(histogram *h, char use_smooth_values){
 	int	i;
	float	sum;

	if( (use_smooth_values==TRUE) && (h->is_smooth==FALSE) ){
		fprintf(stderr, "histogram_normalise : smooth values were requested but no smoothing has taken place for this histogram.\n");
		return FALSE;
	}

	if( h->normalised == NULL )
		if( (h->normalised=(float *)malloc(h->numBins*sizeof(float))) == NULL ){
			fprintf(stderr, "histogram_normalise : could not allocate %zd bytes for normalised array.\n", h->numBins*sizeof(float));
			return FALSE;
		}

	if( use_smooth_values ){
		for(i=0,sum=0.0;i<h->numBins;i++) sum += h->smooth[i];
		for(i=0;i<h->numBins;i++) h->normalised[i] = h->smooth[i] / sum;
	} else {
		for(i=0,sum=0.0;i<h->numBins;i++) sum += (float )(h->bins[i]);
		for(i=0;i<h->numBins;i++) h->normalised[i] = ((float )(h->bins[i])) / sum;
	}

	h->is_normalised = TRUE;
	return TRUE;
}

/* Function to calculate the pixel intensity histogram of a 2D array of type DATATYPE.
   params: **data, 2D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, the subwindow's x and y coordinates (set these to zero for the whole data)
   	   [] w, h, the subwindow's width and height (set these to the whole data's width and height if you want all the data to be considered)
	   [] binSize, the size of the bin
	   [] *minPixel, *maxPixel, addresses to place the minimum and maximum pixel intensities
	      because we have to do this calculation, so we may as well return them
	   [] *numBinx, address to place the number of bins produced
	   [] *minF, *maxF, *meanF, *stdevF, addresses to place
	      the min, max, mean and stdev of the histogram contents, if these are null, then
	      no stats will be done.
   returns: a pointer to a histogram structure. The ith element of the 'bins' entry contains the number of pixels falling
   	    to the ith bin. NOTE that although bins start from zero, pixel ranges for each
   	    bin start from minPixel. So if your pixel values range from 100 to 200
   	    and you have 2 bins, then first bin represents pixel values from 100 to 150
   	    and second bin, pixel values from 150 to 200.
   	    OR NULL if failed (to allocate memory)

   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
histogram	*histogram2D(DATATYPE **data, int x, int y, int w, int h, int binSize){
	int		i, j;
	histogram	*ret;

	if( (ret=(histogram *)malloc(sizeof(histogram))) == NULL ){
		fprintf(stderr, "histogram2D : could not allocate %zd bytes for histogram.\n", sizeof(histogram));
		return NULL;
	}

	ret->smooth = ret->normalised = NULL; ret->is_smooth = ret->is_normalised = FALSE;
	ret->smooth_window_size = -1;

	ret->binSize = binSize;
	min_maxPixel2D(data, x, y, w, h, &(ret->minPixel), &(ret->maxPixel));
	ret->numBins = (int )((ret->maxPixel - ret->minPixel) / binSize) + 1;
	if( (ret->bins=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram2D : could not allocate %zd bytes (min pixel = %d, max pixel = %d, num bins = %d) for bins.\n", ret->numBins*sizeof(int), ret->minPixel, ret->maxPixel, ret->numBins);
		free(ret);
		return NULL;
	}
	if( (ret->cumulative_frequency=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram1D : could not allocate %d integers(2) (min pixel = %d, max pixel = %d, num bins = %d) for cumulative_frequency.\n", ret->numBins, ret->minPixel, ret->maxPixel, ret->numBins);
		free(ret->bins); free(ret);
		return NULL;
	}
	for(i=0;i<ret->numBins;i++) ret->bins[i] = 0;
	/* 0 to binSize-1 goes to first bin, binSize to 2*binSize-1 goes to 2nd bin etc. */
	/* if minPixel not 0 then replace 0 by minPixel, so :
	   minPixel to minPixel+binSize-1 goes to 1st bin etc. */
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			ret->bins[(int )((data[i][j] - ret->minPixel)/ret->binSize)]++;
	ret->cumulative_frequency[0] = ret->bins[0];
	for(i=1;i<ret->numBins;i++)
		ret->cumulative_frequency[i] = ret->cumulative_frequency[i-1] + ret->bins[i];

	if( calculate_histogram(ret) == FALSE ){
		fprintf(stderr, "histogram1D : call to calculate_histogram has failed.\n");
		free(ret->bins); free(ret->cumulative_frequency); free(ret);
		return NULL;
	}
	return ret;
}

/* Function to produce the histogram of pixel intensities
   of a 3D array's subwindow starting at x, y, z(slice)
   inclusive, for w, h and d pixels in the X, Y and Z direction.
   params: **data, 3D array of DATATYPE (this can be defined to whatever - say int or double or short int)
   	   [] x, y, z, the subwindow's top-left corner x, y and z coordinates (set these to zero for the whole data)
   	   [] w, h, d, the subwindow's width, height and depth (set these to the whole data's width, height and depth if you want all the data to be considered)
	   [] binSize, the size of each bin (pixel intensity range)
   	   [] *min, *max, *numBins, address to place the min/max pixel intensities found and the number of bins produced
   returns: a pointer to a histogram structure. The ith element of its 'bins' field array
	    contains the number of pixels falling
   	    to the ith bin. NOTE that although bins start from zero, pixel ranges for each
   	    bin start from minPixel. So if your pixel values range from 100 to 200
   	    and you have 2 bins, then first bin represents pixel values from 100 to 150
   	    and second bin, pixel values from 150 to 200.
   	    OR NULL if failed (to allocate memory)
   note: z represents the slice number, where x and y the x and y coordinates of each slice
	DO NOT FORGET TO USE destroy_histogram(histogram *) to finish with the histogram when not needed any more
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
histogram	*histogram3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, int binSize){
	int		i, j, k;
	histogram	*ret;

	if( (ret=(histogram *)malloc(sizeof(histogram))) == NULL ){
		fprintf(stderr, "histogram3D : could not allocate %zd bytes for ret.\n", sizeof(histogram));
		return NULL;
	}

	ret->smooth = ret->normalised = NULL; ret->is_smooth = ret->is_normalised = FALSE;
	ret->smooth_window_size = -1;

	ret->binSize = binSize;
	min_maxPixel3D(data, x, y, z, w, h, d, &(ret->minPixel), &(ret->maxPixel));
	ret->numBins = (int )((ret->maxPixel - ret->minPixel) / binSize) + 1;
	if( (ret->bins=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram3D : could not allocate %d integers (min pixel=%d, max pixel=%d) for bins.\n", ret->numBins, ret->minPixel, ret->maxPixel);
		return NULL;
	}
	if( (ret->cumulative_frequency=(int *)calloc(ret->numBins, sizeof(int))) == NULL ){
		fprintf(stderr, "histogram3D : could not allocate %d integers (min pixel=%d, max pixel=%d) for cumulative_frequency.\n", ret->numBins, ret->minPixel, ret->maxPixel);
		free(ret->bins); free(ret);
		return NULL;
	}
	for(i=0;i<ret->numBins;i++) ret->bins[i] = 0;
	/* 0 to binSize-1 goes to first bin, binSize to 2*binSize-1 goes to 2nd bin etc. */
	/* if minPixel not 0 then replace 0 by minPixel, so :
	   minPixel to minPixel+binSize-1 goes to 1st bin etc. */
	for(i=x;i<x+w;i++)
		for(j=y;j<y+h;j++)
			for(k=z;k<z+d;k++)
				ret->bins[(int )((data[k][i][j] - ret->minPixel)/ret->binSize)]++;
	ret->cumulative_frequency[0] = ret->bins[0];
	for(i=1;i<ret->numBins;i++)
		ret->cumulative_frequency[i] = ret->cumulative_frequency[i-1] + ret->bins[i];

	if( calculate_histogram(ret) == FALSE ){
		fprintf(stderr, "histogram1D : call to calculate_histogram has failed.\n");
		free(ret->bins); free(ret->cumulative_frequency); free(ret);
		return NULL;
	}
	return ret;
}

void	print_histogram(FILE *fp, histogram *h){
	int	i;
	fprintf(fp, "pixel values range : %d to %d\n", h->minPixel, h->maxPixel);
	fprintf(fp, "pixel value frequencies range : %d to %d\n", h->minFrequency, h->maxFrequency);
	fprintf(fp, "num bins: %d\n", h->numBins);
	
	for(i=0;i<h->numBins;i++)
		fprintf(fp, "%d)\tpixel %d occurs %d times\n", i, i+h->minPixel, h->bins[i]);
}

/* Will calculate the different percentiles, and peak location for given histogram.
   it operates only on *non-zero frequency pixels*. Be careful because the histogram
   might contain pixels whose frequency is zero, those do not count.

   returns: TRUE on success, FALSE on failure (e.g. malloc)
*/
int	calculate_histogram(histogram *h){
	DATATYPE	*sorted, dummy1, dummy2;
	double		dummy3, dummy4;
	register int	i;
	int		numNonZeroPixels = 0;
	float		sum;
	
	statistics1D((DATATYPE *)(h->bins), 0, h->numBins, &dummy1, &dummy2, &dummy3, &dummy4);
	h->minFrequency = (int )dummy1; h->maxFrequency = (int )dummy2;
	h->meanFrequency = (float )dummy3; h->stdevFrequency = (float )dummy4;

	h->histogram_mean = sum = 0.0;
	for(i=0;i<h->numBins;i++){
		if( h->bins[i] > 0 ) numNonZeroPixels++;
		//printf("bins[%d] = %d\n", i*h->binSize+h->minPixel, h->bins[i]);
		h->histogram_mean += h->bins[i] * (i*h->binSize+h->minPixel);
		sum += h->bins[i];
	}
	h->histogram_mean /= sum;

	/* find the min and max pixels which they really occur (e.g. frequency > 0) */
	for(i=0;i<h->numBins;i++) if( h->bins[i] > 0 ){ h->minActivePixel = i + h->minPixel; break; }
	for(i=h->numBins-1;i>=0;i--) if( h->bins[i] > 0 ){ h->maxActivePixel = i + h->minPixel; break; }

	//printf("before was %f, sum is %f and after is %f\n", h->histogram_mean, sum,  h->histogram_mean/sum);

	/* the percentiles are defined as follows:
		median is the T1 value of the middle voxel (ie that voxel that is 50% along the
		line of voxels when all the voxels in the brain are ordered in a line from that
		with the lowest T1 value to that with the highest). The 25th and 75th
		percentiles are the T1 values for the voxel which is 1/4 and 3/4 of the way
		along that line. */
	for(i=0;i<h->numBins;i++) if( h->bins[i] > 0 ) break;
	h->pixel_p25 = i + (h->numBins-i)/4;
	h->pixel_p50 = i + (h->numBins-i)/2;
	h->pixel_p75 = i + (3*(h->numBins-i))/4;

	/* so we will sort the bins according to frequency and choose the pixel values we need
	   note that h->p25(etc.) it gives you a pixel value,
	   if you want to find what the bins value is at that point do:
	   	h->bins[h->p25 - h->minPixel] (an integer)
	   if you need to find out the smooth freq value for that pixel then do (assuming you smoothed the hist before)
	   	h->smooth[h->smooth_p25 - h->minPixel] (a float)
	   and the normalised one:
	   	h->normalised[h->normalised_p25 - h->minPixel] (a float)
	*/
	if( (sorted=sort_histogram(h, HISTOGRAM_SORT_BINS|HISTOGRAM_SORT_ASCENDING)) == NULL ){
		fprintf(stderr, "calculate_histogram : call to sort_histogram (bins) has failed.\n");
		return FALSE;
	}
	for(i=0;i<h->numBins;i++) if( h->bins[i] > 0 ) break;
	h->p25 = sorted[i + (h->numBins-i) / 4];
	h->p50 = sorted[i + (h->numBins-i) / 2];
	h->p75 = sorted[i + (3*(h->numBins-i)) / 4];
	h->peak= sorted[h->numBins-1];
	free(sorted);
	h->pixel_minFrequency = h->pixel_maxFrequency = -1;
	for(i=0;i<h->numBins;i++) if( h->minFrequency == h->bins[i] ){ h->pixel_minFrequency = h->minPixel+i; break; }
	for(i=0;i<h->numBins;i++) if( h->maxFrequency == h->bins[i] ){ h->pixel_maxFrequency = h->minPixel+i; break; }

	if( h->is_smooth ){
		statistics1D_float(h->smooth, 0, h->numBins, &(h->smooth_minFrequency), &(h->smooth_maxFrequency), &(h->smooth_meanFrequency), &(h->smooth_stdevFrequency));
		if( (sorted=sort_histogram(h, HISTOGRAM_SORT_SMOOTH|HISTOGRAM_SORT_ASCENDING)) == NULL ){
			fprintf(stderr, "calculate_histogram : call to sort_histogram (smooth) has failed.\n");
			return FALSE;
		}
		for(i=0;i<h->numBins;i++) if( sorted[i] > 0 ) break;
		h->smooth_p25 = sorted[i + (h->numBins-i) / 4];
		h->smooth_p50 = sorted[i + (h->numBins-i) / 2];
		h->smooth_p75 = sorted[i + (3*(h->numBins-i)) / 4];
		h->smooth_peak= sorted[h->numBins-1];
		free(sorted);
		h->pixel_smooth_minFrequency = h->pixel_smooth_maxFrequency = -1;
		for(i=0;i<h->numBins;i++) if( h->smooth_minFrequency == h->smooth[i] ){ h->pixel_smooth_minFrequency = h->minPixel+i; break; }
		for(i=0;i<h->numBins;i++) if( h->smooth_maxFrequency == h->smooth[i] ){ h->pixel_smooth_maxFrequency = h->minPixel+i; break; }

		/* find the min and max pixels which they really occur (e.g. frequency > 0) */
		for(i=0;i<h->numBins;i++) if( h->smooth[i] > 0 ){ h->smooth_minActivePixel = i + h->minPixel; break; }
		for(i=h->numBins-1;i>=0;i--) if( h->smooth[i] > 0 ){ h->smooth_maxActivePixel = i + h->minPixel; break; }
	}
	if( h->is_normalised ){
		statistics1D_float(h->normalised, 0, h->numBins, &(h->normalised_minFrequency), &(h->normalised_maxFrequency), &(h->normalised_meanFrequency), &(h->normalised_stdevFrequency));
		if( (sorted=sort_histogram(h, HISTOGRAM_SORT_NORMALISED|HISTOGRAM_SORT_ASCENDING)) == NULL ){
			fprintf(stderr, "calculate_histogram : call to sort_histogram (normalised) has failed.\n");
			return FALSE;
		}
		for(i=0;i<h->numBins;i++) if( sorted[i] > 0 ) break;
		h->normalised_p25 = sorted[i + (h->numBins-i) / 4];
		h->normalised_p50 = sorted[i + (h->numBins-i) / 2];
		h->normalised_p75 = sorted[i + (3*(h->numBins-i)) / 4];
		h->normalised_peak= sorted[h->numBins-1];
/*		h->normalised_p75 = sorted[numNonZeroPixels / 4];
		h->normalised_p50 = sorted[numNonZeroPixels / 2];
		h->normalised_p25 = sorted[(3*numNonZeroPixels) / 4];
		h->normalised_peak= sorted[0];*/
		free(sorted);
		h->pixel_normalised_minFrequency = h->pixel_normalised_maxFrequency = -1;
		for(i=0;i<h->numBins;i++) if( h->normalised_minFrequency == h->normalised[i] ){ h->pixel_normalised_minFrequency = h->minPixel+i; break; }
		for(i=0;i<h->numBins;i++) if( h->normalised_maxFrequency == h->normalised[i] ){ h->pixel_normalised_maxFrequency = h->minPixel+i; break; }

		/* find the min and max pixels which they really occur (e.g. frequency > 0) */
		for(i=0;i<h->numBins;i++) if( h->normalised[i] > 0 ){ h->normalised_minActivePixel = i + h->minPixel; break; }
		for(i=h->numBins-1;i>=0;i--) if( h->normalised[i] > 0 ){ h->normalised_maxActivePixel = i + h->minPixel; break; }
	}
		
	return TRUE;
}

/* Scales the histogram 'tobescaled' according to 'reference'.
   What it does: Makes sure that the two histograms end up
   with the same number of pixels.
*/
int	scale_histogram(histogram *reference, histogram *tobescaled, int option){
	double	sumRef, sumTBS, scale_factor;
	int	i, diff,
		I = (option==HISTOGRAM_DONT_COUNT_BACKGROUND_PIXEL) ? 1 : 0;
	DATATYPE	*sorted;

	for(i=I,sumRef=0.0;i<reference->numBins;i++)
		sumRef += (double )(reference->bins[i]);
	for(i=I,sumTBS=0.0;i<tobescaled->numBins;i++)
		sumTBS += (double )(tobescaled->bins[i]);

	if( sumTBS == 0 ){
		fprintf(stderr, "scale_histogram : the histogram to be scaled is empty (no active pixels)!\n");
		return FALSE;
	}
	if( sumRef == 0 ){
		fprintf(stderr, "scale_histogram : the reference histogram is empty (no active pixels)!\n");
		return FALSE;
	}
	scale_factor = sumRef / sumTBS;

	for(i=I;i<tobescaled->numBins;i++)
		tobescaled->bins[i] = (int )(round(((double )(tobescaled->bins[i])) * scale_factor));


	/* now verify that the sum of pixels of T is the same as that of 0 
	   rounding errors will make this a possibility,
	   so remove or add pixels to the first bin, the background */
	for(i=I,sumTBS=0.0;i<tobescaled->numBins;i++) sumTBS += tobescaled->bins[i];
	if( (diff=(sumRef-sumTBS)) == 0 ){
		/* no we are exact */
		calculate_histogram(tobescaled);
		return TRUE;
	}

	/* yes, we need slight correction */

	/* first, sort the bins of tobescaled in descending order */
	if( (sorted=sort_histogram(tobescaled, HISTOGRAM_SORT_BINS|HISTOGRAM_SORT_DESCENDING)) == NULL ){
		fprintf(stderr, "scale_histogram : call to sort_histogram (bins) has failed.\n");
		return FALSE;
	}

	if( diff < 0 ){
		diff = ABS(diff);
		/* remove a pixel from the most popular bins */
		for(i=0;i<diff;i++)
			tobescaled->bins[sorted[i]-tobescaled->minPixel+I]--;
	} else {
		/* add a pixel to the most popular bins (Q: is that the best way?) */
		for(i=0;i<diff;i++)
			tobescaled->bins[sorted[i]-tobescaled->minPixel+I]++;
	}
	free(sorted);

	/* verify */
	for(i=I,sumTBS=0.0;i<tobescaled->numBins;i++) sumTBS += tobescaled->bins[i];
	if( sumRef != sumTBS ){
		fprintf(stderr, "failed (reference histogram has %.0f bins, to-be-scaled has %.0f bins, scaling factor was %f)...\n", sumRef, sumTBS, scale_factor);
		return FALSE;
	}
	calculate_histogram(tobescaled);
	return TRUE;
}	

/*Will do histogram matching of an input image based on a target histogram.
	calculate the histogram (h0) of the image to be transformed (inp)
	and then
	match the histogram to be transformed ´h0´ (with n bins)
	with the target histogram ´ht´ (with m bins)

   returns: TRUE on success, FALSE on failure (e.g. malloc)
*/
int	histogram_matching2D(
	DATATYPE **inp,	/* pixels of 2D image to be altered */
	DATATYPE **out,	/* pixels of the resultant 2D image, altered and all */
	int x, int y,	/* top left corner of region of interest */
	int w, int h, 	/* input/output image dimensions */
	int depth,	/* input/output image depth in bytes -- i.e. 8-bit is 1, 16-bit is 2, etc. */
	histogram *ht,	/* target histogram - match on this */
	int	option	/* HISTOGRAM_MATCHING_LSQ | HISTOGRAM_MATCHING_CDF | HISTOGRAM_MATCHING_EHM_ANDREAS | HISTOGRAM_MATCHING_EHM_MOROVIC
			   ORed with HISTOGRAM_COUNT_BACKGROUND_PIXEL / HISTOGRAM_DONT_COUNT_BACKGROUND_PIXEL
			*/
){
	histogram	*h0;
	int	nlev = 256*256, max, i, j;
	int	index, index2;
	double	offset, factor,
		offset2, factor2,
		*cdf1, *cdf2;
	long	*hist1, *hist2, *count;
	DATATYPE	*table;

	if( !(option & (
		HISTOGRAM_MATCHING_LSQ |
		HISTOGRAM_MATCHING_CDF |
		HISTOGRAM_MATCHING_EHM_ANDREAS |
		HISTOGRAM_MATCHING_EHM_MOROVIC |
		HISTOGRAM_MATCHING_EHM_GONZALEZ)) ){
		fprintf(stderr, "histogram_matching2D : unknown histogram matching method '%d'. Use one of the known ones -- see histogram.h for more.\n", option);
		return FALSE;
	}

	/* calculate the histogram of the input image -- the one we need to change */		
	if( (h0=histogram2D(inp, x, y, w, h, 1)) == NULL ){
		fprintf(stderr, "histogram_matching2D : call to histogram2D failed for input image of dimensions %dx%d and binsize of 1\n", w, h);
		return FALSE;
	}

	if( h0->numBins <= 1 ){
		fprintf(stderr, "histogram_matching2D : the input image is empty - will return an exact image back.\n");
		for(i=0;i<w;i++) for(j=0;j<h;j++) out[i][j] = inp[i][j];
		return TRUE;
	}
		
	/* scale the reference histogram to be of the same size as h0 */
	if( scale_histogram(h0, ht, HISTOGRAM_DONT_COUNT_BACKGROUND_PIXEL) == FALSE ){
		fprintf(stderr, "histogram_matching2D : call to scale_histogram failed for reference histogram\n");
		destroy_histogram(h0);
		return FALSE;
	}

	if( (option & HISTOGRAM_MATCHING_LSQ) ){
		fprintf(stderr, "histogram_matching2D : HISTOGRAM_MATCHING_LSQ not implemented, use HISTOGRAM_MATCHING_EHM_MOROVIC instead.\n");
		return FALSE;

		offset = (double )(h0->minPixel);
		factor = ((double )(nlev-1)) / ((double )(h0->maxPixel - h0->minPixel));

		if( (table=(DATATYPE *)malloc(nlev*sizeof(DATATYPE))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for table.\n", nlev*sizeof(DATATYPE));
			return FALSE;
		}
		if( (count=(long *)malloc(nlev*sizeof(long))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for count.\n", nlev*sizeof(long));
			free(table);
			return FALSE;
		}
		/* build look-up table */
		for(index=0;index<nlev;index++){
			table[index] = (DATATYPE )0;
			count[index] = 0L;
		}
		for(i=0;i<w;i++) for(j=0;j<h;j++){
			index = (int )((((double )(inp[i][j]))-offset)*factor);
			table[index] += /*what?*/
			count[index]++;
		}
		/* average the table with the count array */
		for(index=0;index<nlev;index++){
			if(count[index] != 0) table[index] /= (double)(count[index]);
		}

		/* create output image */
		for(i=0;i<w;i++) for(j=0;j<h;j++){
			index = (int )((((double )(inp[i][j]))-offset)*factor);
			out[i][j] = table[index];
		}
		for(i=0;i<nlev;i++){
			printf("%d -> %d\n", i, table[i]);
		}
		free(count); free(table);
	} else if( (option & HISTOGRAM_MATCHING_CDF) ){
		fprintf(stderr, "histogram_matching2D : HISTOGRAM_MATCHING_CDF not implemented, use HISTOGRAM_MATCHING_EHM_MOROVIC instead.\n");
		return FALSE;

		offset = (double )(h0->minPixel);
		factor = ((double )(nlev-1)) / ((double )(h0->maxPixel - h0->minPixel));

		if( (table=(DATATYPE *)malloc(nlev*sizeof(DATATYPE))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for table.\n", nlev*sizeof(DATATYPE));
			return FALSE;
		}
		/* allocate space for histograms of reference and transform images and cdf's */
		if( (hist1=(long *)malloc(nlev*sizeof(long))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for hist1.\n", nlev*sizeof(long));
			free(table);
			return FALSE;
		}
		if( (hist2=(long *)malloc(nlev*sizeof(long))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for hist2.\n", nlev*sizeof(long));
			free(table); free(hist1);
			return FALSE;
		}
		if( (cdf1=(double *)malloc(nlev*sizeof(double))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for cdf1.\n", nlev*sizeof(double));
			free(table); free(hist1); free(hist2);
			return FALSE;
		}
		if( (cdf2=(double *)malloc(nlev*sizeof(double))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for cdf2.\n", nlev*sizeof(double));
			free(table); free(hist1); free(hist2); free(cdf1);
			return FALSE;
		}

		/* compute cumulative distribution function for transformation image */
		hist1[0] = (h0->minPixel==0) ? h0->bins[0] : 0;
		for(i=1,max=0;i<nlev;i++){
			if( i < h0->minPixel ){ hist1[i] = 0; continue; }
			if( i > h0->maxPixel ){ hist1[i] = max; continue; }
			max = hist1[i] = hist1[i-1] + h0->bins[i-h0->minPixel];
		}
		/* scale cumulative distribution function to [0,1] */
		for(i=0;i<nlev;i++){
			cdf1[i] = ((double )(hist1[i])) / ((double )max);
		}

		/* compute cumulative distribution function for reference histogram */
		hist2[0] = (ht->minPixel==0) ? ht->bins[0] : 0;
		for(i=1,max=0;i<nlev;i++){
			if( i < ht->minPixel ){ hist2[i] = 0; continue; }
			if( i > ht->maxPixel ){ hist2[i] = max; continue; }
			max = hist2[i] = hist2[i-1] + ht->bins[i-ht->minPixel];
		}
		/* scale cumulative distribution function to [0,1] */
		for(i=0;i<nlev;i++){
			cdf2[i] = ((double )(hist2[i])) / ((double )max);
		}

		offset2 = (double )(ht->minPixel);
		factor2 = ((double )(nlev-1)) / ((double )(ht->maxPixel - ht->minPixel));

		/* loop through histograms making the lookup table entries
			table[index] = cdf2 <inverse> [cdf1[index]] */
		for(index=0;index<nlev;index++){
			for(index2=0;index2<nlev;index2++){
				if( cdf2[index2] == cdf1[index] ){
					table[index] = (DATATYPE )(((double )index2)/((double )factor2) + offset2);
					break;
				}
				else if( cdf2[index2] > cdf1[index] ){
					if( index2 == 0 ) table[index] = offset2;
					else {
						if( (cdf2[index2] - cdf1[index]) <= (cdf1[index] - cdf2[index2-1]) ){
							table[index] = (DATATYPE )(((double )index2)/((double )factor2) + offset2);
						} else {
							table[index] = (DATATYPE )(((double )(index2-1))/((double )factor2) + offset2);
						}
					}
					break;
				}
			}
			fprintf(stderr, "%d\n", index);
		}
						
		/* create output image */
		for(i=0;i<w;i++) for(j=0;j<h;j++){
			index = (int )((((double )(inp[i][j]))-offset)*factor + 0.5);
			out[i][j] = table[index];
		}

		for(i=0;i<nlev;i++){
			printf("%d -> %d\n", i, table[i]);
		}

		free(hist1); free(hist2); free(cdf1); free(cdf2); free(table);
	} else if( (option & HISTOGRAM_MATCHING_EHM_MOROVIC) ){
		int	n = h0->numBins, N = n-1,
			m = ht->numBins, M = m-1,
			**LUT, **subtotal[2],
			*CPF, /* cumulative pixel frequency */
			pixelsreq, pixelsrem,
			i, j, ii, jj, k, p, I = 1, J = 1;

		if( (CPF=(int *)malloc(m*sizeof(int))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for CPF.\n", m*sizeof(int));
			return FALSE;
		}
		/* allocate a LUT of n x m,
		   n : number of entries in the histogram of the input image, the one we need to alter,
		   m : number of entries in the histogram of the reference histogram (target)
		*/
		if( (LUT=(int **)malloc(n*sizeof(int *))) == NULL ){
			fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for LUT.\n", n*sizeof(int *));
			free(CPF); return FALSE;
		}
		for(i=0;i<n;i++){
			if( (LUT[i]=(int *)malloc(m*sizeof(int))) == NULL ){
				fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for LUT[%d].\n", m*sizeof(int), i);
				free(CPF); free(LUT); return FALSE;
			}
			/* zero */
			for(j=0;j<m;j++) LUT[i][j] = 0;
		}
		for(j=0;j<2;j++){
			if( (subtotal[j]=(int **)malloc(n*sizeof(int *))) == NULL ){
				fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for subtotal[%d].\n", n*sizeof(int *), j);
				free(CPF); free(LUT); return FALSE;
			}
			for(i=0;i<n;i++){
				if( (subtotal[j][i]=(int *)malloc(m*sizeof(int))) == NULL ){
					fprintf(stderr, "histogram_matching2D : failed to allocate %zd bytes for subtotal[%d][%d].\n", m*sizeof(int), j, i);
					free(CPF); free(LUT); return FALSE;
				}
				/* zero */
				for(ii=0;ii<m;ii++) subtotal[j][i][ii] = 0;
			}
		}

		/* fill in the LUT but ignore background pixels if asked */
//		FILE *fp = fopen("lut", "w");
		for(j=J;j<m;j++){
			for(i=I;i<n;i++){
				pixelsreq = ht->bins[j] - subtotal[0][i][j];
				pixelsrem = h0->bins[i] - subtotal[1][i][j];
				LUT[i][j] = MIN(pixelsreq, pixelsrem);
				if( i < N ) subtotal[0][i+1][j] = subtotal[0][i][j] + LUT[i][j];
				if( j < M ) subtotal[1][i][j+1] = subtotal[1][i][j] + LUT[i][j];
//				if( LUT[i][j] > 0 ) fprintf(fp, "(%d,%d)=(rm=%d,rq=%d,h0=%d,ht=%d,s1=%d,s2=%d,l=%d)\n", i, j, pixelsreq, pixelsrem, h0->bins[i], ht->bins[j], subtotal[0][i][j], subtotal[1][i][j], LUT[i][j]);
//				if( (i == 1) && (j==1) ) printf("i=1(%d,%d)=(rm=%d,rq=%d,h0=%d,ht=%d,s1=%d,s2=%d,l=%d)\n", i, j, pixelsreq, pixelsrem, h0->bins[i], ht->bins[j], subtotal[0][i][j], subtotal[1][i][j], LUT[i][j]);
//				if( j == 1 ) printf("(%d,%d)=(rm=%d,rq=%d,h0=%d,ht=%d,s1=%d,s2=%d,l=%d)\n", i, j, pixelsreq, pixelsrem, h0->bins[i], ht->bins[j], subtotal[0][i][j], subtotal[1][i][j], LUT[i][j]);
			}
		}
//		fclose(fp);

		/* do a sanity check : the sum of LUT row[i] = h0->bins[i]
		   and sum of LUT col[j] = ht->bins[j] */
		for(i=I;i<n;i++){
			for(j=0,k=0;j<m;j++) k += LUT[i][j];
			if( k != h0->bins[i] ) printf("error row %d, %d != %d\n", i, k, h0->bins[i]);
		}			
		for(j=J;j<m;j++){
			for(i=0,k=0;i<n;i++) k += LUT[i][j];
			if( k != ht->bins[j] ) printf("error col %d, %d != %d\n", j, k, ht->bins[j]);
		}			

		/* the lut[i][j]=x tells us to move x pixels from h0[i] to ht[j] */
		/* create the output image */
		for(ii=0;ii<w;ii++) for(jj=0;jj<h;jj++){
			out[ii][jj] = inp[ii][jj];

			/* find the row in the LUT (which is the entry of h0) */
			i = inp[ii][jj] - h0->minPixel;

			/* construct cumulative histogram for that row */
			for(k=1,CPF[0]=LUT[i][0];k<m;k++) CPF[k] = CPF[k-1] + LUT[i][k];
			/* this row could be empty, skip */
			if( CPF[m-1] == 0 ) continue;

			p = lrand48() % CPF[m-1];
			/* find column for which CPF[j] > p */
			for(k=0;k<m;k++) if( CPF[k] > p ) break;
			/* if this is a zero entry in LUT, move backwards in row
			   to find a positive one - we must find one since
			   CPF of this row > 0 */
			while( (k>0) && (LUT[i][k]==0) ) k--;

			/* this is it: convert pixel inp[ii][jj] to 'k' */
			out[ii][jj] = k + ht->minPixel;
			if( LUT[i][k] > 0 ) LUT[i][k]--;
		}

		/* free */
		for(i=0;i<n;i++){
			free(LUT[i]);
			free(subtotal[0][i]); free(subtotal[1][i]);
		}
		free(LUT); free(subtotal[0]); free(subtotal[1]); free(CPF);
	}		

	destroy_histogram(h0);
	return TRUE;
}
