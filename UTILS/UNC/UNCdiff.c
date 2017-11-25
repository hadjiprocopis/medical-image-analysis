#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input1.unc -n input2.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices - first file to compare)\
\n\
\n\t -n secondInputFilename\
\n	(second file to compare)\
\n\
\n\t[-r\
\n\t   (Report similarities rather than differences which\
\n\t    is the default.)]\
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
\nThis program will compare two UNC files, pixel by pixel,\
\nslice by slice to obtain their differences or (if you use\
\nthe '-r' switch, their similarities).";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***secondData;
	char		*inputFilename = NULL, *secondInputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	int		showSimilaritiesFlag = FALSE, W2 = -1, H2 = -1,
			actualNumSlices2 = 0, i, j;

	while( (optI=getopt(argc, argv, "i:es:w:h:x:y:n:r9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'n': secondInputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'r': showSimilaritiesFlag = TRUE; break;

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( secondInputFilename != NULL ) free(secondInputFilename);
		exit(1);
	}
	if( secondInputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second input filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(secondInputFilename);
		exit(1);
	}

	if( (secondData=getUNCSlices3D(secondInputFilename, 0, 0, &W2, &H2, NULL, &actualNumSlices2, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], secondInputFilename);
		free(inputFilename); free(secondInputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (W2!=W) || (H2!=H) || (actualNumSlices2!=actualNumSlices) ){
		fprintf(stderr, "%s : the two input files '%s' and '%s' have width (%d, %d), height (%d, %d) and/or number of slices (%d, %d).\n", argv[0], inputFilename, secondInputFilename, W, W2, H, H2, actualNumSlices, actualNumSlices2);
		free(inputFilename); free(secondInputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(secondData, actualNumSlices2, W2);
		exit(1);
	}


	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(secondInputFilename); free(inputFilename);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(secondData, actualNumSlices2, W2);

				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(secondInputFilename); free(inputFilename);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(secondData, actualNumSlices2, W2);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(secondInputFilename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(secondData, actualNumSlices2, W2);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(secondInputFilename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(secondData, actualNumSlices2, W2);
		exit(1);
	}

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("*** Slice %d ***\n", slice+1);
		if( showSimilaritiesFlag ){
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				if( data[s][i][j] == secondData[slice][i][j] )
					printf("(%d, %d, %d)\t%d = %d\n", slice+1, i, j, data[s][i][j], secondData[slice][i][j]);
		} else {	
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				if( data[s][i][j] != secondData[slice][i][j] )
					printf("(%d, %d, %d)\t%d <> %d\n", slice+1, i, j, data[s][i][j], secondData[slice][i][j]);
		}				
	}
	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(secondData, actualNumSlices2, W2);
	free(inputFilename); free(secondInputFilename);
	exit(0);
}
