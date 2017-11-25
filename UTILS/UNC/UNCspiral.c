#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* what is the max number of specs ? */
#define MAX_SPECS       30


const	char	Examples[] = "\
\n	-i inp.unc -o out.unc ";

const	char	Usage[] = "options as follows:\
\n -o outputFilename\
\n	(Output filename)\
\n\
\n[-i inputFilename\
\n	(optional input file to superimpose the\
\n	 spiral onto it. If this is supplied,\
\n	 the '-d' option below - dimensions -\
\n	 will be ignored.)]\
\n\
\n[-d W:H:S\
\n	(dimensions of the output image.\
\n	 W: width, H: height, S: number of slices.\
\n	 Default is 128 x 128 pixels and 1 slice.)]\
\n\
\n[-c X:Y\
\n	(centre of the spiral at (X, Y).\
\n	 Default is at (W/2,H/2).)]\
\n\
\n[-s L:R:U:D\
\n	(step of the spiral in left (L), right (R),\
\n	 up (U) and down (D). Default is 1 in each direction.)]\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

enum	{LEFT=0, RIGHT=1, UP=2, DOWN=3};

int	main(int argc, char **argv){
	DATATYPE	***dataOut, ***data = NULL;
	char		*outputFilename = NULL, *inputFilename = NULL;
	int		format = 8, optI, actualNumSlices = 1, depth;
	int		W = 128, H = 128, centerX = -1, centerY = -1,
			step[4] = {2, 2, 2, 2}, limit[4] = {1, 2, 2, 1};
	register int	i, j, x, y, s;
	char		shouldContinue;

	while( (optI=getopt(argc, argv, "i:o:ec:d:s:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'c': sscanf(optarg, "%d:%d", &centerX, &centerY); break;
			case 'd': sscanf(optarg, "%d:%d:%d", &W, &H, &actualNumSlices); break;
			case 's': sscanf(optarg, "%d:%d:%d:%d", &(step[LEFT]), &(step[RIGHT]), &(step[UP]), &(step[DOWN])); break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		exit(1);
	}
	if( inputFilename != NULL ){
		W = H = actualNumSlices = -1;
		if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
	}

	if( (dataOut=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %dx%d DATATYPEs of size %zd bytes.\n", argv[0], W, H, sizeof(DATATYPE));
		free(outputFilename);
		exit(1);
	}

	if( inputFilename != NULL ){
		for(s=0;s<actualNumSlices;s++) for(x=0;x<W;x++) for(y=0;y<H;y++) dataOut[s][x][y] = data[s][x][y];
		freeDATATYPE3D(data, actualNumSlices, W);
	} else for(s=0;s<actualNumSlices;s++) for(x=0;x<W;x++) for(y=0;y<H;y++) dataOut[s][x][y] = 0;


	shouldContinue = TRUE;
	if( centerX == -1 ) centerX = W / 2;
	if( centerY == -1 ) centerY = H / 2;
	for(s=0;s<actualNumSlices;s++){
		shouldContinue = TRUE;
		limit[LEFT] = 1; limit[RIGHT] = 2;
		limit[UP] = 2; limit[DOWN] = 1;
		j = 10; x = centerX; y = centerY;
		while( TRUE ){
			for(i=0;i<limit[DOWN];i++){ y++; if( y >= H ){ shouldContinue = FALSE; break; } dataOut[s][x][y] += j; } /* down */
			if( !shouldContinue ) break;
			for(i=0;i<limit[LEFT];i++){ x--; if( x < 0 ){ shouldContinue = FALSE; break; } dataOut[s][x][y] += j; } /* left */
			if( !shouldContinue ) break;
			for(i=0;i<limit[UP];i++){   y--; if( y < 0 ){ shouldContinue = FALSE; break; } dataOut[s][x][y] += j; } /* up   */
			if( !shouldContinue ) break;
			for(i=0;i<limit[RIGHT];i++){x++; if( x >= W ){ shouldContinue = FALSE; break; } dataOut[s][x][y] += j; } /* right*/
			if( !shouldContinue ) break;

//			if( inputFilename == NULL )
				if( j++ > 32000 ) j = 0;
			
			for(i=0;i<4;i++) limit[i] += step[i];
		}
	}
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, actualNumSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, 1, W);
		exit(1);
	}

	free(outputFilename);
	freeDATATYPE3D(dataOut, actualNumSlices, W);

	exit(0);
}
