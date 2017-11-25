#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>


/* format is 8 (greyscale), depth is 2 (pixel size) */

const	char	Examples[] = "\
\n	-p prediction.unc -S standard.unc\
\n\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices which contains the\
\n	 original image which the standard and predictions are based on)\
\n\t -p inputPredictionFilename\
\n	(UNC image file with one or more slices which contains the\
\n	 predictions)\
\n\t -S inputStandardFilename\
\n	(UNC image file with one or more slices which contains the\
\n	 standard image - the correct one)\
\n\
\n\t[-o outputFilename\
\n	(File to store the ASCII output - if omitted, then output\
\n	 goes to stdout)]\
\n\t[-c\
\n	(Do not include a header - as a comment '#' - at the beginning\
\n	 of the output which explains the various fields.)]\
\n\
\n** only one of the following two options can be used (or none):\
\n\t[-t min:max\
\n	(Threshold the prediction input image between 'min' and 'max'\
\n	 pixel intensities prior to calculating stats.)]\
\n\t[-T min:max:step\
\n	(It will do a serious of thresholds starting from min to max,\
\n	 then going to (min+step) to max, then (min+2*step) to max,\
\n	 etc. Each time a new threshold is done, statistics as before\
\n	 will be calculated. Useful for finding an optimum threshold\
\n	 which will maximise true positive and minimise false positives.)]\
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
\n\t[-s sliceNumber [-s s...]]\
\n\
\nThis program\
\ncalculates the discrepancy between 'prediction.unc' and the\
\nstandard 'standard.unc' in the form of true positive (TP)\
\ntrue negative (TN), false positives (FP) and false negatives (FN).\
\nAdditionally, Specificity = TN / (TN+FP), Sensitivity = TP/(TP+FN),\
\nAccuracy = (TP+TN) / (TP+TN+FP+FN).\
\n\
\nThe input command line requires that the original UNC filename is\
\nalso supplied. This is because we need to calculate the TN (i.e.\
\nwhen both standard and prediction are 0) and should not confuse\
\nthose pixels with background pixels which also have 0 value.\
\n\
\nThis program can be used in a situation where an image with lesions\
\n(the original image) was fed into a predictor of lesions and a\
\nprediction (the prediction image) of lesions (as a mask or as a probability\
\nmap) was the result. The exact lesion location is known a priori\
\nand is contained in the 'standard' image as a mask. TP, TN etc.\
\nare all measures of how good the predictor is doing compared\
\nto the real results.\
\n";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";


enum {TP=0, TN=1, FP=2, FN=3, ACCURACY=4, SPECIFICITY=5, SENSITIVITY=6,
      SUCCESS_TP=7, ERROR_FP=8, ERROR_FN=9,
      REAL_POSITIVE=10, REAL_NEGATIVE=11, TOTAL_NUM_PIXELS=12, TP_OVER_FP=13,
      TOTALENUM=14};

void	do_processing(void);
/* make them public so that are accessible to do_processing - can't be bother otherwise */
   	DATATYPE	***standardData, ***predictionData, ***oriPredictionData, ***data;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, slice, actualNumSlices = 0, actualNumSlices2 = 0,
			W2 = -1, H2 = -1, W3 = -1, H3 = -1, actualNumSlices3 = 0,
			slices[1000], allSlices = 0;

	FILE		*outputHandle;
	char		alLSlicesOnlyFlag = FALSE, includeComment = TRUE;
	int		thresholdPixelMin = 0, thresholdPixelMax = 0,
			thresholdRangeMin = 0, thresholdRangeMax = 0, thresholdRangeStep = 0;
	float		**results, *total_results;
	char		*inputStandardFilename = NULL, *inputPredictionFilename = NULL,
			*outputFilename = NULL, *inputFilename = NULL;

