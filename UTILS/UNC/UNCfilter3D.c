#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "";

const	char	Usage[] = "options as follows:\
\n -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** at least one of either the -p ot -f options below\
\n** must be present.\
\n** You may use as many combinations of -p and -f as you wish.\
\n** Notice the field separator ':'\
\n\
\n[-r\
\n	(rescale output pixels within the range 0 to 32767)]\
\n\
\n\t[-S level (Sharpen image)]\
\n\t[-L level (Laplacian of image)]\
\n\t[-O level (Sobel filter)]\
\n\t[-M level (Median filter)]\
\n\t[-A level (Average image)]\
\n\t[-I level (Min image)]\
\n\t[-X level (Max image)]\
\n\t[-P level (Prewitt filter)]\
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

#define	UNKNOWN	0x0
#define	SHARPEN	0x1
#define	SOBEL	0x2
#define	MEDIAN	0x3
#define	AVERAGE	0x4
#define	MIN_V	0x5
#define	MAX_V	0x6
#define	PREWITT	0x7
#define	LAPLACIAN 0x8

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut, ***dataIn;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	int		i, j, width = 1, minOutputColor, maxOutputColor,
			rescaleFlag = FALSE, doFilterType = UNKNOWN;
	float		level;
	
	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:r:eS:L:O:M:A:I:X:P:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case 'S': sscanf(optarg, "%f", &level); doFilterType = SHARPEN; break;
			case 'L': sscanf(optarg, "%f", &level); doFilterType = LAPLACIAN; break;
			case 'O': sscanf(optarg, "%f", &level); doFilterType = SOBEL; break;
			case 'M': sscanf(optarg, "%f", &level); doFilterType = MEDIAN; break;
			case 'A': sscanf(optarg, "%f", &level); doFilterType = AVERAGE; break;
			case 'I': sscanf(optarg, "%f", &level); doFilterType = MIN_V; break;
			case 'X': sscanf(optarg, "%f", &level); doFilterType = MAX_V; break;
			case 'P': sscanf(optarg, "%f", &level); doFilterType = PREWITT; break;

			case 'l': width = atoi(optarg); break;

			case 'r': rescaleFlag = TRUE; sscanf(optarg, "%d:%d", &minOutputColor, &maxOutputColor); break;

			case '9': copyHeaderFlag = TRUE; break;
			
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
	if( doFilterType == UNKNOWN ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A filter type must be specified.\n");
		free(inputFilename); free(outputFilename);
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
	if( numSlices == 1 ){
		fprintf(stderr, "%s : there is/you have selected only 1 slice. You can not find perform a 3D filter on a 2D data!\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
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


	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d x %d x %d.\n", argv[0], numSlices, W, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (dataIn=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d x %d x %d.\n", argv[0], numSlices, W, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(j=y;j<y+h;j++) for(i=x+width;i<x+w-width;i++) dataIn[s][i][j] = data[slice][i][j];
	}

	switch( doFilterType ){
		case SHARPEN:
			sharpen3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case LAPLACIAN:
			laplacian3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case SOBEL:
			sobel3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case MEDIAN:
			median3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case AVERAGE:
			average3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case MIN_V:
			min3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case MAX_V:
			max3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
		case PREWITT:
			prewitt3D(dataIn, x, y, 0, w, h, numSlices, dataOut, level); break;
	}
	
	if( rescaleFlag ){
		DATATYPE	dummy, minPixel, maxPixel;
		for(s=0;s<numSlices;s++){
			minPixel = maxPixel = dataOut[s][x][y];
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( dataOut[s][i][j] < minPixel ) minPixel = dataOut[s][i][j];
				if( dataOut[s][i][j] > maxPixel ) maxPixel = dataOut[s][i][j];
			}
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				dummy = ROUND(SCALE_OUTPUT(dataOut[s][i][j], minOutputColor, maxOutputColor, minPixel, maxPixel));
				dataOut[s][i][j] = dummy;
			}
		}
	}

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(dataIn, numSlices, W);
	freeDATATYPE3D(dataOut, numSlices, W);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	free(inputFilename); free(outputFilename);
	exit(0);
}
