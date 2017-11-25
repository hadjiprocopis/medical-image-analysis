#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

#define	AND	0
#define	OR	1
#define	XOR	2
#define	NOT	3
#define	AVERAGE	4
#define	MINIMUM	5
#define	MAXIMUM	6
#define	ADD	7
#define	SUBTRACT 8
#define	DIVIDE	9
#define	MULTIPLY 10
#define	MULTIPLY_NOT_ZERO 11
#define	DIVIDE_NOT_ZERO 12
#define	SIGMOID_NOT_ZERO	13

const	char	Examples[] = "\
\n	-i input1.unc -i input2.unc -A -o xx.unc\
\n\
\n	The resultant image 'xx.unc' will be created\
\n	as the pixel-by-pixel ANDing of images\
\n	'input1.unc' and 'input2.unc'.\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices. Use this option\
\n	 to define the first image, then the second image etc.\
\n	 At least two input images must be used.)\
\n\
\n\t[-r min:max\
\n	(rescale the output within the range min and max)]\
\n\
\n\t[-n min:max\
\n	(normalise the input within min and max. You want to do that\
\n	 when you specify one of the arithmetic operations (MULTIPLY,\
\n	 DIVIDE, etc.) in order to avoid overflow. For example, if\
\n	 you have 40 images to multiply, each with an average pixel\
\n	 value of 10000, then the result of the multiplication will\
\n	 be 10000^40! If you normalise the input image, say between\
\n	 0 and 1, your resolution will not be lost because the pixel\
\n	 values are integers but the normalised values are floats,\
\n	 and you will avoid the overflow. \
\n\
\n	 In unix,solaris, overflow occurs at +/- 256*256*256*128\
\n	 e.g. 2147483648)]\
\n\
\n\t[-v\
\n	(flag to reverse the image - i.e. pixels>0 become 0 and pixels==0 become 1: result is a binary image)]\
\n\
\n\t[-b B\
\n	(flag to binarise the image by thresholding each pixel at the intensity `B'.\
\n	 Pixels with intensity > B will become 1 and all other pixels will be zero.\
\n	 In case both -v and -b options are used, first we binarise and then we\
\n	 reverse.)\
\n\
\n\t[-t L:H [-t L2:H2 [-t L3:H3]]\
\n	(each input image will be thresholded so that pixels falling\
\n	 outside the range (L,H) are removed, prior to any operation.\
\n	 The threshold range can be the same for ALL input images.\
\n	 In which case use the '-t' option just once.\
\n	 Alternatively, each input image can be thresholded with\
\n	 a different range. In this case there must be as many '-t'\
\n	 options in the command line as there are input files (-i option).)]\
\n\
\n** One of the following is needed:\
\n\
\n\t -A\
\n	AND: The resultant image will be the result of:\
\n		<first image> AND <second image> [... AND ...]\
\n\t -O\
\n	OR: The resultant image will be the result of:\
\n		<first image> OR <second image> [... OR ...]\
\n\t -R\
\n	XOR: The resultant image will be the result of:\
\n		<first image> XOR <second image> [... XOR ...]\
\n		(XOR: one and not the other OR not one and the other)\
\n\t -N\
\n	NOT: The resultant image will be the result of:\
\n		<first image> NOT <second image> [... NOT ...]\
\n\t -V\
\n	AVERAGE: The resultant image will be the result of:\
\n		(<first image> + <second image> [... + ...]) / <number of images>\
\n\t -M\
\n	MINIMUM: The resultant image will be the result of:\
\n		MINIMUM(<first image>, <second image> [... , ...])\
\n\t -X\
\n	MAXIMUM: The resultant image will be the result of:\
\n		MAXIMUM(<first image>, <second image> [... , ...])\
\n\t -D\
\n	ADD: The resultant image will be the result of:\
\n		MINIMUM(256*128-1, <first image> + <second image> [... + ...])\
\n\t -U\
\n	SUBTRACT: The resultant image will be the result of:\
\n		MAXIMUM(0, <first image> - <second image> [... - ...])\
\n\t -I\
\n	DIVIDE: The resultant image will be the result of:\
\n		MAXIMUM(0, <first image> / <second image> [... - ...])\
\n		the result will be zero for pixels of the second image\
\n		which are zero.)\
\n\t -S\
\n	DIVIDE_NOT_ZERO: as above but only between non-zero pixels\
\n\
\n\t -P\
\n	MULTIPLY: The resultant image will be the result of:\
\n		MINIMUM(256*128-1, <first image> * <second image> [... - ...])\
\n\t -Q\
\n	MULTIPLY_NOT_ZERO: as above but will multiply only pixels that are\
\n		not zero.\
\n\t -T factor\
\n	SPECIAL		\
\n\
\n** Notice that the AND, OR, NOT and XOR operations produce binary\
\n   images, e.g. masks, e.g. pixel values will be either zero or one.\
\n\
\n   The 'ADD' and 'SUBTRACT' operations will fix the result within\
\n   the range: 0 and 256*128-1.\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use this options to select a region of interest\
\n   You may use one or more or all of '-w', '-h', '-x' and\
\n   '-y' once. You may use one or more '-s' options in\
\n   order to specify more slices. Slice numbers start from 1.\
\n   These parameters are optional, if not present then the\
\n   whole image, all slices will be used.\
\n\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\
\nThis program will perform a specified operation\
\non UNC images.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define	_SIGMOID(x,a,b) (1.0/(1.0+exp(-(a)*(x)-(b))))

