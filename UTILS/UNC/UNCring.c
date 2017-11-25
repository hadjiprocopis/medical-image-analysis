#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>
#include <ring.h>
#include <IO_roi.h>
#include <roi_utils.h>


const	char	Examples[] = "\
\n	-u inp.unc -r inp.unc.roi -o xx -O 2 -I 2 -n\
\n\
\nan inner ring of width 2 pixels\
\nan outer ring of width 2 pixels\
\nand output all masks as UNC files\
\n";

const	char	Usage[] = "options as follows:\
\n\t -u inputUNCFilename\
\n\
\n	(UNC image file with one or more slices)\
\n\
\n\t -r inputROIFilename\
\n	(ROI file associated with the above UNC volume)\
\n\
\n\t -o outputBasename\
\n	(Output basename - all output files will be preceded\
\n	 by the specified basename)\
\n\
\n*** one or both of the following options is required ***\
\n\t -I in\
\n	(Specifies that a ring of width 'in' pixels should\
\n	 be created at the *inside* of each ROI. An output\
\n	 UNC file will be created as 'outputBasename_in.unc'\
\n	 and will contain the *masks* of those rings.\
\n	 A statistics file will be created as\
\n	 'outputBasename_in.stats' and will contain various\
\n	 useful statistics of the pixels within this ring).\
\n\
\n AND / OR\
\n\
\n\t -O out\
\n	(Specifies that a ring of width 'out' pixels should\
\n	 be created at the *outside* of each ROI. An output\
\n	 UNC file will be created as 'outputBasename_out.unc'\
\n	 and will contain the *masks* of those rings.\
\n	 A statistics file will be created as\
\n	 'outputBasename_out.stats' and will contain various\
\n	 useful statistics of the pixels within this ring).\
\n\
\n*** optional stuff ***\
\n\t[-L	X:Y:Z\
\n	((X,Y,Z) is the position in 3D of a chosen landmark\
\n	 in the scan. (X,Y) are in millimetres - just what\
\n	 what dispunc displays, while Z is the slice number\
\n	 - not millimetres just 1 for the first slice, 10 for\
\n	 the tenth. If this option is given, then an additional\
\n	 column with the distance of the centroid of each roi\
\n	 from this landmark will be output. You can input as\
\n	 many landmarks as you want by using as many '-L' options)]\
\n\t[-l	X:Y\
\n	((X,Y) is the position in 2D - e.g. on each slice, as\
\n	 above but ignore the slice number basically - of a chosen\
\n	 landmark in the scan. (X,Y) are in millimetres - just what\
\n	 what dispunc displays. You can input as\
\n	 many landmarks as you want by using as many '-l' options)]\
\n\
\n\t[-t\
\n	(Each stats file, by default, will contain a header\
\n	giving information about the filenames used, and the\
\n	various fields in the files. These comments are preceded\
\n	by the de facto unix comment designator character, '#'.\
\n	However, dos programs might not understand it and get\
\n	confused (e.g. excel spreadsheet).\
\n	Use this option to specify that no comments should be\
\n	included in the stats files).]\
\n\
\n\t[-n\
\n	(To save space, NO masks will be written as UNC files\
\n	 by default. Use this option to specify otherwise.)]\
\n\t[-m\
\n	(Some lesions are so small that an inside ring of\
\n	 width X pixels is not possible. These rings will\
\n	 only be reported to the output file named:\
\n	 'output_too_small.txt'\
\n	 By using this flag, these rings will be printed\
\n	 to all other files produced, just like every\
\n	 other, normal, ring. Of course their statistics\
\n	 etc. will be ZERO.)]\
\n	\
\n\t[-9\
\n	(tell the program to copy the header/title information\
\n	from the input file to the output files. If there is\
\n	more than 1 input file, then the information is copied\
\n	from the first file.)]\
\n\
\n*** Use the following options to define the pixel size in the X, Y & Z direction ***\
\n\t[-X pixel_size_x\
\n	(the pixel size in the X-direction, default is 1.0 mm\
\n	 Usual value is 0.9375 mm)\
\n\t[-Y pixel_size_y\
\n	(the pixel size in the Y-direction, default is 1.0 mm\
\n	 Usual value is 0.9375 mm)\
\n\t[-Z pixel_size_z\
\n	** this one is of no importance - it is only here for future compatibility **\
\n	(the pixel size in the Z-direction, default is 1.0 mm\
\n	 Usual value is 5 mm or 1 mm etc.)\
\n\
\n** Use the following options to define a region of\
\n   interest for the erosion to take place. You can\
\n   also choose individual slice numbers with repeated '-s options.\
\n   By default, this process will be applied to all the slices.\
\n\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest (upper left corner x-coord)]\
\n\t[-y yCoordOfInterest (upper left cornes y-coord)]\
\n\t[-s sliceNumber [-s s...]] (slice numbers start from 1)\
\n\
\nThis program will take a UNC file and a ROI file describing\
\nN regions of interest. It will then proceed to find pixel groups\
\nwhich form rings inside and/or outside the periphery of each\
\nROI. To specify the width (in pixels) of the inside ring, use\
\nthe '-I' option. Use the '-O' option to specify the width\
\nof the outside ring. You can choose one of the two ring types\
\nor both of them.\
\n\
\nFor each roi, rings (as specified) will be created and written\
\nto UNC files as masks. The mask of each roi will be coloured\
\ndifferently so as to be able to isolate them using a threshold\
\noperation. The rings of the first roi will be coloured with value\
\n1, the second roi will have the value of 2, etc.\
\n\
\nThe inside rings (if requested) will be written to the file\
\n'outputBasename_in.unc', the outside rings (if requested)\
\nwill be written to the file 'outputBasename_out.unc'. If\
\nboth ring types were requested then the ring formed by adding\
\nthe inside and the outside rings will be written to the\
\nfile 'outputBasename_all.unc'.\
\n\
\nThe statistics (min/max/mean/stdev) for the group of pixels in\
\neach ring of each roi will be written to the files:\
\n'outputBasename_in.stats', 'outputBasename_out.stats' and\
\n'outputBasename_all.stats' (depending which options were used).\
\n\
\nEach stats file contains a small header describing the fields.\
\nThis is as follows:\
\n\
\n 1 : roi sequence number, starting from 1 (e.g. the order each roi appears in the roi file)\
\n 2 : pixel value that all points and rings of that roi will have in each of the output unc files\
\n 3 : number of pixels participated in the statistics\
\n 4 : min pixel value\
\n 5 : max pixel value\
\n 6 : mean pixel value\
\n 7 : standard deviation of the pixel values\
\n\
\nThe stats file with both rings (_all.stats) will also have\
\nthe additional fields\
\n\
\n 8 : number of pixels in the inside ring\
\n 9 : min pixel value of inside ring\
\n 10: max pixel value of inside ring\
\n 11: mean pixel value of inside ring\
\n 12: standard deviation of the pixel values of inside ring\
\n\
\n 13: number of pixels in the outside ring\
\n 14: min pixel value of outside ring\
\n 15: max pixel value of outside ring\
\n 16: mean pixel value of outside ring\
\n 17: standard deviation of the pixel values of outside ring\
\n\
\n ** RATIOS of inside ring statistics OVER outside ring statistics **\
\n\
\n 18: number of pixels in the inside over that of outside ring\
\n 19: min pixel value of the inside over that of outside ring\
\n 20: max pixel value of the inside over that of outside ring\
\n 21: mean pixel value of the inside over that of outside ring\
\n 22: standard deviation of the pixel values of the inside over that of outside ring\
\n\
\n ** If a landmark is given, 3D distance of the centroid of each roi from this landmark **\
\n 23+: 2D/3D distance of the centroid of each roi from given landmarks (use -l X:Y or -L X:Y:Z\
\n     for 2D and 3D respectively) use as many landmarks as you want.\
\n\
\nFinally, the name of each roi and the pixel value its rings have\
\nwill be written in file 'outputBasename_pixel_values.txt'.\
\n\
\nNotice that some rois will be too small to have an INSIDE ring.\
\nThese rois will be excluded from the output. There sequence\
\nnumber will be written in to a file 'outputBasename_too_small.txt'.\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

