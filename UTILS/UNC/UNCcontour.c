#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc -o output.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(Output filename)\
\n\
\n\t[-c low:high:color [-c low:high:color [...] ]\
\n	(include only values between low and high.\
\n	 If 'color' is >= 0 then each pixel falling\
\n	 in the isocontour will take the value 'color',\
\n	 otherwise it will retain its own value.\
\n	 Use this option as many times as you like.)\
\n	 \
\n\t[-Z\
\n	(do not count black pixel)]\
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
\nThis program will take a UNC file and for each pixel, it\
\nwill calculate the absolute difference with each of its\
\n8 neighbours. These differences will be written to 8 output\
\nfiles. A ninth output file will contain the mean of the\
\n8 differences.\
\n\
\nUsing the -c option, a thresholding may be done where only\
\ncertain ranges of differences are counted.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut[8+1];
	char		*inputFilename = NULL, *outputBasename = NULL, dummy[1000],
			copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	register int	i, j, k, l, m, n, s, value;
	register	float mean;
	int		ignoreBlackPixel = FALSE, numIsoContours = 0, totalNumPixels = 0, value2;
	spec		isoContour[100];

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:Zc:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'Z': ignoreBlackPixel = TRUE; break;
			case 'c': sscanf(optarg, "%d:%d:%d", &(isoContour[numIsoContours].low), &(isoContour[numIsoContours].high), &(isoContour[numIsoContours].newValue)); numIsoContours++; break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(outputBasename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		free(inputFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		free(inputFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	for(i=0;i<8+1;i++)
		if( (dataOut[i]=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
			free(inputFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W);
			for(j=0;j<i;j++) freeDATATYPE3D(dataOut[j], numSlices, W);
			exit(1);
		}

	printf("%s, init : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		printf("%d ", s+1); fflush(stdout);
		for(i=0;i<W;i++) for(j=0;j<H;j++) for(k=0;k<8+1;k++) dataOut[k][s][i][j] = 0;
	}
	printf("\n");

	printf("%s, process : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=x+1;i<x+w-1;i++) for(j=y+1;j<y+w-1;j++){
			m = 0;
			mean = 0.0;
			for(k=-1;k<=1;k++) for(l=-1;l<=1;l++){
				if( (k==0) && (l==0) ) continue;
				m++;
				if( ignoreBlackPixel && ((data[slice][i+k][j+l] == 0) || (data[slice][i][j] == 0)) ) continue;
				value = ABS( data[slice][i][j] - data[slice][i+k][j+l] );
				mean += value;
				if( numIsoContours == 0 ){ dataOut[m-1][s][i][j] = value; totalNumPixels++; }
				else {
					value2 = 0;
					for(n=0;n<numIsoContours;n++)
						if( (value>=isoContour[n].low) && (value<isoContour[n].high) ){
							dataOut[m-1][s][i][j] = isoContour[n].newValue == -1 ? value : isoContour[n].newValue;
							value2 = 1;
						}
					totalNumPixels += value2;
				}
			}
			mean /= 8.0;
			if( numIsoContours == 0 ) dataOut[8][s][i][j] = ROUND(mean);
			else
				for(n=0;n<numIsoContours;n++)
					if( (mean>=isoContour[n].low) && (mean<isoContour[n].high) )
						dataOut[8][s][i][j] = isoContour[n].newValue == -1 ? mean : isoContour[n].newValue;
		}
	}
	printf(" -> %d pixels\n", totalNumPixels);
	freeDATATYPE3D(data, actualNumSlices, W);

	printf("%s, writing : ", inputFilename); fflush(stdout);
	for(k=0;k<8+1;k++){
		printf("%d ", k+1); fflush(stdout);
		sprintf(dummy, "%s_%d", outputBasename, k+1);
		if( !writeUNCSlices3D(dummy, dataOut[k], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
			free(inputFilename); free(outputBasename);
			for(j=0;j<8+1;j++) freeDATATYPE3D(dataOut[j], numSlices, W);
			exit(1);
		}
		/* now copy the image info/title/header of source to destination */
		if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
			free(inputFilename); free(outputBasename);
			for(j=0;j<8+1;j++) freeDATATYPE3D(dataOut[j], numSlices, W);
			exit(1);
		}
	}
	printf("\n");
	for(k=0;k<8+1;k++) freeDATATYPE3D(dataOut[k], numSlices, W);
	free(inputFilename); free(outputBasename);

	exit(0);
}
