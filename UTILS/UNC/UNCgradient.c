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
\n** at least one of either the -p ot -f options below\
\n** must be present.\
\n** You may use as many combinations of -p and -f as you wish.\
\n** Notice the field separator ':'\
\n\
\n[-r minP:maxP\
\n	(rescale output pixels within the range minP to maxP)]\
\n\
\n[-X | -Y | -Z\
\n	(along which axis to calculate the gradient. Default is\
\n	 along X. In order to do it along Z, you need to have\
\n	 at least 2 slices.)]\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define	G_UNKNOWN	0x0
#define	G_X		0x1
#define	G_Y		0x2
#define	G_Z		0x4

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut, dummy;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	int		i, j, width = 1, minOutputColor, maxOutputColor,
			gradientAlong = G_UNKNOWN, rescaleFlag = FALSE;
	float		divide;
	
	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:r:eXYZl:9")) != EOF)
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
			case 'X': gradientAlong |= G_X; break;
			case 'Y': gradientAlong |= G_Y; break;
			case 'Z': gradientAlong |= G_Z; break;
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
	if( gradientAlong == G_UNKNOWN ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "You must specify the direction of the gradient using one or more of '-X' '-Y' '-Z'\n");
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
	if( (gradientAlong == 'Z') && (numSlices == 1) ){
		fprintf(stderr, "%s : there is/you have selected only 1 slice. You can not find the gradient of the volume along the z-axis!\n", argv[0]);
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

	divide = 0;
	if( (gradientAlong & G_X) != 0 ){
		divide++;
		printf("%s : calculating gradient along x-axis: ", argv[0]); fflush(stdout);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);
			for(j=y;j<y+h;j++) for(i=x+width;i<x+w-width;i++)
				dataOut[s][i][j] = ABS(data[slice][i+width][j] - data[slice][i-width][j]);
		}
		printf("\n");
	}
	if( (gradientAlong & G_Y) != 0 ){
		divide++;
		printf("%s : calculating gradient along y-axis: ", argv[0]); fflush(stdout);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);
			for(i=x;i<x+w;i++) for(j=y+width;j<y+h-width;j++)
				dataOut[s][i][j] += MIN(32767, ABS(data[slice][i][j+width] - data[slice][i][j-width]));
		}
		printf("\n");
	}
	if( (gradientAlong & G_Z) != 0 ){
		divide++;
		printf("%s : calculating gradient along z-axis: ", argv[0]); fflush(stdout);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( (slice < width) || (slice >= (numSlices-width)) ) continue;
			printf("%d ", slice+1); fflush(stdout);
			for(j=y;j<y+h;j++) for(i=x;i<x+w;i++)
				dataOut[s][i][j] += MIN(32767, ABS(data[slice+width][i][j] - data[slice-width][i][j]));
		}
		printf("\n");
	}

	if( rescaleFlag ){
		DATATYPE	minPixel, maxPixel;
		for(s=0;s<numSlices;s++){
			minPixel = maxPixel = dataOut[s][x][y];
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( dataOut[s][i][j] < minPixel ) minPixel = dataOut[s][i][j];
				if( dataOut[s][i][j] > maxPixel ) maxPixel = dataOut[s][i][j];
			}
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				dummy = ROUND(SCALE_OUTPUT(dataOut[s][i][j]/divide, minOutputColor, maxOutputColor, minPixel/divide, maxPixel/divide));
				dataOut[s][i][j] = dummy;
			}
		}
	} else {
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				dummy = ROUND(dataOut[s][i][j]/divide);
				dataOut[s][i][j] = dummy;
			}
	}

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W);
	freeDATATYPE3D(data, actualNumSlices, W);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	free(inputFilename); free(outputFilename);
	exit(0);
}