enum {INSIDE_RING=0, OUTSIDE_RING=1, WHOLE_RING=2, ORIGINAL_IMAGE=3, NUM_RINGS=4};
/* the basename will be appended with this extensions to create the output filenames (for unc '.unc', for stats '.stats') */
const char *outputFileExtensions[NUM_RINGS] = {
	"in", "out", "all", "masks"
};
const char *outputFileDescriptions[NUM_RINGS] = {
	"inside rings", "outside rings", "inside and outside rings", "original image"
};
	
typedef	struct	_ROI_STATS {
	roi		*r;
	DATATYPE	min[NUM_RINGS], max[NUM_RINGS],		/* use the enum above to access each field */
			pixelValue;	/* the pixel value of the points/rings of this roi in the output files (masks) */
	double		mean[NUM_RINGS], stdev[NUM_RINGS],
			ratio_min, ratio_max,	/* ratio of INSIDE / OUTSIDE stats */
			ratio_mean, ratio_stdev,
			ratio_num_points,
			*distance_from_landmarks2D,
			*distance_from_landmarks3D;	/* if the '-l' option was supplied - else -1 */

	int		num_points[NUM_RINGS];
	char		tooSmall;		/* a flag to indicate that the roi is too small and that the inner ring contains no pixels */
} roiStats;

