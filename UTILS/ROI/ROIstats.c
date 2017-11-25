#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
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
\t[-u uncFile\
	(optional UNC file which is associated with the input\
	 roi file. If supplied, it will dump information about\
	 mean and stdev of pixel values within each roi.)]\
\t[-p\
	(flag to denote that instead of writing general stats\
	 calculated over all the points inside the ROI, it should\
	 write location information (only) about each pixel on\
	 the periphery of the ROI)]\
\
\t[-x pixel_size_x\
	(define the size of pixels in the x-direction)]\
\t[-y pixel_size_y\
	(define the size of pixels in the y-direction)]\
\t[-z pixel_size_z\
	(define the size of pixels in the z-direction)]\
\
This program will take a roi file and calculate location stats:\
	(centroid_x,centroid_y) (num_points_inside) bounding_box(x,y,w,h)\
for each roi in that file.\
If a UNC image volume is also supplied with the '-u' option,\
pixel information will also be written:\
	(min,max,mean,stdev of pixels inside)\
\
Alternatively, if the '-p' flag is used, only information about\
points on the periphery of each roi will be written:\
(x, y) (r, theta) (dx, dy, dy/dx), (dr, dtheta)\
\
A comment at the beginning of the output will contain information\
about the fields and the files used.\
Additionally, the stats for each roi will be separated with a\
comment line containing the name of the roi.\
";

const   char    Author[] = "Andreas Hadjiprocopis, NMR, ION, 2002";
			
int	main(int argc, char **argv){
	int		optI;
	char		*inputFilename = NULL, *inputUNCFilename = NULL,
			*outputFilename = NULL;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0,
			*dummy;

	roi		**inputRois;
	int		numInputRois;

	DATATYPE	***data = NULL;
	int		depth, format, actualNumSlices = 0, W = -1, H = -1;
	char		writeInfoForEachROIPeripheralPoint = FALSE;

	while( (optI=getopt(argc, argv, "i:o:x:y:z:u:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'p': writeInfoForEachROIPeripheralPoint = TRUE; break;
			case 'x': pixel_size_x = atof(optarg); break;
			case 'y': pixel_size_y = atof(optarg); break;
			case 'z': pixel_size_z = atof(optarg); break;
			case 'u': inputUNCFilename = strdup(optarg); break;
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
	if( !rois_calculate(inputRois, numInputRois) ){
		fprintf(stderr, "%s : call to rois_calculate has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		exit(1);
	}

	if( inputUNCFilename != NULL ){
		if( (data=getUNCSlices3D(inputUNCFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputUNCFilename);
			free(inputFilename); free(outputFilename); rois_destroy(inputRois, numInputRois);
			exit(1);
		}
		if( (dummy=malloc(W*H*sizeof(float))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for dummy.\n", argv[0], W*H*sizeof(float));
			free(inputFilename); free(outputFilename); rois_destroy(inputRois, numInputRois);
			exit(1);
		}
		rois_associate_with_unc_volume(inputRois, numInputRois, data, dummy);
		free(dummy); freeDATATYPE3D(data, actualNumSlices, W);
		free(inputUNCFilename);
	}
		
	if( !write_rois_stats_to_file(outputFilename, inputRois, numInputRois, writeInfoForEachROIPeripheralPoint) ){
		fprintf(stderr, "%s : call to write_rois_to_file has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		exit(1);
	}

	rois_destroy(inputRois, numInputRois);
	free(inputFilename); free(outputFilename);
	exit(0);
}
