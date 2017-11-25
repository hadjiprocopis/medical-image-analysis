#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-i input.unc -o output.unc -w 10 -h 10 -x 20 -y 20 -9\
\n";

const	char	Usage[] = "options as follows:\
\n\t-i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t-o outputFilename\
\n	(Output filename)\
\n\
\n\t[-f w\
\n	(Draw a rectangular frame around the image borders of width 'w'.\
\n	 This will increase the cropped area by 'w' pixels on *each* side.\
\n	 The pixel value of the border will be equal to 69%% of the maximum\
\n	 pixel value of the output image. This is so that it is not too bright\
\n	 and causes problems in seeing clearly the real image inside.\
\n\
\n	 There is no need to tell you that once you put a frame around the image,\
\n	 the output can *only* be used for display purposes. You can not use it\
\n	 to get histograms etc. The image has been modified...***be warned ****)]\
\n\
\n** define the crop area using :\
\n\t -w width\
\n\t -h height\
\n\t -x topLeftX\
\n\t -y topLeftY\
\n\
\n\
\n\t[-9\
\n	(tell the program to copy the header/title information\
\n	from the input file to the output files. If there is\
\n	more than 1 input file, then the information is copied\
\n	from the first file.)]\
\n\
\n** optionally select the slices you want to include:\
\n\t[-s sliceNumber [-s s...]]\
\n\
\nThis program will extract a rectangular area (crop) from\
\nthe image over all (or those defined with the `-s' option) slices.\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = 0, h = 0, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0, frameWidth = 0;
	int		i, j;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:9f:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'f': frameWidth = atoi(optarg); break;
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
	if( (w<=0) || (h<=0) || (x<0) || (y<0) ){
		fprintf(stderr, "%s : the region to crop has to be defined using the options '-w -h -x -y'.\n", argv[0]);
		free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
		
	if( (x+w+2*frameWidth) > W ){
		fprintf(stderr, "%s : crop area falls outside image width (xoffset=%d + (width=%d)x2 + border=%d > inputImageWidth=%d).\n", argv[0], x, w, frameWidth, W);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h+2*frameWidth) > H ){
		fprintf(stderr, "%s : crop area falls outside image height (yoffset=%d + (height=%d)x2 + border=%d > inputImageHeight=%d).\n", argv[0], y, h, frameWidth, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlices, w+2*frameWidth, h+2*frameWidth)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * (w+2*frameWidth) * (h+2*frameWidth) * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	x = MAX(0, x-frameWidth); y = MAX(0, y-frameWidth);
	w = MIN(W, w+frameWidth); h = MIN(H, h+frameWidth);

	printf("%s, (%d,%d,%d,%d) slice : ", inputFilename, x, y, w, h); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<w;i++) for(j=0;j<h;j++)
			dataOut[s][i][j] = data[slice][i+x][j+y];
	}
	printf("\n");
	freeDATATYPE3D(data, actualNumSlices, W);

	if( frameWidth > 0 ){
		DATATYPE bP = maxPixel3D(dataOut, 0, 0, 0, w, h, numSlices);
		int	hh = h - 1, ww = w - 1, k;

		bP = MAX(1, bP*0.69); /* not to be too bright, but also think if it is a mask */
		printf("%s, drawing a border of pixel value %d, around the cropped area * note: the cropped area will increase by %d pixels on each side ... ", inputFilename, bP, frameWidth); fflush(stdout);
		for(s=0;s<numSlices;s++){
			for(i=0;i<w;i++)
				for(k=0;k<frameWidth;k++)
					dataOut[s][i][k] = dataOut[s][i][hh-k] = bP;
			for(j=0;j<h;j++)
				for(k=0;k<frameWidth;k++)
					dataOut[s][k][j] = dataOut[s][ww-k][j] = bP;
		}
		printf("done\n");
	}
	if( !writeUNCSlices3D(outputFilename, dataOut, w, h, 0, 0, w, h, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(dataOut, numSlices, w);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, w); /* i have added the frame width to the 'w' before */

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