int	main(int argc, char **argv){
	FILE		*outputHandle;
	DATATYPE	***data, ***dataOut, **scratchPadIn, **scratchPadOut,
			***insideRing, ***outsideRing, ***wholeRing, ***maskedImage,
			**statsPad, ***p[NUM_RINGS];
	char		*inputUNCFilename = NULL, *inputROIFilename = NULL,
			*outputBasename = NULL, copyHeaderFlag = FALSE,
			outputFilename[1000],
			headersOnFlag = TRUE,  writeUNCFlag = FALSE,
			doNotIncludeTooSmall = TRUE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1,
			depth, format, slice, actualNumSlices = 0,
			optI;

	roi		**inputRois;
	int		widthOfInsideRing = 0, widthOfOutsideRing = 0,
			numInputRois, inp, i, j, numTooSmall = 0, colN;
	roiStats	*myRoiStats;
	float		pixel_size_x = 1.0, pixel_size_y = 1.0, pixel_size_z = 1.0;
	double		Landmark3D[1000][3], Landmark2D[1000][2]; /* x, y, z */
	int		numLandmarks3D = 0, numLandmarks2D = 0;

	while( (optI=getopt(argc, argv, "u:o:es:w:h:x:y:9r:I:O:X:Y:Z:tnml:L:")) != EOF)
		switch( optI ){
			case 'u': inputUNCFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 'r': inputROIFilename = strdup(optarg); break;
			case 'I': widthOfInsideRing = atoi(optarg); break;
			case 'O': widthOfOutsideRing = atoi(optarg); break;
			case 't': headersOnFlag = FALSE; break;
			case 'm': doNotIncludeTooSmall = FALSE; break;
			case 'n': writeUNCFlag = TRUE; break;
                        case 'w': w = atoi(optarg); break;
                        case 'h': h = atoi(optarg); break;
                        case 'x': x = atoi(optarg); break;
                        case 'y': y = atoi(optarg); break;
			case 'L': sscanf(optarg, "%lf:%lf:%lf", &(Landmark3D[numLandmarks3D][0]), &(Landmark3D[numLandmarks3D][1]), &(Landmark3D[numLandmarks3D][2])); numLandmarks3D++; break;
			case 'l': sscanf(optarg, "%lf:%lf", &(Landmark2D[numLandmarks2D][0]), &(Landmark2D[numLandmarks2D][1])); numLandmarks2D++; break;
			case 'X': pixel_size_x = atof(optarg); break;
			case 'Y': pixel_size_y = atof(optarg); break;
			case 'Z': pixel_size_z = atof(optarg); break;

			case '9': copyHeaderFlag = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( inputUNCFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input UNC filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputUNCFilename);
		exit(1);
	}
	if( inputROIFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input ROI filename must be specified.\n");
		free(outputBasename); free(inputUNCFilename);
		exit(1);
	}
	if( (widthOfInsideRing==0) && (widthOfOutsideRing==0) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "At least one of ring types (inside or outside or both) must be specified along with its width in pixels (use -I and/or -O options).\n");
		free(outputBasename); free(inputUNCFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputUNCFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputUNCFilename);
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		exit(1);
	}
	if( w < 0 ) w = W;
	if( h < 0 ) h = H;

	if( (dataOut=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}
	if( (scratchPadIn=callocDATATYPE2D(W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for scratchPadIn data.\n", argv[0], W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}
	if( (scratchPadOut=callocDATATYPE2D(W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for scratchPadOut data.\n", argv[0], W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadOut, W);
		exit(1);
	}
	if( (statsPad=callocDATATYPE2D(NUM_RINGS, W*H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for stats pad.\n", argv[0], NUM_RINGS * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W);
		exit(1);
	}
	if( (insideRing=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for insideRing data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}
	if( (outsideRing=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for outsideRing data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}
	if( (wholeRing=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for wholeRing data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}
	if( (maskedImage=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for maskedImage data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
		exit(1);
	}

	/* read the roi file */
	if( read_rois_from_file(inputROIFilename, &inputRois, &numInputRois) != 0 ){
		fprintf(stderr, "%s : call to read_rois_from_file has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS);  freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
		exit(1);
	}

	if( !rois_calculate(inputRois, numInputRois) ){
		fprintf(stderr, "%s : call to rois_calculate has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		rois_destroy(inputRois, numInputRois);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
		exit(1);
	}

	if( !rois_convert_millimetres_to_pixels(inputRois, numInputRois, pixel_size_x, pixel_size_y, pixel_size_z) ){
		fprintf(stderr, "%s : call to rois_calculate has failed for file '%s'.\n", argv[0], inputROIFilename);
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		rois_destroy(inputRois, numInputRois);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
		exit(1);
	}

	if( (myRoiStats=(roiStats *)malloc(numInputRois*sizeof(roiStats))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for myRoiStats.\n", argv[0], numInputRois*sizeof(roiStats));
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		rois_destroy(inputRois, numInputRois);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
		exit(1);
	}		

	for(slice=0;slice<actualNumSlices;slice++) for(i=0;i<W;i++) for(j=0;j<H;j++)
		insideRing[slice][i][j] = outsideRing[slice][i][j] = wholeRing[slice][i][j] = maskedImage[slice][i][j] = 0;

	fprintf(stderr, "%s, processing %d rois ... (roi number, pixel value) :", inputROIFilename, numInputRois); fflush(stdout);
	for(inp=0;inp<numInputRois;inp++){
		if( numLandmarks2D > 0 )
			if( (myRoiStats[inp].distance_from_landmarks2D=(double *)malloc(numLandmarks2D*sizeof(double))) == NULL ){
				fprintf(stderr, "%s : could not allocate %zd bytes for myRoiStats[%d].distance_from_landmarks2D.\n", argv[0], numLandmarks2D*sizeof(double), inp);
				free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
				rois_destroy(inputRois, numInputRois);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
				free(myRoiStats);
				exit(1);
			}
		if( numLandmarks3D > 0 )
			if( (myRoiStats[inp].distance_from_landmarks3D=(double *)malloc(numLandmarks3D*sizeof(double))) == NULL ){
				fprintf(stderr, "%s : could not allocate %zd bytes for myRoiStats[%d].distance_from_landmarks3D.\n", argv[0], numLandmarks3D*sizeof(double), inp);
				free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
				rois_destroy(inputRois, numInputRois);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
				free(myRoiStats);
				exit(1);
			}

		myRoiStats[inp].r = inputRois[inp];
		myRoiStats[inp].tooSmall = FALSE;

		myRoiStats[inp].ratio_min = myRoiStats[inp].ratio_max = myRoiStats[inp].ratio_mean = myRoiStats[inp].ratio_stdev = myRoiStats[inp].ratio_num_points = 0.0;
		for(i=0;i<NUM_RINGS;i++){
			myRoiStats[inp].num_points[i] = 0;
			myRoiStats[inp].min[i] = myRoiStats[inp].max[i] = 0;
			myRoiStats[inp].mean[i] = myRoiStats[inp].stdev[i] = 0.0;
		}
		myRoiStats[inp].pixelValue = inp + 100; /* output pixel value for this roi - change that for each roi because we will need to extract each roi separately afterwards */

		fprintf(stderr, " %d(%d)", inp+1, myRoiStats[inp].pixelValue); fflush(stderr);
		slice = inputRois[inp]->slice;
		for(i=0;i<W;i++) for(j=0;j<H;j++) scratchPadIn[i][j] = 0;
		for(i=0;i<inputRois[inp]->num_points_inside;i++){
			maskedImage[slice][inputRois[inp]->points_inside[i]->X][inputRois[inp]->points_inside[i]->Y] =
			statsPad[ORIGINAL_IMAGE][myRoiStats[inp].num_points[ORIGINAL_IMAGE]++] = scratchPadIn[inputRois[inp]->points_inside[i]->X][inputRois[inp]->points_inside[i]->Y] = data[slice][inputRois[inp]->points_inside[i]->X][inputRois[inp]->points_inside[i]->Y];
		}

		/* inside ring */
		if( widthOfInsideRing > 0 ){
			/* remember: width, distance */
			if( (i=ring(scratchPadIn,
				widthOfInsideRing, -widthOfInsideRing,
				x, y, w, h, W, H, myRoiStats[inp].pixelValue, scratchPadOut)) == -1 ){
				fprintf(stderr, "%s : call to ring has failed for roi %d (inside ring).\n", argv[0], inp);
				free(inputUNCFilename); free(inputROIFilename); free(outputBasename);
				rois_destroy(inputRois, numInputRois);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
				exit(1);
			}
			if( i == 0 ){ myRoiStats[inp].tooSmall = TRUE; numTooSmall++; }

			for(i=0;i<W;i++) for(j=0;j<H;j++) if( scratchPadOut[i][j] > 0 ) insideRing[slice][i][j] = scratchPadOut[i][j];
		}
		/* outside ring */
		if( widthOfOutsideRing > 0 ){
			/* remember: width, distance */
			if( ring(scratchPadIn,
				widthOfOutsideRing, 0,
				x, y, w, h, W, H, myRoiStats[inp].pixelValue, scratchPadOut) == -1 ){
				fprintf(stderr, "%s : call to ring has failed for roi %d (outside ring).\n", argv[0], inp);
				free(inputUNCFilename); free(inputROIFilename); free(outputBasename);
				rois_destroy(inputRois, numInputRois);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
				exit(1);
			}
			for(i=0;i<W;i++) for(j=0;j<H;j++) if( scratchPadOut[i][j] > 0 ) outsideRing[slice][i][j] = scratchPadOut[i][j];
		}
		/* whole ring and stats */
		if( (widthOfInsideRing>0) && (widthOfOutsideRing>0) ){
			for(i=0;i<W;i++) for(j=0;j<H;j++){
				if( insideRing[slice][i][j] == myRoiStats[inp].pixelValue ){
					statsPad[INSIDE_RING][myRoiStats[inp].num_points[INSIDE_RING]++] =
					statsPad[WHOLE_RING][myRoiStats[inp].num_points[WHOLE_RING]++] = data[slice][i][j];
					wholeRing[slice][i][j] = myRoiStats[inp].pixelValue;
				}
				if( outsideRing[slice][i][j] == myRoiStats[inp].pixelValue ){
					statsPad[OUTSIDE_RING][myRoiStats[inp].num_points[OUTSIDE_RING]++] =
					statsPad[WHOLE_RING][myRoiStats[inp].num_points[WHOLE_RING]++] = data[slice][i][j];
					wholeRing[slice][i][j] = myRoiStats[inp].pixelValue;
				}
			}
		}

		if( myRoiStats[inp].num_points[INSIDE_RING] > 0 )
			statistics1D(statsPad[INSIDE_RING], 0, myRoiStats[inp].num_points[INSIDE_RING], &(myRoiStats[inp].min[INSIDE_RING]), &(myRoiStats[inp].max[INSIDE_RING]), &(myRoiStats[inp].mean[INSIDE_RING]), &(myRoiStats[inp].stdev[INSIDE_RING]));

		if( myRoiStats[inp].num_points[OUTSIDE_RING] > 0 )
			statistics1D(statsPad[OUTSIDE_RING], 0, myRoiStats[inp].num_points[OUTSIDE_RING], &(myRoiStats[inp].min[OUTSIDE_RING]), &(myRoiStats[inp].max[OUTSIDE_RING]), &(myRoiStats[inp].mean[OUTSIDE_RING]), &(myRoiStats[inp].stdev[OUTSIDE_RING]));
			
		if( myRoiStats[inp].num_points[WHOLE_RING] > 0 ){
			statistics1D(statsPad[WHOLE_RING], 0, myRoiStats[inp].num_points[WHOLE_RING], &(myRoiStats[inp].min[WHOLE_RING]), &(myRoiStats[inp].max[WHOLE_RING]), &(myRoiStats[inp].mean[WHOLE_RING]), &(myRoiStats[inp].stdev[WHOLE_RING]));
			if( myRoiStats[inp].num_points[OUTSIDE_RING] > 0 )
				myRoiStats[inp].ratio_num_points = ((double )myRoiStats[inp].num_points[INSIDE_RING]) / ((double )myRoiStats[inp].num_points[OUTSIDE_RING]);
			if( myRoiStats[inp].min[OUTSIDE_RING] > 0 )
				myRoiStats[inp].ratio_min = ((double )myRoiStats[inp].min[INSIDE_RING]) / ((double )myRoiStats[inp].min[OUTSIDE_RING]);
			if( myRoiStats[inp].max[OUTSIDE_RING] > 0 )
				myRoiStats[inp].ratio_max = ((double )myRoiStats[inp].max[INSIDE_RING]) / ((double )myRoiStats[inp].max[OUTSIDE_RING]);
			if( myRoiStats[inp].mean[OUTSIDE_RING] > 0 )
				myRoiStats[inp].ratio_mean= myRoiStats[inp].mean[INSIDE_RING] / myRoiStats[inp].mean[OUTSIDE_RING];
			if( myRoiStats[inp].stdev[OUTSIDE_RING] > 0 )
				myRoiStats[inp].ratio_stdev= myRoiStats[inp].stdev[INSIDE_RING] / myRoiStats[inp].stdev[OUTSIDE_RING];
		}		
		if( myRoiStats[inp].num_points[ORIGINAL_IMAGE] > 0 )
			statistics1D(statsPad[ORIGINAL_IMAGE], 0, myRoiStats[inp].num_points[ORIGINAL_IMAGE], &(myRoiStats[inp].min[ORIGINAL_IMAGE]), &(myRoiStats[inp].max[ORIGINAL_IMAGE]), &(myRoiStats[inp].mean[ORIGINAL_IMAGE]), &(myRoiStats[inp].stdev[ORIGINAL_IMAGE]));
		/* if doLandmakrs then get the distance from the centroid of the roi from landmark */
		for(j=0;j<numLandmarks2D;j++){
			myRoiStats[inp].distance_from_landmarks2D[j] = sqrt( SQR(inputRois[inp]->centroid_x-Landmark2D[j][0]) + SQR(inputRois[inp]->centroid_y-Landmark2D[j][1]) );
		}
		for(j=0;j<numLandmarks3D;j++){
			myRoiStats[inp].distance_from_landmarks3D[j] = sqrt( SQR(inputRois[inp]->centroid_x-Landmark3D[j][0]) + SQR(inputRois[inp]->centroid_y-Landmark3D[j][1]) + SQR(((double )(inputRois[inp]->slice))-Landmark3D[j][2]) );
		}
	}
	fprintf(stderr, "\n");

	p[0] = insideRing; p[1] = outsideRing; p[2] = wholeRing; p[3] = maskedImage;
	for(i=0;i<NUM_RINGS;i++){
		if( (i == INSIDE_RING) && (widthOfInsideRing <= 0) ) continue;
		if( (i == OUTSIDE_RING) && (widthOfOutsideRing <= 0) ) continue;
		if( (i == WHOLE_RING) && ((widthOfInsideRing<=0) || (widthOfOutsideRing<=0)) ) continue;

		fprintf(stderr, "%s : %s", argv[0], outputFileDescriptions[i]);

		if( writeUNCFlag ){
			sprintf(outputFilename, "%s_%s.unc", outputBasename, outputFileExtensions[i]);
			if( !writeUNCSlices3D(outputFilename, p[i], W, H, 0, 0, W, H, NULL, actualNumSlices, format, OVERWRITE) ){
				fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
				free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
				rois_destroy(inputRois, numInputRois);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
				exit(1);
			}
			if( copyHeaderFlag )
				/* now copy the image info/title/header of source to destination */
				if( !copyUNCInfo(inputUNCFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
					fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputUNCFilename, outputFilename);
					free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
					rois_destroy(inputRois, numInputRois);
					freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
					exit(1);
				}
			fprintf(stderr, " masks in '%s' and their", outputFilename);
		}

		sprintf(outputFilename, "%s_%s.stats", outputBasename, outputFileExtensions[i]);
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
			rois_destroy(inputRois, numInputRois);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
			exit(1);
		}

		colN = 1;
		if( headersOnFlag ){
			fprintf(outputHandle, "# File holding the statistics of %s of UNC file '%s' and ROI file '%s'.\n", outputFileDescriptions[i], inputUNCFilename, inputROIFilename);
			fprintf(outputHandle, "# Fields are as following:\n");
			fprintf(outputHandle, "# %d : roi sequence number, starting from 1 (e.g. the order each roi appears in the roi file)\n", colN); colN++;
			fprintf(outputHandle, "# %d : pixel value that all points and rings of that roi will have in each of the output unc files\n", colN); colN++;
			fprintf(outputHandle, "# %d : number of pixels participated in the statistics\n", colN); colN++;
		
			fprintf(outputHandle, "# %d : min pixel value\n", colN); colN++;
			fprintf(outputHandle, "# %d : max pixel value\n", colN); colN++;
			fprintf(outputHandle, "# %d : mean pixel value\n", colN); colN++;
			fprintf(outputHandle, "# %d : standard deviation of the pixel values\n", colN); colN++;
		}
		if( (i == WHOLE_RING) && headersOnFlag ){
			fprintf(outputHandle, "# the following fields are related to the inside rings\n");
			fprintf(outputHandle, "# %d : number of pixels in the inside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : min pixel value of inside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : max pixel value of inside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : mean pixel value of inside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : standard deviation of the pixel values of inside ring\n", colN); colN++;

			fprintf(outputHandle, "# the following fields are related to the outside rings\n");
			fprintf(outputHandle, "# %d : number of pixels in the outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : min pixel value of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : max pixel value of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : mean pixel value of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : standard deviation of the pixel values of outside ring\n", colN); colN++;

			fprintf(outputHandle, "# the following fields are the RATIOS of INSIDE / OUTSIDE ring statistics\n");
			fprintf(outputHandle, "# %d : number of pixels in the inside over that of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : min pixel value of the inside over that of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : max pixel value of the inside over that of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : mean pixel value of the inside over that of outside ring\n", colN); colN++;
			fprintf(outputHandle, "# %d : standard deviation of the pixel values of the inside over that of outside ring\n", colN); colN++;
		}			
		for(j=0;j<numLandmarks2D;j++){
			fprintf(outputHandle, "# %d : 2D distance of the centroid of each roi from 2D landmark at (%.2f,%.2f) in each slice\n", colN, Landmark2D[j][0], Landmark2D[j][1]);
			colN++;
		}
		for(j=0;j<numLandmarks3D;j++){
			fprintf(outputHandle, "# %d : 3D distance of the centroid of each roi from 3D landmark at (%.2f,%.2f,%.0f)\n", colN, Landmark3D[j][0], Landmark3D[j][1], Landmark3D[j][2]);
			colN++;
		}

		for(inp=0;inp<numInputRois;inp++){
			if( myRoiStats[inp].tooSmall && doNotIncludeTooSmall ) continue;
			fprintf(outputHandle, "%d %d %d %d %d %f %f",
				inp+1, myRoiStats[inp].pixelValue,
				myRoiStats[inp].num_points[i],
				myRoiStats[inp].min[i], myRoiStats[inp].max[i],
				myRoiStats[inp].mean[i], myRoiStats[inp].stdev[i]);
			if( i == WHOLE_RING ){
				fprintf(outputHandle, " %d %d %d %f %f %d %d %d %f %f %f %f %f %f %f",
					myRoiStats[inp].num_points[INSIDE_RING],
					myRoiStats[inp].min[INSIDE_RING], myRoiStats[inp].max[INSIDE_RING],
					myRoiStats[inp].mean[INSIDE_RING], myRoiStats[inp].stdev[INSIDE_RING],

					myRoiStats[inp].num_points[OUTSIDE_RING],
					myRoiStats[inp].min[OUTSIDE_RING], myRoiStats[inp].max[OUTSIDE_RING],
					myRoiStats[inp].mean[OUTSIDE_RING], myRoiStats[inp].stdev[OUTSIDE_RING],

					myRoiStats[inp].ratio_num_points,
					myRoiStats[inp].ratio_min, myRoiStats[inp].ratio_max,
					myRoiStats[inp].ratio_mean, myRoiStats[inp].ratio_stdev);
			}
			for(j=0;j<numLandmarks2D;j++)
				fprintf(outputHandle, " %f", myRoiStats[inp].distance_from_landmarks2D[j]);
			for(j=0;j<numLandmarks3D;j++)
				fprintf(outputHandle, " %f", myRoiStats[inp].distance_from_landmarks3D[j]);
			fprintf(outputHandle, "\n");
		}
		fclose(outputHandle);

		fprintf(stderr, " stats in '%s'.\n", outputFilename);

	}

	/* report those rois that they were too small to have an inside ring */
	/* and also write them onto a file */
	if( numTooSmall > 0 ){
		sprintf(outputFilename, "%s_too_small.txt", outputBasename);
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
			rois_destroy(inputRois, numInputRois);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
			exit(1);
		}
		for(inp=0;inp<numInputRois;inp++)
			if( myRoiStats[inp].tooSmall )
				fprintf(outputHandle, "roi %d, slice %d, pixel value %d, of file %s is too small to have an inside ring.\n", inp+1, myRoiStats[inp].r->slice+1, myRoiStats[inp].pixelValue, inputROIFilename);
		fclose(outputHandle);
		fprintf(stderr, "%s : %d rois were missed because they were too small. There names are written in '%s'.\n", argv[0], numTooSmall, outputFilename);
	}
	
	sprintf(outputFilename, "%s_pixel_values.txt", outputBasename);
	if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
		fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
		free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
		rois_destroy(inputRois, numInputRois);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W); freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS); freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W); freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);
		exit(1);
	}
	if( headersOnFlag ){
		fprintf(outputHandle, "# File holding the pixel value that each roi in file '%s' has:\n", inputROIFilename);
		fprintf(outputHandle, "# Fields are as following:\n");
		fprintf(outputHandle, "# 1 : roi sequence number, starting from 1 (e.g. the order each roi appears in the roi file)\n");
		fprintf(outputHandle, "# 2 : the slice number the roi is in (starting from 1).\n");
		fprintf(outputHandle, "# 3 : the name of the roi (sometimes there is no name) as it appears in the the 'Name' field of a ROI file.\n");
		fprintf(outputHandle, "# 4 : the pixel value this roi's rings will have in all 'UNC' files.\n");
	}
	for(inp=0;inp<numInputRois;inp++)
		fprintf(outputHandle, "%d %d '%s'\t%d\n",
			inp+1, myRoiStats[inp].r->slice + 1, myRoiStats[inp].r->name,
			myRoiStats[inp].pixelValue);
	fclose(outputHandle);

	fprintf(stderr, "%s : the pixel values for each roi are written in '%s'.\n", argv[0], outputFilename);
	
	if( numLandmarks2D > 0 ) for(inp=0;inp<numInputRois;inp++) free(myRoiStats[inp].distance_from_landmarks2D);
	if( numLandmarks3D > 0 ) for(inp=0;inp<numInputRois;inp++) free(myRoiStats[inp].distance_from_landmarks3D);
	free(myRoiStats);

	free(inputUNCFilename); free(inputROIFilename);free(outputBasename);
	rois_destroy(inputRois, numInputRois);
	freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(dataOut, actualNumSlices, W);
	freeDATATYPE2D(scratchPadIn, W); freeDATATYPE2D(scratchPadOut, W); freeDATATYPE2D(statsPad, NUM_RINGS);
	freeDATATYPE3D(wholeRing, actualNumSlices, w); freeDATATYPE3D(outsideRing, actualNumSlices, W);
	freeDATATYPE3D(insideRing, actualNumSlices, W);freeDATATYPE3D(maskedImage, actualNumSlices, W);

	exit(0);
}
