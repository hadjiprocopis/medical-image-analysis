#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <contour.h>

int     window_3x3_index[9][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1} , {0, 0} , {0,  1},
        {1, -1} , {1, 0} , {1,  1}
};

const	char	Usage[] = "options as follows ([] indicates optional):\
\n\
\n\t -i inputFilename\
\n\
\n\t -o outputFilaname\
\n\
\n\t[-n newColor		(Specifies the new pixel value eroded\
\n			 pixels should have - 0 is the default.\
\n			 If you set this value to a white pixel\
\n			 value, say 2000, you will get an\
\n			 outline of the boundaries)]\
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
\n\t[-s sliceNumber [-s s...]] (slice numbers start from 1)\
\n\
\nThis program will draw an outline around the skulp where the\
\nborder between brain and background is.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define	DEFAULT_THRESHOLD_LOW_VALUE	1
#define	DEFAULT_THRESHOLD_HIGH_VALUE	32767
#define	DEFAULT_NEW_COLOR		1

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice,
			i, j, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	spec		thresholdSpec;
	int		 ii, jj, k,
			ret = 0, seedX = -1, seedY = -1,
			max_recursion_level = 0;
	DATATYPE	**scratchPad, **background;
	connected_object *con_obj;


	/* default values in erosion.h */
	/* what has pixel intensities between low and high IS BRAIN and will be eroded */
	thresholdSpec.low = DEFAULT_THRESHOLD_LOW_VALUE;
	thresholdSpec.high = DEFAULT_THRESHOLD_HIGH_VALUE;
	thresholdSpec.newValue = DEFAULT_NEW_COLOR;

	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:t:n:r:vp9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'n': thresholdSpec.newValue = (DATATYPE )atoi(optarg); break;

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
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( w < 0 ) w = W; if( h < 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d).\n", argv[0], W);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d).\n", argv[0], H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( (dataOut=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %dx%dx%d DATATYPEs of size %zd bytes.\n", argv[0], actualNumSlices, w, h, sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }

	printf("%s '%s': brain pixel values from %d (including) to %d (excluding).\n", argv[0], inputFilename, thresholdSpec.low, thresholdSpec.high);

	printf("%s, slice :", argv[0]); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf(" %d", slice); fflush(stdout);
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
			if( data[s][i][j] == 0 ){
				seedX = i; seedY = j;
				i = x+w+1; break;
			}
		if( seedX < 0 ){
			fprintf(stderr, "%s : could not find any background pixels in the image supplied.\n", argv[0]);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}

		if( (scratchPad=callocDATATYPE2D(x+w, y+h)) == NULL ){
			fprintf(stderr, "%s : call to calloc2D has failed for %dx%d (1)\n", argv[0], x+w, y+h);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		if( (background=callocDATATYPE2D(x+w, y+h)) == NULL ){
			fprintf(stderr, "%s : call to calloc2D has failed for %dx%d (2)\n", argv[0], x+w, y+h);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(scratchPad, x+w);
			exit(1);
		}

		/* call the connected_object routine to find the one connected object which contains the
		   top-left pixel which we will assume belongs to the background. Firstly, the input
		   image will be reversed - all 0 pixels will become 1 and all non-zero will become zero */
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( data[s][i][j] > 0 ) scratchPad[i][j] = 0; else scratchPad[i][j] = 1;
			dataOut[s][i][j] = 0;
		}

		if( (con_obj=find_connected_object2D(scratchPad, seedX, seedY, x, y, 0, w, h, background, NEIGHBOURS_4, 1, 32768, &max_recursion_level)) == NULL ){
			fprintf(stderr, "erode_periphery2D : call to find_connected_object2D has failed for seed (%d, %d).\n", seedX, seedY);
			freeDATATYPE2D(scratchPad, x+w);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		freeDATATYPE2D(scratchPad, x+w);

		/* right, now we have an array which contains 1 if the point is background and 0 otherwise */
		/* so now go and erode all the pixels which are at the boundary of back/non-back pixels */
		for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+h-1;j++) {
			if( background[i][j] == 1 ){
				for(k=0;k<9;k++){
					if( k == 5 ) continue;
					ii = i + window_3x3_index[k][0];
					jj = j + window_3x3_index[k][1];
					if( IS_WITHIN(data[s][ii][jj], thresholdSpec.low, thresholdSpec.high) ){
						/* if it is neighbouring to a non-backg. pixel */
						dataOut[s][i][j] = thresholdSpec.newValue;
						ret++;
						break;
					}
				}
			}
		}
		freeDATATYPE2D(background, x+w);
		connected_object_destroy(con_obj);
	}
	printf("\n");

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, actualNumSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D failed for file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}		

	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(dataOut, actualNumSlices, W);

	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	free(inputFilename); free(outputFilename);
	exit(0);
}
