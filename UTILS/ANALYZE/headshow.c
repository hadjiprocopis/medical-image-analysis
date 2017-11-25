/* headshow.c - details from  SPM / analyze headers */

#include <stdio.h>
#include <memory.h>

/* Header define from Analyze authors */
#include "dbh.h"

#define A_HEADER_SIZE	348

/* Designed to use wildcard command line inputs, expanded by a
microsoft extension to standard call - e.g. headshow *.hdr
will expand to headshow image1.hdr image2.hdr etc.  This expansion 
requires add of setargv.obj to link in Visual C++*/

int main( int argc, char *argv[ ], char *envp[ ] )
{
	struct dsr dsrNew;
	int i, iF;
	FILE* fp;
	short int sint_ptr[5];

	if (argc <  2) {
		printf("Format is: %s <header>\n\n",	argv[0]);
		printf("%s shows information from analyze headers .\n\n", argv[0]);
		return(1); 
	} else {	
		for (iF = 1; iF < argc; iF++) {		/* loop through files */
			fp = fopen(argv[iF], "rb");
			if (fp == NULL) {
				printf("\n\nCan't open file %s to read header\n\n", argv[iF]);
				return(1);
			} else { /* file ready to read */
				i = fread(&dsrNew, A_HEADER_SIZE, 1, fp);
				fclose(fp);

				/* spm makes odd use of origin field */
				memcpy(sint_ptr, &(dsrNew.hist.originator), 10);

				/* Show data */
				printf("Data for header %s \n\n"
					"Datatype: %d\n"
					"Vox units:%s\n"
					"PixDim X: %f\n"
					"PixDim Y: %f\n"
					"PixDim Z: %f\n"
					"Dim X:    %d\n"
					"Dim Y:    %d\n"
					"Dim Z:    %d\n"
					"Descrip:  %s\n"
					"Origin X: %d\n"
					"Origin Y: %d\n"
					"Origin Z: %d\n"
					"Scale:    %f\n",
					argv[iF],
					dsrNew.dime.datatype,
					dsrNew.dime.vox_units,
					dsrNew.dime.pixdim[1],
					dsrNew.dime.pixdim[2],
					dsrNew.dime.pixdim[3],
					dsrNew.dime.dim[1],
					dsrNew.dime.dim[2],
					dsrNew.dime.dim[3],
					dsrNew.hist.descrip,
					sint_ptr[0],
					sint_ptr[1],
					sint_ptr[2],
					dsrNew.dime.funused1
					);

			} /* if file readable */
		} /* for file loop */
	printf("\n\n");
	return 0;
	}	/* else arg count was OK */
}



