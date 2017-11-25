#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t[-o outputFilename\
\n	(file to contain the threshold pixel value.\
\n	 If omitted, the value will be printed to stdout.)]\
\n\
\n\t[-r\
\n	(do the process starting from the highest pixel\
\n	 intensities.)]\
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
\nThis program will take a volume and find the threshold between\
\nbackground noise and useful data. This will be dumped to stdout.";

const	char	Author[] = "D-S YOO, 1994 & Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	FILE		*outputFilehandle = stdout;
	DATATYPE	***data, ***data2;
	char		*inputFilename = NULL, *outputFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, actualNumSlices = 0, slice,
			optI, slices[1000], allSlices = 0, value;
	int		i, j, LOHI = TRUE;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:r")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'r': LOHI = FALSE; break;
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
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); if( outputFilename != NULL ) free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); if( outputFilename != NULL ) free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (data2=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d slices %dx%d each.\n", argv[0], numSlices, W, H);
		free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}		
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=0;i<W;i++) for(j=0;j<H;j++) data2[s][i][j] = data[slice][i][j];
	}
	freeDATATYPE3D(data, actualNumSlices, W);

	if( LOHI ){
		if( otsu_find_background_threshold3D_LOHI(data2, x, y, 0, W, H, numSlices, &value) == FALSE ){
			fprintf(stderr, "%s : call to otsu_find_background_threshold3D has failed.\n", argv[0]);
			free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
			freeDATATYPE3D(data2, numSlices, W);
			exit(1);
		}
	} else {
		if( otsu_find_background_threshold3D_HILO(data2, x, y, 0, W, H, numSlices, &value) == FALSE ){
			fprintf(stderr, "%s : call to otsu_find_background_threshold3D has failed.\n", argv[0]);
			free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
			freeDATATYPE3D(data2, numSlices, W);
			exit(1);
		}
	}		
	if( outputFilename != NULL )
		if( (outputFilehandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			free(inputFilename); if( outputFilename != NULL ) free(outputFilename);
			freeDATATYPE3D(data2, numSlices, W);
			exit(1);
		}
	fprintf(outputFilehandle, "%d\n", value);
	if( outputFilename != NULL ){
		fclose(outputFilehandle);
		free(outputFilename);
	}

	freeDATATYPE3D(data2, numSlices, W);
	free(inputFilename);

	exit(0);
}
