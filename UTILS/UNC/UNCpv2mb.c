#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

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
\n\t -o outputBasename\
\n\
\n\t[-I numIterations]\
\n\
\n\t[-S seed]\
\n\
\n\t[-v\
\n	(verbose flag)]\
\n\
\n\t[-t stdev|entropy\
\n	(test type - what quantity is to be minimised\
\n	Default is 'stdev')]\
\n\
\n\t[-p percentage\
\n	(the tolerance in picking up partial volume\
\n	 pixels - the higher this percentage, the\
\n	 more partial volume pixels will be picked.)]\
\n\
\n\t[-P percentage\
\n	(the tolerance in picking up pure tissue\
\n	 pixels - the higher this percentage, the\
\n	 more pure tissue pixels will be picked.)]\
\n	 \
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
\nEach pixel is assumed to be a partial volume pixel consisting\
\nof 2 components - pairs of {air, wm, gm, csf}.\
\nThis program will separate the 2 components of each pixel using\
\nthe formula  a = (I0-IB)/(IA-IB)\
\nI0: pixel's intensity\
\nIA: first component intensity\
\nIB: second component intensity\
\na: percent of first A component (and 1-a percent of second component).";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define		MAX_NUM_ITERATIONS	1000000
int		numIterations = MAX_NUM_ITERATIONS;
int		SetSignalHandler(void);
void		SignalHandler(int /*signal_number*/);

#define		NUM_SCRATCHS	4
typedef enum {AIR=0, WM=1, GM=2, CSF=3} tissueType;
typedef enum {AIR_WM=0, WM_GM=1, WM_CSF=2, GM_CSF=3} pvType;

/* controls pv selection - symmetric */
#define	DEF_PV_RANGE_LOW	0.395
#define	DEF_PV_RANGE_HIGH	(1.0-DEF_PV_RANGE_LOW)
/* controls pure tissue selection - symmetric */
#define	DEF_PURE_TISSUE_RANGE_LOW	0.25
#define	DEF_PURE_TISSUE_RANGE_HIGH (1.0-DEF_PURE_TISSUE_RANGE_LOW)

#define	TEST_STDEV	0x1
#define	TEST_ENTROPY	0x2

