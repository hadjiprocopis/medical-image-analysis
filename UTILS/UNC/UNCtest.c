#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\nThis program produces a UNC file of size 256x256, of\
\n1 slice. The pixel intensity of the voxel at (0,0,0)\
\nis -32768 and increases from left to right and from top\
\nto bottom to 32767. The resultant file may be used to\
\ntest that reading and writing UNC files across platforms\
\nis consistent.\
\nTo see if two UNC files are the same use the command\
\nUNCdiff -i first.unc -n second.unc\
\n"; 

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***dataOut;
	char		*outputFilename = NULL;
	int		W = 256, H = 256, numSlices = 1,
			format = 8, optI, i, j, k;

	while( (optI=getopt(argc, argv, "o:")) != EOF)
		switch( optI ){
			case 'o': outputFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( outputFilename == NULL ){
		fprintf(stderr, "%s : an output filename is required.\n", argv[0]);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		exit(1);
	}

	k = 0;
	for(j=0;j<H;j++) for(i=0;i<W;i++)
		dataOut[0][i][j] = k++ - 32768;

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, 1, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W);

	exit(0);
}
