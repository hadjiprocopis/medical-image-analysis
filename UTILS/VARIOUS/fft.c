#include <stdio.h>
#include <stdlib.h>
/* this program works with fftw v2 not 3 */
#include <fftw3.h>

#include <unistd.h> /* for getopt */

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
	-i input.unc -o output.unc\
";

const	char	Usage[] = "options as follows:\
\t -i inputFilename\
	(text file with numbers representing the real,imag or just real numbers in time domain)\
\
\t -o outputFilenaem\
	(Output filename)\
\
\t[-c	(flag to indicate that the input data is\
	a set of (REAL, IMAGINARY) pairs. It will,\
	thus, calculate the complex Fourier transform\
	of this data.)]\
\
\t[-C	(flag to indicate that the input data is\
	a set of (IMAGINARY, REAL) pairs. It will,\
	thus, calculate the complex Fourier transform\
	of this data.)]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	FILE		*inputHandle, *outputHandle;
	char		*inputFilename = NULL, *outputFilename = NULL;
	fftw_plan	fftwPlan;
	int		complexFlag = FALSE, n, i, optI, firstRealFlag = TRUE;
	float		dummy, dummy2;
	fftw_complex	*in, *out;

	while( (optI=getopt(argc, argv, "i:o:ecC")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'c': complexFlag = TRUE; firstRealFlag = TRUE; break;
			case 'C': complexFlag = TRUE; firstRealFlag = FALSE; break;
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
	if( (inputHandle=fopen(inputFilename, "r")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for reading.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	/* count how many items of data */
	for(n=0;;n++){
		fscanf(inputHandle, "%f", &dummy);
		if( feof(inputHandle) ) break;
	}
	if( complexFlag ){
		if( (n % 2) != 0 )
			fprintf(stderr, "%s : warning, you specified that the input file contains complex numbers (e.g. pairs of floats), but the total number of these numbers is odd. Skipping last number.\n", argv[0]);
		n /= 2;
	}

	rewind(inputHandle);		
	if( (in=(fftw_complex *)malloc(n * sizeof(fftw_complex))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for input array.\n", argv[0], n * sizeof(fftw_complex));
		fclose(inputHandle);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( (out=(fftw_complex *)malloc(n * sizeof(fftw_complex))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output array.\n", argv[0], n * sizeof(fftw_complex));
		fclose(inputHandle);
		free(inputFilename); free(outputFilename); free(in);
		exit(1);
	}

	if( complexFlag ){
		if( firstRealFlag )
			for(i=0;i<n;i++){
				fscanf(inputHandle, "%f%f", &dummy, &dummy2);
				in[i][0] = dummy; in[i][1] = dummy2;
			}
		else
			for(i=0;i<n;i++){
				fscanf(inputHandle, "%f%f", &dummy, &dummy2);
				in[i][0] = dummy2; in[i][1] = dummy;
			}
	} else
		for(i=0;i<n;i++){
			fscanf(inputHandle, "%f", &dummy);
			in[i][1] = 0.0; in[i][0] = dummy;
		}
	fclose(inputHandle);
/*	if( (fftwPlan=fftw_create_plan(n, FFTW_FORWARD, FFTW_MEASURE)) == NULL ){
		fprintf(stderr, "%s : could not create fftw plan for %d numbers/pairs.\n", argv[0], n);
		free(inputFilename); free(outputFilename); free(in); free(out);
		exit(1);
	}		
*/
	if( (fftwPlan=fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_MEASURE)) == NULL ){
		fprintf(stderr, "%s : could not create fftw plan for %d numbers/pairs.\n", argv[0], n);
		free(inputFilename); free(outputFilename); free(in); free(out);
		exit(1);
	}		

	fftw_execute(fftwPlan);
	fftw_destroy_plan(fftwPlan);
	if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for writing output data.\n", argv[0], outputFilename);
		fftw_destroy_plan(fftwPlan);
		free(inputFilename); free(outputFilename); free(in); free(out);
	}

	if( complexFlag ){
		if( firstRealFlag )
			for(i=0;i<n;i++){
				dummy = out[i][0]; dummy2 = out[i][1];
				fprintf(outputHandle, "%f\t%f\n", dummy, dummy2);
			}
		else
			for(i=0;i<n;i++){
				dummy2 = out[i][0]; dummy = out[i][1];
				fprintf(outputHandle, "%f\t%f\n", dummy, dummy2);
			}
	} else
		for(i=0;i<n;i++) fprintf(outputHandle, "%f\n", out[i][0]);

	fclose(outputHandle);
	free(inputFilename); free(outputFilename); free(in); free(out);
	exit(0);
}
