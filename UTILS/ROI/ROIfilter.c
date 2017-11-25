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
\t -i inputFilename\
	(ROI file - the same format a dispunc reads and\
	 writes)\
\
\t -o outputFilename\
	(Output filename)\
\
\t[-u inputUNCFilename\
	(optional UNC filename associated with this ROI.\
	 If this is supplied, then filtering based on\
	 criteria such as min/max/mean/stdev of pixels\
	 inside each ROI can be done.)]\
\
\t[-n\
	(reverse the results of filtering)]\
\
* One or more of the following is required:\
\t -I P:a:b\
	REMOVE all irregular regions whose circumference\
	(e.g. number of points on the roi contour) IS NOT\
	between the numbers 'a' and 'b'\
\
\t -E [X|Y|A|B|R]:a:b\
	remove all elliptical regions whose specified\
	property does not fall between the numbers 'a'\
	and 'b'. 'X' and 'Y' refer to the centre of the\
	ellipsis. 'A' and 'B' refer to the horizontal\
	and vertical radii of the ellipsis. 'R' refers\
	to the angle of rotation of the ellipsis (in radians).\
\
\t -R [X|Y|W|H]:a:b\
	remove all rectangular regions whose specified\
	property does not fall between the numbers 'a'\
	and 'b'. 'X' and 'Y' refer to the left-top\
	corner of the region. 'W' and 'H' refer to the\
	width and the height of the region.\
\
\t -A [x|y|X|Y|Z|W|H|A|n|m|M|S]:a:b\
	remove ANY region (i.e. irregular, rectangular\
	or elliptical) whose specified property does\
	not fall between the numbers 'a' and 'b'.\
	'x' and 'y' refer to the centroid of the roi.\
	'X' and 'Y' refer to the left-top corner of the\
	rectangle enclosing the roi. 'Z' refers to the slice\
	number of the roi, it starts from 1. 'W' and 'H' refer\
	to the width and height of the rectangle.\
	'N' refers to the number of pixels the roi covers.\
	'n' and 'm' refer to the minimum and maximum\
	pixel values within the roi (if available - e.g if\
        a UNC filename was specified using the '-u' option).\
	'M' and 'S' refer to the mean and standard\
	deviation of the pixel values within the\
	roi (if available).]\
\
\t -A t:[I|R|E]\
	remove any region whose type is one of the\
	specified I, R or E (for irregular, rectangular\
	or elliptical).\
