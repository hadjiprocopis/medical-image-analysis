#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-h header.txt -i image.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i uncFilename\
\n	(filename of already existing UNC image\
\n       to set its header to the contents of the\
\n       input header file)\
\n\
\n\t -h headerFilename\
\n	(Text file containing a UNC header)\
\n\
\nThis program will read the header information\
\nin the file specifed and modify the header of\
\nthe UNC image accordingly\
\nThe input header file should be a text file where each\
\nline is of the form:\
\n     name=value\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	char		*headerFilename = NULL, *uncFilename = NULL;
	int		optI;
	IMAGE		*im;
	int		numEntries;
	FILE		*fp;

	while( (optI=getopt(argc, argv, "i:h:e")) != EOF)
		switch( optI ){
			case 'i': uncFilename = strdup(optarg); break;
			case 'h': headerFilename = strdup(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( headerFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( uncFilename != NULL ) free(uncFilename);
		exit(1);
	}
	if( uncFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(headerFilename);
		exit(1);
	}
	if( (im=imopen(uncFilename, UPDATE)) == NULL ){
		fprintf(stderr, "%s : call to imopen has failed for UNC image file '%s'.\n", argv[0], uncFilename);
		free(headerFilename); free(uncFilename);
		exit(1);
	}
	if( (fp=fopen(headerFilename, "r")) == NULL ){
		fprintf(stderr, "%s : could not open header file '%s' for reading.\n", argv[0], headerFilename);
		free(headerFilename); free(uncFilename);
		exit(1);
	}
	if( (im->file_info=readUNCInfoFromTextFile(fp, NULL, NULL, &numEntries)) < 0 ){
		fprintf(stderr, "%s : call to readUNCInfoFromTextFile has failed for header file '%s'.\n", argv[0], headerFilename);
		free(headerFilename); free(uncFilename);
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	if( numEntries == 0 ){		
		fprintf(stderr, "%s : something wrong with header file '%s' - read 0 entries!!!\n", argv[0], headerFilename);
		free(headerFilename); free(uncFilename);
		exit(1);
	}
	imclose(im);
	fprintf(stdout, "%s : read %d header entries from file '%s' and modified '%s' accordingly.\n", argv[0], numEntries, headerFilename, uncFilename);
	free(headerFilename); free(uncFilename);
	exit(0);
}