int	main(int argc, char **argv){
	DATATYPE	***data, ***scratch[NUM_SCRATCHS];
	char		*inputFilename = NULL, *outputBasename = NULL, dummy[1000],
			*testTypeString = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;

	float		PV_RANGE_LOW = DEF_PV_RANGE_LOW, PV_RANGE_HIGH = DEF_PV_RANGE_HIGH,
			PURE_TISSUE_RANGE_LOW = DEF_PURE_TISSUE_RANGE_LOW, PURE_TISSUE_RANGE_HIGH = DEF_PURE_TISSUE_RANGE_HIGH,
			pvTolerance = -1.0, pureTissueTolerance = -1.0;

	register int	i, j, k;
	register float	a, I0;
		
	int		iters = 0, *numPixels[NUM_SCRATCHS], seed = (int )time(0),
			*actualPixelsPerSlice, *minNumPixels[NUM_SCRATCHS],
			foundMinimum[NUM_SCRATCHS], verboseFlag = FALSE,
			testType = TEST_STDEV, totalFoundMinimum[NUM_SCRATCHS];
	float		*minWMI[NUM_SCRATCHS], *minGMI[NUM_SCRATCHS], *minCSFI[NUM_SCRATCHS], *minAIRI[NUM_SCRATCHS],
			*mean[NUM_SCRATCHS], *stdev[NUM_SCRATCHS],
			*entropy[NUM_SCRATCHS], *maxEntropy[NUM_SCRATCHS],
			*minStdev[NUM_SCRATCHS], dummyF[NUM_SCRATCHS],
			INTENSITY[4][4], noisePercentage = -1.0;

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:I:S:vt:n:p:P:9")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'I': numIterations = atoi(optarg); break;
			case 'S': seed = atoi(optarg); break;
			case 'v': verboseFlag = TRUE; break;
			case 't': testTypeString = strdup(optarg); break;
			case 'n': noisePercentage = atof(optarg) / 100.0; break;
			case 'p': pvTolerance = atof(optarg) / 100.0; break;
			case 'P': pureTissueTolerance = atof(optarg) / 100.0; break;
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
		fprintf(stderr, "An output basename must be specified.\n");
		free(inputFilename);
		exit(1);
	}
	if( testTypeString != NULL ){
		if( toupper(testTypeString[0]) == 'S' ) testType = TEST_STDEV;
		else if( toupper(testTypeString[0]) == 'E' ) testType = TEST_ENTROPY;
		else {
			fprintf(stderr, "%s : illegal test type '%s' following '-t' option\n", argv[0], testTypeString);
			free(inputFilename); free(outputBasename); free(testTypeString);
			exit(1);
		}
		free(testTypeString);
	}
	if( pvTolerance > 0.0 ){
		PV_RANGE_LOW = 0.5 -  pvTolerance/2.0;
		PV_RANGE_HIGH = 0.5 + pvTolerance/2.0;
	}
	if( pureTissueTolerance > 0.0 ){
		PURE_TISSUE_RANGE_LOW = 0.5 - pureTissueTolerance/2.0;
		PURE_TISSUE_RANGE_HIGH= 0.5 + pureTissueTolerance/2.0;
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
	for(i=0;i<NUM_SCRATCHS;i++){
		if( (scratch[i]=callocDATATYPE3D(numSlices, W, H)) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for output scratch [%d].\n", argv[0], numSlices * W * H * sizeof(DATATYPE), i);
			free(inputFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		numPixels[i] = (int *)malloc(numSlices * sizeof(int));
		minNumPixels[i] = (int *)malloc(numSlices * sizeof(int));
		minWMI[i] = (float *)malloc(numSlices * sizeof(float));
		minGMI[i] = (float *)malloc(numSlices * sizeof(float));
		minCSFI[i] = (float *)malloc(numSlices * sizeof(float));
		minAIRI[i] = (float *)malloc(numSlices * sizeof(float));
		mean[i] = (float *)malloc(numSlices * sizeof(float));
		stdev[i] = (float *)malloc(numSlices * sizeof(float));
		minStdev[i] = (float *)malloc(numSlices * sizeof(float));
		entropy[i] = (float *)malloc(numSlices * sizeof(float));
		maxEntropy[i] = (float *)malloc(numSlices * sizeof(float));
		actualPixelsPerSlice = (int *)malloc(numSlices * sizeof(int));
		foundMinimum[i] = totalFoundMinimum[i] = 0;
	}

	if( SetSignalHandler() == FALSE ){
		fprintf(stderr, "%s : call to SetSignalHandler has failed.\n", argv[0]);
		free(inputFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minAIRI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
	}

	for(s=0;s<numSlices;s++)
		for(k=0;k<NUM_SCRATCHS;k++){
			minAIRI[k][s] = minWMI[k][s] = minGMI[k][s] = minCSFI[k][s] = 0.0;
			maxEntropy[k][s] = -1.0;
			minStdev[k][s] = 10000000000.0;
			minNumPixels[k][s] = 0;
		}

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		actualPixelsPerSlice[s] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( data[slice][i][j] > 0 ) actualPixelsPerSlice[s]++;
		}
	}

	srand48(seed);

	fprintf(stderr, "%s : ", argv[0]); fflush(stderr);
	while( iters++ < numIterations ){
		fprintf(stderr, "%d ", iters); fflush(stderr);
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( (noisePercentage > 0.0) && (iters > 5) ){
				for(k=0;k<NUM_SCRATCHS;k++){
					entropy[k][s] = stdev[k][s] = mean[k][s] = 0.0;
					numPixels[k][s] = 0;

					INTENSITY[CSF][k] = minCSFI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));
					do { a = minGMI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));  /*printf("a(1) %f > %f\n", a, minCSFI[k][s]-85);*/ } while( (a >= (minCSFI[k][s]-85)) && (numIterations>0) );
					INTENSITY[GM][k] = a;
					do { a = minWMI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));  /*printf("a(2) %f > %f\n", a, minGMI[k][s]-85);*/ } while( a >= (minGMI[k][s]-85) && (numIterations>0) );
					INTENSITY[WM][k] = a;
					do { a = minAIRI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));  /*printf("a(2) %f > %f\n", a, minWMI[k][s]-85);*/ } while( a >= (minWMI[k][s]-85) && (numIterations>0) );
					INTENSITY[AIR][k] = a;
				}
			} else {
				for(k=0;k<NUM_SCRATCHS;k++){
					entropy[k][s] = stdev[k][s] = mean[k][s] = 0.0;
					numPixels[k][s] = 0;
				
					INTENSITY[CSF][k] = SCALE_OUTPUT(drand48(), 600, 750, 0.0, 1.0);
					while( (INTENSITY[GM][k] = SCALE_OUTPUT(drand48(), 450, 600, 0.0, 1.0)) >= (INTENSITY[CSF][k]-85) );
					while( (INTENSITY[WM][k] = SCALE_OUTPUT(drand48(), 350, 575, 0.0, 1.0)) >= (INTENSITY[GM][k]-85) );
					while( (INTENSITY[AIR][k] = SCALE_OUTPUT(drand48(), 0, 250, 0.0, 1.0)) >= (INTENSITY[WM][k]-85) );
				}
			}
			for(k=0;k<NUM_SCRATCHS;k++) foundMinimum[k] = 0;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				for(k=0;k<NUM_SCRATCHS;k++) scratch[k][s][i][j] = 0;
				if( (I0=data[slice][i][j]) == 0 ) continue;

				/*	a I_A + b I_B = I_0
					a = \frac{I_0 - I_B}{I_A - I_B} */
				if( IS_WITHIN(I0, INTENSITY[AIR][AIR_WM], INTENSITY[WM][AIR_WM]) ){
					a = ((float )(I0 - INTENSITY[AIR][AIR_WM])) / ((float )(INTENSITY[WM][AIR_WM]-INTENSITY[AIR][AIR_WM]));
					scratch[AIR_WM][s][i][j] = ROUND(a * 1000.0);
				}
				if( IS_WITHIN(I0, INTENSITY[WM][WM_GM], INTENSITY[GM][WM_GM]) ){
					a = ((float )(I0 - INTENSITY[WM][WM_GM])) / ((float )(INTENSITY[GM][WM_GM]-INTENSITY[WM][WM_GM]));
					scratch[WM_GM][s][i][j] = ROUND(a * 1000.0);
					a = ((float )(I0 - INTENSITY[WM][WM_GM])) / ((float )(INTENSITY[CSF][WM_GM]-INTENSITY[WM][WM_GM]));
					scratch[WM_CSF][s][i][j] = ROUND(a * 1000.0);
				}
				if( IS_WITHIN(I0, INTENSITY[GM][GM_CSF], INTENSITY[CSF][GM_CSF]) ){
					a = ((float )(I0 - INTENSITY[GM][GM_CSF])) / ((float )(INTENSITY[CSF][GM_CSF]-INTENSITY[GM][GM_CSF]));
					scratch[GM_CSF][s][i][j] = ROUND(a * 1000.0);
					a = ((float )(I0 - INTENSITY[WM][GM_CSF])) / ((float )(INTENSITY[CSF][GM_CSF]-INTENSITY[WM][GM_CSF]));
					scratch[WM_CSF][s][i][j] = ROUND(a * 1000.0);
				}
			}

			for(k=0;k<NUM_SCRATCHS;k++){
				mean[k][s] /= (numPixels[k][s]+1);
				for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
					if( scratch[k][s][i][j] == 0 ) continue;
					stdev[k][s] += SQR(scratch[k][s][i][j] - mean[k][s]);
					entropy[k][s] += scratch[k][s][i][j] * log(scratch[k][s][i][j]);
				}
				stdev[k][s] /= (numPixels[k][s]+1);
				entropy[k][s] /= (numPixels[k][s]+1);
				if( (testType==TEST_STDEV) && (stdev[k][s] < minStdev[k][s]) ){
					maxEntropy[k][s] = 0.0;
					minStdev[k][s] = stdev[k][s];
					minWMI[k][s] = INTENSITY[WM][k];
					minGMI[k][s] = INTENSITY[GM][k];
					minCSFI[k][s] = INTENSITY[CSF][k];
					minAIRI[k][s] = INTENSITY[AIR][k];
					minNumPixels[k][s] = numPixels[k][s];
					totalFoundMinimum[k]++; foundMinimum[k]++;
					continue;
				}
				if( (testType==TEST_ENTROPY) && (entropy[k][s] > maxEntropy[k][s]) ){
					minStdev[k][s] = 0.0;
					maxEntropy[k][s] = entropy[k][s];
					minWMI[k][s] = INTENSITY[WM][k];
					minGMI[k][s] = INTENSITY[GM][k];
					minCSFI[k][s] = INTENSITY[CSF][k];
					minAIRI[k][s] = INTENSITY[AIR][k];
					minNumPixels[k][s] = numPixels[k][s];
					totalFoundMinimum[k]++; foundMinimum[k]++;
					continue;
				}
			}
		}
	}

	printf("\nFinal after %d iters : ", iters-1);
	for(k=0;k<NUM_SCRATCHS;k++) dummyF[k] = 0;
	for(k=0;k<NUM_SCRATCHS;k++)
		for(s=0;s<numSlices;s++){
			dummyF[AIR] += minAIRI[k][s];
			dummyF[WM] += minWMI[k][s];
			dummyF[GM] += minGMI[k][s];
			dummyF[CSF] += minCSFI[k][s];
		}
	dummyF[AIR] /= (NUM_SCRATCHS*numSlices);
	dummyF[WM] /= (NUM_SCRATCHS*numSlices);
	dummyF[GM] /= (NUM_SCRATCHS*numSlices);
	dummyF[CSF] /= (NUM_SCRATCHS*numSlices);
	printf("air=%.0f\twm=%.0f\tgm=%.0f\tcsf=%.0f\n", dummyF[AIR], dummyF[WM], dummyF[GM], dummyF[CSF]);

	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			for(k=0;k<NUM_SCRATCHS;k++) scratch[k][s][i][j] = 0;
			if( (I0=data[slice][i][j]) == 0 ) continue;

			if( IS_WITHIN(I0, dummyF[AIR], dummyF[WM]) ){
				a = ((float )(I0 - dummyF[AIR])) / ((float )(dummyF[WM]-dummyF[AIR]));
				scratch[AIR_WM][s][i][j] = ROUND(a * 1000.0);
			}
			if( IS_WITHIN(I0, dummyF[WM], dummyF[GM]) ){
				a = ((float )(I0 - dummyF[WM])) / ((float )(dummyF[GM]-dummyF[WM]));
				scratch[WM_GM][s][i][j] = ROUND(a * 1000.0);
				a = ((float )(I0 - dummyF[WM])) / ((float )(dummyF[CSF]-dummyF[WM]));
				scratch[WM_CSF][s][i][j] = ROUND(a * 1000.0);
			}
			if( IS_WITHIN(I0, dummyF[GM], dummyF[CSF]) ){
				a = ((float )(I0 - dummyF[GM])) / ((float )(dummyF[CSF]-dummyF[GM]));
				scratch[GM_CSF][s][i][j] = ROUND(a * 1000.0);
				a = ((float )(I0 - dummyF[WM])) / ((float )(dummyF[CSF]-dummyF[WM]));
				scratch[WM_CSF][s][i][j] = ROUND(a * 1000.0);
			}
		}
	}

	sprintf(dummy, "%s_pv_air_wm.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[AIR_WM], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minAIRI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_pv_gm_csf.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[GM_CSF], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minAIRI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_pv_wm_csf.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[WM_CSF], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minAIRI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_pv_wm_gm.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[WM_GM], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minAIRI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	freeDATATYPE3D(data, actualNumSlices, W);
	for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);

	for(i=0;i<NUM_SCRATCHS;i++){	
		free(minAIRI[i]); free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);
		free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);
		free(numPixels[i]); free(minNumPixels[i]);
	}
	free(actualPixelsPerSlice);
	free(inputFilename); free(outputBasename);
	exit(0);
}

void	SignalHandler(int signal_number){
	printf("SignalHandler : caught signal %d\n\n", signal_number);
	if( signal(signal_number, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SignalHandler : warning!, could not re-set signal number %d\n", signal_number);
		return;
	}
	switch( signal_number ){
		case    SIGINT:
		case    SIGQUIT:
		case    SIGTERM:numIterations = -1;
				fprintf(stderr, "SignalHandler : I will quit now (after this iteration is finished ...).\n");
				break;
	}
}

int	SetSignalHandler(void){
	if( signal(SIGINT, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGINT\n");
		return FALSE;
	}
	if( signal(SIGQUIT, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGINT\n");
		return FALSE;
	}
	if( signal(SIGTERM, SignalHandler) == SIG_ERR ){
		fprintf(stderr, "SetSignalHandler : could not set signal handler for SIGINT\n");
		return FALSE;
	}
	return TRUE;
}


