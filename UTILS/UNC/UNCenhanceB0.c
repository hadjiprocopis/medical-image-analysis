#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

int	solve_system(float **coefficients, float **inverse, float *result, int size);

int	Inverse(float **element, int num, float **result);
void	Determinant(float **element, int num, float *result);
int	Factorial(int	num);
void	Rotate(int *number, int total);

const	char	Examples[] = "\
\n	-i input1.unc -n input2.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i lowResolutionFilename\
\n	(UNC image file with one or more slices - first file to compare)\
\n\
\n\t -I highResolutionFilename\
\n	(second file to compare)\
\n\
\n\t -o outputFilename\
\n	(output filename)\
\n\
\n\t[-t T\
\n	(The larger 'T' is the smaller is the effect and the\
\n	 smoother the resultant image might appear.)]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use this options to select a region of interest\
\n   You may use one or more or all of '-w', '-h', '-x' and\
\n   '-y' once. You may use one or more '-s' options in\
\n   order to specify more slices. Slice numbers start from 1.\
\n   These parameters are optional, if not present then the\
\n   whole image, all slices will be used.\
\n\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\
\nThis program will take a B0 image and a higher resolution\
\nEPI image and try to blow each pixel of the B0 by taking into\
\nconsideration the variation in the intensities of the pixels.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***lowResData, ***highResData, ***dataOut;
	char		*lowResFilename = NULL, *highResFilename = NULL, copyHeaderFlag = FALSE,
			*outputFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	int		W2 = -1, H2 = -1,
			actualNumSlices2 = 0, i, j;
	int		hSx, hSy, ratioResW, ratioResH;
	float		mean, dx, sensitivity = 1.0, L;

	while( (optI=getopt(argc, argv, "i:es:w:h:x:y:I:o:t:9")) != EOF)
		switch( optI ){
			case 'i': lowResFilename = strdup(optarg); break;
			case 'I': highResFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 't': sensitivity = atof(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( lowResFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified (low res).\n");
		if( highResFilename != NULL ) free(highResFilename);
		exit(1);
	}
	if( highResFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second input filename must be specified (high res).\n");
		free(lowResFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(lowResFilename); free(highResFilename);
		exit(1);
	}
	if( (lowResData=getUNCSlices3D(lowResFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], lowResFilename);
		free(lowResFilename); free(highResFilename);
		exit(1);
	}
	if( (highResData=getUNCSlices3D(highResFilename, 0, 0, &W2, &H2, NULL, &actualNumSlices2, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], highResFilename);
		free(lowResFilename); free(highResFilename);
		freeDATATYPE3D(lowResData, actualNumSlices, W);
		exit(1);
	}

	ratioResW = W2 / W; ratioResH = H2 / H;

	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, lowResFilename);
				free(highResFilename); free(lowResFilename);
				freeDATATYPE3D(lowResData, actualNumSlices, W); freeDATATYPE3D(highResData, actualNumSlices2, W2);

				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(highResFilename); free(lowResFilename);
				freeDATATYPE3D(lowResData, actualNumSlices, W); freeDATATYPE3D(highResData, actualNumSlices2, W2);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(lowResFilename); free(highResFilename);
		freeDATATYPE3D(lowResData, actualNumSlices, W); freeDATATYPE3D(highResData, actualNumSlices2, W2);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(lowResFilename); free(highResFilename);
		freeDATATYPE3D(lowResData, actualNumSlices, W); freeDATATYPE3D(highResData, actualNumSlices2, W2);
		exit(1);
	}

	if( (dataOut=callocDATATYPE3D(numSlices, W2, H2)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed.\n", argv[0]);
		free(lowResFilename); free(highResFilename);
		freeDATATYPE3D(lowResData, actualNumSlices, W); freeDATATYPE3D(highResData, actualNumSlices2, W2);
		exit(1);
	}
	for(s=0;s<numSlices;s++)
		for(i=0;i<W2;i++) for(j=0;j<H2;j++) dataOut[s][i][j] = 0;
		
	L = (float )(ratioResH*ratioResW);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=x;i<x+w-1;i++) for(j=y;j<y+h-1;j++){
			mean = 0.0;
			for(hSy=0;hSy<ratioResH;hSy++) for(hSx=0;hSx<ratioResW;hSx++)
				mean += highResData[slice][i*ratioResW+hSx][j*ratioResH+hSy];
			mean /= L;
			for(hSy=0;hSy<ratioResH;hSy++) for(hSx=0;hSx<ratioResW;hSx++){
				dx = mean == 0.0 ? 0.0 : ( (float )(highResData[slice][i*ratioResW+hSx][j*ratioResH+hSy]) - mean ) / mean;
printf("%d, %d = %f\n", i, j, dx);
				dx /= sensitivity;
				dataOut[s][i*ratioResW+hSx][j*ratioResH+hSy] = MAX(0, ROUND( ((float )(lowResData[slice][i][j])) * (1.0 + dx) ));
			}
		}
	}
	
	freeDATATYPE3D(lowResData, actualNumSlices, W);
	freeDATATYPE3D(highResData, actualNumSlices2, W2);

	if( !writeUNCSlices3D(outputFilename, dataOut, W2, H2, 0, 0, W2, H2, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W2);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W2);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(highResFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], highResFilename, outputFilename);
		free(lowResFilename); free(highResFilename); free(outputFilename);
		exit(1);
	}
	free(lowResFilename); free(highResFilename);
	free(outputFilename);

	exit(0);
}


