#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* what is the max number of specs ? */
#define MAX_SPECS       30


const	char	Examples[] = "\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -p 25:35:200 -f 40:50:500 -p 80:100:-1\
\n\
\nThis example will operate only on slices 0,1 and 4 and the\
\nregion bounded by (10,10,23,23) of file 'inp.unc'.\
\n** It will change the intensities of those pixels whose\
\n   intensity is between 5 (incl) and 15 excl to 100.\
\n** It will change the intensities of those pixels whose\
\n   intensity is between 25 (incl) and 35 excl to 200.\
\n** It will change the intensities of those pixels whose\
\n   frequency of occurence (in the histogram)\
\n   is between 40 (incl) and 50 excl to 500.\
\n** It will *LEAVE UNCHANGED* those pixels whose\
\n   intensity is between 80 (incl) and 100 excl\
\n   (notice the -1 value for 'newColor')\
\n** All other pixels including all pixels falling outside\
\n   the region of interest and slices specified will be\
\n   left unchanged to original intensities.\
\n\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -p 25:35:200 -f 40:50:500 -p 80:100:-1 -d 1000\
\n\
\nAs before, but all other pixels falling within ROI and\
\nselected slices will have their intensities change to 1000.\
\n\
\n	-i inp.unc -o out.unc -s 0 -s 1 -s 4 -x 10 -y 10 -w 13 -h 13 -p 5:15:100 -f 40:50:500 -a 1000\
\n\
\nThe pixels that satisfy the following two criteria and\
\nwithin the ROI will have their pixel values changed to\
\n1000:\
\n** intensity is between 5 (incl) and 15 excl to 100\
\n** frequency of occurence (in the histogram)\
\n   is between 40 (incl) and 50 excl to 500.";

