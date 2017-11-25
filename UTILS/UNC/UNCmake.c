#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename1 -i inputFilename2 [-i inputFilename3 [...]]\
\n	(two or more UNC files with 1 or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
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

int	main(int argc, char **argv){
	DATATYPE	**data[1000], ***dummy;
	char		*inputFilename[100], *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	int		numInputFilenames = 0, oW = -1, oH = -1, i, inp,
			totalNumSlices;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename[numInputFilenames++] = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
			
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( numInputFilenames < 2 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "At least two input filenames must be specified.\n");
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		for(i=0;i<numInputFilenames;i++) free(inputFilename[i]);
		exit(1);
	}

	for(inp=0,totalNumSlices=0;inp<numInputFilenames;inp++){
		W = H = actualNumSlices = -1; numSlices = 0;
		if( (dummy=getUNCSlices3D(inputFilename[inp], 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
			exit(1);
		}
		if( oW == -1 ){ oW = W; oH = H; }
		else {
			if( (oW != W) || (oH != H) ){
				fprintf(stderr, "%s : slices in '%s' have different size (%dx%d) than previous's (%dx%d).\n", argv[0], inputFilename[inp], W, H, oW, oH);
				for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
				for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], oW);
				freeDATATYPE3D(dummy, actualNumSlices, W);
				exit(1);
			}
		}
		if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
		else {
			for(s=0;s<numSlices;s++){
				if( slices[s] >= actualNumSlices ){
					fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename[inp]);
					for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
					for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], W);
					freeDATATYPE3D(dummy, actualNumSlices, W);
					exit(1);
				} else if( slices[s] < 0 ){
					fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
					for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
					for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], W);
					freeDATATYPE3D(dummy, actualNumSlices, W);
					exit(1);
				}
			}
		}
		if( w <= 0 ) w = W; if( h <= 0 ) h = H;
		if( (x+w) > W ){
			fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for file '%s'.\n", argv[0], x, w, W, inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
			for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], oW);
			freeDATATYPE3D(dummy, actualNumSlices, W);
			exit(1);
		}
		if( (y+h) > H ){
			fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for file '%s'.\n", argv[0], y, h, H, inputFilename[inp]);
			for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
			for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], oW);
			freeDATATYPE3D(dummy, actualNumSlices, W);
			exit(1);
		}
		//printf("%s : %d slices\n", inputFilename[inp], numSlices);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			data[totalNumSlices++] = dummy[slice];
		}
	}

	if( !writeUNCSlices3D(outputFilename, data, w, h, x, y, w, h, NULL, totalNumSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
		for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], W);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename[0], outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename[0], outputFilename);
		for(i=numInputFilenames;i-->0;){ free(inputFilename[i]); }
		free(outputFilename);
		exit(1);
	}


	for(i=0;i<numInputFilenames;i++) free(inputFilename[i]); free(outputFilename);
	for(i=0;i<totalNumSlices;i++) freeDATATYPE2D(data[i], W);
	exit(0);
}
