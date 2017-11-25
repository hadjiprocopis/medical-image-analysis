#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc\
\n\
\nor,\
\n	-i input.unc -s 2 -s 4 -x 10 -y 20 -w 50 -h 45\
\n\
\nto get stats for the portion of the image of the\
\nsecond and fourth slice with left-hand corner at (10,20),\
\nwidth 50 and height 45 pixels.\
\n\
\nif you do not want the background pixel (assumed to be 0)\
\nto be counted in the statistics then use the '-Z' flag:\
\n\
\n	-i input.unc -s 2 -s 4 -x 10 -y 20 -w 50 -h 45 -Z\
\n\
\nThis will give all the stats without considering pixels\
\nof 0 intensity.";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t[-Z\
\n	(flag to indicate that the black pixel should\
\n	 not be counted in the statistics).]\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* add a new stat at the end of these defines, do not forget to update NUM_STATS */
#define	MEAN		0
#define	STDEV		1
#define ENTROPY 	2
#define	MIN_PIXEL	3
#define	MAX_PIXEL	4
#define	NUM_PIXELS	5

/* this is the total number of stats, above */
#define	NUM_STATS	6

char *statNames[NUM_STATS] = {
	"Mean", "Stdev", "Entropy", "minPixel", "maxPixel", "numPixels"
};

int	main(int argc, char **argv){
	FILE		*outputHandle = stdout;
	DATATYPE	***data, *dummy = NULL;
	char		*inputFilename = NULL, *outputFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	int		doNotCountBlackPixel = FALSE, i, j;

	char		commentDesignator = '#';
	float		*hold[NUM_STATS], totalHold[NUM_STATS];

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:Z")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'Z': doNotCountBlackPixel = TRUE; break; 
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
		free(inputFilename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); if( outputFilename != NULL) free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
 			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); if( outputFilename != NULL) free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); if( outputFilename != NULL) free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); if( outputFilename != NULL) free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	for(i=0;i<NUM_STATS;i++){
		if( (hold[i]=(float *)malloc(numSlices*sizeof(float))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for test %d\n", argv[0], numSlices*sizeof(float), i);
			free(inputFilename); if( outputFilename != NULL) free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
	}
	if( doNotCountBlackPixel ){
		if( (dummy=(DATATYPE *)malloc(w*h*sizeof(DATATYPE))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for dummy.\n", argv[0], w*h*sizeof(DATATYPE));
			free(inputFilename); if( outputFilename != NULL) free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
	}

	if( outputFilename != NULL ){
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			free(inputFilename); if( outputFilename != NULL) free(outputFilename); free(outputFilename);
			exit(1);
		}
	}

	totalHold[STDEV] = totalHold[MEAN] =
	totalHold[ENTROPY] = totalHold[NUM_PIXELS] = 0.0;

	totalHold[MIN_PIXEL] = 10000000.0; totalHold[MAX_PIXEL] = -1.0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;

		hold[MEAN][s] =
		hold[ENTROPY][s] = hold[NUM_PIXELS][s] = 0.0;
		hold[MIN_PIXEL][s] = 10000000.0;
		hold[MAX_PIXEL][s] = -1.0;
		
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doNotCountBlackPixel && (data[slice][i][j] == 0) ) continue;

			hold[MEAN][s] += data[slice][i][j];
			totalHold[MEAN] += data[slice][i][j];
			if( data[slice][i][j] > 0 ){
				hold[ENTROPY][s] += data[slice][i][j] * log(data[slice][i][j]);
				totalHold[ENTROPY] += data[slice][i][j] * log(data[slice][i][j]);
			}
			if( data[slice][i][j] < hold[MIN_PIXEL][s] ) hold[MIN_PIXEL][s] = data[slice][i][j];
			if( data[slice][i][j] > hold[MAX_PIXEL][s] ) hold[MAX_PIXEL][s] = data[slice][i][j];
			if( data[slice][i][j] < totalHold[MIN_PIXEL] ) totalHold[MIN_PIXEL] = data[slice][i][j];
			if( data[slice][i][j] > totalHold[MAX_PIXEL] ) totalHold[MAX_PIXEL] = data[slice][i][j];


			hold[NUM_PIXELS][s]++;
		}
		if( hold[NUM_PIXELS][s] > 0 ) hold[MEAN][s] /= hold[NUM_PIXELS][s];
		hold[STDEV][s] = 0.0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doNotCountBlackPixel && (data[slice][i][j] == 0) ) continue;
			hold[STDEV][s] += SQR(hold[MEAN][s] - data[slice][i][j]);
		}
		if( hold[NUM_PIXELS][s] > 0 ) hold[STDEV][s] = sqrt(hold[STDEV][s]/hold[NUM_PIXELS][s]);

		totalHold[NUM_PIXELS] += hold[NUM_PIXELS][s];
	}
	if( totalHold[NUM_PIXELS] > 0 ) totalHold[MEAN] /= totalHold[NUM_PIXELS];
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doNotCountBlackPixel && (data[slice][i][j] == 0) ) continue;
			totalHold[STDEV] += SQR(totalHold[MEAN] - data[slice][i][j]);
		}
	}
	if( totalHold[NUM_PIXELS] > 0 ) totalHold[STDEV] = sqrt(totalHold[STDEV]/totalHold[NUM_PIXELS]);

	fprintf(outputHandle, "%c slice(1)", commentDesignator);
	for(i=0;i<NUM_STATS;i++) fprintf(outputHandle, " %s(%d)", statNames[i], i+2);
	fprintf(outputHandle, "\n");

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;

		fprintf(outputHandle, "%d ", slice+1);
		for(i=0;i<NUM_STATS;i++) fprintf(outputHandle, " %f", hold[i][s]);
		fprintf(outputHandle, "\n");
	}
	fprintf(outputHandle, "%c %d", commentDesignator, numSlices+1);
	for(i=0;i<NUM_STATS;i++) fprintf(outputHandle, " %f", totalHold[i]);
	fprintf(outputHandle, "\n");

	freeDATATYPE3D(data, actualNumSlices, W);
	free(inputFilename);
	if( outputHandle != stdout ) fclose(outputHandle);
	for(i=0;i<NUM_STATS;i++) free(hold[i]);
	exit(0);
}