int	main(int argc, char **argv){
	DATATYPE	***data = NULL, ***dataOut = NULL;
	char		*inputFilename[1000],
			*outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0, numInputFilenames = 0, f, ff,
			opFlag = -1;

	float		***fDataOut = NULL, ***nDataOut = NULL;
	int		W2 = -1, H2 = -1, actualNumSlices2 = 0,
			reverseFlag = FALSE, binariseThreshold = -1,
			rescaleOutputRangeMin = 0, rescaleOutputRangeMax = 0;
	register int	i, j, k, s, slice;
	spec		tSpec[1000];
	int		numtSpec = 0;
	float		minPixel, maxPixel,
			normaliseInputMin = 0.0, normaliseInputMax = 0.0,
			sigmoidFactor1 = 1.0, sigmoidFactor2 = 0.0;

	while( (optI=getopt(argc, argv, "i:es:w:h:x:y:o:AORNVMXDUIPQST:r:9t:vb:n:")) != EOF)
		switch( optI ){
			case 'i': inputFilename[numInputFilenames++] = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'A': opFlag = AND; break;
			case 'O': opFlag = OR; break;
			case 'R': opFlag = XOR; break;
			case 'N': opFlag = NOT; break;
			case 'V': opFlag = AVERAGE; break;
			case 'M': opFlag = MINIMUM; break;
			case 'X': opFlag = MAXIMUM; break;
			case 'D': opFlag = ADD; break;
			case 'U': opFlag = SUBTRACT; break;
			case 'I': opFlag = DIVIDE; break;
			case 'S': opFlag = DIVIDE_NOT_ZERO; break;
			case 'P': opFlag = MULTIPLY; break;
			case 'Q': opFlag = MULTIPLY_NOT_ZERO; break;
			case 'T': opFlag = SIGMOID_NOT_ZERO; sscanf(optarg, "%f:%f", &sigmoidFactor1, &sigmoidFactor2); break;
			case 'r': sscanf(optarg, "%d:%d", &rescaleOutputRangeMin, &rescaleOutputRangeMax); break;
			case 'n': sscanf(optarg, "%f:%f", &normaliseInputMin, &normaliseInputMax); break;
			case 'v': reverseFlag = TRUE; break;
			case 'b': binariseThreshold = atoi(optarg); break;
			case 't': sscanf(optarg, "%d:%d", &(tSpec[numtSpec].low), &(tSpec[numtSpec].high)); numtSpec++; break;

			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;

			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}

	if( numInputFilenames < 2 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "At least two input filenames must be specified.\n");
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]);
		exit(1);
	}
	if( opFlag < 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An operation must be specified.\n");
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]);
		free(outputFilename);
		exit(1);
	}
	if( (numtSpec > 1) && (numtSpec!=numInputFilenames) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "The number of threshold levels (which seems to be %d) can be zero (e.g. no threshold to be applied to any input image - that is do not use the '-t' option at all) or one (e.g. one threshold to be applied to all the input images) or as many as the number of input images (which is %d) in which case each threshold corresponds to one input UNC volume in the order of appearance in the command line.\n", numtSpec, numInputFilenames);
		exit(1);
	}

