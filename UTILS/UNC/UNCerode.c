#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Usage[] = "options as follows ([] indicates optional):\
\n\
\n\t -i inputFilename\
\n\
\n\t -o outputFilaname\
\n\
\n\t[-t low:high		(Specifies the intensities of pixels that\
\n			 belong to the brain. What falls outside\
\n			 this range will be considered non-brain.\
\n			 Erosion is the process by which brain\
\n			 pixels become non-brain pixels.)]\
\n\
\n\t[-n newColor		(Specifies the new pixel value eroded\
\n			 pixels should have - 0 is the default.\
\n			 If you set this value to a white pixel\
\n			 value, say 2000, you will get an\
\n			 outline of the boundaries)]\
\n\
\n\t[-r iterations	(Specifies the number of passes for the\
\n			 erosion procedure. The default is 1.\
\n			 Make sure that the eroded pixels color\
\n			 (defined with the -n option) falls outside\
\n			 the intensities range you have defined\
\n			 with the '-t' option.)]\
\n\
\n\t[-v			(Verbose flag - says more information about\
\n			 the erosion operation. The 3 numbers that will\
\n			 be displayed in brackets will be\
\n			 (slice number, pixels eroded for this slice,\
\n			 and total pixels eroded so far))]\
\n\
\n\t[-p			(Erode periphery pixels only flag)]\
\n\
\n\t[-R filename\
\n			(write eroded pixels in file 'filename')]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use the following options to define a region of\
\n   interest for the erosion to take place. You can\
\n   also chose individual slice numbers with repeated '-s options.\
\n   By default, this process will be applied to all the slices.\
\n   \
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest (upper left corner x-coord)]\
\n\t[-y yCoordOfInterest (upper left cornes y-coord)]\
\n\t[-s sliceNumber [-s s...]] (slice numbers start from 1)";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define	DEFAULT_THRESHOLD_LOW_VALUE	1
#define	DEFAULT_THRESHOLD_HIGH_VALUE	32768
#define	DEFAULT_NEW_COLOR		0

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL,
			copyHeaderFlag = FALSE, *reverseFilename = NULL;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, totalPixelsEroded = 0,
			pixelsEroded = 0, i, j, r, numIterations = 1, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0, verboseFlag = FALSE,
			erodePeripheryOnlyFlag = FALSE;
	spec		thresholdSpec;
	DATATYPE	***dataIn;

	/* default values in erosion.h */
	/* what has pixel intensities between low and high IS BRAIN and will be eroded */
	thresholdSpec.low = DEFAULT_THRESHOLD_LOW_VALUE;
	thresholdSpec.high = DEFAULT_THRESHOLD_HIGH_VALUE;
	thresholdSpec.newValue = DEFAULT_NEW_COLOR;

	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:t:n:r:vp9R:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'v': verboseFlag = TRUE; break;
			case 'r': numIterations = atoi(optarg); break;
			case 't': sscanf(optarg, "%d:%d", &(thresholdSpec.low), &(thresholdSpec.high)); break;
			case 'n': thresholdSpec.newValue = (DATATYPE )atoi(optarg); break;
			case 'p': erodePeripheryOnlyFlag = TRUE; break;
			case 'R': reverseFilename = strdup(optarg); break;

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename); free(reverseFilename);
		exit(1);
	}
	if( w < 0 ) w = W; if( h < 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d).\n", argv[0], W);
		free(inputFilename); free(outputFilename); free(reverseFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d).\n", argv[0], H);
		free(inputFilename); free(outputFilename); free(reverseFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( (dataIn=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %dx%dx%d DATATYPEs of size %zd bytes (dataIn).\n", argv[0], actualNumSlices, w, h, sizeof(DATATYPE));
		free(inputFilename); free(outputFilename); free(reverseFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %dx%dx%d DATATYPEs of size %zd bytes (dataOut).\n", argv[0], actualNumSlices, w, h, sizeof(DATATYPE));
		free(inputFilename); free(outputFilename); free(reverseFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataIn, actualNumSlices, W);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }

	printf("%s '%s': brain pixel values from %d (including) to %d (excluding).\n", argv[0], inputFilename, thresholdSpec.low, thresholdSpec.high);

	/* copy original data to output */
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=0;i<W;i++) for(j=0;j<H;j++)
			dataIn[slice][i][j] = data[slice][i][j];
	}

	if( erodePeripheryOnlyFlag ){
		for(r=0;r<numIterations;r++){
			if( verboseFlag ){ printf("erosion pass %d: ", r+1); fflush(stdout); }
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				pixelsEroded = erode_periphery2D(dataIn[slice], x, y, w, h, dataOut[slice], thresholdSpec);
				totalPixelsEroded += pixelsEroded;
				if( verboseFlag ){ printf("(%d,%d,%d) ", slice+1, pixelsEroded, totalPixelsEroded); fflush(stdout);}
				for(i=0;i<W;i++) for(j=0;j<H;j++) dataIn[slice][i][j] = dataOut[slice][i][j];
			}
			if( verboseFlag ) printf("\n"); 
		}
	} else {
		for(r=0;r<numIterations;r++){
			if( verboseFlag ){ printf("erosion pass %d: ", r+1); fflush(stdout); }
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				pixelsEroded = erode2D(dataIn[slice], x, y, w, h, dataOut[slice], thresholdSpec);
				totalPixelsEroded += pixelsEroded;
				if( verboseFlag ){ printf("(%d,%d,%d) ", slice+1, pixelsEroded, totalPixelsEroded); fflush(stdout);}
				for(i=0;i<W;i++) for(j=0;j<H;j++) dataIn[slice][i][j] = dataOut[slice][i][j];
			}
			if( verboseFlag ) printf("\n"); 
		}
	}

	printf("%s, '%s' : %d pixels eroded\n", argv[0], inputFilename, totalPixelsEroded);
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D failed for file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename); free(reverseFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, actualNumSlices, W);
		freeDATATYPE3D(dataIn, actualNumSlices, W);
		exit(1);
	}		

	if( reverseFilename != NULL ){
		/* show only the eroded pixels */
		if( verboseFlag ) printf("reversing ... results into '%s'\n", reverseFilename);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			for(i=0;i<W;i++) for(j=0;j<H;j++) {
				dataIn[slice][i][j] = 0;
				if( data[slice][i][j] != dataOut[slice][i][j] ) dataIn[slice][i][j] = data[slice][i][j];
			}
		}
		if( !writeUNCSlices3D(reverseFilename, dataIn, W, H, 0, 0, W, H, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D failed for file '%s'.\n", argv[0], reverseFilename);
			free(inputFilename); free(outputFilename); free(reverseFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, actualNumSlices, W);
			freeDATATYPE3D(dataIn, actualNumSlices, W);
			exit(1);
		}		
	}

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=0;i<W;i++) for(j=0;j<H;j++) {
			if( dataOut[slice][i][j] < 0 ) printf("XXX : %d %d %d\n", slice, i, j);
		}
	}

	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(dataOut, actualNumSlices, W);
	freeDATATYPE3D(dataIn, actualNumSlices, W);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ){
		if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
			free(inputFilename); free(outputFilename); free(reverseFilename);
			exit(1);
		}
		if( reverseFilename != NULL )
			if( !copyUNCInfo(inputFilename, reverseFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
				fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, reverseFilename);
				free(inputFilename); free(outputFilename); free(reverseFilename);
				exit(1);
			}
	}

	free(inputFilename); free(outputFilename); free(reverseFilename);
	exit(0);
}
