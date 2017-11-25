#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
const char Usage[] = "options as follows";
//const char ShortUsage[] = "options as follows";
/*
const char Usage[] = "options as follows ([] indicates optional):\
\n
\n\t -i inputFilename (UNC format)\
\n\t -o outputFilename (UNC format)\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n\t[-j (this help message)]\
\n\t[-v (verbose flag, it says what it does)]\
\n\t[-k (the output will be a binary file ->\
\n		(minOutputPixel:maxOutputPixel - see -r option)\
\n		regions with low color mean that\
\n		the image pixels did not meet the\
\n		criteria imposed. E.g. if you were\
\n		interested in pixels whose frequency\
\n		of occurence was between so and so.)]\
\n\t[-n (flag to include original image slice\
\n               at the BEGINING of all the maps for the\
\n               slice)]\
\n\t[-l (flag to indicate that when a ROI (region of\
\n               interest) is specified by -x, -y, -w\
\n               and -h the image outside the ROI will\
\n               be shown as it was. In the absence of\
\n               this flag, the image outside the ROI\
\n               is black. The image outside the ROI\
\n               can be removed by specifying the '-g'\
\n               option. In this case, the saved UNC\
\n               file will have the dimensions of the ROI.\
\n               When the '-l' option is present, the '-g' \
\n               option is AUTOMATICALLY present too.)]\
\n\t[-g (flag to indicate that in the case a ROI has been\
\n               specified, the saved file will have the\
\n               same dimensions as the original image.\
\n               The image falling outside the ROI will\
\n               be black or in the presence of the '-l' \
\n               option, the same as the original image.)]\
\n\t[-z n   (flag to indicate that in the case a ROI has been\
\n               specified and the image outside ROI is not\
\n               to be removed, a border of n pixels is to be\
\n               drawn around the ROI with the maximum pixel\
\n               intensity of the image.)]\
\n** at least one of the options below must be specified.\
\n   You can specify as many maps of the same type as you\
\n   want (e.g. -f 10:20 -f 20:30)\
\n\
\n\t[-r min:max (scale output map pixel values to be between\
\n               min and max - in case they are too small,\
\n               etc. If this option is not used DEFAULT\
\n               values will be used, so if you can not\
\n               see anything adjust this value. Also note\
\n               that in case a ROI has been specified and\
\n               the image outside ROI should be present,\
\n               it is a good idea NOT to use this option\
\n               so that the min and max pixel intensities\
\n               of the ROI are the same as those for the\
\n               whole image.)]\
\n\
\n** for the options below, a window size is calculated from\
\n   specified w and h with (2*w+1, 2*h+1) e.g. w is half the\
\n   window width plus the center pixel of the window\
\n\
\n\
\n** this is a shortcut for having all of the above options. Note\
\n   that when this option is specified, any other maps specified\
\n   will be ignored\
\n\
\n\t[-a w:h:W:H:lo:hi\
\n              (ALL of the above in this help file's order with\
\n                     w, h, W, H, lo and hi IN COMMON)]";
*/
const	char	ShortUsage[] = "options as follows ([] means optional):\
\n\t -i inputFilename (UNC format)\
\n\t -o outputFilename (UNC format)\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\t[-v (verbose)]\
\n\t[-k (output binary image, 1 means satisfies criteria imposed, 0 otherwise)]\
\n\t[-n (include original image)]\
\n\t[-l (show image with ROI, makes -g=TRUE)]\
\n\t[-g (with ROI, size of output image = size of input)]\
\n\t[-z width (draw small border around ROI of width 'width')]\
\n\t[-r min:max (output color range, do not use with ROI)]\
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
\n** shortcut to include all of the above, use it as many times\
\n\t[-a w:h:W:H:lo:hi (options apply to all)]\
\n\
\n\t[-j (SHOW LONGER HELP PAGE WITH MORE DETAIL)]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* the min and max output colors, numbers will be scaled to fit this range */
#define	DEFAULT_MIN_OUTPUT_COLOR	0
#define	DEFAULT_MAX_OUTPUT_COLOR	1000

/* if you are adding any more map types then add at the end its LETTER as in -'letter'
   and enum it equal to the last integer plus 1,
   for example, if -x invokes the new map type and 10 is the last integer in the enum
   then add ', X=11' at the end of this enum statement.
   *** DO NOT FORGET **** to increment the #define NUM_FILTER_TYPES to plus 1 */
enum	{A=0, SHARPEN=1, LAPLACIAN=2, SOBEL=3, MEDIAN=4, AVERAGE=5, MIN_V=6, MAX_V=7, PREWITT=8};