for(ff=0;ff<numInputFilenames;ff++){
	fprintf(stdout, "%s : reading (%d of %d), ", inputFilename[ff], ff+1, numInputFilenames); fflush(stdout);
	if( (data=getUNCSlices3D(inputFilename[ff], 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename[ff]);
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
		exit(1);
	}
	if( ff == 0 ){
		W2 = W; H2 = H; actualNumSlices2 = actualNumSlices;
		if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
		else {
			for(s=0;s<numSlices;s++){
				if( slices[s] >= actualNumSlices ){
					fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename[ff]);
					for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
					exit(1);
				} else if( slices[s] < 0 ){
					fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
					for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
					exit(1);
				}
			}
		}
		if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
			for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		if( (fDataOut=callocFLOAT3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
			for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		if( (nDataOut=callocFLOAT3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
			for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
	}
	if( (W2!=W) || (H2!=H) || (actualNumSlices2!=actualNumSlices) ){
		fprintf(stderr, "%s : input files have width (%d, %d), height (%d, %d) and/or number of slices (%d, %d).\n", argv[0], W, W2, H, H2, actualNumSlices, actualNumSlices2);
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W); freeFLOAT3D(fDataOut, numSlices, W);
		exit(1);
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W); freeFLOAT3D(fDataOut, numSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W); freeFLOAT3D(fDataOut, numSlices, W);
		exit(1);
	}

	if( numtSpec > 0 ){
		if( numtSpec > 1 ) f = ff; else f = 0;
		fprintf(stderr, "%s , %s : removing pixels falling outside the range %d and %d ... ", argv[0], inputFilename[ff], tSpec[f].low, tSpec[f].high); fflush(stderr);
		for(s=0,k=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) if( !IS_WITHIN(data[slice][i][j], tSpec[f].low, tSpec[f].high) ){ data[slice][i][j] = 0; k++; }
		}
		fprintf(stderr, "%d pixels removed.\n", k);
	}

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			nDataOut[s][i][j] = (float )data[slice][i][j];
			dataOut[s][i][j] = 0;
		}
	}
	if( normaliseInputMin < normaliseInputMax ){
		minPixel = maxPixel = nDataOut[0][x][y];
		for(s=0;s<numSlices;s++){
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( nDataOut[s][i][j] < minPixel ) minPixel = nDataOut[s][i][j];
				if( nDataOut[s][i][j] > maxPixel ) maxPixel = nDataOut[s][i][j];
			}
		}
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				nDataOut[s][i][j] = SCALE_OUTPUT(nDataOut[s][i][j], normaliseInputMin, normaliseInputMax, minPixel, maxPixel);
	}

	fprintf(stdout, "processing :"); fflush(stdout);
	if( ff == 0 ){
		/* initialise */
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			fprintf(stdout, " %d", slice+1); fflush(stdout);
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				fDataOut[s][i][j] = nDataOut[s][i][j];
		}
	} else {
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			fprintf(stdout, " %d", slice+1); fflush(stdout);
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				switch( opFlag ){
					case	AND: fDataOut[s][i][j] = ((fDataOut[s][i][j]>0) && (nDataOut[s][i][j]>0)) ? 1 : 0 ; break;
					case	OR : fDataOut[s][i][j] = ((fDataOut[s][i][j]>0) || (nDataOut[s][i][j]>0)) ? 1 : 0 ; break;
					case	XOR: fDataOut[s][i][j] = ((((fDataOut[s][i][j]>0) && (nDataOut[s][i][j]==0))) || (((fDataOut[s][i][j]==0) && (nDataOut[s][i][j]>0)))) ? 1 : 0; break;

					case	ADD:
					case	AVERAGE: fDataOut[s][i][j] += nDataOut[s][i][j]; break;
					case	MINIMUM: fDataOut[s][i][j] = MIN(dataOut[s][i][j], nDataOut[s][i][j]); break;
					case	MAXIMUM: fDataOut[s][i][j] = MAX(dataOut[s][i][j], nDataOut[s][i][j]); break;
					case	SUBTRACT: fDataOut[s][i][j] -= nDataOut[s][i][j]; break;
					case	MULTIPLY: fDataOut[s][i][j] *= nDataOut[s][i][j]; break;
					case	MULTIPLY_NOT_ZERO:
						if( fDataOut[s][i][j] == 0 ) fDataOut[s][i][j] = nDataOut[s][i][j];
						else if( nDataOut[s][i][j] != 0 ) fDataOut[s][i][j] *= nDataOut[s][i][j];
						break;
					case	DIVIDE: fDataOut[s][i][j] = nDataOut[s][i][j]==0.0 ? 0.0 : fDataOut[s][i][j] / nDataOut[s][i][j]; break;
					case	DIVIDE_NOT_ZERO:
						if( fDataOut[s][i][j] == 0 ) fDataOut[s][i][j] = nDataOut[s][i][j];
						else if( nDataOut[s][i][j] != 0 ) fDataOut[s][i][j] /= nDataOut[s][i][j];
						break;
					case	SIGMOID_NOT_ZERO:
						if( (s==13)&&(i==100)&&(j==120) ) printf("%f, %f\n", nDataOut[s][i][j], _SIGMOID(nDataOut[s][i][j], sigmoidFactor1, sigmoidFactor2));
						if( fDataOut[s][i][j] == 0 ) fDataOut[s][i][j] = _SIGMOID(nDataOut[s][i][j], sigmoidFactor1, sigmoidFactor2);
						else fDataOut[s][i][j] *= _SIGMOID(nDataOut[s][i][j], sigmoidFactor1, sigmoidFactor2);

						if( (s==13)&&(i==100)&&(j==120) ) printf("%f\n", fDataOut[s][i][j]);
						break;
				}
		}
	}
	freeDATATYPE3D(data, actualNumSlices, W);
	fprintf(stdout, "\n");
}

	if( opFlag == AVERAGE )
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				fDataOut[s][i][j] /= (float )numInputFilenames;

	if( normaliseInputMin < normaliseInputMax ){
		minPixel = maxPixel = fDataOut[0][x][y];
		for(s=0;s<numSlices;s++){
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( fDataOut[s][i][j] < minPixel ) minPixel = fDataOut[s][i][j];
				if( fDataOut[s][i][j] > maxPixel ) maxPixel = fDataOut[s][i][j];
			}
		}
		printf("%s : rescaling (%.0f,%.0f) within (%d,%d) for the output.\n", argv[0], minPixel, maxPixel, rescaleOutputRangeMin, rescaleOutputRangeMax);
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				dataOut[s][i][j] = ROUND(SCALE_OUTPUT(fDataOut[s][i][j], rescaleOutputRangeMin, rescaleOutputRangeMax, minPixel, maxPixel));
	} else 
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				dataOut[s][i][j] = ROUND(MIN(32767, MAX(0, fDataOut[s][i][j])));

	if( binariseThreshold > 0 )
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				dataOut[s][i][j] = (dataOut[s][i][j]>binariseThreshold) ? 1 : 0;
	if( reverseFlag )
		for(s=0;s<numSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				dataOut[s][i][j] = (dataOut[s][i][j]==0) ? 1 : 0;

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W); freeFLOAT3D(fDataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, W); freeFLOAT3D(fDataOut, numSlices, W);
	freeFLOAT3D(nDataOut, numSlices, W);

	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename[0], outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename[0], outputFilename);
		for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
		exit(1);
	}

	for(f=0;f<numInputFilenames;f++) free(inputFilename[f]); free(outputFilename);
	exit(0);
}
