#define	NORMALISE_ROW_WISE	1
#define	NORMALISE_COL_WISE	2
#define	NORMALISE_ALL		4

float	**normalise2D_float(float **/*data*/, int /*numCols*/, int /*numRows*/, int /*mode*/, int */*excludeCols*/, int /*num_excludeCols*/, int */*excludeRows*/, int /*num_excludeRows*/, int /*outputRangeLow*/, int /*outputRangeHigh*/, float */*min*/, float */*max*/);
double	**normalise2D_double(double **/*data*/, int /*numCols*/, int /*numRows*/, int /*mode*/, int */*excludeCols*/, int /*num_excludeCols*/, int */*excludeRows*/, int /*num_excludeRows*/, int /*outputRangeLow*/, int /*outputRangeHigh*/, double */*min*/, double */*max*/);
int	**normalise2D_int(int **/*data*/, int /*numCols*/, int /*numRows*/, int /*mode*/, int */*excludeCols*/, int /*num_excludeCols*/, int */*excludeRows*/, int /*num_excludeRows*/, int /*outputRangeLow*/, int /*outputRangeHigh*/, int */*min*/, int */*max*/);
