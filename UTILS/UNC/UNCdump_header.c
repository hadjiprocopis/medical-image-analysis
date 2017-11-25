#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-i input.unc -o header.txt\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename of text file to contain the header)\
\n\
\nThis program will dump the header information of the\
\ninput file to the output file which will be a text file\
\nand contain a line for each entry in the header.\
\nThe line will be of the form:\
\n     name=value\
\nThe operation of this program is identical to 'header -i'\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	char		*inputFilename = NULL, *outputFilename = NULL;
	int		optI;
	IMAGE		*im;
	int		numEntries;
	FILE		*fp;

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
	if( (im=imopen(inputFilename, READ)) == NULL ){
		fprintf(stderr, "%s : call to imopen has failed for UNC image file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( (fp=fopen(outputFilename, "w")) == NULL ){
		fprintf(stderr, "%s : could not open output file '%s' for writing.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	if( (numEntries=writeUNCInfoToTextFile(fp, im->file_info)) < 0 ){
		fprintf(stderr, "%s : call to writeUNCInfoToTextFile has failed.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	if( numEntries == 0 ){		
		fprintf(stderr, "%s : something wrong with header of UNC image file '%s' - wrote 0 entries!!!\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	fprintf(stdout, "%s : wrote %d header entries of UNC image file '%s'.\n", argv[0], numEntries, inputFilename);
	free(inputFilename); free(outputFilename);
	exit(0);
}
