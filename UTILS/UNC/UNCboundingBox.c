#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

#define	DEFAULT_NUM_PIXELS_AVERAGE		4
#define	DEFAULT_PIXEL_PERCENTAGE_DIFFERENCE	50.0

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n\
\n	-i input.unc -o output.unc -p 35.0 -n 4\
\n	Boundaries are defined as the change in average pixel\
\n	intensity over 4 pixels of over than 35 percent.\
\n\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-p pixelPercentageDifference\
\n	(A pixel is a boundary if the average of numPixels pixels on its left (or top)\
\n	is greater (for right/bottom boundaries) or less (for left/top boundaries)\
\n	by this percentage than the average of numOPixels pixels on its right (or bottom).\
\n	How many pixels the averaging will take, e.g. 'numPixels' can be selected using\
\n	the '-n' switch below.\
\n\
\n	If this option is not used, then the bounding box will be find by\
\n	scanning the image for the first and last non-background pixels.)]\
\n\
\n\t[-n numPixels\
\n	(How many pixels before or after current pixel to average when comparing\
\n	 changes of intensity at boundaries.)]\
\n\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** Use these options to select a region of interest\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	register int	globalX, globalY, globalW, globalH, i, j, k,
			x_min, x_max, y_min, y_max, ii, jj, bW, bH;
	int		numPixelsAverage = DEFAULT_NUM_PIXELS_AVERAGE, strictFlag = TRUE;
	register float	p1, p2, pixelPercentageDifference = DEFAULT_PIXEL_PERCENTAGE_DIFFERENCE;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:p:n:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'p': strictFlag = FALSE; pixelPercentageDifference = atof(optarg) / 100.0 ; break;
			case 'n': numPixelsAverage = atoi(optarg); break;
			case '9': copyHeaderFlag = TRUE; break;
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
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	if( (2*numPixelsAverage) >= W ){
		fprintf(stderr, "%s : number of pixels to average times two (-n switch, %d) should not exceed width (%d) or height (%d) of image.\n", argv[0], numPixelsAverage, W, H);
		free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
		
	printf("%s, slice : ", inputFilename); fflush(stdout);
	globalW = globalH = -1;
	globalX = globalY = 100000;
	if( strictFlag ){
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);
			x_min = y_min = x_max = y_max = 0;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				if( data[slice][i][j] > 0 ){ x_min = i; i = x+w+1; break; }
			for(i=x+w-1;i>=x;i--) for(j=y;j<y+h;j++)
				if( data[slice][i][j] > 0 ){ x_max = i; i = -1; break; }
			for(j=y;j<y+h;j++) for(i=x;i<x+w;i++)
				if( data[slice][i][j] > 0 ){ y_min = j; j = y+h+1; break; }
			for(j=y+h-1;j>=y;j--) for(i=x;i<x+w;i++)
				if( data[slice][i][j] > 0 ){ y_max = j; j = -1; break; }

			if( (bW=(x_max-x_min)) <= 0 ) continue;
			if( (bH=(y_max-y_min)) <= 0 ) continue;

			printf(" (%d, %d, %d, %d) ", x_min, y_min, bW, bH);
			globalX = MIN(x_min, globalX);
			globalY = MIN(y_min, globalY);
			globalW = MAX(bW, globalW);
			globalH = MAX(bH, globalH);
		}
	} else {
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);
			x_min = y_min = 10000000.0; x_max = y_max = -1;
			for(j=0;j<H;j++){
				x = x_min;
				for(i=numPixelsAverage;i<W-numPixelsAverage;i++){
					for(k=0,p1=0.0,p2=0.0;k<numPixelsAverage;k++){
						p1 += data[slice][i-k-1][j];
						p2 += data[slice][i+k][j];
					}
					if( (p2-p1) > pixelPercentageDifference*p1 ){ x = i + numPixelsAverage - 1; break; }
				}
				if( x < x_min ) x_min = x;
				x = x_max;
				for(i=W-numPixelsAverage;i>=numPixelsAverage;i--){
					for(k=0,p1=0.0,p2=0.0;k<numPixelsAverage;k++){
						p1 += data[slice][i-k-1][j];
						p2 += data[slice][i+k][j];
					}
					if( (p2-p1) < pixelPercentageDifference*p1 ){ x = i - numPixelsAverage + 1; break; }
				}
				if( x > x_max ) x_max = x;
			}
			for(i=0;i<W;i++){
				y = y_min;
				for(j=numPixelsAverage;j<H-numPixelsAverage;j++){
					for(k=0,p1=0.0,p2=0.0;k<numPixelsAverage;k++){
						p1 += data[slice][i][j-k-1];
						p2 += data[slice][i][j+k];
					}
					if( (p2-p1) > pixelPercentageDifference*p1 ){ y = j + numPixelsAverage - 1; break; }
				}
				if( y < y_min ) y_min = y;
				y = y_max;
				for(j=H-numPixelsAverage;j>=numPixelsAverage;j--){
					for(k=0,p1=0.0,p2=0.0;k<numPixelsAverage;k++){
						p1 += data[slice][i][j-k-1];
						p2 += data[slice][i][j+k];
					}
					if( (p2-p1) < pixelPercentageDifference*p1 ){ y = j - numPixelsAverage + 1; break; }
				}
				if( y > y_max ) y_max = y;
			}
			if( x_min >= W ) x_min = 0;
			if( x_max <= 0 ) x_max = W;
			if( y_min >= H ) y_min = 0;
			if( y_max <= 0 ) y_max = H;
			if( x_min > x_max ){ x = x_min; x_min = x_max; x_max = x; }
			if( y_min > y_max ){ y = y_min; y_min = y_max; y_max = y; }
			if( (w=(x_max-x_min)) == 0 ){
				fprintf(stderr, "%s : something is wrong with image, left and right boundaries (%d, %d) coincide.\n", argv[0], x_min, x_max);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}		
			if( (h=(y_max-y_min)) == 0 ){
				fprintf(stderr, "%s : something is wrong with image, top and bottom boundaries (%d, %d) coincide.\n", argv[0], y_min, y_max);
				free(inputFilename); free(outputFilename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
			printf(" (%d, %d, %d, %d) ", x_min, y_min, w, h);
			globalX = MIN(x_min, globalX);
			globalY = MIN(y_min, globalY);
			globalW = MAX(w, globalW);
			globalH = MAX(h, globalH);
		}
	}
	printf("\n");

	printf("%s : Bounding box for whole image: left-top corner at (%d, %d), dimensions are (%d, %d).\n", inputFilename, globalX, globalY, globalW, globalH);

	if( (dataOut=callocDATATYPE3D(numSlices, globalW, globalH)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * globalW * globalH * sizeof(DATATYPE));
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=globalX,ii=0;i<globalW+globalX;i++,ii++) for(j=globalY,jj=0;j<globalH+globalY;j++,jj++)
			dataOut[s][ii][jj] = data[slice][i][j];
	}	
	freeDATATYPE3D(data, actualNumSlices, W);
	if( !writeUNCSlices3D(outputFilename, dataOut, globalW, globalH, 0, 0, globalW, globalH, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlices, globalW);
	if( copyHeaderFlag )
		/* now copy the image info/title/header of source to destination */
		if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
			free(inputFilename); free(outputFilename);
			exit(1);
		}
	free(inputFilename); free(outputFilename);
	exit(0);
}



