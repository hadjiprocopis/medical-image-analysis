#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i inp.unc -o out.unc -r 0:32767 -9 -a\
\n\
\nwill scale the pixel values of 'inp.unc' to the\
\nrange 0:32767 (e.g. the whole two-byte gamut).\
\n\
\n	-i inp.unc -o out.unc -m 12.32 -9\
\n\
\nwill multiply each pixel value with the constant '12.32'\
\nThe result will be rounded (e.g. 1.2 -> 1 and 1.6 -> 2)\
\n";

const	char	Usage[] = "options as follows:\
\n -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n -o outputFilename\
\n	(Output filename)\
\n\
\n[-a\
\n	(Scaling will be done in a slice-by-slice\
\n	 fashion. Note that the default is that\
\n	 scaling is done over all slices.)]\
\n\
\n[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n** One of the two following options must be used:\
\n-r low:high\
\n	(The new range of pixel values.)\
\n\
\n	or\
\n-m M\
\n	(multiply each pixel by the number 'M'.)\
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
\nThis program will scale the range of the pixel values of\
\nthe input UNC file to the specified range (-r option)\
\nOR will multiply each pixel value with a constant factor\
\n(hence achieving a user-defined scaling or shrinking for this matter\
\nif the constant factor is between 0 and 1).\
\n\
\nWhen using the '-r' option (e.g. let the program automatically\
\nscale the pixels to use the whole range of the two-byte gamut)\
\nscaling can be done in a slice-by-slice fashion or over the whole\
\nimage. By default it is done over the whole image, but this can\
\nchange by using the '-a' option.\
\n\
\nThis program has been compiled for Linux and Solaris.\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001 (livantes@soi.city.ac.uk)";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	float		outputRangeMin = -1, outputRangeMax = -1,
			inputRangeMin = -1, inputRangeMax = -1,
			multiplyFactor = -1;
	int		i, j, doScaleOverWholeImage = TRUE;
	char		verboseFlag = FALSE;

	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:r:eac9m:v")) != EOF)
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
			case 'r': sscanf(optarg, "%f:%f", &outputRangeMin, &outputRangeMax); break;
			case 'a': doScaleOverWholeImage = FALSE; break;
			case 'm': multiplyFactor = atof(optarg); break;
			case 'v': verboseFlag = TRUE; break;
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
	if( (multiplyFactor==-1) && ((outputRangeMin<0) || (outputRangeMax<=0) || (outputRangeMin>=outputRangeMax)) ){
		fprintf(stderr, "%s : invalid output pixel range specified.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( ((outputRangeMin==-1)||(outputRangeMax==-1)) && (multiplyFactor==-1) ){
		fprintf(stderr, "%s : one of the '-r' or the '-m' options must be used.\n", argv[0]);
		free(inputFilename); free(outputFilename);
		exit(1);
	}
	if( (outputRangeMin>=0) && (outputRangeMax>=0) && (multiplyFactor>=0) ){
		fprintf(stderr, "%s : only one of the '-r' or the '-m' options must be used.\n", argv[0]);
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
		fprintf(stderr, "%s : call to callocDATATYPE3D has failed for %d x %d x %d.\n", argv[0], numSlices, W, H);
		free(inputFilename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	for(s=0;s<numSlices;s++) for(i=x;i<x+w;i++) for(j=y;j<y+h;j++) dataOut[s][i][j] = 0;

	if( multiplyFactor == -1 ){
		if( doScaleOverWholeImage ){
			inputRangeMin = inputRangeMax = data[(allSlices==0)?slices[0]:0][x][y];
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
					if( data[slice][i][j] == 0 ) continue;
					if( data[slice][i][j] < inputRangeMin ) inputRangeMin = data[slice][i][j];
					if( data[slice][i][j] > inputRangeMax ) inputRangeMax = data[slice][i][j];
				}
			}
			if( verboseFlag )
				printf("%s : input pixel range=(%d, %d) over all slices, output=(%d, %d)\n", inputFilename, (int )inputRangeMin, (int )inputRangeMax, (int )outputRangeMin, (int )outputRangeMax);
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					if( data[slice][i][j] > 0 )
						dataOut[s][i][j] = (DATATYPE )ROUND(SCALE_OUTPUT(data[slice][i][j], outputRangeMin, outputRangeMax, inputRangeMin, inputRangeMax));
			}
		} else {
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				inputRangeMin = inputRangeMax = data[slice][x][y];
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
					if( data[slice][i][j] == 0 ) continue;
					if( data[slice][i][j] < inputRangeMin ) inputRangeMin = data[slice][i][j];
					if( data[slice][i][j] > inputRangeMax ) inputRangeMax = data[slice][i][j];
				}
				if( verboseFlag )
					printf("%s : slice %d, input pixel range=(%d, %d), output=(%d, %d)\n", inputFilename, slice+1, (int )inputRangeMin, (int )inputRangeMax, (int )outputRangeMin, (int )outputRangeMax);
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
					if( data[slice][i][j] > 0 )
						dataOut[s][i][j] = (DATATYPE )ROUND(SCALE_OUTPUT(data[slice][i][j], outputRangeMin, outputRangeMax, inputRangeMin, inputRangeMax));
			}
		}
	} else
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				dataOut[s][i][j] = (DATATYPE )ROUND(((float )(data[slice][i][j])) * multiplyFactor);
		}

	freeDATATYPE3D(data, actualNumSlices, W);

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
	free(inputFilename); free(outputFilename);
	exit(0);
}
