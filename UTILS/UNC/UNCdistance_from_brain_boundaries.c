#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <contour.h>

#include <filters.h>
#include <histogram.h>
#include <statistics.h>
#include <threshold.h>
#include <erosion.h>

/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-9\
\n	(tell the program to copy the header/title information\
\n	from the input file to the output files. If there is\
\n	more than 1 input file, then the information is copied\
\n	from the first file.)]\
\n\
\n** Use these options to select a region of interest\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define _NUMDATAOUT 8

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut[8];
	char		*inputFilename = NULL, *outputBasename = NULL, outputFilename[2000],
			copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	register int	i, j;
	int		*boundariesV[2], *boundariesH[2];
	
	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case '9': copyHeaderFlag = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputBasename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	for(i=0;i<8;i++)
		if( (dataOut[i]=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
	for(i=0;i<2;i++){
		if( (boundariesV[i]=(int *)malloc(W*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for boundariesV[%d].\n", argv[0], W*sizeof(int), i);
			exit(1);
		}
		if( (boundariesH[i]=(int *)malloc(W*sizeof(int))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for boundariesV[%d].\n", argv[0], W*sizeof(int), i);
			exit(1);
		}
	}

	printf("%s, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;	
		printf("%d ", slice+1); fflush(stdout);

		/* initialise */
		for(i=0;i<W;i++) for(j=0;j<2;j++) boundariesV[j][i] = -1;
		for(i=0;i<H;i++) for(j=0;j<2;j++) boundariesH[j][i] = -1;

		for(i=x;i<x+w;i++){
			for(j=y;j<y+h;j++)
				if( data[slice][i][j] > 0 ){ boundariesV[0][i] = j; break; }
			for(j=y+h-1;j>=y;j--)
				if( data[slice][i][j] > 0 ){ boundariesV[1][i] = j; break; }
		}			
		for(j=y;j<y+h;j++){
			for(i=x;i<x+w;i++)
				if( data[slice][i][j] > 0 ){ boundariesH[0][j] = i; break; }
			for(i=x+w-1;i>=x;i--)
				if( data[slice][i][j] > 0 ){ boundariesH[1][j] = i; break; }
		}			

/*		for(i=x;i<x+w;i++){
			if( (j=boundariesV[0][i]) >= 0 ) dataOut[0][s][i][j-1] = 500;
			if( (j=boundariesV[1][i]) >= 0 ) dataOut[0][s][i][j+1] = 500;
		}
		for(j=y;j<y+h;j++){
			if( (i=boundariesH[0][j]) >= 0 ) dataOut[0][s][i-1][j] = 500;
			if( (i=boundariesH[1][j]) >= 0 ) dataOut[0][s][i+1][j] = 500;
		}*/

		/* now we have for each x, the top and bottom boundaries and
		   for each y, the left and right boundaries */
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( data[slice][i][j] <= 0 ){
				dataOut[0][s][i][j] = dataOut[1][s][i][j] =
				dataOut[2][s][i][j] = dataOut[3][s][i][j] = 0;
				continue;
			}
			// if you want more dataOut's increase _NUMDATAOUT
			dataOut[0][s][i][j] = j - boundariesV[0][i];
			dataOut[1][s][i][j] = boundariesV[1][i] - j;
			dataOut[2][s][i][j] = i - boundariesH[0][j];
			dataOut[3][s][i][j] = boundariesH[1][j] - i;

			dataOut[4][s][i][j] = sqrt(SQR(dataOut[0][s][i][j]) + SQR(dataOut[2][s][i][j]));
			dataOut[5][s][i][j] = sqrt(SQR(dataOut[0][s][i][j]) + SQR(dataOut[3][s][i][j]));
			dataOut[6][s][i][j] = sqrt(SQR(dataOut[1][s][i][j]) + SQR(dataOut[2][s][i][j]));
			dataOut[7][s][i][j] = sqrt(SQR(dataOut[1][s][i][j]) + SQR(dataOut[3][s][i][j]));
		}
	}
	printf("\n");

	freeDATATYPE3D(data, actualNumSlices, W);
	for(i=0;i<2;i++){ free(boundariesV[i]); free(boundariesH[i]); }

	printf("%s, writing output files :", argv[0]); fflush(stdout);
	for(i=0;i<_NUMDATAOUT;i++){
		sprintf(outputFilename, "%s_%d.unc", outputBasename, i+1);
		if( !writeUNCSlices3D(outputFilename, dataOut[i], W, H, 0, 0, W, H, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
			exit(1);
		}
		if( copyHeaderFlag )
			/* now copy the image info/title/header of source to destination */
			if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
				fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
				free(inputFilename); free(outputBasename);
				exit(1);
			}
		printf(" %s", outputFilename); fflush(stdout);
	}
	printf("\n");

	for(i=0;i<8;i++) freeDATATYPE3D(dataOut[i], numSlices, W);


	free(inputFilename); free(outputBasename);
	exit(0);
}
