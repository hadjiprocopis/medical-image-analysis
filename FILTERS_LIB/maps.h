#ifndef	_MAPS_VISIT

/* different operation types a map can do:
   it can compare a single pixel's property to the whole image property's or...
   a small window, is a window centered around each pixel.
   a large window is a window centered around the small window (bigger)
*/
typedef	enum {	SINGLE_PIXEL=0, /* usually just display the property of each pixel */
		SMALL_WINDOW=1, /* usually just display some overall property of a sub window */
		COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE=2, /* subtract single pixel property to that of whole image */
		COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW=3,/* subtract single pixel property to that of a sub-window */
		COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE=4, /* subtract overall property of sub-window to that of whole image */
		COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW=5/* subtract overall property of sub-window to that of a larger sub-window, both centered around pixel */
} mapOperation;

/* dataOut can be left NULL if you do not want to know about the values of the comparisons,
   but just changed/unchanged in the changeMap. In this case ignore minOutputColor and maxOutputColor */
int	map_mean2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMean);
int	map_stdev2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageStdev);
int	map_minPixel2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMinPixel);
int	map_maxPixel2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, double wholeImageMaxPixel);

int	map_frequency_mean2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist);
int	map_frequency_stdev2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist);
int	map_frequency_min2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist);
int	map_frequency_max2D(DATATYPE **/*data*/, char **/*changeMap*/, DATATYPE **/*dataOut*/, DATATYPE minOutputColor, DATATYPE maxOutputColor, int x, int y, int w, int h, int sw, int sh, int lw, int lh, double lo, double hi, logicalOperation logop, mapOperation mapop, histogram *hist);

#define	_MAPS_VISIT
#endif