const	char	Usage[] = "options as follows:\
\n -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n -o outputFilename\
\n	(Output filename)\
\n\
\n** at least one of either the -p ot -f options below\
\n** must be present.\
\n** You may use as many combinations of -p and -f as you wish.\
\n** Notice the field separator ':'\
\n\
\n[-p low:high:newColor\
\n	(substitutes all PIXEL values between low (incl)\
\n	 and high (excl) with newColor.\
\n	 If newColor is '-1' then these pixels will be\
\n	 left unchanged.)]\
\n\
\n[-f low:high:newColor\
\n	(substitutes all pixel values with FREQUENCY of\
\n	 occurence between low (incl) and high (excl) with\
\n	 newColor.\
\n	 If newColor is '-1' then these pixels will be\
\n	 left unchanged.)]\
\n\
\n[-d defaultColor\
\n	(Color to replace all pixels left unchanged by all\
\n	 -p and -f operations, if not specified unchanged\
\n	 pixels will show with their original value).]\
\n\
\n[-a newColor\
\n	(Option indicating whether results of -p and -f\
\n	 operations should be ANDed together, rather\
\n	 than ORed (default)).\
\n	 The pixel intensity of qualifying pixels\
\n	 will be newColor.\
\n\
\n	 By AND we mean that threshold pixels will\
\n	 be those that were within the limits of each\
\n	 '-p' and '-f' specification.\
\n	 By OR we mean that threshold pixels will be\
\n	 those that were within the limits of at\
\n	 least one '-p' or '-f' specification.\
\n\
\n	 This option is not relevant when only '-p'\
\n	 or only '-f' options were specified. It has\
\n	 to operate on a mixture of '-p' and '-f'.)]\
\n\
\n[-v\
\n	(Flag to tell the program to be verbose and\
\n	 report what is doing etc.)]\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut;
	char		*inputFilename = NULL, *outputFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	spec		pixelSpec[MAX_SPECS], frequencySpec[MAX_SPECS];
	DATATYPE	**dataP = NULL, **dataF = NULL;
	char		**changeMapP = NULL, **changeMapF = NULL, ANDcolorFlag = FALSE;
	int		i, j, numFrequencySpecs = 0, numPixelSpecs = 0, count, totalCount,
			defaultColor = THRESHOLD_LEAVE_UNCHANGED, ANDcolor = 0, verboseFlag = FALSE;
	histogram	*hist;

	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:p:f:d:a:ev9")) != EOF)
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
			case 'p': sscanf(optarg, "%d:%d:%d", &(pixelSpec[numPixelSpecs].low), &(pixelSpec[numPixelSpecs].high), &(pixelSpec[numPixelSpecs].newValue));
				  numPixelSpecs++; break;
			case 'f': sscanf(optarg, "%d:%d:%d", &(frequencySpec[numFrequencySpecs].low), &(frequencySpec[numFrequencySpecs].high), &(frequencySpec[numFrequencySpecs].newValue));
				  numFrequencySpecs++; break;
			case 'd': defaultColor = atoi(optarg); break;
			case 'a': ANDcolor = atoi(optarg); ANDcolorFlag = TRUE; break;
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
	if( (numPixelSpecs<=0) && (numFrequencySpecs<=0) ){
		fprintf(stderr, "%s : At least one pixel or frequency specification ('-p' or '-f' switches) must be specified.\n", argv[0]);
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

	if( numPixelSpecs > 0 ){
		if( (dataP=callocDATATYPE2D(W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %dx%d DATATYPE of size %zd bytes.\n", argv[0], W, H, sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		if( (changeMapP=(char **)calloc2D(W, H, sizeof(char))) == NULL ){
			fprintf(stderr, "%s : could not allocate %dx%d chars.\n", argv[0], W, H);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(dataP, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
	}
	if( numFrequencySpecs > 0 ){
		if( (dataF=callocDATATYPE2D(W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %dx%d DATATYPE of size %zd bytes.\n", argv[0], W, H, sizeof(DATATYPE));
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			if( dataP != NULL ) freeDATATYPE2D(dataP, W);
			if( changeMapP != NULL ) free2D((void **)changeMapP, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
		if( (changeMapF=(char **)calloc2D(W, H, sizeof(char))) == NULL ){
			fprintf(stderr, "%s : could not allocate %dx%d chars.\n", argv[0], W, H);
			free(inputFilename); free(outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE2D(dataF, W);
			if( dataP != NULL ) freeDATATYPE2D(dataP, W);
			if( changeMapP != NULL ) free2D((void *)changeMapP, W);
			freeDATATYPE3D(dataOut, numSlices, W);
			exit(1);
		}
	}

	if( !verboseFlag ) printf("%s : slice ", argv[0]);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( verboseFlag ) printf("%s : slice %d\n", inputFilename, slice+1);
		else { printf("%d ", slice+1); fflush(stdout); }

		for(i=0;i<W;i++) for(j=0;j<H;j++) dataOut[s][i][j] = data[slice][i][j];

		/* do pixel intensity thresholding */
		if( numPixelSpecs > 0 ){
			for(i=0;i<W;i++) for(j=0;j<H;j++){ dataP[i][j] = data[slice][i][j]; changeMapP[i][j] = unchanged; }
			count = pixel_threshold2D(dataP, x, y, w, h, pixelSpec, numPixelSpecs, defaultColor, changeMapP);
			totalCount += count;
			if( verboseFlag ){
				printf("%s : pixel intensity thresholding for slice %d, changed %d pixels\n", argv[0], slice, count);
				for(i=0;i<numPixelSpecs;i++){
					if( pixelSpec[i].newValue < 0 ){
						printf(" * intensities from %d to %d have been left unchanged.\n", pixelSpec[i].low, pixelSpec[i].high); 
					} else{ printf(" * intensities from %d to %d have been changed to %d\n", pixelSpec[i].low, pixelSpec[i].high, pixelSpec[i].newValue);}
				}
				if( defaultColor == THRESHOLD_LEAVE_UNCHANGED )
					printf(" * all other pixel intensities were left unchanged\n");
				else printf("  all other pixels intensities have been changed to %d\n", defaultColor);
			}
		}

		if( numFrequencySpecs > 0 ){
			for(i=0;i<W;i++) for(j=0;j<H;j++){ dataF[i][j] = data[slice][i][j]; changeMapF[i][j] = unchanged; }
			/* we have to calculate the histogram first */
			if( (hist=histogram2D(dataF, x, y, w, h, 1)) == NULL ){
				fprintf(stderr, "%s : call to histogram2D failed.\n", argv[0]);
				if( dataP ) freeDATATYPE2D(dataP, W);
				if( changeMapP ) free2D((void **)changeMapP, W);
				if( dataF ) freeDATATYPE2D(dataF, W);
				if( changeMapF ) free2D((void **)changeMapF, W);
				freeDATATYPE3D(data, actualNumSlices, W);
				freeDATATYPE3D(dataOut, numSlices, W);
				exit(1);
			}
			count = frequency_threshold2D(dataF, x, y, w, h, frequencySpec, numFrequencySpecs, defaultColor, *hist, changeMapF);
			totalCount += count;
			if( verboseFlag ){
				printf("%s : pixel intensity frequencies thresholding for slice %d, changed %d pixels\n", argv[0], slice, count);
				for(i=0;i<numFrequencySpecs;i++){
					if( frequencySpec[i].newValue < 0 ){
						printf(" * frequencies from %d to %d have been left unchanged\n", frequencySpec[i].low, frequencySpec[i].high);
					} else {
						printf(" * frequencies from %d to %d have been changed to %d\n", frequencySpec[i].low, frequencySpec[i].high, frequencySpec[i].newValue);
					}
				}
				if( defaultColor == THRESHOLD_LEAVE_UNCHANGED )
					printf(" * all other pixel intensities were left unchanged\n");
				else printf("  all other pixels intensities have been changed to %d\n", defaultColor);
			}
			destroy_histogram(hist);
		}

		/* now calculate final result as AND/OR etc. */
		if( dataP && dataF && ANDcolorFlag ){
			int	ANDCount = 0;
			if( verboseFlag )
				printf("anding results: "); fflush(stdout);
			if( ANDcolor >= 0 ){ /* replace the result with ANDcolor */
				for(i=x;i<x+w;i++)
					for(j=y;j<y+h;j++) /* changed and unchanged are defined in threshold.h (enums) to mean pixel has changed or not */
						if( (changeMapP[i][j]==changed) &&
						    (changeMapF[i][j]==changed) ){
							dataOut[s][i][j] = ANDcolor;
							ANDCount++;
						    }
						else dataOut[s][i][j] = defaultColor;
			} else { /* the result should be the original color */
				for(i=x;i<x+w;i++)
					for(j=y;j<y+h;j++) /* changed and unchanged are defined in threshold.h (enums) to mean pixel has changed or not */
						if( (changeMapP[i][j]==changed) &&
						    (changeMapF[i][j]==changed) ){
							dataOut[s][i][j] = data[s][i][j];
							ANDCount++;
						    }
						else dataOut[s][i][j] = defaultColor;
			}			
			if( verboseFlag )
				printf(" %d pixels were changed.\n", ANDCount);
		} else if( dataP && dataF && (ANDcolorFlag==FALSE) ){ /* or OR */
			for(i=x;i<x+w;i++)
				for(j=y;j<y+h;j++){
					if( changeMapP[i][j] == changed ){
						if( changeMapF[i][j] == changed )  dataOut[s][i][j] = (dataF[i][j]+dataP[i][j])/2;
						else dataOut[s][i][j] = dataP[i][j];
					} else if( changeMapF[i][j] == changed ){
						dataOut[s][i][j] = dataF[i][j];
					} else if( (changeMapP[i][j] != unchanged) ||
						   (changeMapF[i][j] != unchanged) ) dataOut[s][i][j] = defaultColor;
				}
		} else if( dataP ){
			for(i=x;i<x+w;i++)
				for(j=y;j<y+h;j++) dataOut[s][i][j] = dataP[i][j];
		} else if( dataF ){
			for(i=x;i<x+w;i++)
				for(j=y;j<y+h;j++) dataOut[s][i][j] = dataF[i][j];
		}
	}
	if( !verboseFlag ) printf("\n");
	freeDATATYPE3D(data, actualNumSlices, W);

	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		freeDATATYPE3D(dataOut, numSlices, W);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}

	free(inputFilename); free(outputFilename);
	freeDATATYPE3D(dataOut, numSlices, W);
	if( dataP != NULL ) freeDATATYPE2D(dataP, W);
	if( changeMapP != NULL ) free2D((void **)changeMapP, W);
	if( dataF != NULL ) freeDATATYPE2D(dataF, W);
	if( changeMapF != NULL ) free2D((void **)changeMapF, W);
	exit(0);
}
