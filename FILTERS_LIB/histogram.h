#ifndef _HISTOGRAM_HEADER

#define	HISTOGRAM_SORT_BINS		1
#define	HISTOGRAM_SORT_SMOOTH		2
#define	HISTOGRAM_SORT_NORMALISED	4
#define	HISTOGRAM_SORT_ASCENDING	8
#define	HISTOGRAM_SORT_DESCENDING	16

/* do histogram matching using the CDF method */
#define	HISTOGRAM_MATCHING_CDF		32

/* do histogram matching using the LSQ method */
#define	HISTOGRAM_MATCHING_LSQ		64

/* do histogram matching using ANDREAS own method (this is exact histogram matching) */
#define	HISTOGRAM_MATCHING_EHM_ANDREAS	128

/* do histogram matching using GONZALEZ method (this is exact histogram matching) */
#define	HISTOGRAM_MATCHING_EHM_GONZALEZ	256

/* do histogram matching using MOROVIC method (this is exact histogram matching) */
#define	HISTOGRAM_MATCHING_EHM_MOROVIC	512

/* flag to ignore the background pixel (first pixel in the bin) */
#define	HISTOGRAM_COUNT_BACKGROUND_PIXEL	1024
#define	HISTOGRAM_DONT_COUNT_BACKGROUND_PIXEL	2048

/* below are a lot of vars starting with 'smooth_', a histogram has its bins but also
   if we choose to smooth it it will have a smoothed bins data and also if we normalised it.
   It is likely that the smooth data will have different properties from the bins data
   (e.g. different min,max,percentiles etc.) so we have 2 copies for each of this properties.
   Also remember that if you want to find the smooth value at pixel value 10
   you should do:
	smooth freq value = h->smooth[10 - h->minPixel] (a float)
   for just frequency value (not smooth/normalised):
	bins freq value = h->bins[10 - h->minPixel] (an integer)
   and for normalised freq do:
			= h->normalised[10 - h->minPixel] (a float)
   if you want to find the 25th percentile smoothed:
	p25 smooth value = h->smooth[p->smooth_p25 - p->minPixel]
*/

/* structure to hold a histogram */
struct _HISTOGRAM {
	int		*bins,	 /* array whose ith element holds the frequency of occurence of
				  ***** be careful here ***** >>>> the (i+minPixel) color
			          and NOT color i.

			          If you want to find the frequency of occurence of pixel value
			          X, do bins[X-h.minPixel] */
			binSize,
			*cumulative_frequency, /* like bins but each item contains the sum of all the contents of the previous bins */
			numBins; /* if binSize == 1, this is equal to the unique number of colors in the image */
	DATATYPE	minPixel, maxPixel, /* the minimum and maximum pixel intensities in the image */
			pixel_p25, pixel_p50, pixel_p75,	/* pixel percentiles: line all pixels in a line ascending to pixel value and find 1/4, 1/2 and 3/4 along that line */
			pixel_minFrequency, pixel_maxFrequency,	/* pixel intensities for the min and max frequencies */
			pixel_smooth_minFrequency, pixel_smooth_maxFrequency,
			pixel_normalised_minFrequency, pixel_normalised_maxFrequency;
	int		minFrequency, maxFrequency;	/* stats about the contents of the bins array which represents frequency of occurence naturally */
	float		meanFrequency, stdevFrequency,	/* mean and stdev of the bins array */
			smooth_meanFrequency, smooth_stdevFrequency, /* for the smooth array (if any) */
			smooth_minFrequency, smooth_maxFrequency,
			normalised_meanFrequency, normalised_stdevFrequency, /* for the normalised array (if any) */
			normalised_minFrequency, normalised_maxFrequency,
			histogram_mean;		/* the mean pixel value */

	/* pixel values for median etc. */
	DATATYPE	peak,		/* value of the most frequent (see maxFrequency) pixel */
			smooth_peak,	/* value of the most frequent pixel after smoothing occured - it might differ from peak (yes?), above */
			normalised_peak,
			minActivePixel, maxActivePixel,	/* the min and max pixels that really occur (e.g. frequency > 0) */
			smooth_minActivePixel, smooth_maxActivePixel,
			normalised_minActivePixel, normalised_maxActivePixel,

			p25, p50, p75, /* 25th, 50th and 75th percentiles - p50 is also known as MEDIAN */
			/* same as the warning above, p25 holds a pixel value which ranges from minPixel
			   to maxPixel, if you want to find the frequency for this pixel value, do
			   h->bins[p25-h->minPixel],
			   also, these values are calculated on *non-zero frequency pixels ONLY*
			   */
			normalised_p25, normalised_p50, normalised_p75,
			smooth_p25, smooth_p50, smooth_p75; /* same as above but guided by the smooth freq values (if calculated) rather than the bins */

	/* smoothed and normalised versions of the histogram with smoothWindowSize specified here */
	float		*smooth,
			*normalised;
	char		is_smooth, is_normalised;	/* flags to indicate whether it has been smooth/normalised this histogram (TRUE,FALSE)*/
	int		smooth_window_size;
};

/* construct histograms from 1D, 2D and 3D data */
histogram	*histogram1D(DATATYPE *data, int offset, int size, int binSize);
histogram	*histogram2D(DATATYPE **data, int x, int y, int w, int h, int binSize);
histogram	*histogram3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, int binSize);

void		destroy_histogram(histogram */*a_histogram*/);
DATATYPE	*sort_histogram(histogram *, int /*sort_type (OR combination of the above defines)*/);
int		histogram_smooth(histogram *, int /*smooth_window_size*/, char /*use_normalised_values*/);
int		histogram_normalise(histogram *, char /*use_smooth_values*/);
int		calculate_histogram(histogram *);
void		print_histogram(FILE *fp, histogram *);


int		histogram_matching2D(
			DATATYPE **/*inp*/, /* pixels of 2D image to be altered */
			DATATYPE **/*out*/, /* the resultant pixels here */
			int /*x*/, int /*y*/,   /* top left corner of region of interest */
			int /*w*/, int /*h*/,   /* input image dimensions */
			int /*depth*/,      /* input image depth -- i.e. 8-bit, 16-bit, etc. */
			histogram */*ht*/,   /* target histogram - match on this */
			int /*option*/ 	/* HISTOGRAM_MATCHING_LSQ | HISTOGRAM_MATCHING_CDF */
);

int		scale_histogram(histogram */*metro*/, histogram */*tobescaled*/, int /*option*/);

#define	_HISTOGRAM_HEADER
#endif

