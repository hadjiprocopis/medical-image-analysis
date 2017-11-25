#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <filters.h>
#include <IO_roi.h>
#include <roi_utils.h>

const   char    Examples[] = "\
	-i input.unc -o output.unc\
";
const   char    Usage[] = "options as follows:\
\t -i inputFilename\
	(ROI file - the same format a dispunc reads and\
	 writes)\
\
\t -o outputFilename\
	(Output filename)\
\
\t[-m\
	(modify the slice number of each roi so as to\
	 reflect the new total number of slices in the\
	 output roi file)]\
	\
\t[-f firstSlice\
	(specify the first slice to be extracted\
	 default is the first slice)]\
\t[-f lastSlice\
	(specify the last slice to be extracted\
	 default is the last slice)]\
\t[-s stepSize\
	(specify a step size when extracting slices\
	 from first to last - default is 1)]\
\
* the following 2 options may be used in conjuction with\
* the 3 options above or independently\
\
\t[-E\
	(extract only even slices - between 'first'\
	 and 'last' and with step size specified.)]\
\t[-O\
	(extract only odd slices - between 'first'\
	 and 'last' and with step size specified.)]\
	\
\t[-x pixel_size_x\
        (define the size of pixels in the x-direction)]\
\t[-y pixel_size_y\
        (define the size of pixels in the y-direction)]\
\t[-z pixel_size_z\
        (define the size of pixels in the z-direction)]\
\
";

const   char    Author[] = "Andreas Hadjiprocopis, NMR, ION, 2002";
			
int	main(int argc, char **argv){
	int		optI;
	char		*inputFilename = NULL,
			*outputFilename = NULL;

	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0;
	roi		**inputRois, **outputRois;
	int		numInputRois, numOutputRois;

	int		step = 1, first = 0, last = -1, maxZ, i,
			sliceConversion[1000], currentSlice;
	char		doOdd = FALSE, doEven = FALSE, doModifySliceNumbers = FALSE;

	while( (optI=getopt(argc, argv, "i:o:x:y:s:OEf:l:m")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': step = atoi(optarg); break;
			case 'E': doEven = TRUE; doOdd = FALSE; break;
			case 'O': doOdd = TRUE; doEven = FALSE; break;
			case 'f': first = atoi(optarg); break;
			case 'l': last = atoi(optarg); break;
			case 'm': doModifySliceNumbers = TRUE; break;

                        case 'x': pixel_size_x = atof(optarg); break;
                        case 'y': pixel_size_y = atof(optarg); break;
                        case 'z': pixel_size_z = atof(optarg); break;

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

	if( read_rois_from_file(inputFilename, &inputRois, &numInputRois) != 0 ){
		fprintf(stderr, "%s : call to read_rois_from_file has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	maxZ = -1;
	for(i=0;i<numInputRois;i++) if( inputRois[i]->slice > maxZ ) maxZ = inputRois[i]->slice;
	if( last == -1 ) last = maxZ;
	else if( last > maxZ ){
		fprintf(stderr, "%s : the last slice specified should not exceed %d, the max slice number of all rois.\n", argv[0], maxZ);
		free(inputFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		exit(1);
	}

	/* allocate max number of output rois */
	if( (outputRois=(roi **)malloc(numInputRois*sizeof(roi *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for outputRois (%d rois).\n", argv[0], numInputRois*sizeof(roi *), numInputRois);
		free(inputFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		exit(1);
	}
	for(i=0;i<1000;i++) sliceConversion[i] = -1; currentSlice = 0;
	numOutputRois = 0;
	printf("%s, %s:", argv[0], inputFilename); fflush(stdout);
	if( doOdd || doEven ){
		for(i=0;i<numInputRois;i++)
			if( (((inputRois[i]->slice+1)%2)==doOdd) &&
			    ((inputRois[i]->slice+1)>=first) &&
			    ((inputRois[i]->slice+1)<=last) &&
			    (((inputRois[i]->slice+1-first)%step) == 0)
			){
				outputRois[numOutputRois++] = inputRois[i];
				printf(" %d", inputRois[i]->slice+1); fflush(stdout);
				if( sliceConversion[inputRois[i]->slice] == -1 )
					sliceConversion[inputRois[i]->slice] = currentSlice++;
			}
	} else
		for(i=0;i<numInputRois;i++)
			if( ((inputRois[i]->slice+1)>=first) &&
			    ((inputRois[i]->slice+1)<=last) &&
			    (((inputRois[i]->slice+1-first)%step) == 0)
			){
				outputRois[numOutputRois++] = inputRois[i];
				printf(" %d", inputRois[i]->slice+1); fflush(stdout);
				if( sliceConversion[inputRois[i]->slice] == -1 )
					sliceConversion[inputRois[i]->slice] = currentSlice++;
			}
	printf("\n");
	/* done */

	if( doModifySliceNumbers )
		for(i=0;i<numOutputRois;i++)
			outputRois[i]->slice = sliceConversion[outputRois[i]->slice];
	

	if( !write_rois_to_file(outputFilename, outputRois, numOutputRois, pixel_size_x, pixel_size_y) ){
		fprintf(stderr, "%s : call to write_rois_to_file has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		rois_destroy(outputRois, numOutputRois);
		exit(1);
	}

	rois_destroy(inputRois, numInputRois);
	free(outputRois); /* not destroy because is a soft copy of inputRois */
	free(inputFilename); free(outputFilename);

	exit(0);
}
