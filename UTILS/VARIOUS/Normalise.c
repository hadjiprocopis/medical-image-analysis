#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\
";

const	char	Usage[] = "options as follows:\
\t -i inputFilename\
	(ascii data file arranged in rows and cols, separated by white space)\
\
\t -o outputFilename\
	(Output filename)\
\
\t[-r R\
	(declare that row R - rows start from 0 - should not be normalised.\
	 In order to declare more than one excluded rows, use this option\
	 as many times.)]\
\t[-c C\
	(declare that column C - columns start from 0 - should not be normalised.\
	 In order to declare more than one excluded columns, use this option\
	 as many times.)]\
\
\t[-R\
	(do normalisation row-wise, e.g. normalise each row - except the excluded\
	 ones - independently.)]\
\t[-C\
	(do normalisation column-wise, e.g. normalise each column - except the excluded\
	 ones - independently.)]\
\
\t[-n l:h\
	(the output range of the normalised data should be between 'l' inclusive and\
	 'h' exclusive.)]\
\
\
** Use this options to select a region of interest\
   You may use one or more or all of '-w', '-h', '-x' and\
   '-y' once. You may use one or more '-s' options in\
   order to specify more slices. Slice numbers start from 1.\
   These parameters are optional, if not present then the\
   whole image, all slices will be used.\
\
\t[-w widthOfInterest]\
\t[-h heightOfInterest]\
\t[-x xCoordOfInterest]\
\t[-y yCoordOfInterest]\
\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* how many characters per line ? assume 10 characters per column */
#define	MAX_LINE_WIDTH	100000

int	main(int argc, char **argv){
	FILE		*handle;
	double		**data, **normalised_data, a_number,
			outputRangeLow = 0.0, outputRangeHigh = 1.0;
	char		*inputFilename = NULL, *outputFilename = NULL,
			a_line[MAX_LINE_WIDTH], *p;
	int		optI, excludedRows[1000], num_excludedRows = 0,
			excludedCols[1000], num_excludedCols = 0,
			numRows = 0, numCols = 0, mode = NORMALISE_ALL,
			old_numCols = 0, i, j;

	while( (optI=getopt(argc, argv, "i:o:es:c:r:RCn:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;

			case 's': sscanf(optarg, "%d:%d", &numRows, &numCols); break;
			case 'r': excludedRows[num_excludedRows++] = atoi(optarg); break;
			case 'c': excludedCols[num_excludedCols++] = atoi(optarg); break;

			case 'R': mode = NORMALISE_ROW_WISE; break;
			case 'C': mode = NORMALISE_COL_WISE; break;

			case 'n': sscanf(optarg, "%lf:%lf", &outputRangeLow, &outputRangeHigh); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}

	if( (handle=fopen(inputFilename, "r")) == NULL ){
		fprintf(stderr, "%s : could not open input file '%s' for reading.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	if( (numRows==0) || (numCols==0) ){
		/* find the number of rows and columns based on number of newlines etc. */
		while( 1 ){
			if( feof(handle) ) break;
			if( feof(handle) || (fgets(a_line, MAX_LINE_WIDTH, handle) == NULL) ) break; /* EOF  or something wrong, assume EOF */
			/* comments */
			for(i=0;i<strlen(a_line);i++) if( a_line[i] == '#' ){ a_line[i] = '\0'; break; }
			numCols = 0;
			p = &(a_line[0]);
			while( (p!=NULL) && sscanf(p, "%lf", &a_number) ){
				numCols++;
				while( (p != NULL) && ((*p==' ')||(*p=='\t')) ) p++;
				while( (p != NULL) && !((*p==' ')||(*p=='\t')||(*p=='\n')) ) p++;
				if( *p =='\n' ) break;
			}
			if( old_numCols == 0 ) old_numCols = numCols;
			else if( old_numCols != numCols ){
				fprintf(stderr, "%s : the number of columns in line %d of file %s differs from the number of columns of previous lines (%d != %d).\n", argv[0], numRows+1, inputFilename, old_numCols, numCols);
			}
				
			numRows++;
		}
		rewind(handle);
		fprintf(stderr, "%s, %s : found %d rows and %d columns.\n", argv[0], inputFilename, numRows, numCols);
	}

	for(i=0;i<num_excludedCols;i++)
		if( (excludedCols[i] >= numCols) || (excludedCols[i] < 0) ){
			fprintf(stderr, "%s, %s : excluded column numbers must be in the range 0 (inclusive) to %d (exclusive) - rows and column specs start from 0.\n", argv[0], inputFilename, numCols);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
	for(i=0;i<num_excludedRows;i++)
		if( (excludedRows[i] >= numCols) || (excludedRows[i] < 0) ){
			fprintf(stderr, "%s, %s : excluded row numbers must be in the range 0 (inclusive) to %d (exclusive) - rows and column specs start from 0.\n", argv[0], inputFilename, numCols);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
	
	if( (data=(double **)malloc(numCols*sizeof(double *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for data.\n", argv[0], numCols*sizeof(double *));
		fclose(handle);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	for(i=0;i<numCols;i++)
		if( (data[i]=(double *)malloc(numRows*sizeof(double))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for data[%d].\n", argv[0], numCols*sizeof(double *), i);
			fclose(handle);
			free(inputFilename); free(outputFilename);
			exit(1);
		}

	for(j=0;j<numRows;j++) for(i=0;i<numCols;i++) fscanf(handle, "%lf", &(data[i][j]));
	fclose(handle);

	if( (normalised_data=normalise2D_double(
		data, numCols, numRows, mode,
		excludedCols, num_excludedCols,
		excludedRows, num_excludedRows,
		outputRangeLow, outputRangeHigh, NULL, NULL)) == NULL ){
		fprintf(stderr, "%s : call to normalise2D_double has failed for data read from %s (size of %d x %d).\n", argv[0], inputFilename, numCols, numRows);
		free(inputFilename); free(outputFilename);
		for(i=0;i<numCols;i++) free(data[i]); free(data);
		exit(1);
	}

	if( (handle=fopen(outputFilename, "w")) == NULL ){
		fprintf(stderr, "%s : could not open output file '%s' for writing.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		for(i=0;i<numCols;i++) free(data[i]); free(data);
		for(i=0;i<numCols;i++) free(normalised_data[i]); free(normalised_data);
		exit(1);
	}
	for(i=0;i<numRows;i++){
		for(j=0;j<numCols-1;j++) fprintf(handle, "%f ", normalised_data[j][i]);
		fprintf(handle, "%f\n", normalised_data[j][i]);
	}
	fclose(handle);

	
	free(inputFilename); free(outputFilename);
	for(i=0;i<numCols;i++) free(data[i]); free(data);
	for(i=0;i<numCols;i++) free(normalised_data[i]); free(normalised_data);
	exit(0);
}

