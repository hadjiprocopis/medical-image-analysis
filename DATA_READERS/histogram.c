#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "filters.h"

extern	double	*parse_histogram_data(char *, int *);

/* Function to read a histogram file
   params: filename, the name of the file that contains the data (or null for stdin)

   returns: a pointer to a histogram structure if successful or NULL otherwise
   author: Andreas Hadjiprocopis, NMR, ION, 2001
*/
histogram	*read_histogram_from_file(char *filename){
	double		*data;
	histogram	*ret;
	DATATYPE	*dummy;
	int		i, j, k, numPixels, numBins;

	if( (data=parse_histogram_data(filename, &numBins)) == NULL ){
		fprintf(stderr, "read_histogram_from_file : call to parse_histogram_data has failed for file '%s'.\n", filename==NULL?strdup("<stdin>"):filename);
		return NULL;
	}

	/* a stupid thing to do but simple: we will create an image which will
	   contain the same pixel distribution as the 'data', then call histogram1D routine */
	for(i=1,numPixels=0;i<numBins;i+=2) numPixels += data[i];
	if( (dummy=(DATATYPE *)malloc(numPixels * sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "read_histogram_from_file : could not allocate %zd bytes for DATATYPE.\n", numPixels * sizeof(DATATYPE));
		free(data);
		return NULL;
	}
	k = 0;
	for(i=1;i<numBins;i+=2)
		for(j=0;j<data[i];j++) dummy[k++] = (DATATYPE )(data[i-1]);

	if( (ret=histogram1D(dummy, 0, numPixels, 1)) == NULL ){
		fprintf(stderr, "read_histogram_from_file : call to histogram1D has failed.\n");
		free(data); free(dummy);
		return NULL;
	}
	free(data); free(dummy);
	return ret;
}
