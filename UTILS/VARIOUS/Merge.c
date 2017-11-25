#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	READ_BUFFER_SIZE	10000
#define	WRITE_BUFFER_SIZE	1000000

#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif

int	main(int argc, char **argv){
	FILE	**fp;
	char	*bufferIn,
		*bufferOut,
		continueFlag, *p;
	int	i, j, numFiles, totalStrlen;

	if( argc < 3 ){
		fprintf(stderr, "Usage : %s file1 file2 [file3 ... fileN]\n", argv[0]);
		fprintf(stderr, "This program will take the contents of two or\n");
		fprintf(stderr, "more text files and place them side by side.\n");
		fprintf(stderr, "separated by a single tab.\n");
		fprintf(stderr, "The number of output lines will be equal to the\n");
		fprintf(stderr, "minimum of the number of lines of all input files\n");
		exit(1);
	}

	if( (fp=(FILE **)malloc((argc-1)*sizeof(FILE *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for fp.\n", argv[0], (argc-1)*sizeof(FILE *));
		exit(1);
	}
	if( (bufferIn=(char *)malloc((READ_BUFFER_SIZE+1)*sizeof(char))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for bufferIn.\n", argv[0], (READ_BUFFER_SIZE+1)*sizeof(char));
		free(fp);
		exit(1);
	}
	if( (bufferOut=(char *)malloc((WRITE_BUFFER_SIZE+1)*sizeof(char))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for bufferOut.\n", argv[0], (WRITE_BUFFER_SIZE+1)*sizeof(char));
		free(fp); free(bufferIn);
		exit(1);
	}
	for(i=1;i<argc;i++){
		if( (fp[i-1]=fopen(argv[i], "r")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for reading.\n", argv[0], argv[i]);
			for(j=0;j<(i-1);j++) fclose(fp[j]);
		}
	}
	numFiles = argc - 1;

	continueFlag = TRUE;
	while( continueFlag ){
		bufferOut[0] = '\0';
		for(i=0,totalStrlen=0;i<numFiles;i++){
			fgets(bufferIn, READ_BUFFER_SIZE, fp[i]);
			if( feof(fp[i]) ){ continueFlag = FALSE; break; }
			/* remove a newline (at the end) */
			if( (p=strrchr(bufferIn, '\n')) != NULL ) *p = '\0';
			totalStrlen += strlen(bufferIn) + 1;
			if( totalStrlen >= WRITE_BUFFER_SIZE ){
				fprintf(stderr, "%s : write-buffer overflow detected, increase the size of WRITE_BUFFER_SIZE (now is %d) and recompile.\n", argv[0], WRITE_BUFFER_SIZE);
				for(j=0;j<numFiles;j++) fclose(fp[j]); free(fp);
				free(bufferIn); free(bufferOut);
				exit(1);
			}
			strcat(bufferOut, bufferIn);
			if( i != (numFiles-1) ) strcat(bufferOut, "\t");
		}
		if( continueFlag ) fprintf(stdout, "%s\n", bufferOut);
	}

	for(j=0;j<numFiles;j++) fclose(fp[j]); free(fp);
	free(bufferIn); free(bufferOut);

	return 0;
}
