#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-9\
\n	(tell the program to copy the header/title information\
\n	from the input file to the output files. If there is\
\n	more than 1 input file, then the information is copied\
\n	from the first file.)]\
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
	DATATYPE	***dataOut;
	char		*outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			format = 8,
			optI, slices[1000], allSlices = 0;
	int		i;

	while( (optI=getopt(argc, argv, "o:es:w:h:x:y:9")) != EOF)
		switch( optI ){
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case '9': copyHeaderFlag = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		exit(1);
	}
	numSlices = 1; w = 12; h = 1;
	if( (dataOut=callocDATATYPE3D(numSlices, w, h)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(outputFilename);
		exit(1);
	}
	for(i=0;i<w/3;i++) dataOut[0][i][0] = 75 + lrand48() % 50;
	for(i=w/3;i<2*w/3;i++) dataOut[0][i][0] = 175 + lrand48() % 50;
	for(i=2*w/3;i<w;i++) dataOut[0][i][0] = 275 + lrand48() % 50;

	if( !writeUNCSlices3D(outputFilename, dataOut, w, h, 0, 0, w, h, (allSlices==0) ? slices : NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(outputFilename);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, w);

	free(outputFilename);
	exit(0);
}
