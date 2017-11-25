/* imgconv.c - converts analyze 16 bit images to/from DEC/Intel byte order */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Header define from Analyze authors */
#include "dbh.h"

#define A_HEADER_SIZE	348

/* function headers */
int iconv( int  );
short int siconv( short int );
float fconv( float fOld );

/* Designed to use wildcard command line inputs, expanded by a
microsoft extension to standard call - e.g. imgconv *.img
will expand to imgconv image1.img image2.img etc */

int main( int argc, char *argv[ ], char *envp[ ] )
{
	struct stat sbuf;
	int i, iF;
	short int *sint_ptr;
	FILE* fp;

	if (argc < 2) {
		printf("Format is: %s <source image>\n\n",	argv[0]);
		printf("%s converts analyze images from (e.g.) Sun format\n"
			"to DEC / Intel format by byte reversal of 16 bit integers.\n\n"
			"It accepts as input a filename or wildcard string which identifies\n"
			"analyze images, and _replaces_ the original image(s) with a converted\n"
			"image of the same name.\n\n", argv[0]);
		return(1);
	} else {	
		for (iF = 1; iF < argc; iF++) {		/* loop through files */
			fp = fopen(argv[iF], "rb");
			if (fp == NULL) {
				printf("\n\nCan't open file %s to read image\n\n", argv[iF]);
				return(1);
			} else { /* file ready to read */
				if (stat(argv[iF], &sbuf)) {
					printf("\n\nCan't stat file %s\n\n", argv[iF]);
					return(1);
				}
				sint_ptr = malloc(sbuf.st_size);
				i = fread(sint_ptr, (size_t)sizeof(char), (size_t)sbuf.st_size, fp);

				printf("Read %d bytes from file of size %lld\n\n", i, sbuf.st_size);

				fclose(fp);

				for (i=0; i< (sbuf.st_size / 2); i++)
					sint_ptr[i] = siconv(sint_ptr[i]);

				fp = fopen(argv[iF], "wb");
				if (fp == NULL) {
					printf("\n\nCan't reopen file %s to write image\n\n", argv[iF]);
					return(1);
				} else {	/* file ready to write */
					i = fwrite(sint_ptr, 1, sbuf.st_size, fp);
					fclose(fp);
					printf("Written image %s\n", argv[iF]);
				}
			} /* if file readable */
		} /* for file loop */
	printf("\n\n%s has written %d images(s).\n\n", argv[0], --iF);
	return 0;
	}	/* else arg count was OK */
}



/* converts integer */
int iconv( int iOld )
{
	char* pcOrig;
	char* pcNew;
	int i, j, iNew;

	pcOrig = (char *) &iOld;
	pcNew = (char *) &iNew;

	j = sizeof(int)-1;
	for (i = 0; i<sizeof(int); i++) {
		pcNew[j] = pcOrig[i];
		j--;
		}	

	return iNew;
}

/* converts short integer */
short int siconv( short int iOld )
{
	char* pcOrig;
	char* pcNew;
	short int iNew;

	pcOrig = (char *) &iOld;
	pcNew = (char *) &iNew;

	pcNew[0] = pcOrig[1];
	pcNew[1] = pcOrig[0];

	return iNew;
}

/* converts float */
float fconv( float fOld )
{
	char* pcOrig;
	char* pcNew;
	int i, j;
	float fNew;

	pcOrig = (char *) &fOld;
	pcNew = (char *) &fNew;

	j = sizeof(float)-1;
	for (i = 0; i<sizeof(float); i++) {
		pcNew[j] = pcOrig[i];
		j--;
		}	

	return fNew;
}
