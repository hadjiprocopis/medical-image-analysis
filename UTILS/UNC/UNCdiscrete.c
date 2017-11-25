#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -o outputFilename\
\n	(output file)\
\n\
\n\t[-Z\
\n	(ignore the background - black - pixel)]\
\n\
\n\t[-W\
\n	(do calculations of ranges over whole\
\n	 volume rather than slice by slice.)]\
\n\
\n* Only 1 of the two following options is expected *\
\n\t -r stepDiscreteLevel\
\n	(the distance between one discrete level\
\n	 and the next)\
\n\
\n* OR *\
\n\
\n\t -n numDiscreteLevels\
\n	(the number of discrete levels)\
\n	\
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
\nThis program will take a UNC volume and reduce the number\
\nof unique pixel intensities. The number of final number\
\nof colors (option '-n') or the width between adjacent\
\ncolor levels (option '-r') should be given.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, actualNumSlices = 0, slice,
			optI, slices[1000], allSlices = 0;
	int		i, j, numDiscreteLevels = 0, stepDiscreteLevels = 0,
			ignoreBlackPixel = FALSE, minPixel[200], maxPixel[200],
			wholeBrainMinPixel, wholeBrainMaxPixel,
			doWholeBrainFlag = FALSE, start;
	float		width;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:Zr:n:W9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'n': numDiscreteLevels = atoi(optarg); break;
			case 'r': stepDiscreteLevels = atoi(optarg); break;
			case 'Z': ignoreBlackPixel = TRUE; break;
			case 'W': doWholeBrainFlag = TRUE; break;
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
		if( outputFilename != NULL ) free(outputFilename);
		exit(1);
	}
	if( outputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( (numDiscreteLevels>0) && (stepDiscreteLevels>0) ){
		fprintf(stderr, "%s : only one of the '-n' and '-r' options must be used.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}		
	if( (numDiscreteLevels<=0) && (stepDiscreteLevels<=0) ){
		fprintf(stderr, "%s : at least one of the '-n' and '-r' options must be used.\n", argv[0]);
		free(inputFilename); free(outputFilename);
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
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d slices %dx%d each.\n", argv[0], numSlices, W, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	/* find min/max pixel for specified slices */
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		minPixel[s] = 32768; maxPixel[s] = -1;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			dataOut[s][i][j] = 0;
			if( doWholeBrainFlag && (data[slice][i][j]==0) ) continue;
			if( data[slice][i][j] > maxPixel[s] ) maxPixel[s] = data[slice][i][j];
			if( data[slice][i][j] < minPixel[s] ) minPixel[s] = data[slice][i][j];
		}
	}
	wholeBrainMinPixel = minPixel[0]; wholeBrainMaxPixel = maxPixel[0];
	for(s=0;s<numSlices;s++){
		if( maxPixel[s] > wholeBrainMaxPixel ) wholeBrainMaxPixel = maxPixel[s];
		if( minPixel[s] < wholeBrainMinPixel ) wholeBrainMinPixel = minPixel[s];
	}
	
	if( numDiscreteLevels > 0 ){
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( doWholeBrainFlag ){
				start = wholeBrainMinPixel;
				width = ((float )(wholeBrainMaxPixel - wholeBrainMinPixel)) / ((float )numDiscreteLevels);
			} else {
				start = minPixel[s];
				width = ((float )(maxPixel[s] - minPixel[s])) / ((float )numDiscreteLevels);
			}
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( doWholeBrainFlag && (data[slice][i][j]==0) ) continue;
				dataOut[s][i][j] = ROUND(width * ROUND(((float )(data[slice][i][j] - start)) / width)) + start;
			}
		}
		printf("%s : got approximately a separation of %d\n", argv[0], ROUND(((float )(wholeBrainMaxPixel - wholeBrainMinPixel)) / ((float )numDiscreteLevels)));
	} else {
		width = (float )stepDiscreteLevels;
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( doWholeBrainFlag ) start = wholeBrainMinPixel;
			else start = minPixel[s];
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( doWholeBrainFlag && (data[slice][i][j]==0) ) continue;
				dataOut[s][i][j] = ROUND(width * ROUND(((float )(data[slice][i][j] - start)) / width)) + start;
			}
		}
		printf("%s : got approximately %d levels\n", argv[0], ROUND((wholeBrainMaxPixel - wholeBrainMinPixel) / width));
	}
		

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D failed for file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	freeDATATYPE3D(data, actualNumSlices, W);
	freeDATATYPE3D(dataOut, numSlices, W);

	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	free(inputFilename);
	free(outputFilename);

	exit(0);
}