\
** NOTE that all the ranges above work as follows:\
	the range min:max\
	means (>= min) and (<max). Note the less than\
	max (and not equal). If you want to include slices\
	2 and 3, you should specify -A Z:2:4\
	while only slice 2: -A Z:2:3\
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
	DATATYPE	***data;
	int		optI;
	char		*inputROIFilename = NULL, *inputUNCFilename = NULL,
			*outputFilename = NULL;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0;

	roi		**inputRois, **outputRois[3];
	int		numInputRois, numOutputRois[3],
			numCriteria = 0,
			i, j,
			W = -1, H = -1, depth, format, actualNumSlices = 0; 
	float		rMin[1000], rMax[1000], *dummy;
	char		a, b, doIrregular = FALSE,
			doElliptical = FALSE, doRectangular = FALSE,
			doAll = FALSE, reverse = FALSE, needUNC = FALSE;
	roiFilterType	fType[1000];

	while( (optI=getopt(argc, argv, "i:o:x:y:z:I:E:R:A:nu:")) != EOF)
		switch( optI ){
			case 'i': inputROIFilename = strdup(optarg); break;
			case 'u': inputUNCFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'x': pixel_size_x = atof(optarg); break;
			case 'y': pixel_size_y = atof(optarg); break;
			case 'z': pixel_size_z = atof(optarg); break;
			case 'n': reverse = TRUE; break;
			case 'A': doAll = TRUE;
				  sscanf(optarg, "%c", &a);
				  if( a != 't' ) sscanf(optarg, "%c:%f:%f", &a, &(rMin[numCriteria]), &(rMax[numCriteria]));
				  else {
				  	fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_TYPE;
					sscanf(optarg, "%c:%c", &a, &b);
					switch( toupper(b) ){
						case 'I': rMin[numCriteria] = IRREGULAR_ROI_REGION; break;
						case 'E': rMin[numCriteria] = ELLIPTICAL_ROI_REGION; break;
						case 'R': rMin[numCriteria] = RECTANGULAR_ROI_REGION; break;
						default :
							fprintf(stderr, "%s : the last parameter of the '-A t:' option must be one of 'I', 'E' or 'R'.\n", argv[0]);
							exit(1);
					}
					numCriteria++; break;
				  }							
				  switch( a ){
				  	case 'x': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_CENTROID_X; break;
				  	case 'y': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_CENTROID_Y; break;
				  	case 'X': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_X0; break;
				  	case 'Y': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_Y0; break;
				  	case 'Z': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_SLICE_NUMBER; break;
				  	case 'W': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_WIDTH; break;
				  	case 'H': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_HEIGHT; break;
				  	case 'N': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_NUM_POINTS_INSIDE; break;
				  	case 'n': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_MIN_PIXEL; needUNC = TRUE; break;
				  	case 'm': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_MAX_PIXEL; needUNC = TRUE; break;
				  	case 'M': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_MEAN_PIXEL; needUNC = TRUE; break;
				  	case 'S': fType[numCriteria] = ROI_FILTER_TYPE_GENERAL_STDEV_PIXEL; needUNC = TRUE; break;
					default:
						fprintf(stderr, "%s : the first parameter of option '-A' must be one of 'x', 'y', 'X', 'Y', 'W', 'H', 'n', 'm', 't', 'N', 'M' or 'S'.\n", argv[0]);
						exit(1);
				  }
				  numCriteria++; break;
			case 'I': doIrregular = TRUE;
				  sscanf(optarg, "%c:%f:%f", &a, &(rMin[numCriteria]), &(rMax[numCriteria]));
				  switch( a ){
				  	case 'P': fType[numCriteria] = ROI_FILTER_TYPE_IRREGULAR_NUM_POINTS; break;
					default:
						fprintf(stderr, "%s : the first parameter of option '-I' must be one of 'P'.\n", argv[0]);
						exit(1);
				  }
				  numCriteria++; break;
			case 'E': doElliptical = TRUE;
				  sscanf(optarg, "%c:%f:%f", &a, &(rMin[numCriteria]), &(rMax[numCriteria]));
				  switch( a ){
				  	case 'X': fType[numCriteria] = ROI_FILTER_TYPE_ELLIPTICAL_EX0; break;
				  	case 'Y': fType[numCriteria] = ROI_FILTER_TYPE_ELLIPTICAL_EY0; break;
				  	case 'A': fType[numCriteria] = ROI_FILTER_TYPE_ELLIPTICAL_EA; break;
				  	case 'B': fType[numCriteria] = ROI_FILTER_TYPE_ELLIPTICAL_EB; break;
				  	case 'R': fType[numCriteria] = ROI_FILTER_TYPE_ELLIPTICAL_ROT; break;
					default:
						fprintf(stderr, "%s : the first parameter of option '-E' must be one of 'x', 'y', 'a', 'b' or 'r'.\n", argv[0]);
						exit(1);
				  }
				  numCriteria++; break;
			case 'R': doRectangular = TRUE;
				  sscanf(optarg, "%c:%f:%f", &a, &(rMin[numCriteria]), &(rMax[numCriteria]));
				  switch( a ){
				  	case 'X': fType[numCriteria] = ROI_FILTER_TYPE_RECTANGULAR_X0; break;
				  	case 'Y': fType[numCriteria] = ROI_FILTER_TYPE_RECTANGULAR_Y0; break;
				  	case 'W': fType[numCriteria] = ROI_FILTER_TYPE_RECTANGULAR_WIDTH; break;
				  	case 'H': fType[numCriteria] = ROI_FILTER_TYPE_RECTANGULAR_HEIGHT; break;
					default:
						fprintf(stderr, "%s : the first parameter of option '-R' must be one of 'x', 'y', 'w' or 'h'.\n", argv[0]);
						exit(1);
				  }
				  numCriteria++; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( needUNC && (inputUNCFilename == NULL) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "If you want to your the criteria 'm', 'n', 'M' & 'S' (min, max, mean and stdev of pixels inside roi) you need to supply a valid UNC volume associated with this ROI using the '-u' option.\n");
		if( outputFilename != NULL ) free(outputFilename);
		if( inputROIFilename != NULL ) free(inputROIFilename);
		exit(1);
	}

	if( numCriteria == 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "At least one filter criterion must be specified (using at least one of '-A', '-I', '-E' or '-R').\n");
		if( outputFilename != NULL ) free(outputFilename);
		if( inputROIFilename != NULL ) free(inputROIFilename);
		exit(1);
	}
	if( inputROIFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputROIFilename);
		exit(1);
	}

	fprintf(stdout, "%s : reading rois from '%s' ... ", argv[0], inputROIFilename); fflush(stdout);
	if( read_rois_from_file(inputROIFilename, &inputRois, &numInputRois) != 0 ){
		fprintf(stderr, "%s : call to read_rois_from_file has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputROIFilename); free(outputFilename);
		exit(1);
	}
	fprintf(stdout, " done. Read %d rois.\n", numInputRois);

	if( (pixel_size_x!=1.0) || (pixel_size_y!=1.0) ){
		fprintf(stdout, "%s : converting rois to millimetres using pixel_size_x = %f and pixel_size_y = %f ... ", argv[0], pixel_size_x, pixel_size_y); fflush(stdout);
		if( !rois_convert_pixels_to_millimetres(inputRois, numInputRois, pixel_size_x, pixel_size_y, pixel_size_z) ){
			fprintf(stderr, "%s : call to rois_convert_pixels_to_millimetres has failed for %d rois.\n", argv[0], numInputRois);
			free(inputROIFilename); free(outputFilename);
			rois_destroy(inputRois, numInputRois);
			exit(1);
		}
		fprintf(stdout, "done.\n");
	}

	fprintf(stdout, "%s : calculating rois ... ", argv[0]); fflush(stdout);
	if( !rois_calculate(inputRois, numInputRois) ){
		fprintf(stderr, "%s : call to rois_calculate has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputROIFilename); free(outputFilename);
		rois_destroy(inputRois, numInputRois);
		exit(1);
	}
	fprintf(stdout, "done.\n");

	if( inputUNCFilename != NULL ){
		fprintf(stdout, "%s : reading UNC file '%s' ... ", argv[0], inputUNCFilename); fflush(stdout);
		if( (data=getUNCSlices3D(inputUNCFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
			fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputUNCFilename);
			free(inputROIFilename); free(outputFilename); free(inputUNCFilename);
			rois_destroy(inputRois, numInputRois);
			exit(1);
		}
		if( (dummy=malloc(W*H*sizeof(float))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for dummy.\n", argv[0], W*H*sizeof(float));
			free(inputROIFilename); free(outputFilename);
			rois_destroy(inputRois, numInputRois);
			exit(1);
		}
		fprintf(stdout, "associating with rois ... "); fflush(stdout);
		rois_associate_with_unc_volume(inputRois, numInputRois, data, dummy);
		free(dummy);
		freeDATATYPE3D(data, actualNumSlices, W);
		fprintf(stdout, "done.\n");
	}
	
	for(i=0;i<3;i++) numOutputRois[i] = numInputRois;
	i = 0; outputRois[i] = NULL;
	if( doIrregular ){
		fprintf(stdout, "%s : doing irregular roi filtering ... ", argv[0]); fflush(stdout);
		outputRois[i] = rois_filter(inputRois, numInputRois, &(numOutputRois[i]), IRREGULAR_ROI_REGION, numCriteria, fType, rMin, rMax, reverse);
		fprintf(stdout, "done. %d roi(s), of %d, from file '%s' satisfy the imposed criteria for the irregular region rois.\n", numOutputRois[i], numInputRois, inputROIFilename);
		i++;
	}
	if( doRectangular ){
		fprintf(stdout, "%s : doing rectangular roi filtering ... ", argv[0]); fflush(stdout);
		outputRois[i] = rois_filter(i==0?inputRois:outputRois[i-1], i==0?numInputRois:numOutputRois[i-1], &(numOutputRois[i]), RECTANGULAR_ROI_REGION, numCriteria, fType, rMin, rMax, reverse);
		fprintf(stdout, "done. %d roi(s), of %d, from file '%s' satisfy the imposed criteria for the rectangular region rois.\n", numOutputRois[i], numInputRois, inputROIFilename);
		i++;
	}
	if( doElliptical ){
		fprintf(stdout, "%s : doing elliptical roi filtering ... ", argv[0]); fflush(stdout);
		outputRois[i] = rois_filter(i==0?inputRois:outputRois[i-1], i==0?numInputRois:numOutputRois[i-1], &(numOutputRois[i]), ELLIPTICAL_ROI_REGION, numCriteria, fType, rMin, rMax, reverse);
		fprintf(stdout, "done. %d roi(s), of %d, from file '%s' satisfy the imposed criteria for the elliptical region rois.\n", numOutputRois[i], numInputRois, inputROIFilename);
		i++;
	}
	if( doAll ){
		fprintf(stdout, "%s : doing any roi filtering ... ", argv[0]); fflush(stdout);
		outputRois[i] = rois_filter(i==0?inputRois:outputRois[i-1], i==0?numInputRois:numOutputRois[i-1], &(numOutputRois[i]), UNSPECIFIED_ROI_REGION, numCriteria, fType, rMin, rMax, reverse);
		fprintf(stdout, "done. %d roi(s), of %d, from file '%s' satisfy the imposed criteria for ALL the region type rois.\n", numOutputRois[i], numInputRois, inputROIFilename);
		i++;
	}
	i--;

	rois_destroy(inputRois, numInputRois);

	if( i > 0 )
		fprintf(stdout, "%s : total %d roi(s), of %d, from file '%s' satisfy the imposed criteria.\n", argv[0], numOutputRois[i], numInputRois, inputROIFilename);

	if( outputRois[i] != NULL ){
		if( !write_rois_to_file(outputFilename, outputRois[i], numOutputRois[i], pixel_size_x, pixel_size_y) ){
			fprintf(stderr, "%s : call to write_rois_to_file has failed for output file '%s'.\n", argv[0], outputFilename);
			free(inputROIFilename); free(outputFilename);
			rois_destroy(inputRois, numInputRois);
			for(j=0;j<=i;j++) rois_destroy(outputRois[j], numOutputRois[j]);
			exit(1);
		}
	}
	for(j=0;j<=i;j++) rois_destroy(outputRois[j], numOutputRois[j]);

	free(inputROIFilename); free(outputFilename);
	
	exit(0);
}
