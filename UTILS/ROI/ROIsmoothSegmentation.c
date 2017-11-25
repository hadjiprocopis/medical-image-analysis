#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <filters.h>
#include <IO.h>
#include <Alloc.h>
#include <roi_utils.h>

const   char    Examples[] = "\
	-i input.unc -o output.unc\
";
const   char    Usage[] = "options as follows:\
\t -r ROIinputFilename\
	(ROI file - the same format a dispunc reads and\
	 writes)\
\t -u UNCinputFilename\
	(UNC image file associated with the ROI file specified\
         - the same format a dispunc reads and writes)\
\
\t -o outputBasename\
	(Output filename)\
\
\t -W min:max\
	(The white matter pixels intensity threshold from min to max)\
\t -G min:max\
	(The grey matter pixels intensity threshold from min to max)\
\t[-p P\
	(Optionally specify the maximum area - in pixels -\
	 of any region that is moved from WM to GM to CSF)\
	\
\t[-x pixel_size_x\
	(define the size of pixels in the x-direction)]\
\t[-y pixel_size_y\
	(define the size of pixels in the y-direction)]\
\t[-z pixel_size_z\
	(define the size of pixels in the z-direction)]\
\
\
This program will take a UNC image file and a ROI file marking\
all objects in the image. Given the two thresholds for WM and\
GM, and an optional upper limit of the area of objects to be\
moved, objects will be arranged in three output ROI files\
for WM, GM and CSF.\
";

const   char    Author[] = "Andreas Hadjiprocopis, NMR, ION, 2002";

enum {WM=0, GM=1, CSF=2, DEF=3};

const char *outputExtension[4] = {"WM", "GM", "CSF", "DEFAULT"};

int	main(int argc, char **argv){
	DATATYPE	***data, *dummy, p;
	int		optI;
	char		*inputROIFilename = NULL, *inputUNCFilename = NULL,
			*outputBasename = NULL, outputFilename[10000];
	int		depth, format, actualNumSlices = 0, W, H;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0;
	float		mean;
	roi		**inputRois, **outputRois[4];
	int		numInputRois, numOutputRois[4],
			pixelWM = 0, pixelGM = 0, maxNumPixelsPerRoi = -1;
	register int	i, j, sum, inp;

	while( (optI=getopt(argc, argv, "u:r:o:x:y:z:p:W:G:C:")) != EOF)
		switch( optI ){
			case 'r': inputROIFilename = strdup(optarg); break;
			case 'u': inputUNCFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 'x': pixel_size_x = atof(optarg); break;
			case 'y': pixel_size_y = atof(optarg); break;
			case 'z': pixel_size_z = atof(optarg); break;
			case 'W': pixelWM = atoi(optarg); break;
			case 'G': pixelGM = atoi(optarg); break;
			case 'p': maxNumPixelsPerRoi = atoi(optarg); break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( (pixelWM<=0) || (pixelGM<=0) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "You must set WM and GM pixel intensity thresholds using options '-W' and '-G'.\n");
		exit(1);
	}		
	if( inputROIFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output basename must be specified.\n");
		free(inputROIFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputUNCFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputROIFilename);
		exit(1);
	}
	if( (dummy=malloc(W*H*sizeof(DATATYPE))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for dummy.\n", argv[0], W*H*sizeof(DATATYPE));
		free(inputROIFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}		
	if( read_rois_from_file(inputROIFilename, &inputRois, &numInputRois) != 0 ){
		fprintf(stderr, "%s : call to read_rois_from_file has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputROIFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	for(i=0;i<4;i++) if( (outputRois[i]=(roi **)malloc(numInputRois*sizeof(roi *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for outputRois[%d].\n", argv[0], numInputRois*sizeof(roi *), i);
		free(inputROIFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
		
	if( !rois_calculate(inputRois, numInputRois) ){
		fprintf(stderr, "%s : call to rois_calculate has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputROIFilename); free(outputBasename);
		rois_destroy(inputRois, numInputRois);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( (pixel_size_x!=1.0) || (pixel_size_y!=1.0) )
		if( !rois_convert_pixels_to_millimetres(inputRois, numInputRois, pixel_size_x, pixel_size_y, pixel_size_z) ){
			fprintf(stderr, "%s : call to rois_convert_pixels_to_millimetres has failed for %d rois.\n", argv[0], numInputRois);
			free(inputROIFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}

	for(inp=0;inp<numInputRois;inp++){
		if( maxNumPixelsPerRoi > 0 ) if( inputRois[inp]->num_points_inside > maxNumPixelsPerRoi ){
			/* it goes nowhere, it is too big */
			outputRois[DEF][numOutputRois[DEF]++] = inputRois[inp];
			continue;
		}
		
		for(i=0,j=0,sum=0;i<inputRois[inp]->num_points_inside;i++) if( (p=data[inputRois[inp]->points_inside[i]->Z][inputRois[inp]->points_inside[i]->X][inputRois[inp]->points_inside[i]->Y]) > 0 ){ sum += p; j++; }
		mean = ((float )sum) / ((float )j);
		if( mean < pixelWM ) outputRois[WM][numOutputRois[WM]++] = inputRois[inp];
		else if( mean < pixelGM ) outputRois[GM][numOutputRois[GM]++] = inputRois[inp];
		else outputRois[CSF][numOutputRois[CSF]++] = inputRois[inp];
	}

	fprintf(stdout, "%s : total %d roi(s), of %d, from file '%s' satisfy the imposed criteria.\n", argv[0], numOutputRois[i], numInputRois, inputROIFilename);

	for(i=0;i<4;i++){
		sprintf(outputFilename, "%s_%s.roi", outputBasename, outputExtension[i]);
		if( !write_rois_to_file(outputFilename, outputRois[i], numOutputRois[i], pixel_size_x, pixel_size_y) ){
			fprintf(stderr, "%s : call to write_rois_to_file has failed for output file '%s'.\n", argv[0], outputBasename);
			free(inputROIFilename); free(outputBasename);
			rois_destroy(inputRois, numInputRois);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
	}
	rois_destroy(inputRois, numInputRois);
	for(i=0;i<4;i++) free(outputRois[i]);
	free(inputROIFilename); free(outputBasename); free(dummy); freeDATATYPE3D(data, actualNumSlices, W);
	
	exit(0);
}
