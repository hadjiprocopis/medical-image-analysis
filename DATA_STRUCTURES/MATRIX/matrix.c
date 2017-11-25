#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "matrix.h"

static	char *delimiter_string_strtok_r (char *s, const char *delim, const int strlen_delim, char **save_ptr);

matrix	*new_matrix(
	int	nr,
	int	nc,
	char	**column_names,
	char	**row_names,
	double	**data
){
	matrix *ret;
	int	i, j;
	if( (ret=(matrix *)malloc(sizeof(matrix))) == NULL ){
		fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : could not allocate %zd bytes for matrix.\n", sizeof(matrix));
		return NULL;
	}
	ret->nc = nc; ret->nr = nr;

	if( (ret->d=callocDOUBLE2D(nr, nc)) == NULL ){
		fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : call to callocDOUBLE2D(nr=%d,nc=%d) has failed.\n", nr, nc);
		free(ret);
		return NULL;
	}
	if( column_names != NULL ){
		if( (ret->column_names=(char **)malloc(nc*sizeof(char))) == NULL ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : could not allocate %zd bytes for %d column_names.\n", nc*sizeof(char), nc);
			freeDOUBLE2D(ret->d, nr); free(ret);
			return NULL;
		}
		for(i=0;i<nc;i++){ if( column_names[i] == NULL ){ ret->column_names[i] = strdup("NA"); } else { ret->column_names[i] = strdup(column_names[i]); } }
	} else { ret->column_names = NULL; }
	if( row_names != NULL ){
		if( (ret->row_names=(char **)malloc(nr*sizeof(char))) == NULL ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : could not allocate %zd bytes for %d row_names.\n", nr*sizeof(char), nr);
			freeDOUBLE2D(ret->d, nr); free(ret);
			return NULL;
		}
		for(i=0;i<nr;i++){ if( row_names[i] == NULL ){ ret->row_names[i] = strdup("NA"); } else { ret->row_names[i] = strdup(row_names[i]); } }
	} else { ret->row_names = NULL; }
	if( data != NULL ){
		double	*pDataColumnDst, *pDataColumnSrc,
			**pDataRowDst, **pDataRowSrc;
		/* we could use memcpy but risk if non-contiguous 2d array (only when each row is a pointer to some other row somewhere else) */
		for(i=0,pDataRowSrc=&(data[0]),pDataRowDst=&(ret->d[0]);i<nr;i++,pDataRowSrc++,pDataRowDst++){
			for(j=0,pDataColumnSrc=*pDataRowSrc,pDataColumnDst=*pDataRowDst;j<nc;j++,pDataColumnSrc++,pDataColumnDst++){
				*pDataColumnDst = *pDataColumnSrc;
			}
		}
	}
	return ret;
}
void	destroy_matrix(matrix *m){
	int	i;
	freeDOUBLE2D(m->d, m->nr);
	if( m->row_names != NULL ){ for(i=0;i<m->nr;i++){ free(m->row_names[i]); } free(m->row_names); }
	if( m->column_names != NULL ){ for(i=0;i<m->nc;i++){ free(m->column_names[i]); } free(m->column_names); }
}
int	write_matrix(
	FILE	*fh,
	matrix	*m,
	char	*separator
){
	double	*pDataRow, **pDataColumn;
	int	i, j;
	if( m->column_names != NULL ){
		if( m->row_names != NULL ){ fputs("RowNames", fh);fputs(separator, fh); }
printf("FFF: '%s'\n", m->column_names[1]);
		fputs(m->column_names[0], fh);
		for(i=1;i<m->nc;i++){
			fputs(separator, fh); fputs(m->column_names[i], fh);
		}
		fputc('\n', fh);
	}
	if( m->row_names != NULL ){
		for(i=0,pDataColumn=&(m->d[0]);i<m->nr;i++,pDataColumn++){
			fputs(m->row_names[i], fh);
			for(j=0,pDataRow=*pDataColumn;j<m->nc;j++,pDataRow++){
				fprintf(fh, "%s%lf", separator, *pDataRow);
			}
			fputc('\n', fh);
		}
	} else {
		for(i=0,pDataColumn=&(m->d[0]);i<m->nr;i++,pDataColumn++){
			pDataRow=*pDataColumn;
			fprintf(fh, "%lf", *pDataRow); pDataRow++;
			for(j=1;j<m->nc;j++,pDataRow++){
				fprintf(fh, "%s%lf", separator, *pDataRow);
			}
			fputc('\n', fh);
		}
	}
	return TRUE;	
}

