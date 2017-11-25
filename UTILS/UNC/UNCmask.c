#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* the default color for whatever falls outside the mask */
#define	DEFAULT_OFF_COLOR	0

const	char	Examples[] = "\
\n	-i input.unc -o output.unc -m mask.unc\
\n\
\nwill use the mask.unc UNC file as a mask onto input.unce\
\nto produce the output.unc file. If mask.unc contains\
\nmore than one slice, the program expects that the number\
\nof slices in input.unc and mask.unc are the same.\
\nIn this case, each slice in mask.unc will be used\
\nto transform the corrsponding slice in the input.unc\
\nfile.\
\nThe output.unc file's pixels will have the value of\
\n0 if the input file's pixels fall outside the mask\
\nor the input file's pixel value if that pixel falls\
\ninside the mask.\
\n\
\n	-i input.unc -o output.unc -m mask.unc -f 1000\
\n\
\nas above, except that the value for those pixels falling\
\noutside the mask will not be 0 but 1000.\
\n\
\n	-i input.unc -o output.unc -m mask.unc -n\
\n\
\nAs the first example but those pixels which were included\
\nby the mask, will not be excluded and vice versa.\
\n\
\n	-i input.unc -o output.unc -m mask.unc -t 0:150\
\n\
\nAs the first example, but pixels in the input image\
\nwill be included if the value of the corrsponding pixel\
\nof the mask image is withing 0 and 150.\
\n\
\n	-i input.unc -o output.unc -m mask.unc -s 1 -s 4 -s 8\
\n\
\nAs the first example, but the output file will contain\
\nonly 3 slices: slices 1, 4 and 8 of the input image will\
\nbe masked to either the *only slice* of the mask file if\
\nit contains only 1 slice, or slices 1, 4 and 8 of the mask file\
\nwhen it contains exactly the same number of slices as the\
\ninput.\
\n\
\n	-i input.unc -o output.unc -m mask.unc -s 1 -s 4 -s 8 -x 20 -y 30 -w 40 -h 50\
\n\
\nAs above, but this time only the portion of the image defined\
\nby the rectangle whose top-left corner is at (20,30) and\
\nof dimensions 30x40 will be affected - all other pixels outside\
\nthis rectangle *will be included in the output image unmodified*.\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t -m maskFilename\
\n	(UNC image file containing the mask. The number\
\n	of slices in this file should either be 1\
\n	or equal to the number of slices of the\
\n	input file. See below for more information on\
\n	the mask.)\
\n\
\n\t[-n	(Negate flag. If this option is used, the result\
\n	will be inverted.)]\
\n\
\n\t[-f	pixelValue\
\n	(The value of pixels not falling within the mask.\
\n	The default is 0.)]\
\n\
\n\t[-t	lo:hi\
\n	(Range of pixel values. If the mask's pixel value\
\n	falls within this range, then the mask is active\
\n	else is not active. Essentially, by using this\
\n	option you threshold the mask first to\
\n		if( lo <= maskPixelValue < hi ) -> 1\
\n		else -> 0\
\n	)]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use this options to select a region of interest whose\
\n   histogram you need to calculate. You may use one or more\
\n   or all of '-w', '-h', '-x' and '-y' once.\
\n   You may use one or more '-s' options in order to specify\
\n   more slices. Slice numbers start from 1.\
\n   These parameters are optional, if not present then the\
\n   whole image, all slices will be used.\
\n\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\
\n** Note that if the mask contains 1 slice, this slice will\
\nbe used for the transformation of each slice in the input\
\nfile - or only those slices specified by '-s' options.\
\nOn the other hand, if the mask file contains more than\
\n1 slice, the program expects that the number of slices\
\nof the mask file *is equal* to the number of slices\
\nof the input image. In this case, the ith mask file slice\
\nwill be used to transform the ith slice of the input\
\nfile.\
\n\
\n** What is a mask?\
\nSimply put a mask is NOT an image but a matrix of boolean\
\nvalues (TRUE, FALSE). The size of this matrix corresponds to\
\nthe size of the input image. The mask transformation of an\
\ninput image will produce an output image whose pixel at (i,j)\
\nhas a value equal to the value of pixel (i,j) of the original\
\nimage if the entry in the mask matrix at the same location is\
\nTRUE or 0 if FALSE.\
\n\
\nIn this sense, if you use the '-n' option, this logic will be\
\ninverted and the resultant image will be the negation of\
\nwhat would usually be produced.\
\n\
\nAlso, you can specify a color value, with the '-f' option, for\
\nthe FALSE pixels (those that get the 0 value by default).\
\n\
\nNow, for the sake of simplicity, we assume that a mask IS an\
\nimage file. That gives us the flexibility to use existing\
\nimage files as masks. By default, a pixel in the mask image\
\nwill have the value of TRUE  if its value is not 0, and FALSE\
\nif it is 0.\
\n\
\nThe '-t lo:hi' option allows for the redefinition of the\
\naforementioned threshold levels. For example, '-t 100:500'\
\nwill consider all pixels whose values lie between 100\
\n(inclusive) and 500 (exclusive) as TRUE and FALSE otherwise\
\n(use the '-n' option to invert this).\
\n\
\n** Use the '-e' option to get some examples.\
\n";


