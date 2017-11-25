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
\nThis program will copy the header information of the\
\ninput file to the output file. Both UNC files must exist.\
\nThis program is equivalent to the utility `cpiminfo'.\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	char		*inputFilename = NULL, *outputFilename = NULL;
	int		optI;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
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

	if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	free(inputFilename); free(outputFilename);
	exit(0);
}
