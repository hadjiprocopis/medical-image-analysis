#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Usage[] = "options as follows ([] indicates optional):\
\n\
\n\t -i inputFilename [-i secondInputFilename [etc.]]\
\n	(one or more input filenames of UNC volumes)\
\n\
\n\t -o outputFilaname\
\n\
\n\t[-c	(flag to include x,y,z (z = slice number) coordinates of\
\n	 each pixel value.)]\
\n\
\n\t[-X x\
\n	(will print only pixels whose x-coordinate is 'x'.\
\n	 'x' must be greater or equal to 1 and less than\
\n	 the width of the image.)]\
\n\
\n \t[-Y y\
\n	(will print only pixels whose y-coordinate is 'y'.\
\n	 'y' must be greater or equal to 0 and less than\
\n	 the height of the image.)]\
\n\
\n \t[-Z z\
\n	(will print only pixels whose z-coordinate (slice\
\n	 number is 'z'.\
\n	 'z' must be greater to 0 and less than or equal to\
\n	 the number slices in the image.)]\
\n\
\n \t[-z\
\n	(DO NOT print any background (intensity = 0) pixels.)]\
\n\
\n \t[-m M\
\n 	(multiply each pixel value by M - result will come out\
\n	 in floating point numbers)]\
\n\
\n \t[-a A\
\n 	(add to each pixel value the number A - result will come out\
\n	 in floating point numbers)]\
\n \t[-t\
\n 	(do not show any comments to the output)]\
\n\
\n** Use the following options to define a region of\
\n   interest for the operation to take place. You can\
\n   also chose individual slice numbers with repeated '-s options.\
\n   By default, this process will be applied to all the slices.\
\n   \
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest (upper left corner x-coord)]\
\n\t[-y yCoordOfInterest (upper left cornes y-coord)]\
\n\t[-s sliceNumber [-s s...]] (slice numbers start from 1)";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	FILE		*outputHandle = stdout;
	DATATYPE	***data[100];
	char		*inputFilename[100], *outputFilename = NULL;
	int		numInputFilenames = 0, oW = -1, oH = -1,
			totalNumSlices;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice,
			i, j, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0,
			scanLineX = -1, scanLineY = -1, scanLineZ = -1;
	int		includeCoordinates = FALSE, includeBlackPixel = TRUE, inp;
	float		multiplyFactor = 0.0, addFactor = 0.0;
	char		showComments = TRUE;

	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:cX:Y:Z:zm:a:t")) != EOF)
		switch( optI ){
			case 'i': inputFilename[numInputFilenames++] = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'X': scanLineX = atoi(optarg); break;
			case 'Y': scanLineY = atoi(optarg); break;
			case 'Z': scanLineZ = atoi(optarg); break;
			case 'm': multiplyFactor = atof(optarg); break;
			case 'a': addFactor = atof(optarg); break;
			case 't': showComments = FALSE; break;
			case 'z': includeBlackPixel = FALSE; break;
			case 'c': includeCoordinates = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( numInputFilenames == 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "At least one input filename must be specified.\n");
		exit(1);
	}
	if( outputFilename != NULL ){
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			for(i=numInputFilenames;i-->0;){ free(inputFilename[i]); }
			free(outputFilename);
			exit(1);
		}
	}

	for(inp=0,totalNumSlices=0;inp<numInputFilenames;inp++){
		W = H = actualNumSlices = -1;
		if( (data[inp]=getUNCSlices3D(inputFilename[inp], 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
			for(i=0;i<inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
			if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
			exit(1);
		}
		if( oW == -1 ){ oW = W; oH = H; }
		else {
			if( (oW != W) || (oH != H) ){
				fprintf(stderr, "%s : slices in '%s' have different size (%dx%d) than previous's (%dx%d).\n", argv[0], inputFilename[inp], W, H, oW, oH);
				for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
				for(i=0;i<=inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
				if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
				exit(1);
			}
		}
		if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
		else {
			for(s=0;s<numSlices;s++){
				if( slices[s] >= actualNumSlices ){
					fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename[inp]);
					for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
					for(i=0;i<=inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
					if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
					exit(1);
				} else if( slices[s] < 0 ){
					fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
					for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
					for(i=0;i<=inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
					if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
					exit(1);
				}
			}
		}
		if( w <= 0 ) w = W; if( h <= 0 ) h = H;
		if( (x+w) > W ){
			fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for file '%s'.\n", argv[0], x, w, W, inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
			for(i=0;i<=inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
			if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
			exit(1);
		}
		if( (y+h) > H ){
			fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for file '%s'.\n", argv[0], y, h, H, inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
			for(i=0;i<=inp;i++) freeDATATYPE3D(data[i], actualNumSlices, W);
			if( outputFilename != NULL ){ free(outputFilename); fclose(outputHandle); }
			exit(1);
		}
	}

	scanLineZ--;

	if( showComments ){
		fprintf(outputHandle, "# from ( %d , %d ) for ( %d x %d ), number of slices : %d\n", x, y, w, h, numSlices);
		if( scanLineX >= 0 ) fprintf(outputHandle, "# scan line X : %d\n", scanLineX);
		if( scanLineY >= 0 ) fprintf(outputHandle, "# scan line Y : %d\n", scanLineY);
		if( scanLineZ >  0 ) fprintf(outputHandle, "# scan line Z : %d\n", scanLineZ);
		fprintf(outputHandle, "# file(s) : %s", inputFilename[0]);
		for(inp=1;inp<numInputFilenames;inp++) fprintf(outputHandle, ", %s", inputFilename[inp]);
		fprintf(outputHandle, "\n");
	}

	if( (multiplyFactor == 0.0) && (addFactor == 0.0) ){
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( (scanLineZ > 0) && (slice != scanLineZ) ) continue;
			for(i=x;i<x+w;i++){
				if( (scanLineX > 0) && (scanLineX != i) ) continue;
				for(j=y;j<y+h;j++){
					if( (scanLineY > 0) && (scanLineY != j) ) continue;
					if( (includeBlackPixel==FALSE) && (data[0][slice][i][j]==0) ) continue;
					if( includeCoordinates ) fprintf(outputHandle, "%d %d %d ", i, j, slice+1);
					fprintf(outputHandle, "%d", data[0][slice][i][j]);
					for(inp=1;inp<numInputFilenames;inp++) 
						fprintf(outputHandle, " %d", data[inp][slice][i][j]);
					fprintf(outputHandle, "\n");
				}
			}
		}
	} else {
		if( multiplyFactor == 0.0 ) multiplyFactor = 1.0; /* only an add factor */
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( (scanLineZ > 0) && (slice != scanLineZ) ) continue;
			for(i=x;i<x+w;i++){
				if( (scanLineX > 0) && (scanLineX != i) ) continue;
				for(j=y;j<y+h;j++){
					if( (scanLineY > 0) && (scanLineY != j) ) continue;
					if( (includeBlackPixel==FALSE) && (data[0][slice][i][j]==0) ) continue;
					if( includeCoordinates ) fprintf(outputHandle, "%d %d %d ", i, j, slice+1);
					fprintf(outputHandle, "%f", ((float )data[0][slice][i][j]) * multiplyFactor + addFactor);
					for(inp=1;inp<numInputFilenames;inp++) 
						fprintf(outputHandle, " %f", ((float )data[inp][slice][i][j]) * multiplyFactor + addFactor);
					fprintf(outputHandle, "\n");
				}
			}
		}
	}		
	if( outputFilename != NULL ){ fclose(outputHandle); free(outputFilename); }
	for(i=0;i<numInputFilenames;i++){
		free(inputFilename[i]);
		freeDATATYPE3D(data[i], actualNumSlices, W);
	}

	exit(0);
}
