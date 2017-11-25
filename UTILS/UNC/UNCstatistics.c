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
\n\t[-a\
\n	(only print overall stats, do not print any\
\n	 stats for each slice. The default is to print\
\n	 stats for each slice and overall stats).]\
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

int	main(int argc, char **argv){
	DATATYPE	***data, *dummy = NULL;
	char		*inputFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	double		mean, stdev, meanMean = 0.0, stdevStdev = 0.0;
	int		minPixel, maxPixel, minMin = 10000000, maxMax = -10000000,
			doNotCountBlackPixel = FALSE, numPixelsCounted, i, j,
			numNonEmptySlices, totalNumPixelsCounted,
			sum, sumSum = 0;
	char		doNotPrintSlices = FALSE;

	while( (optI=getopt(argc, argv, "i:es:w:h:x:y:Za")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'Z': doNotCountBlackPixel = TRUE; break;
			case 'a': doNotPrintSlices = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
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
				free(inputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( doNotCountBlackPixel ){
		if( (dummy=(DATATYPE *)malloc(w*h*sizeof(DATATYPE))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for dummy.\n", argv[0], w*h*sizeof(DATATYPE));
			free(inputFilename); freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
	}
	printf("File:%s, width=%d, height=%d, num slices=%d%s\n", inputFilename, w, h, numSlices, doNotCountBlackPixel?strdup(" (black pixel was not counted)"):strdup(""));

	numNonEmptySlices = 0;
	totalNumPixelsCounted = 0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( doNotCountBlackPixel ){
			numPixelsCounted = 0;
			sum = 0;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) if( data[slice][i][j] > 0 ){
				dummy[numPixelsCounted++] = data[slice][i][j];
				sum += data[slice][i][j];
			}
			if( numPixelsCounted == 0 ){ mean = 0; stdev = 0; minPixel = maxPixel = 0; }
			else {
				statistics1D(dummy,0, numPixelsCounted, &minPixel, &maxPixel, &mean, &stdev);
				numNonEmptySlices++;
			}
		} else {
			sum = 0;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) sum += data[slice][i][j];
			statistics2D(data[slice], x, y, w, h, &minPixel, &maxPixel, &mean, &stdev);
			numNonEmptySlices++;
			numPixelsCounted = (x+w-1)*(y+h-1);
		}
		if( !doNotPrintSlices )
			printf("slice:****** %d *****\nintensity:\t%d to %d\nmean:\t\t%.2f\nstdev:\t\t%.2f\nnum pixels:\t%d\nsum intens.:\t%d\n", slice+1, minPixel, maxPixel, mean, stdev, numPixelsCounted, sum);
		minMin = MIN(minMin, minPixel);
		maxMax = MAX(maxMax, maxPixel);
		sumSum += sum;
		totalNumPixelsCounted += numPixelsCounted;
	}
	/* do stats for all slices */
	meanMean = ((double )sumSum) / ((double )totalNumPixelsCounted);
	stdevStdev = 0.0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( doNotCountBlackPixel ){
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) if( data[slice][i][j] > 0 ) stdevStdev += SQR(meanMean - ((double )(data[slice][i][j])));
		} else {
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) stdevStdev += SQR(meanMean - ((double )(data[slice][i][j])));
		}		
	}
	stdevStdev = sqrt(stdevStdev) / ((double )totalNumPixelsCounted);			

	if( (numSlices > 1) || doNotPrintSlices )
		printf("\nOverall statistics:\nintensity:\t%d to %d\nmean:\t\t%.2f\nstdev:\t\t%.2f\nnum pixels:\t%d\nsum intens.:\t%d\n", minMin, maxMax, meanMean, stdevStdev, totalNumPixelsCounted, sumSum);

	if( doNotCountBlackPixel ) free(dummy);
	freeDATATYPE3D(data, actualNumSlices, W);
	free(inputFilename);
	exit(0);
}