/* how many different maps we have ? e.g. -f, -q etc. count also those that they do not need spec, if any */
/* change this constant if you add or remove map types (last enum+1)*/
#define	NUM_FILTER_TYPES			(8+1)
/* foreach map type, how many different specs can the user give? e.g. how many -f options at the same command line */
#define	MAX_SPECS_PER_FILTER		10

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut,
			minOutputColor = DEFAULT_MIN_OUTPUT_COLOR, maxOutputColor = DEFAULT_MAX_OUTPUT_COLOR,
			borderColor = DEFAULT_MIN_OUTPUT_COLOR,
			globalMinPixel, globalMaxPixel;
	char		inputFilename[1000] = {"\0"}, outputFilename[1000] = {"\0"},
			includeOriginalImage = FALSE, verbose = FALSE,
			showImageOutsideROI = FALSE, retainImageDimensions = FALSE,
			outputColorRangeSpecified = FALSE, copyHeaderFlag = FALSE;
	int		numSlices = 0, X, Y, W, H, inX, inY, inW, inH,
			depth, format, s, n, borderThickness = 0,
			optI, slices[1000], allSlices = 0, numSlicesOut = 0,
			nSpecs[NUM_FILTER_TYPES], nSpecs_dup[NUM_FILTER_TYPES],
			filterOrder[(NUM_FILTER_TYPES-1) * MAX_SPECS_PER_FILTER],
			sOut, numFilters, currentFilter, m,
			d1, d2;
	histogram	*pixelFrequencies = NULL;
	register int	i, j, slice, param1, param2, param3, param4;
	double		globalMean, globalStdev, d3, d4;
	float		lo, hi;
	area		SPECS[NUM_FILTER_TYPES][MAX_SPECS_PER_FILTER], ROI;

	/* initialise */
	for(i=0;i<NUM_FILTER_TYPES;i++) for(j=0;j<MAX_SPECS_PER_FILTER;j++) {
		nSpecs[i] = 0;
		SPECS[i][j].a = SPECS[i][j].b = SPECS[i][j].c = SPECS[i][j].d = 0;
		SPECS[i][j].lo = -1000000.0; SPECS[i][j].hi = 10000000;
	}
	numFilters = 0;
	ROI.a = ROI.b = 0; ROI.c = ROI.d = -1; /* region of interest init */

	/* read & process args */
	opterr = 0;
	while( (optI=getopt(argc, argv, "i:o:w:h:x:y:s:vknlgz:r:S:L:O:M:A:I:X:P:a:j?9")) != EOF)
		switch( optI ){
			case 'i': strcpy(inputFilename, optarg); break;
			case 'o': strcpy(outputFilename, optarg); break;
			case 'x': ROI.a = atoi(optarg); break;
			case 'y': ROI.b = atoi(optarg); break;
			case 'w': ROI.c = atoi(optarg); break;
			case 'h': ROI.d = atoi(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg); break;
			case 'v': verbose = TRUE; break;
			case 'k': break;
			case 'n': includeOriginalImage = TRUE; break;
			case 'l': showImageOutsideROI = TRUE; retainImageDimensions = TRUE; break;
			case 'g': retainImageDimensions = TRUE; break;
			case 'z': borderColor = maxOutputColor; borderThickness = atoi(optarg); break;
			case 'r': sscanf(optarg, "%d:%d", &(minOutputColor), &(maxOutputColor)); outputColorRangeSpecified = TRUE; break;

			case 'S': sscanf(optarg, "%lf", &(SPECS[SHARPEN][nSpecs[SHARPEN]].lo)); nSpecs[SHARPEN]++; filterOrder[numFilters++] = SHARPEN; break;
			case 'L': sscanf(optarg, "%lf", &(SPECS[LAPLACIAN][nSpecs[LAPLACIAN]].lo)); nSpecs[LAPLACIAN]++; filterOrder[numFilters++] = LAPLACIAN; break;
			case 'O': sscanf(optarg, "%lf", &(SPECS[SOBEL][nSpecs[SOBEL]].lo)); nSpecs[SOBEL]++; filterOrder[numFilters++] = SOBEL; break;
			case 'M': sscanf(optarg, "%lf", &(SPECS[MEDIAN][nSpecs[MEDIAN]].lo)); nSpecs[MEDIAN]++; filterOrder[numFilters++] = MEDIAN; break;
			case 'A': sscanf(optarg, "%lf", &(SPECS[AVERAGE][nSpecs[AVERAGE]].lo)); nSpecs[AVERAGE]++; filterOrder[numFilters++] = AVERAGE; break;
			case 'I': sscanf(optarg, "%lf", &(SPECS[MIN_V][nSpecs[MIN_V]].lo)); nSpecs[MIN_V]++; filterOrder[numFilters++] = MIN_V; break;
			case 'X': sscanf(optarg, "%lf", &(SPECS[MAX_V][nSpecs[MAX_V]].lo)); nSpecs[MAX_V]++; filterOrder[numFilters++] = MAX_V; break;
			case 'P': sscanf(optarg, "%lf", &(SPECS[PREWITT][nSpecs[PREWITT]].lo)); nSpecs[PREWITT]++; filterOrder[numFilters++] = PREWITT; break;
			case 'a': sscanf(optarg, "%lf", &(SPECS[A][nSpecs[A]].lo)); nSpecs[A]++; break;

			case 'j': fprintf(stderr, "Usage : %s %s\n%s\n\n", argv[0], Usage, Author);
				  exit(0);
			case '9': copyHeaderFlag = TRUE; break;
			
			default:
			case '?': fprintf(stderr, "Usage : %s %s\n%s\n\n", argv[0], ShortUsage, Author);
				  fprintf(stderr, "Unknown option '-%c' or option requires argument(s) to follow.\n", optopt);
				  exit(1);
		}
	if( inputFilename[0]=='\0' ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		exit(1);
	}
	if( outputFilename[0]=='\0' ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		exit(1);
	}
	if( nSpecs[A] > 0 ){
		/* the -a option was selected, cancel all other filter specs and copy the a specs to them */
		/* do it for as many -a options were specified */
		for(i=0;i<NUM_FILTER_TYPES;i++){
			if( i == A ) continue;
			for(n=0;n<nSpecs[A];n++){
				SPECS[i][n].a = SPECS[A][n].a;
				SPECS[i][n].b = SPECS[A][n].b;
				SPECS[i][n].c = SPECS[A][n].c;
				SPECS[i][n].d = SPECS[A][n].d;
				SPECS[i][n].lo =SPECS[A][n].lo;
				SPECS[i][n].hi =SPECS[A][n].hi;
				filterOrder[n*(NUM_FILTER_TYPES-1) + i] = i;
			}
			nSpecs_dup[i] = nSpecs[i] = nSpecs[A];
		}
		numFilters = (NUM_FILTER_TYPES-1) * nSpecs[A];
	} else {
		/* how many filter specs in total ? */
		for(i=0;i<NUM_FILTER_TYPES;i++) nSpecs_dup[i] = nSpecs[i];
	}

	/* image dimensions */
	inX = (retainImageDimensions==TRUE ? 0:ROI.a); inY = (retainImageDimensions==TRUE ? 0:ROI.b);
	X = (retainImageDimensions==TRUE ? ROI.a:0); Y = (retainImageDimensions==TRUE ? ROI.b:0);
	inW = (retainImageDimensions==TRUE ? -1:ROI.c); inH = (retainImageDimensions==TRUE ? -1:ROI.d);
	/* inW and inH contain the whole image dimensions, W and H contain the ROI dimensions
	   if we only have to save the ROI (e.g. retainImageDimensions==FALSE) W=inW, H=inH and X=Y=0
	   and we will not load the whole image but just the ROI part, otherwise W<inW and H<inH.
	   So, use inW and inH for allocating data and initialising the output data but use W and H
	   for operating within the ROI */

	if( numSlices == 0 ){
		data = getUNCSlices3D(inputFilename, inX, inY, &inW, &inH, NULL, &numSlices, &depth, &format);
		allSlices = 1;
	} else data = getUNCSlices3D(inputFilename, inX, inY, &inW, &inH, slices, &numSlices, &depth, &format);
	if( data == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for input file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}

	W = ROI.c<=0 ? inW:ROI.c; H = ROI.d<=0 ? inH:ROI.d;
	if( (numSlicesOut=(numFilters+((includeOriginalImage==TRUE)?1:0))*numSlices) < numSlices ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "At least one filter specification must be given - if you are unsure use '-a w:h:W:H' for all map types.\n");
		freeDATATYPE3D(data, numSlices, inW);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlicesOut, inW, inH)) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d x %d(slices) of DATATYPEs, %zd bytes each.\n", argv[0], inW, inH, numSlicesOut, sizeof(DATATYPE));
		freeDATATYPE3D(data, numSlices, inW);
		exit(1);
	}
	for(s=0,sOut=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		/* do some global stats for this slice but only inside ROI */
		if( (pixelFrequencies=histogram2D(data[slice], X, Y, W, H, 1)) == NULL ){
			fprintf(stderr, "%s : call to histogram2D has failed for file '%s', slice was '%d', region of interest was (%d,%d,%d,%d).\n", argv[0], inputFilename, slice, X, Y, W, H);
			freeDATATYPE3D(data, numSlices, inW);
			freeDATATYPE3D(dataOut, numSlicesOut, inW);
			exit(1);
		}			
		mean_stdev2D(data[slice], X, Y, W, H, &globalMean, &globalStdev);
		/* if we show the whole image, and output color ranges not specified, used those of
		   image AND NOT defaults */
		if( showImageOutsideROI && !outputColorRangeSpecified ){ maxOutputColor = pixelFrequencies->maxPixel; minOutputColor = pixelFrequencies->minPixel; }

		min_maxPixel2D(data[slice], X, Y, W, H, &globalMinPixel, &globalMaxPixel);

		/* write the original slice first, if requested */
		if( includeOriginalImage ){
			/* map the original image to min/max colors */
			for(i=X;i<X+W;i++) for(j=Y;j<Y+H;j++)
				dataOut[sOut][i][j] = ROUND(SCALE_OUTPUT((float )(data[slice][i][j]), (float )minOutputColor, (float )maxOutputColor, (float )globalMinPixel, (float )globalMaxPixel));

			/* draw a border around ROI -- for options that do not involve subwindows */
			if( showImageOutsideROI && (borderThickness>0) ){
				for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
				for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
			}
			sOut++;
		}

		for(m=0;m<numFilters;m++){
			if( filterOrder[m] == A ) continue;
			/* initialise the output data, black outside ROI or original image outside ROI, depending on flags */
			if( retainImageDimensions && showImageOutsideROI )
				for(i=inX;i<inW;i++) for(j=inY;j<inH;j++) dataOut[sOut][i][j] = data[slice][i][j];
			currentFilter = filterOrder[m];
			n = nSpecs_dup[currentFilter]-nSpecs[currentFilter]; nSpecs[currentFilter]--;
			param1 = SPECS[currentFilter][n].a; param2 = SPECS[currentFilter][n].b;
			param3 = SPECS[currentFilter][n].c; param4 = SPECS[currentFilter][n].d;
			    lo = SPECS[currentFilter][n].lo;    hi = SPECS[currentFilter][n].hi;
			switch( filterOrder[m] ){
				case SHARPEN: /* [-S level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -S %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					sharpen2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case LAPLACIAN: /* [-L level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -L %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					laplacian2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case SOBEL: /* [-O level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -O %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					sobel2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case MEDIAN: /* [-M level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -M %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					median2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case AVERAGE: /* [-A level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -A %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					average2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case MIN_V: /* [-I level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -I %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					min2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case MAX_V: /* [-X level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -X %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					max2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
				case PREWITT: /* [-P level (sharpen filter with level 'level')] */
					if( verbose ){ printf("file '%s', input slice %d, -P %f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					prewitt2D(data[slice], X, Y, W, H, dataOut[sOut], lo);
					break;
 				default:
					fprintf(stderr, "%s : filter %d not implemented yet...\n", argv[0], filterOrder[m]);
					break;
			} /* end switch */
			if( outputColorRangeSpecified ){
				statistics2D(dataOut[sOut], X, Y, W, H, &d1, &d2, &d3, &d4);
				/* map the image to min/max colors */
				for(i=X;i<X+W;i++) for(j=Y;j<Y+H;j++)
					dataOut[sOut][i][j] = ROUND(SCALE_OUTPUT((float )(dataOut[sOut][i][j]), (float )minOutputColor, (float )maxOutputColor, (float )d1, (float )d2));
			}
			/* draw a border around ROI -- for options that do not involve subwindows */
			if( showImageOutsideROI && (borderThickness>0) ){
				for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
				for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
			}
			if( verbose ){ statistics2D(dataOut[sOut], X, Y, W, H, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
			sOut++;
		} /* for(m=0;m<numFilters;m++) */

		/* initialise specs again, for second slice etc. */
		for(m=0;m<numFilters;m++) nSpecs[filterOrder[m]] = nSpecs_dup[filterOrder[m]];

		free(pixelFrequencies->bins); /* free the histogram, we will do another for next slice */
	} /* for(s=0,sOut=0;s<numSlices;s++) */

	/* done with original data and histograms */
	freeDATATYPE3D(data, numSlices, inW);
	if( pixelFrequencies != NULL ) destroy_histogram(pixelFrequencies);

	/* write images out */
	if( sOut > 0 ){
		if( !writeUNCSlices3D(outputFilename, dataOut, inW, inH, 0, 0, inW, inH, NULL, sOut, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
			freeDATATYPE3D(dataOut, numSlicesOut, inW);
			exit(1);
		}
		/* now copy the image info/title/header of source to destination */
		if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
			freeDATATYPE3D(dataOut, numSlicesOut, inW);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s : no output was produced! Check your maps' specs.\n", argv[0]);
		freeDATATYPE3D(dataOut, numSlicesOut, inW);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlicesOut, inW);

	exit(0);
}