int	main(int argc, char **argv){
	int		i, s, optI;
	
	outputHandle = stdout;
	while( (optI=getopt(argc, argv, "S:p:i:o:es:w:h:x:y:at:cT:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'S': inputStandardFilename = strdup(optarg); break;
			case 'p': inputPredictionFilename = strdup(optarg); break;
			case 'o': outputFilename = strdup(optarg); break;
			case 'c': includeComment = FALSE; break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 't': sscanf(optarg, "%d:%d", &thresholdPixelMin, &thresholdPixelMax); break;
			case 'T': sscanf(optarg, "%d:%d:%d", &thresholdRangeMin, &thresholdRangeMax, &thresholdRangeStep); break;
			case 'a': alLSlicesOnlyFlag = TRUE; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input UNC filename must be specified.\n");
		exit(1);
	}
	if( inputStandardFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input 'standard' UNC filename must be specified.\n");
		exit(1);
	}
	if( inputPredictionFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input 'prediction' UNC filename must be specified.\n");
		exit(1);
	}
	if( (thresholdPixelMin>0) && (thresholdRangeMin>0) ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "Only one of '-t' and '-T' can be used - not both.\n");
		exit(1);
	}
	if( outputFilename != NULL )
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open output file '%s' for writing.\n", argv[0], outputFilename);
			exit(1);
		}

	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}
	if( (oriPredictionData=getUNCSlices3D(inputPredictionFilename, 0, 0, &W2, &H2, NULL, &actualNumSlices2, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputPredictionFilename);
		exit(1);
	}
	if( (standardData=getUNCSlices3D(inputStandardFilename, 0, 0, &W3, &H3, NULL, &actualNumSlices3, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}
	if( (W2!=W) || (H2!=H) || (actualNumSlices2!=actualNumSlices) ){
		fprintf(stderr, "%s : the input files standard='%s' and original='%s' have width (%d, %d), height (%d, %d) and/or number of slices (%d, %d).\n", argv[0], inputStandardFilename, inputFilename, W, W2, H, H2, actualNumSlices, actualNumSlices2);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(standardData, actualNumSlices3, W3);
		freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}
	if( (W3!=W) || (H3!=H) || (actualNumSlices3!=actualNumSlices) ){
		fprintf(stderr, "%s : the two input files standard='%s' and prediction='%s' have width (%d, %d), height (%d, %d) and/or number of slices (%d, %d).\n", argv[0], inputPredictionFilename, inputFilename, W, W2, H, H2, actualNumSlices, actualNumSlices2);
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(standardData, actualNumSlices3, W3);
		freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}
	if( (predictionData=callocDATATYPE3D(actualNumSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for predictionData.\n", argv[0], actualNumSlices * W * H * sizeof(DATATYPE));
		freeDATATYPE3D(data, actualNumSlices, W);
		freeDATATYPE3D(standardData, actualNumSlices3, W3);
		freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}

	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputStandardFilename);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d).\n", argv[0], x, w, W);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d).\n", argv[0], y, h, H);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}

	if( (results=(float **)malloc(TOTALENUM*sizeof(float *))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for results.\n", argv[0], TOTALENUM*sizeof(float *));
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}
	if( (total_results=(float *)malloc(TOTALENUM*sizeof(float))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for total_results.\n", argv[0], TOTALENUM*sizeof(float));
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
		exit(1);
	}
		
	for(i=0;i<TOTALENUM;i++)
		if( (results[i]=(float *)malloc(numSlices*sizeof(float))) == NULL ){
			fprintf(stderr, "%s : could not allocate %zd bytes for results[%d].\n", argv[0], numSlices*sizeof(float), i);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(standardData, actualNumSlices3, W3); freeDATATYPE3D(oriPredictionData, actualNumSlices2, W2);
			exit(1);
		}
	
	if( thresholdRangeStep > 0 ){
		for(i=thresholdRangeMin;i<thresholdRangeMax;i+=thresholdRangeStep){
			thresholdPixelMin = i;
			thresholdPixelMax = thresholdRangeMax;
			do_processing();
		}
	} else do_processing();

	freeDATATYPE3D(standardData, actualNumSlices, W);
	freeDATATYPE3D(oriPredictionData, actualNumSlices, W);
	freeDATATYPE3D(predictionData, actualNumSlices, W);
	freeDATATYPE3D(data, actualNumSlices, W);

	for(i=0;i<TOTALENUM;i++) free(results[i]); free(results);
	free(total_results);
	exit(0);
}

void	do_processing(void){
	int	i, j, s;

	if( thresholdPixelMin < thresholdPixelMax ){
		/* threshold */
		printf("%s , thresholding (%d to %d) : ", inputPredictionFilename, thresholdPixelMin, thresholdPixelMax); fflush(stdout);
		for(s=0;s<actualNumSlices;s++){
			printf("%d ", s+1); fflush(stdout);
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
				if( (oriPredictionData[s][i][j]>=thresholdPixelMin) && (oriPredictionData[s][i][j]<thresholdPixelMax) ) predictionData[s][i][j] = 1;
				else predictionData[s][i][j] = 0;
			}
		}
		printf("\n");
	} else {
		for(s=0;s<actualNumSlices;s++)
			for(i=x;i<x+w;i++) for(j=y;j<y+h;j++)
				if( oriPredictionData[s][i][j] > 0 ) predictionData[s][i][j] = 1;
				else predictionData[s][i][j] = 0;
	}

	printf("%s/%s, calculating : ", inputStandardFilename, inputPredictionFilename); fflush(stdout);
	for(i=0;i<TOTALENUM;i++) total_results[i] = 0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<TOTALENUM;i++) results[i][s] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( data[slice][i][j] <= 0 ) continue;
			if( standardData[slice][i][j] > 0 ) results[REAL_POSITIVE][s]++;
			else results[REAL_NEGATIVE][s]++;
			results[TOTAL_NUM_PIXELS][s]++;

			if( (predictionData[slice][i][j]>0) && (standardData[slice][i][j]>0) ) results[TP][s]++;
			if( (predictionData[slice][i][j]<=0) && (standardData[slice][i][j]<=0) ) results[TN][s]++;
			if( (predictionData[slice][i][j]>0) && (standardData[slice][i][j]<=0) ) results[FP][s]++;
			if( (predictionData[slice][i][j]<=0) && (standardData[slice][i][j]>0) ) results[FN][s]++;
		}
		if( (results[TP][s] + results[FN][s]) == 0.0 ) results[SENSITIVITY][s] = 0.0;
		else results[SENSITIVITY][s] = results[TP][s] / (results[TP][s] + results[FN][s]);

		if( (results[TN][s] + results[FP][s]) == 0.0 ) results[SPECIFICITY][s] = 0.0;
		else results[SPECIFICITY][s] = results[TN][s] / (results[TN][s] + results[FP][s]);

		if( (results[TP][s] + results[TN][s] + results[FP][s] + results[FN][s]) == 0.0 ) results[ACCURACY][s] = 0.0;
		else results[ACCURACY][s]    = (results[TP][s] + results[TN][s]) / (results[TP][s] + results[TN][s] + results[FP][s] + results[FN][s]);

		if( results[REAL_POSITIVE][s] == 0.0 ) results[SUCCESS_TP][s] = 0.0;
		else results[SUCCESS_TP][s] = results[TP][s] / results[REAL_POSITIVE][s];

		if( results[REAL_POSITIVE][s] == 0.0 ) results[ERROR_FP][s] = 0.0;
		else results[ERROR_FP][s] = results[FP][s] / results[REAL_POSITIVE][s];

		if( results[REAL_POSITIVE][s] == 0.0 ) results[ERROR_FN][s] = 0.0;
		else results[ERROR_FN][s] = results[FN][s] / results[REAL_POSITIVE][s];

		if( results[FP][s] == 0.0 ) results[TP_OVER_FP][s] = results[TP][s];
		else results[TP_OVER_FP][s] = results[TP][s] / results[FP][s];

		total_results[TP] += results[TP][s];
		total_results[TN] += results[TN][s];
		total_results[FP] += results[FP][s];
		total_results[FN] += results[FN][s];
		total_results[REAL_POSITIVE] += results[REAL_POSITIVE][s];
		total_results[REAL_NEGATIVE] += results[REAL_NEGATIVE][s];
		total_results[TOTAL_NUM_PIXELS] += results[TOTAL_NUM_PIXELS][s];
	}
	if( (total_results[TP] + total_results[FN]) == 0 ) total_results[SENSITIVITY] = 0.0;
	else total_results[SENSITIVITY] = total_results[TP] / (total_results[TP] + total_results[FN]);

	if( (total_results[TN] + total_results[FP]) == 0.0 ) total_results[SPECIFICITY] = 0.0;
	else total_results[SPECIFICITY] = total_results[TN] / (total_results[TN] + total_results[FP]);

	if( (total_results[TP] + total_results[TN] + total_results[FP] + total_results[FN]) == 0.0 ) total_results[ACCURACY] = 0.0;
	else total_results[ACCURACY]    = (total_results[TP] + total_results[TN]) / (total_results[TP] + total_results[TN] + total_results[FP] + total_results[FN]);

	if( total_results[REAL_POSITIVE] == 0.0 ) total_results[SUCCESS_TP] = 0.0;
	else total_results[SUCCESS_TP] = total_results[TP] / total_results[REAL_POSITIVE];

	if( total_results[REAL_POSITIVE] == 0.0 ) total_results[ERROR_FP] = 9999.9;
	else total_results[ERROR_FP] = total_results[FP] / total_results[REAL_POSITIVE];

	if( total_results[REAL_POSITIVE] == 0.0 ) total_results[ERROR_FN] = 999.9;
	else total_results[ERROR_FN] = total_results[FN] / total_results[REAL_POSITIVE];

	if( total_results[FP] == 0.0 ) total_results[TP_OVER_FP] = total_results[TP];
	else total_results[TP_OVER_FP] = total_results[TP] / total_results[FP];

	printf("\n");

	if( thresholdRangeStep > 0 ){
		if( thresholdPixelMin == thresholdRangeMin ){
			/* put legend and comment only once */
			if( includeComment ){
				fprintf(outputHandle, "# original image : %s - %.0f brain pixels\n# standard image : %s - %.0f real positive pixels\n# prediction image : %s - %.0f predicted positive pixels\n# TP=true positive, TN=true negative, FP=false positive, FN=false negative\n# SENsitivity = TP / (TP+FN)\n# SPEcificity = TN / (TN+FP)\n# ACCuracy = (TP+TN)/(TP+TN+FP+FN)\n# STP = success TP = TP / actual positives (from the standard image file) : you want this as high as possible\n# EFN = error FN = FN / actual positives (from the standard image file) : you want this as low as possible\n# EFP = errorFP = FP / actual positives (from the standard image file) : you want this as low as possible\n",  inputFilename, total_results[TOTAL_NUM_PIXELS], inputStandardFilename, total_results[REAL_POSITIVE], inputPredictionFilename, total_results[TP]+total_results[FP]);
				fprintf(outputHandle, "# Slice\tminThre\tmaxThre\tTP(4)\tTN(5)\tFP(6)\tFN(7)\tTP/FP(8)ACC,9%%\tSPE,10%%\tSEN,11%%\tSTP,12%%\tEFN,13%%\tEFP,14%%\n");
			}
		}
	} else {
		if( includeComment ){
			fprintf(outputHandle, "# original image : %s - %.0f brain pixels\n# standard image : %s - %.0f real positive pixels\n# prediction image : %s - %.0f predicted positive pixels\n# TP=true positive, TN=true negative, FP=false positive, FN=false negative\n# SENsitivity = TP / (TP+FN)\n# SPEcificity = TN / (TN+FP)\n# ACCuracy = (TP+TN)/(TP+TN+FP+FN)\n# STP = success TP = TP / actual positives (from the standard image file) : you want this as high as possible\n# EFN = error FN = FN / actual positives (from the standard image file) : you want this as low as possible\n# EFP = errorFP = FP / actual positives (from the standard image file) : you want this as low as possible\n",  inputFilename, total_results[TOTAL_NUM_PIXELS], inputStandardFilename, total_results[REAL_POSITIVE], inputPredictionFilename, total_results[TP]+total_results[FP]);
			if( (thresholdPixelMin < thresholdPixelMax) )
				fprintf(outputHandle, "# this image has been thresholded to include only pixels with intensity from %d to %d\n", thresholdPixelMin, thresholdPixelMax);
			fprintf(outputHandle, "# Slice\tTP(1)\tTN(2)\tFP(3)\tFN(4)\tTP/FP(5)ACC,6%%\tSPE,7%%\tSEN,8%%\tSTP,9%%\tEFN,10%%\tEFP,11%%\n");
		}
	}

	if( !alLSlicesOnlyFlag ){
		for(s=0;s<numSlices;s++){
			fprintf(outputHandle, "   %d\t", ((allSlices==0) ? slices[s] : s) + 1);
			if( thresholdRangeStep > 0 ) fprintf(outputHandle, "%d\t%d\t", thresholdPixelMin, thresholdPixelMax);
			fprintf(outputHandle, "%.0f\t%.0f\t%.0f\t%.0f\t%.5f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			results[TP][s],
			results[TN][s],
			results[FP][s],
			results[FN][s],
			results[TP_OVER_FP][s],
			results[ACCURACY][s] * 100.0,
			results[SPECIFICITY][s] * 100.0,
			results[SENSITIVITY][s] * 100.0,
			results[SUCCESS_TP][s] * 100.0,
			results[ERROR_FN][s] * 100.0,
			results[ERROR_FP][s] * 100.0);
		}
	}
	fprintf(outputHandle, "   9999\t");
	if( thresholdRangeStep > 0 ) fprintf(outputHandle, "%d\t%d\t", thresholdPixelMin, thresholdPixelMax);
	fprintf(outputHandle, "%.0f\t%.0f\t%.0f\t%.0f\t%.5f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		total_results[TP],
		total_results[TN],
		total_results[FP],
		total_results[FN],
		total_results[TP_OVER_FP],
		total_results[ACCURACY] * 100.0,
		total_results[SPECIFICITY] * 100.0,
		total_results[SENSITIVITY] * 100.0,
		total_results[SUCCESS_TP] * 100.0,
		total_results[ERROR_FN] * 100.0,
		total_results[ERROR_FP] * 100.0);

	fflush(outputHandle);
}
