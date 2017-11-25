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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

#define		MAX_NUM_ITERATIONS	1000000
int		numIterations = MAX_NUM_ITERATIONS;
int		SetSignalHandler(void);
void		SignalHandler(int /*signal_number*/);

#define		NUM_SCRATCHS	6
/* basically, the number of the defines below must be equal to the NUM_SCRATCHS above */
#define		WM		0
#define		GM		1
#define		CSF		2
#define		PV_WM_GM	3
#define		PV_WM_CSF	4
#define		PV_GM_CSF	5
#define		NUM_TISSUE	(NUM_SCRATCHS/2)

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
	register float	a, A, B, C;
		
	int		iters = 0, *numPixels[NUM_SCRATCHS], seed = (int )time(0),
			*actualPixelsPerSlice, *minNumPixels[NUM_SCRATCHS],
			foundMinimum[NUM_SCRATCHS], verboseFlag = FALSE,
			testType = TEST_STDEV, totalFoundMinimum[NUM_SCRATCHS];
	float		*minWMI[NUM_SCRATCHS], *minGMI[NUM_SCRATCHS], *minCSFI[NUM_SCRATCHS],
			*mean[NUM_SCRATCHS], *stdev[NUM_SCRATCHS],
			*entropy[NUM_SCRATCHS], *maxEntropy[NUM_SCRATCHS],
			*minStdev[NUM_SCRATCHS], percentPixels,
			WM_INTENSITY[NUM_SCRATCHS], GM_INTENSITY[NUM_SCRATCHS],
			CSF_INTENSITY[NUM_SCRATCHS], noisePercentage = -1.0;

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
		for(i=0;i<NUM_SCRATCHS;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
	}

	for(s=0;s<numSlices;s++)
		for(k=0;k<NUM_SCRATCHS;k++){
			minWMI[k][s] = minGMI[k][s] = minCSFI[k][s] = 0.0;
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

	while( iters++ < numIterations ){
		for(s=0;s<numSlices;s++){

			slice = (allSlices==0) ? slices[s] : s;
			if( (noisePercentage > 0.0) && (iters > 5) ){
				for(k=0;k<NUM_SCRATCHS;k++){
					entropy[k][s] = stdev[k][s] = mean[k][s] = 0.0;
					numPixels[k][s] = 0;

					CSF_INTENSITY[k] = minCSFI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));
					do { a = minGMI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));  /*printf("a(1) %f > %f\n", a, minCSFI[k][s]-85);*/ } while( (a >= (minCSFI[k][s]-85)) && (numIterations>0) );
					GM_INTENSITY[k] = a;
					do { a = minWMI[k][s] * (1.0 + SCALE_OUTPUT(drand48(), -noisePercentage, noisePercentage, 0.0, 1.0));  /*printf("a(2) %f > %f\n", a, minGMI[k][s]-85);*/ } while( a >= (minGMI[k][s]-85) && (numIterations>0) );
					WM_INTENSITY[k] = a;
				}
			} else {
				for(k=0;k<NUM_SCRATCHS;k++){
					entropy[k][s] = stdev[k][s] = mean[k][s] = 0.0;
					numPixels[k][s] = 0;
				
					CSF_INTENSITY[k] = SCALE_OUTPUT(drand48(), 600, 750, 0.0, 1.0);
					while( (GM_INTENSITY[k] = SCALE_OUTPUT(drand48(), 450, 600, 0.0, 1.0)) >= (CSF_INTENSITY[k]-85) );
					while( (WM_INTENSITY[k] = SCALE_OUTPUT(drand48(), 350, 575, 0.0, 1.0)) >= (GM_INTENSITY[k]-85) );
				}
			}
			for(k=0;k<NUM_SCRATCHS;k++) foundMinimum[k] = 0;
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				for(k=0;k<NUM_SCRATCHS;k++) scratch[k][s][i][j] = 0;
				if( data[slice][i][j] == 0 ) continue;

				A = B = C = 0.0;

				/*	a I_A + b I_B = I_0
					a = \frac{I_0 - I_B}{I_A - I_B} */

			/* assuming that this is a PV between WM and CSF, A is the percentage of WM (1-A is that of CSF) */
			a = ((float )(data[slice][i][j] - CSF_INTENSITY[PV_WM_CSF])) / ((float )(WM_INTENSITY[PV_WM_CSF] - CSF_INTENSITY[PV_WM_CSF]));
				if( (a>0.0) && (a<1.0) ) A = a;

			/* in pv between GM and CSF, B is the percentage of GM */
			a = ((float )(data[slice][i][j] - CSF_INTENSITY[PV_GM_CSF])) / ((float )(GM_INTENSITY[PV_GM_CSF] - CSF_INTENSITY[PV_GM_CSF]));
				if( (a>0.0) && (a<1.0) ) B = a;

			/* in pv between WM and GM, C is the percentage of GM */
			a = ((float )(data[slice][i][j] - WM_INTENSITY[PV_WM_GM])) / ((float )(GM_INTENSITY[PV_WM_GM] - WM_INTENSITY[PV_WM_GM]));
				if( (a>0.0) && (a<1.0) ) C = a;

			/* pv */
				if( IS_WITHIN(A, PV_RANGE_LOW, PV_RANGE_HIGH) ){
					scratch[PV_WM_CSF][s][i][j] = data[slice][i][j];
					mean[PV_WM_CSF][s] += scratch[PV_WM_CSF][s][i][j];
					numPixels[PV_WM_CSF][s]++;
				}
				if( IS_WITHIN(B, PV_RANGE_LOW, PV_RANGE_HIGH) ){
					scratch[PV_GM_CSF][s][i][j] = data[slice][i][j];
					mean[PV_GM_CSF][s] += scratch[PV_GM_CSF][s][i][j];
					numPixels[PV_GM_CSF][s]++;
				}
				if( IS_WITHIN(C, PV_RANGE_LOW, PV_RANGE_HIGH) ){
					scratch[PV_WM_GM][s][i][j] = data[slice][i][j];
					mean[PV_WM_GM][s] += scratch[PV_WM_GM][s][i][j];
					numPixels[PV_WM_GM][s]++;
				}
			/* pure tissue */
				if( (A>PURE_TISSUE_RANGE_HIGH) && (C>PURE_TISSUE_RANGE_HIGH) ){
					scratch[WM][s][i][j] = data[slice][i][j];
					mean[WM][s] += scratch[WM][s][i][j];
					numPixels[WM][s]++;
				}
				if( (B>PURE_TISSUE_RANGE_HIGH) && (C<PURE_TISSUE_RANGE_LOW) ){
					scratch[GM][s][i][j] = data[slice][i][j];
					mean[GM][s] += scratch[GM][s][i][j];
					numPixels[GM][s]++;
				}
				if( (A>PURE_TISSUE_RANGE_HIGH) && (B>PURE_TISSUE_RANGE_HIGH) ){
					scratch[CSF][s][i][j] = data[slice][i][j];
					mean[CSF][s] += scratch[CSF][s][i][j];
					numPixels[CSF][s]++;
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
					minWMI[k][s] = WM_INTENSITY[k];
					minGMI[k][s] = GM_INTENSITY[k];
					minCSFI[k][s] = CSF_INTENSITY[k];
					minNumPixels[k][s] = numPixels[k][s];
					totalFoundMinimum[k]++; foundMinimum[k]++;
					continue;
				}
				if( (testType==TEST_ENTROPY) && (entropy[k][s] > maxEntropy[k][s]) ){
					minStdev[k][s] = 0.0;
					maxEntropy[k][s] = entropy[k][s];
					minWMI[k][s] = WM_INTENSITY[k];
					minGMI[k][s] = GM_INTENSITY[k];
					minCSFI[k][s] = CSF_INTENSITY[k];
					minNumPixels[k][s] = numPixels[k][s];
					totalFoundMinimum[k]++; foundMinimum[k]++;
					continue;
				}
			}
		}
		if( verboseFlag && ((iters % 1) == 0) ){
			printf("\n*** Iter %d - hit mimimum (", iters);
			for(k=0;k<NUM_SCRATCHS;k++) printf("%d/%d%s", foundMinimum[k], totalFoundMinimum[k], (k==(NUM_SCRATCHS-1))?"":",");
			printf("), hit rate (");
			for(k=0;k<NUM_SCRATCHS;k++) printf("%.1f%s", 100.0 * foundMinimum[k]/iters, (k==(NUM_SCRATCHS-1))?"":",");
			printf(" %%) ***\n");
			for(s=0;s<numSlices;s++){
				slice = (allSlices==0) ? slices[s] : s;
				printf("    slice %d, %d pixels:\n\tmin stdev = (", slice+1, actualPixelsPerSlice[s]);
				for(k=0;k<NUM_TISSUE;k++) printf("%.2f%s", minStdev[k][s], (k==(NUM_TISSUE-1))?"":",");
				printf(")/(");
				for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%.2f%s", minStdev[k][s], (k==(NUM_SCRATCHS-1))?"":",");
				printf(")\n\tmin entropy = (");
				for(k=0;k<NUM_TISSUE;k++) printf("%.2f%s", maxEntropy[k][s], (k==(NUM_TISSUE-1))?"":",");
				printf(")/(");
				for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%.2f%s", maxEntropy[k][s], (k==(NUM_SCRATCHS-1))?"":",");
				printf(")\n\tboundaries = (");
				for(k=0;k<NUM_TISSUE;k++) printf("[%.0f,%.0f,%.0f]%s", minWMI[k][s], minGMI[k][s], minCSFI[k][s], (k==(NUM_TISSUE-1))?"":" ");
				printf(")/(");
				for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("[%.0f,%.0f,%.0f]%s", minWMI[k][s], minGMI[k][s], minCSFI[k][s], (k==(NUM_SCRATCHS-1))?"":",");
				printf(")\n\tnum pixels = (");
				for(k=0;k<NUM_TISSUE;k++) printf("%d%s", minNumPixels[k][s], (k==(NUM_TISSUE-1))?"":",");
				printf(")/(");
				for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%d%s", minNumPixels[k][s], (k==(NUM_SCRATCHS-1))?"":",");
				printf(") (");
				for(k=0;k<NUM_TISSUE;k++){
					percentPixels = 100.0 * ((float )minNumPixels[k][s]) / ((float )actualPixelsPerSlice[s]);
					printf("%.1f%s", percentPixels, (k==(NUM_TISSUE-1))?"":",");
				}
				printf(" %%)/(");
				for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++){
					percentPixels = 100.0 * ((float )minNumPixels[k][s]) / ((float )actualPixelsPerSlice[s]);
					printf("%.1f%s", percentPixels, (k==(NUM_SCRATCHS-1))?"":",");
				}
				printf(" %%)\n");
			}
		}
	}

	iters--;
	if( verboseFlag ){
		printf("\nDone, after %d iterations, seed was %d - hit mimimum (", iters, seed);
		for(k=0;k<NUM_SCRATCHS;k++) printf("%d/%d%s", foundMinimum[k], totalFoundMinimum[k], (k==(NUM_SCRATCHS-1))?"":",");
		printf("), hit rate (");
		for(k=0;k<NUM_SCRATCHS;k++) printf("%.1f%s", 100.0 * foundMinimum[k]/iters, (k==(NUM_SCRATCHS-1))?"":",");
		printf(" %%)\n");
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("    slice %d, %d pixels:\n\tmin stdev = (", slice+1, actualPixelsPerSlice[s]);
			for(k=0;k<NUM_TISSUE;k++) printf("%f%s", minStdev[k][s], (k==(NUM_TISSUE-1))?"":",");
			printf(")/(");
			for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%.2f%s", minStdev[k][s], (k==(NUM_SCRATCHS-1))?"":",");
			printf(")\n\tmin entropy = (");
			for(k=0;k<NUM_TISSUE;k++) printf("%.2f%s", maxEntropy[k][s], (k==(NUM_TISSUE-1))?"":",");
			printf(")/(");
			for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%.2f%s", maxEntropy[k][s], (k==(NUM_SCRATCHS-1))?"":",");
			printf(")\n\tboundaries = (");
			for(k=0;k<NUM_TISSUE;k++) printf("[%.0f,%.0f,%.0f]%s", minWMI[k][s], minGMI[k][s], minCSFI[k][s], (k==(NUM_TISSUE-1))?"":" ");
			printf(")/(");
			for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("[%.0f,%.0f,%.0f]%s", minWMI[k][s], minGMI[k][s], minCSFI[k][s], (k==(NUM_SCRATCHS-1))?"":",");
			printf(")\n\tnum pixels = (");
			for(k=0;k<NUM_TISSUE;k++) printf("%d%s", minNumPixels[k][s], (k==(NUM_TISSUE-1))?"":",");
			printf(")/(");
			for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++) printf("%d%s", minNumPixels[k][s], (k==(NUM_SCRATCHS-1))?"":",");
			printf(") (");
			for(k=0;k<NUM_TISSUE;k++){
				percentPixels = 100.0 * ((float )minNumPixels[k][s]) / ((float )actualPixelsPerSlice[s]);
				printf("%.1f%s", percentPixels, (k==(NUM_TISSUE-1))?"":",");
			}
			printf(" %%)/(");
			for(k=NUM_TISSUE;k<NUM_SCRATCHS;k++){
				percentPixels = 100.0 * ((float )minNumPixels[k][s]) / ((float )actualPixelsPerSlice[s]);
				printf("%.1f%s", percentPixels, (k==(NUM_SCRATCHS-1))?"":",");
			}
			printf(" %%)\n");
		}
	}

	sprintf(dummy, "%s_pv_gm_csf.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[PV_GM_CSF], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_pv_wm_csf.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[PV_WM_CSF], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_pv_wm_gm.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[PV_WM_GM], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_wm.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[WM], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_gm.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[GM], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
		free(actualPixelsPerSlice);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, dummy, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, dummy);
		exit(1);
	}

	sprintf(dummy, "%s_csf.unc", outputBasename);
	if( !writeUNCSlices3D(dummy, scratch[CSF], W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], dummy);
		free(inputFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
		for(i=0;i<NUM_SCRATCHS;i++) freeDATATYPE3D(scratch[i], numSlices, W);
		for(i=0;i<NUM_TISSUE;i++){free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);free(mean[i]); free(stdev[i]); free(minStdev[i]); free(entropy[i]); free(maxEntropy[i]);free(numPixels[i]); free(minNumPixels[i]);}
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

	for(i=0;i<NUM_TISSUE;i++){	
		free(minWMI[i]); free(minGMI[i]); free(minCSFI[i]);
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


