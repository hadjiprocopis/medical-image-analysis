/* headconv.c - converts SPM / analyze headers to/from DEC/Intel byte order */

#include <stdio.h>
#include <memory.h>

/* Header define from Analyze authors */
#include "dbh.h"
#include "image.h"

#define A_HEADER_SIZE	348

/* function headers */
int iconv( int  );
short int siconv( short int );
float fconv( float fOld );

/* Designed to use wildcard command line inputs, expanded by a
microsoft extension to standard call - e.g. headconv *.hdr
will expand to headconv image1.hdr image2.hdr etc */

int main( int argc, char *argv[ ], char *envp[ ] )
{
	struct dsr dsrOrig;
	struct dsr dsrNew;
	int i, iF;
	short int sint_ptr[5];
	FILE* fp;

	if (argc < 2) {
		printf("Format is: %s <source header>\n\n",	argv[0]);
		printf("%s converts analyze headers from (e.g.) Sun format\n"
			"to DEC / Intel format by byte reversal of integers etc.\n\n"
			"It accepts as input a filename or wildcard string which identifies\n"
			"analyze headers, and _replaces_ the original header with a converted\n"
			"header of the same name.\n\n", argv[0]);
		return(1);
	} else {	
		for (iF = 1; iF < argc; iF++) {		/* loop through files */
			fp = fopen(argv[iF], "rb");
			if (fp == NULL) {
				printf("\n\nCan't open file %s to read header\n\n", argv[iF]);
				return(1);
			} else { /* file ready to read */
				i = fread(&dsrOrig, A_HEADER_SIZE, 1, fp);
				fclose(fp);

				/* copy to new header buffer - for no real reason */
				dsrNew = dsrOrig;

				/* convert data */
				dsrNew.hk.sizeof_hdr = iconv(dsrNew.hk.sizeof_hdr);
				dsrNew.hk.extents = iconv(dsrNew.hk.extents);
				dsrNew.hk.session_error = siconv(dsrNew.hk.session_error);

				for (i=0; i<8; i++)
					dsrNew.dime.dim[i] = siconv(dsrNew.dime.dim[i]);
				dsrNew.dime.unused1 = siconv(dsrNew.dime.unused1);
				dsrNew.dime.datatype = siconv(dsrNew.dime.datatype);
				dsrNew.dime.bitpix = siconv(dsrNew.dime.bitpix);
				dsrNew.dime.dim_un0 = siconv(dsrNew.dime.dim_un0);

				for (i=0; i<8; i++)
					dsrNew.dime.pixdim[i] = fconv(dsrNew.dime.pixdim[i]);
				dsrNew.dime.vox_offset = fconv(dsrNew.dime.vox_offset);
				dsrNew.dime.funused1 = fconv(dsrNew.dime.funused1);
				dsrNew.dime.funused2 = fconv(dsrNew.dime.funused2);
				dsrNew.dime.funused3 = fconv(dsrNew.dime.funused3);
				dsrNew.dime.cal_max = fconv(dsrNew.dime.cal_max);
				dsrNew.dime.cal_min = fconv(dsrNew.dime.cal_min);

				dsrNew.dime.compressed = iconv(dsrNew.dime.compressed);
				dsrNew.dime.verified = iconv(dsrNew.dime.verified);
				dsrNew.dime.glmax = iconv(dsrNew.dime.glmax);
				dsrNew.dime.glmin = iconv(dsrNew.dime.glmin);

				/* spm makes odd use of origin field */
				memcpy(sint_ptr, &(dsrNew.hist.originator), 10);
				for (i=0; i<5; i++)
					sint_ptr[i] = siconv(sint_ptr[i]); 
				memcpy(&(dsrNew.hist.originator), sint_ptr, 10);		

				dsrNew.hist.views = iconv(dsrNew.hist.views);
				dsrNew.hist.vols_added = iconv(dsrNew.hist.vols_added);
				dsrNew.hist.start_field = iconv(dsrNew.hist.start_field);
				dsrNew.hist.field_skip = iconv(dsrNew.hist.field_skip);
				dsrNew.hist.omax = iconv(dsrNew.hist.omax);
				dsrNew.hist.omin = iconv(dsrNew.hist.omin);
				dsrNew.hist.smax = iconv(dsrNew.hist.smax);
				dsrNew.hist.smin = iconv(dsrNew.hist.smin);

				fp = fopen(argv[iF], "wb");
				if (fp == NULL) {
					printf("\n\nCan't reopen file %s to write header\n\n", argv[iF]);
					return(1);
				} else {	/* file ready to write */
					i = fwrite(&dsrNew, A_HEADER_SIZE, 1, fp);
					fclose(fp);
					printf("Written header %s\n", argv[iF]);
				}
			} /* if file readable */
		} /* for file loop */
	printf("\n\n%s has written %d header(s).\n\n", argv[0], --iF);
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