int	read_matrix(
	FILE	*fh,
	int	*_nr,
	int	*_nc,
	int	have_column_names, /* true if have 1 line of header = column names */
	int	have_row_names, /* true if have 1 extra column at the beggining = row names */
	matrix	**m,
	char	*separator
){
	char	a_line[10000], *a_token, *savePtr = NULL;
	double	**pDataRow, *pDataColumn;
	int	i, l, nc, nr;
	int	col_num = 0, separator_length = strlen(separator);

	if( *_nr <= 0 ){ /* we do not know how many rows (assume 1 row per line!!!) */
		nr = 0;
		while( fgets(a_line, 10000, fh) && !feof(fh) ){ nr++; }
		rewind(fh);
		if( have_column_names ){ nr--; } /* does not include the header */
		*_nr = nr;
	} else { nr = *_nr; }
	printf("ret->nr=%d\n",  nr);
	if( (nr == 0) || (have_row_names&&(nr<=1)) ){
		fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : there are no rows of data in input file.\n");
		return FALSE;
	}
	if( *_nc <= 0 ){ /* we do not know how many columns */
		fgets(a_line, 10000, fh);
		for(a_token=delimiter_string_strtok_r(a_line, separator, separator_length, &savePtr),nc=0;a_token;a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),nc++){ printf("token: %s\n", a_token); }
		if( have_row_names ){ nc--; }
		*_nc = nc;
		rewind(fh);
	} else { nc = *_nc; }

	matrix *ret;
	if( *m != NULL ){
		ret = *m;
		if( nc != ret->nc ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : supplied matrix has different number of columns (%d) from file (%d).\n", ret->nc, *_nc);
			return FALSE;
		}
		if( nr != ret->nr ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : supplied matrix has different number of rows (%d) from file (%d).\n", ret->nr, *_nr);
			return FALSE;
		}
	} else {
		if( (ret=new_matrix(nr, nc, NULL, NULL, NULL)) == NULL ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : call to new_matrix has failed for nr=%d, nc=%d.\n", nr, nc);
			return FALSE;
		}
		*m = ret;
	}

	/* get the column names */
	if( have_column_names ){
		if( (ret->column_names=(char **)malloc(nc*sizeof(char *))) == NULL ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : could not allocate %zd bytes for %d column_names.\n", nc*sizeof(char *), nc);
			destroy_matrix(ret);
			return FALSE;
		}
		/* only 1 line for column names */
		fgets(a_line, 10000, fh); if( a_line[l=strlen(a_line)-1]=='\n' ) a_line[l] = '\0';
		a_token=delimiter_string_strtok_r(a_line, separator, separator_length, &savePtr); /* ignore the first colname if rownames exist, because it is the name of the rownames column, which we use our own when we write and we have no use for it during processing */
		for(a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),col_num=0;a_token&&(col_num<=nc);a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),col_num++){
			if( *a_token == '\0' ){
				ret->column_names[col_num] = strdup("NA");
			} else {
				ret->column_names[col_num] = strdup(a_token);
			}
		}
	}
	if( have_row_names ){
		if( (ret->row_names=(char **)malloc(nr*sizeof(char))) == NULL ){
			fprintf(stderr, "DATA_STRUCTURES/MATRIX/matrix.c : could not allocate %zd bytes for %d row_names.\n", nr*sizeof(char), nr);
			destroy_matrix(ret);
			return FALSE;
		}
		for(i=0,pDataRow=&(ret->d[0]);i<nr;i++,pDataRow++){
			fgets(a_line, 10000, fh); if( a_line[l=strlen(a_line)-1]=='\n' ) a_line[l] = '\0';
			a_token = delimiter_string_strtok_r(a_line, separator, separator_length, &savePtr);
			ret->row_names[i] = strdup(a_token);
			for(a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),col_num=0,pDataColumn=*pDataRow;a_token&&(col_num<=nc);a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),col_num++,pDataColumn++){
				*pDataColumn = atof(a_token);
			}
		}
	} else {
		for(i=0,pDataRow=&(ret->d[0]);i<nr;i++,pDataRow++){
			fgets(a_line, 10000, fh); if( a_line[l=strlen(a_line)-1]=='\n' ) a_line[l] = '\0';
			for(a_token=delimiter_string_strtok_r(a_line, separator, separator_length, &savePtr),col_num=0,pDataColumn=*pDataRow;a_token&&(col_num<=nc);a_token=delimiter_string_strtok_r(NULL, separator, separator_length, &savePtr),col_num++,pDataColumn++){
				*pDataColumn = atof(a_token);
			}
		}
	}
	printf("read_data_from_ASCII_file : read %d rows each of %d items (columns).\n", nr, nc);
	return TRUE;
}	
/* our own strtok in order to have string separators (not just a string of char separators as is the default) */
/* my version of strtok to work with a string delimiter not a string of delimiter characters matching any one of them,
   the difference between gnulibc's is the use of strpbrk
*/
static	char *delimiter_string_strtok_r (char *s, const char *delim, const int strlen_delim, char **save_ptr){
	char *token; 
		
	if( s == NULL ){
		/* re-entry */
		s = *save_ptr;
	}
//      printf("entering s='%s'\n", s);
	if( (s==NULL) || (*s == '\0') ) return NULL;

	token = s;
	s = strstr(s, delim); /* returns the begginning of where it found delim, so we need to advance by strlen(delim)-1, see below */

	if( s == NULL ){
		/* This token finishes the string - can't find any delimiter in there */
		*save_ptr = strchr(token, '\0');
	} else {
		*s = '\0';
		s+=strlen_delim-1;
		*s = '\0';
		*save_ptr = s + 1;
	}
	return token;
}