int	solve_system(float **coefficients, float **inverse, float *result, int size){
	/* for example, a system of 3 unknowns (x,y,z)
		a11 * x + a12 * y + a13 * z = a14
		...
		a31 * ..		    = a34
		coefficients: a11 a12 ... a14 <- Row 1
			      ...
			      a31 ..      a34 <- Row 3

		matrix[i][j] i represents ROW number and j COLUMN number.

		result : (x y z)
		inverse: the inverse of the coefficients matrix except the LAST COLUMN
		in this case, size = 3
	*/

	int	i, j;
	float	sum;

	/*for(i=0;i<size;i++){ for(j=0;j<size+1;j++) printf("%f ", coefficients[i][j]); printf("\n"); }*/

	if( Inverse(coefficients, size, inverse) == FALSE ) return FALSE;
	else {
		for(i=0;i<size;i++){
			for(j=0,sum=0.0;j<size;j++)
				sum += inverse[i][j] * coefficients[j][size];
			result[i] = sum;
			/*printf("res = %f\n", sum);*/
		}
	}
	return TRUE;
}

int	Inverse(float **element, int num, float **result)
{
	int	i, j, k, l, I, J, sign;
	float	determ, **Cofactor, **dummy;

	if( (Cofactor=(float **)calloc(num, sizeof(float *))) == NULL ){
		fprintf(stderr, "Could not allocate memory for %d+1 **floats (Cofactor).\n", num);
		exit(-1);
	}
	for(i=0;i<num;i++)
		if( (Cofactor[i]=(float *)calloc(num, sizeof(float))) == NULL ){
			fprintf(stderr, "Could not allocate memory for %d+1 *floats (Cofactor).\n", num);
			exit(-1);
		}
	if( (dummy=(float **)calloc(num, sizeof(float *))) == NULL ){
		fprintf(stderr, "Could not allocate memory for %d+1 **floats (dummy).\n", num);
		exit(-1);
	}
	for(i=0;i<num;i++)
		if( (dummy[i]=(float *)calloc(num, sizeof(float))) == NULL ){
			fprintf(stderr, "Could not allocate memory for %d+1 *floats (dummy).\n", num);
			exit(-1);
		}

	Determinant(element, num, &determ);
	if( determ == 0.0 ){
		for(i=0;i<num;i++){
			free(dummy[i]);
			free(Cofactor[i]);
		}
		free(dummy);
		free(Cofactor);
		return(FALSE);
	}
	/* find cofactors of element matrix */
	/* write onto transpose of element matrix */
	for(i=0;i<num;i++)
		for(j=0;j<num;j++){
			J = 0;I = 0;
			for(k=0;k<num;k++){
				if( k == i ) continue;
				for(l=0;l<num;l++){
					if( l == j ) continue;
					dummy[I][J] = element[k][l];
					I++;
				}
				J++;I = 0;
			}
			Determinant(dummy, num-1, &(Cofactor[j][i]));
		}
			
	for(i=0;i<num;i++)
		for(j=0;j<num;j++){
			if( ((i+j)%2) == 0 ) sign = 1;
			else sign = -1;
			result[i][j] = sign * Cofactor[i][j] / determ;
		}

	for(i=0;i<num;i++){
		free(dummy[i]);
		free(Cofactor[i]);
	}
	free(dummy); free(Cofactor);
	return(TRUE);
}
	
void	Determinant(float **element, int num, float *result)
{
	int	index[100], i, j, k, BreakFlag,
		EqualElementsFlag, Violation;
	float	Sum = 0.0, Product;
		
	for(i=0;i<100;i++) index[i] = 0;

	BreakFlag = FALSE;
	while( !BreakFlag ){
		for(j=0;j<num;j++){
			if( j != (num-1) ){
				EqualElementsFlag = TRUE;
				for(k=j+1;k<num;k++)
					if( index[k] != (num-1) ){
						EqualElementsFlag = FALSE;
						break;
					}
				if( EqualElementsFlag )
					Rotate( &(index[j]), num );
			} else Rotate( &(index[j]), num );
		}
		
		EqualElementsFlag = FALSE;
		Violation = 0;
		for(j=0;j<num;j++){
			for(k=0;k<num;k++){
				if( j == k ) continue;
				if( !EqualElementsFlag && (index[j]==index[k]) )
					EqualElementsFlag = TRUE;
				if( (j<k) && (index[j]>index[k]) )
					Violation++;
			}
		}
		if( !EqualElementsFlag ){
			Product = 1.0;
			for(j=0;j<num;j++) Product *= element[j][index[j]];
			if( (Violation%2) == 0 )
				Sum += Product;
			else	Sum -= Product;
		}

		if( index[0] == (num-1) ){
			EqualElementsFlag = TRUE;
			for(i=1;i<num;i++)
				if( index[i] != (num-1) ){
					EqualElementsFlag = FALSE;
					break;
				}
			if( EqualElementsFlag )
				BreakFlag = TRUE;
		}
	}
	*result = Sum;
}

int	Factorial(int	num)
{
	int	ret = 1, i;
	
	for(i=1;i<=num;i++)
		ret *= i;

	return(ret);
}

void	Rotate(int *number, int total)
{
	*number = (*number) + 1;

	if( *number == total ) 
		*number = 0;
}	
