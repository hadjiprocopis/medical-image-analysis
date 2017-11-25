#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "";
/*\n	-i input.unc -o output.unc\
\n";
*/
const	char	Usage[] = "options as follows:";
/*
\n\t -i inputFilename\
\n	(UNC image file with one or more slices to be read\
\n       and its histogram matched to specified histogram)\
\n\
\n\t -o outputFilename\
\n	(UNC output filename)\
\n\
\n ** ONE of the FOLLOWING TWO options must be specified
\n\t -t targetHistogramFilename\
\n	(the filename of the target histogram)\
\n\
\n\ OR
\n\
\n\t -m targetImageFilename\
\n	(the filename of the UNC image whose histogram\
\n       you need to use as reference for the matching)\
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
\n\t[-W widthOfInterest for target image, with -m option only]\
\n\t[-H heightOfInterest for target image, with -m option only]\
\n\t[-X xCoordOfInterest, with -m option only]\
\n\t[-Y yCoordOfInterest, with -m option only]\
\n\t[-S sliceNumber [-S s...], with -m option only]";
*/
const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut, ***targetData = NULL;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE,
			*targetHistogramFilename = NULL, *targetImageFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			x1 = 0, y1 = 0, w1 = -1, h1 = -1, slices1[1000],
			depth, format, s, slice, actualNumSlices = 0,
			W1 = -1, H1 = -1, depth1, format1,
			actualNumSlices1 = 0, numSlices1 = 0,
			optI, slices[1000], allSlices = 0, allSlices1 = 0;
	histogram	*referenceHistogram = NULL;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:9:X:Y:H:W:S:m:t:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'S': slices1[numSlices1++] = atoi(optarg) - 1; break;
			case 't': targetHistogramFilename = strdup(optarg); break;
			case 'm': targetImageFilename = strdup(optarg); break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'W': w1 = atoi(optarg); break;
			case 'H': h1 = atoi(optarg); break;
			case 'X': x1 = atoi(optarg); break;
			case 'Y': y1 = atoi(optarg); break;
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
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (targetImageFilename==NULL) && (targetHistogramFilename==NULL) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A target Image (option -m) or Histogram (option -t) filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
		
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( targetImageFilename != NULL ){
		if( numSlices1 > 0 ) actualNumSlices1 = numSlices1;
		if( (targetData=getUNCSlices3D(targetImageFilename, x1, y1, &W1, &H1, numSlices1==0?NULL:slices1, &actualNumSlices1, &depth1, &format1)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], targetImageFilename);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		if( numSlices1 == 0 ){ numSlices1 = actualNumSlices1; allSlices1 = 1; }

		if( w1 <= 0 ) w1 = W1; if( h1 <= 0 ) h1 = H1;
		if( (x1+w1) > W1 ){
			fprintf(stderr, "%s : region of interest of target image falls outside image width (%d + %d > %d).\n", argv[0], x1, w1, W1);
			free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(targetData, actualNumSlices1, W1);
			exit(1);
		}
		if( (y1+h1) > H1 ){
			fprintf(stderr, "%s : region of interest of target image falls outside image height (%d + %d > %d).\n", argv[0], y1, h1, H1);
			free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(targetData, actualNumSlices1, W1);
			exit(1);
		}
		if( (referenceHistogram=histogram3D(targetData, x1, y1, 0, w1, h1, numSlices1, 1)) == NULL ){
			fprintf(stderr, "%s : call to histogram3D has failed for target image file %s.\n", argv[0], targetImageFilename);
			free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(targetData, actualNumSlices1, W1);
			exit(1);
		}
		freeDATATYPE3D(targetData, actualNumSlices1, W1);
	}
	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	printf("%s, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		if( !histogram_matching2D(
			data[slice], dataOut[s],
			x, y, w, h, depth,
			referenceHistogram,
			HISTOGRAM_MATCHING_EHM_MOROVIC
		) ){
			fprintf(stderr, "%s : call to histogram_matching2D has failed for slice %d.\n", argv[0], s);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
	}
	printf("\n");
	freeDATATYPE3D(data, actualNumSlices, W);

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W);

	if( copyHeaderFlag )
		/* now copy the image info/title/header of source to destination */
		if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}

	free(inputFilename); free(outputFilename);
	exit(0);
}
