#ifndef _IO_MATRIX_H
#define	_IO_MATRIX_H

struct	_MATRIX {
	int	nr,	/* the number of rows */
		nc;	/* the number of columns */
	char	**column_names,	/* the column names if any */
		**row_names;	/* the row names if any */
	double	**d;	/* the actual data accessed as [row][col] */
};

typedef	struct _MATRIX	matrix;

matrix	*new_matrix(int nr, int nc, char **column_names, char **row_names, double **d);
void	destroy_matrix(matrix *m);
int	write_matrix(FILE *fh, matrix *m, char *separator);
int	read_matrix(FILE *fh, int *nr, int *nc, int have_column_names, int have_row_names, matrix **m, char *separator);

#endif