const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL,
			copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	DATATYPE	***mask;
	char		*maskFilename = NULL;
	int		low = 1, hi = 1000000, inverseFlag = FALSE,
			offColor = DEFAULT_OFF_COLOR, i, j, maskSlice,
			mW = -1, mH = -1, maskNumSlices = -1, mDepth, mFormat;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:m:nt:f:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'm': maskFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'f': offColor = atoi(optarg); break;
			case 'n': inverseFlag = TRUE; break;
			case 't': sscanf(optarg, "%d:%d", &low, &hi); break;
			case '9': copyHeaderFlag = TRUE; break;
			case 'e': fprintf(stderr, "Here are some examples:\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( maskFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A mask filename must be specified.\n");
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( low > hi ){
		fprintf(stderr, "%s : low pixel value for mask must not exceed high.\n", argv[0]);
		free(inputFilename); free(outputFilename); free(maskFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename); free(maskFilename);
		exit(1);
	}

	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers (%d) must not exceed %d, the total number of slices in file '%s'.\n", argv[0], slices[s], actualNumSlices, inputFilename);
				free(inputFilename); free(outputFilename); free(maskFilename);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputFilename); free(maskFilename);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d).\n", argv[0], W);
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d).\n", argv[0], H);
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (mask=getUNCSlices3D(maskFilename, 0, 0, &mW, &mH, NULL, &maskNumSlices, &mDepth, &mFormat)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], maskFilename);
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (mW!=W) || (mH!=H) ){
		fprintf(stderr, "%s : mask image geometry (%d x %d) does not match that of the input (%d x %d)\n", argv[0], mW, mH, W, H);
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(mask, maskNumSlices, mW);
		exit(1);
	}
	if( (maskNumSlices != 1) && (maskNumSlices != actualNumSlices) ){
		fprintf(stderr, "%s : mask's number of slices (%d) are not the same as those of the input file or requested (%d).\n", argv[0], maskNumSlices, numSlices); 
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(mask, maskNumSlices, mW);
		exit(1);
	}
		
	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename); free(maskFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(mask, maskNumSlices, mW);
		exit(1);
	}

	printf("%s, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		maskSlice = (maskNumSlices>1) ? slice : 0;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<W;i++) for(j=0;j<H;j++) dataOut[s][i][j] = data[slice][i][j];
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
			dataOut[s][i][j] = (IS_WITHIN(mask[maskSlice][i][j], low, hi) ^ (inverseFlag)) ? data[slice][i][j] : offColor;
	}
	printf("\n");
	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(mask, maskNumSlices, mW);
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W);

	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	free(inputFilename); free(outputFilename); free(maskFilename);
	exit(0);
}
