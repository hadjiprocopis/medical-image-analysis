#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"

#include "filters.h"
#include "normalise.h"

/*
	**data : the array of floats to be normalised indexed as data[col_index][row_index]

	numCols, numRows: the dimensions of **data ([numCols][numRows])
	mode: one of NORMALISE_ROW_WISE, NORMALISE_COL_WISE, NORMALISE_ALL
		will normalise each row separately, or each column separately or all the data at once.
	*excludeCols: an array of size num_excludeCols. it contains column indices (0 to numCols-1)
	which should not participate in the normalisation.
	*excludedRows: as above
	
	outputRangeLow, outputRangeHigh: the output range of normalised data
	
	returns a 2D array of floats containing the normalised data. the size of this array
	is the same as that of **data (and is also indexed the same way). if any rows/cols were
	excluded, the output data will contain these numbers unchanged.

	*min & *max are pointers (to arrays) to hold the min and max found.
	if they are null then this feature will be ignored.
	if mode == NORMALISE_ROW_WISE then min and max should be of length = num ROWS
	if mode == NORMALISE_COL_WISE then min and max should be of length = num COLS
	otherwise, if mode is over the whole data, min and max need not be arrays but
	just pointers to int, to return the single min and max values.
	
	you should not forget to free the returned data after done with it.
*/
double	**normalise2D_double(double **data, int numCols, int numRows, int mode, int *excludeCols, int num_excludeCols, int *excludeRows, int num_excludeRows, int outputRangeLow, int outputRangeHigh, double *_min, double *_max){
	double		**ret;
	char		excluded;
	int		j, numColsIndex, numRowsIndex, *cols_index, *rows_index;
	register int	i, col, row;
	register double	A = outputRangeHigh-outputRangeLow, B, C, min, max;

	if( (mode!=NORMALISE_ALL) && (mode!=NORMALISE_ROW_WISE) && (mode!=NORMALISE_COL_WISE) ){
		fprintf(stderr, "normalise2D_double : illegal mode %d (see normalise.h for legal modes.\n", mode);
		return NULL;
	}

	if( (numColsIndex=(numCols-num_excludeCols)) <= 0 ){
		fprintf(stderr, "normalise2D_double : all columns excluded!\n");
		return NULL;
	}
	if( (numRowsIndex=(numRows-num_excludeRows)) <= 0 ){
		fprintf(stderr, "normalise2D_double : all rows excluded!\n");
		return NULL;
	}
	if( (cols_index=(int *)malloc(numColsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_double : could not allocate %zd bytes for cols_index.\n", numColsIndex*sizeof(int));
		return NULL;
	}
	for(col=0,j=0;col<numCols;col++){
		excluded = FALSE;
		for(i=0;i<num_excludeCols;i++) if( col == excludeCols[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		cols_index[j++] = col;
	}
		
	if( (rows_index=(int *)malloc(numRowsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_double : could not allocate %zd bytes for rows_index.\n", numRowsIndex*sizeof(int));
		free(cols_index);
		return NULL;
	}
	for(row=0,j=0;row<numRows;row++){
		excluded = FALSE;
		for(i=0;i<num_excludeRows;i++) if( row == excludeRows[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		rows_index[j++] = row;
	}
	
	if( (ret=(double **)malloc(numCols*sizeof(double *))) == NULL ){
		fprintf(stderr, "normalise2D_double : could not allocate %zd bytes for ret.\n", numCols*sizeof(double *));
		free(cols_index); free(rows_index);
		return NULL;
	}
	for(i=0;i<numCols;i++)
		if( (ret[i]=(double *)malloc(numRows*sizeof(double))) == NULL ){
			fprintf(stderr, "normalise2D_double : could not allocate %zd bytes for ret[%d].\n", numRows*sizeof(double), i);
			free(cols_index); free(rows_index);
			return NULL;
		}
		
	for(row=0;row<num_excludeRows;row++) for(col=0;col<numCols;col++) ret[col][excludeRows[row]] = data[col][excludeRows[row]];
	for(col=0;col<num_excludeCols;col++) for(row=0;row<numRows;row++) ret[excludeCols[col]][row] = data[excludeCols[col]][row];

	if( mode == NORMALISE_ROW_WISE ){
		for(row=0;row<numRowsIndex;row++){
			min = max = data[cols_index[0]][rows_index[row]];
			for(col=0;col<numColsIndex;col++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(col=0;col<numColsIndex;col++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(col=0;col<numColsIndex;col++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[row] = min;
			if( _max != NULL ) _max[row] = max;
		}
	} else if( mode == NORMALISE_COL_WISE ){
		for(col=0;col<numColsIndex;col++){
			min = max = data[cols_index[col]][rows_index[0]];
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(row=0;row<numRowsIndex;row++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[col] = min;
			if( _max != NULL ) _max[col] = max;
		}
	} else {
		min = max = data[cols_index[0]][rows_index[0]];
		for(col=0;col<numColsIndex;col++){
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}
		}

		if( min == max ) /* no variation, set all to min */
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
		else {
			B = max - min;
			C = A / B;
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++)
				ret[cols_index[col]][rows_index[row]] =
					C * (data[cols_index[col]][rows_index[row]] - min)
					+ outputRangeLow;
		}
		if( _min != NULL ) *_min = min;
		if( _max != NULL ) *_max = max;
	}

	free(cols_index); free(rows_index);
	return ret;
}
float	**normalise2D_float(float **data, int numCols, int numRows, int mode, int *excludeCols, int num_excludeCols, int *excludeRows, int num_excludeRows, int outputRangeLow, int outputRangeHigh, float *_min, float *_max){
	float		**ret;
	char		excluded;
	int		j, numColsIndex, numRowsIndex, *cols_index, *rows_index;
	register int	i, col, row;
	register float	A = outputRangeHigh-outputRangeLow, B, C, min, max;

	if( (mode!=NORMALISE_ALL) && (mode!=NORMALISE_ROW_WISE) && (mode!=NORMALISE_COL_WISE) ){
		fprintf(stderr, "normalise2D_float : illegal mode %d (see normalise.h for legal modes.\n", mode);
		return NULL;
	}

	if( (numColsIndex=(numCols-num_excludeCols)) <= 0 ){
		fprintf(stderr, "normalise2D_float : all columns excluded!\n");
		return NULL;
	}
	if( (numRowsIndex=(numRows-num_excludeRows)) <= 0 ){
		fprintf(stderr, "normalise2D_float : all rows excluded!\n");
		return NULL;
	}
	if( (cols_index=(int *)malloc(numColsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_float : could not allocate %zd bytes for cols_index.\n", numColsIndex*sizeof(int));
		return NULL;
	}
	for(col=0,j=0;col<numCols;col++){
		excluded = FALSE;
		for(i=0;i<num_excludeCols;i++) if( col == excludeCols[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		cols_index[j++] = col;
	}
		
	if( (rows_index=(int *)malloc(numRowsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_float : could not allocate %zd bytes for rows_index.\n", numRowsIndex*sizeof(int));
		free(cols_index);
		return NULL;
	}
	for(row=0,j=0;row<numRows;row++){
		excluded = FALSE;
		for(i=0;i<num_excludeRows;i++) if( row == excludeRows[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		rows_index[j++] = row;
	}
	
	if( (ret=(float **)malloc(numCols*sizeof(float *))) == NULL ){
		fprintf(stderr, "normalise2D_float : could not allocate %zd bytes for ret.\n", numCols*sizeof(float *));
		free(cols_index); free(rows_index);
		return NULL;
	}
	for(i=0;i<numCols;i++)
		if( (ret[i]=(float *)malloc(numRows*sizeof(float))) == NULL ){
			fprintf(stderr, "normalise2D_float : could not allocate %zd bytes for ret[%d].\n", numRows*sizeof(float), i);
			free(cols_index); free(rows_index);
			return NULL;
		}
		
	for(row=0;row<num_excludeRows;row++) for(col=0;col<numCols;col++) ret[col][excludeRows[row]] = data[col][excludeRows[row]];
	for(col=0;col<num_excludeCols;col++) for(row=0;row<numRows;row++) ret[excludeCols[col]][row] = data[excludeCols[col]][row];

	if( mode == NORMALISE_ROW_WISE ){
		for(row=0;row<numRowsIndex;row++){
			min = max = data[cols_index[0]][rows_index[row]];
			for(col=0;col<numColsIndex;col++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(col=0;col<numColsIndex;col++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(col=0;col<numColsIndex;col++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[row] = min;
			if( _max != NULL ) _max[row] = max;
		}
	} else if( mode == NORMALISE_COL_WISE ){
		for(col=0;col<numColsIndex;col++){
			min = max = data[cols_index[col]][rows_index[0]];
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(row=0;row<numRowsIndex;row++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[col] = min;
			if( _max != NULL ) _max[col] = max;
		}
	} else {
		min = max = data[cols_index[0]][rows_index[0]];
		for(col=0;col<numColsIndex;col++){
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}
		}

		if( min == max ) /* no variation, set all to min */
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
		else {
			B = max - min;
			C = A / B;
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++)
				ret[cols_index[col]][rows_index[row]] =
					C * (data[cols_index[col]][rows_index[row]] - min)
					+ outputRangeLow;
		}
		if( _min != NULL ) *_min = min;
		if( _max != NULL ) *_max = max;
	}

	free(cols_index); free(rows_index);
	return ret;
}
int	**normalise2D_int(int **data, int numCols, int numRows, int mode, int *excludeCols, int num_excludeCols, int *excludeRows, int num_excludeRows, int outputRangeLow, int outputRangeHigh, int *_min, int *_max){
	int		**ret;
	char		excluded;
	int		j, numColsIndex, numRowsIndex, *cols_index, *rows_index;
	register int	i, col, row;
	register int	A = outputRangeHigh-outputRangeLow, B, C, min, max;

	if( (mode!=NORMALISE_ALL) && (mode!=NORMALISE_ROW_WISE) && (mode!=NORMALISE_COL_WISE) ){
		fprintf(stderr, "normalise2D_int : illegal mode %d (see normalise.h for legal modes.\n", mode);
		return NULL;
	}

	if( (numColsIndex=(numCols-num_excludeCols)) <= 0 ){
		fprintf(stderr, "normalise2D_int : all columns excluded!\n");
		return NULL;
	}
	if( (numRowsIndex=(numRows-num_excludeRows)) <= 0 ){
		fprintf(stderr, "normalise2D_int : all rows excluded!\n");
		return NULL;
	}
	if( (cols_index=(int *)malloc(numColsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_int : could not allocate %zd bytes for cols_index.\n", numColsIndex*sizeof(int));
		return NULL;
	}
	for(col=0,j=0;col<numCols;col++){
		excluded = FALSE;
		for(i=0;i<num_excludeCols;i++) if( col == excludeCols[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		cols_index[j++] = col;
	}
		
	if( (rows_index=(int *)malloc(numRowsIndex*sizeof(int))) == NULL ){
		fprintf(stderr, "normalise2D_int : could not allocate %zd bytes for rows_index.\n", numRowsIndex*sizeof(int));
		free(cols_index);
		return NULL;
	}
	for(row=0,j=0;row<numRows;row++){
		excluded = FALSE;
		for(i=0;i<num_excludeRows;i++) if( row == excludeRows[i] ){ excluded = TRUE; break; }
		if( excluded ) continue;
		rows_index[j++] = row;
	}
	
	if( (ret=(int **)malloc(numCols*sizeof(int *))) == NULL ){
		fprintf(stderr, "normalise2D_int : could not allocate %zd bytes for ret.\n", numCols*sizeof(int *));
		free(cols_index); free(rows_index);
		return NULL;
	}
	for(i=0;i<numCols;i++)
		if( (ret[i]=(int *)malloc(numRows*sizeof(int))) == NULL ){
			fprintf(stderr, "normalise2D_int : could not allocate %zd bytes for ret[%d].\n", numRows*sizeof(int), i);
			free(cols_index); free(rows_index);
			return NULL;
		}
		
	for(row=0;row<num_excludeRows;row++) for(col=0;col<numCols;col++) ret[col][excludeRows[row]] = data[col][excludeRows[row]];
	for(col=0;col<num_excludeCols;col++) for(row=0;row<numRows;row++) ret[excludeCols[col]][row] = data[excludeCols[col]][row];

	if( mode == NORMALISE_ROW_WISE ){
		for(row=0;row<numRowsIndex;row++){
			min = max = data[cols_index[0]][rows_index[row]];
			for(col=0;col<numColsIndex;col++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(col=0;col<numColsIndex;col++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(col=0;col<numColsIndex;col++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[row] = min;
			if( _max != NULL ) _max[row] = max;
		}
	} else if( mode == NORMALISE_COL_WISE ){
		for(col=0;col<numColsIndex;col++){
			min = max = data[cols_index[col]][rows_index[0]];
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}

			if( min == max ) /* no variation, set all to min */
				for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
			else {
				B = max - min;
				C = A / B;
				for(row=0;row<numRowsIndex;row++)
					ret[cols_index[col]][rows_index[row]] =
						C * (data[cols_index[col]][rows_index[row]] - min)
						+ outputRangeLow;
			}
			if( _min != NULL ) _min[col] = min;
			if( _max != NULL ) _max[col] = max;
		}
	} else {
		min = max = data[cols_index[0]][rows_index[0]];
		for(col=0;col<numColsIndex;col++){
			for(row=0;row<numRowsIndex;row++){
				if( data[cols_index[col]][rows_index[row]] < min ) min = data[cols_index[col]][rows_index[row]];
				if( data[cols_index[col]][rows_index[row]] > max ) max = data[cols_index[col]][rows_index[row]];
			}
		}

		if( min == max ) /* no variation, set all to min */
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++) ret[cols_index[col]][rows_index[row]] = outputRangeLow;
		else {
			B = max - min;
			C = A / B;
			for(col=0;col<numColsIndex;col++) for(row=0;row<numRowsIndex;row++)
				ret[cols_index[col]][rows_index[row]] =
					C * (data[cols_index[col]][rows_index[row]] - min)
					+ outputRangeLow;
		}
		if( _min != NULL ) *_min = min;
		if( _max != NULL ) *_max = max;
	}

	free(cols_index); free(rows_index);
	return ret;
}

