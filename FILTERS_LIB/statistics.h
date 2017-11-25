/* 1D stats function prototypes */
double		mean1D(DATATYPE *data, int offset, int size);
double		stdev1D(DATATYPE *data, int offset, int size);
void		mean_stdev1D(DATATYPE *data, int offset, int size, double *mean, double *stdev);
DATATYPE	minPixel1D(DATATYPE *data, int offset, int size);
DATATYPE	maxPixel1D(DATATYPE *data, int offset, int size);
void		min_maxPixel1D(DATATYPE *data, int offset, int size, DATATYPE *min, DATATYPE *max);
void		statistics1D(DATATYPE *data, int offset, int size, DATATYPE *min, DATATYPE *max, double *mean, double *stdev);
double		frequency_mean1D(DATATYPE *data, int offset, int size, histogram hist);
double		frequency_stdev1D(DATATYPE *data, int offset, int size, histogram hist);
void		frequency_mean_stdev1D(DATATYPE *data, int offset, int size, histogram hist, double *mean, double *stdev);
int		frequency_min1D(DATATYPE *data, int offset, int size, histogram hist);
int		frequency_max1D(DATATYPE *data, int offset, int size, histogram hist);
void		frequency_min_max1D(DATATYPE *data, int offset, int size, histogram hist, int *, int *);
void		frequency1D(DATATYPE *data, int offset, int size, histogram hist, int *, int *, double *, double *);

/* 2D stats function prototypes */
double		mean2D(DATATYPE **data, int x, int y, int w, int h);
double		stdev2D(DATATYPE **data, int x, int y, int w, int h);
void		mean_stdev2D(DATATYPE **data, int x, int y, int w, int h, double *mean, double *stdev);
DATATYPE	minPixel2D(DATATYPE **data, int x, int y, int w, int h);
DATATYPE	maxPixel2D(DATATYPE **data, int x, int y, int w, int h);
void		min_maxPixel2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE *min, DATATYPE *max);
void		statistics2D(DATATYPE **data, int x, int y, int w, int h, DATATYPE *min, DATATYPE *max, double *mean, double *stdev);
double		frequency_mean2D(DATATYPE **data, int x, int y, int w, int h, histogram hist);
double		frequency_stdev2D(DATATYPE **data, int x, int y, int w, int h, histogram hist);
void		frequency_mean_stdev2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, double *mean, double *stdev);
int		frequency_min2D(DATATYPE **data, int x, int y, int w, int h, histogram hist);
int		frequency_max2D(DATATYPE **data, int x, int y, int w, int h, histogram hist);
void		frequency_min_max2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, int *, int *);
void		frequency2D(DATATYPE **data, int x, int y, int w, int h, histogram hist, int *, int *, double *, double *);

/* 3D stats function prototypes */
/* z and d are associated with the slice-axis */
/* if you want to get stats in a subwindow as follows
   from slice 3 and for 2 slices
   from x=100 and for 15 pixels
   from y=120 and for 25 pixels do:
   statistics3D(data, 100, 120, 3, 15, 25, 2 ... ) */
double	mean3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d);
double	stdev3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d);
void		mean_stdev3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, double *mean, double *stdev);
DATATYPE	minPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d);
DATATYPE	maxPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d);

void		min_maxPixel3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE *min, DATATYPE *max);
void		statistics3D(DATATYPE ***data, int x, int y, int z, int w, int h, int d, DATATYPE *min, DATATYPE *max, double *mean, double *stdev);


/* 1D float stats function prototypes */
float		mean1D_float(float *data, int offset, int size);
float		stdev1D_float(float *data, int offset, int size);
void		mean_stdev1D_float(float *data, int offset, int size, float *mean, float *stdev);
float		minPixel1D_float(float *data, int offset, int size);
float		maxPixel1D_float(float *data, int offset, int size);
void		min_maxPixel1D_float(float *data, int offset, int size, float *min, float *max);
void		statistics1D_float(float *data, int offset, int size, float *min, float *max, float *mean, float *stdev);
/* 1D double stats function prototypes */
double		mean1D_double(double *data, int offset, int size);
double		stdev1D_double(double *data, int offset, int size);
void		mean_stdev1D_double(double *data, int offset, int size, double *mean, double *stdev);
double		minPixel1D_double(double *data, int offset, int size);
double		maxPixel1D_double(double *data, int offset, int size);
void		min_maxPixel1D_double(double *data, int offset, int size, double *min, double *max);
void		statistics1D_double(double *data, int offset, int size, double *min, double *max, double *mean, double *stdev);


