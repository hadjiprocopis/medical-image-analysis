#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

#define	mWM	0
#define	mGM	1
#define	mCSF	2
 
const	char	Examples[] = "\
\n	-W actual_wm.unc -G actual_gm.unc -C actual_csf.unc -m predicted_wm.unc -g predicted_gm.unc -c predicted_csf.unc -o output\
\n";

const	char	Usage[] = "options as follows:\
\nGold standard files:\
\n\t -W whiteMatterFilename\
\n	(UNC image file with one or more slices holding the *gold standard* WM probability maps or just masks)\
\n\
\n\t -G greyMatterFilename\
\n	(UNC image file with one or more slices holding the *gold standard* GM probability maps or just masks)\
\n\
\n\t -C csfFilename\
\n	(UNC image file with one or more slices holding the *gold standard* CSF probability maps or just masks)\
\n\
\nPredicted segmentation files:\
\n\t -m whiteMatterFilename\
\n	(UNC image file with one or more slices holding the *predicted* WM probability maps or just masks)\
\n\
\n\t -g greyMatterFilename\
\n	(UNC image file with one or more slices holding the *predicted* GM probability maps or just masks)\
\n\
\n\t -c csfFilename\
\n	(UNC image file with one or more slices holding the *predicted* CSF probability maps or just masks)\
\n\
\n\t[-o outputFilename\
\n	(Optional output filename. The output will be ascii. This can go\
\n	 either to stdout or to this filename if you wish.)]\
\n\t[-a\
\n	(flag to tell not to make a slice-by-slice report, just totals)]\
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
\nThis program will evaluate the results of a WM/GM/CSF segmentation.\
\nThis is done by supplying the mask files of the predicted segmentations\
\nfor the three compartments (WM/GM/CSF) AND the mask files of a\
\ngold-standard segmentation.\
\nFor each tissue type, quantities such as true positive, false negative\
\nwill be calculated, leading to specificity, sensitivity, etc.\
\n\
\nThe basic idea here is that a pixel is classified as X but its\
\ntrue classification is Y (X may be the same as Y).\
\n\
\nIf X is the same as Y then this pixel was segmented correctly.\
\n\
\nUsing this notion, for each slice a matrix is derived as follows:\
\n    WM  GM  CSF\
\nWM  A1  A2  A3\
\nGM  B1  B2  B3\
\nCSF C1  C2  C3\
\n\
\nwhere A1 is the number of pixels predicted to be WM and\
\nwere actually WM, while A2 is the number of pixels predicted\
\nto be WM but were actually GM. Similarly, B3 was the number\
\nof pixels predicted to be GM but actually were CSF (always\
\naccording to the gold standard files).\
\n\
\nThen for WM,\
\n	true positive, TP = A1\
\n	false negative, FN = B1 + C1\
\n	false positive, FP = A2 + A3\
\n	true negative, TN = B2 + B3 + C2 + C3\
\nFor GM,\
\n	true positive, TP = B2\
\n	false negative, FN = A2 + C2\
\n	false positive, FP = B1 + B3\
\n	true negative, TN = A1 + A3 + C1 + C3\
\nFor CSF,\
\n	true positive, TP = C3\
\n	false negative, FN = A3 + B3\
\n	false positive, FP = C1 + C2\
\n	true negative, TN = A1 + A2 + B1 + B2\
\n\
\nthen the following quantities (for each tissue type) are calculated:\
\n	SEN (sensitivity) = TP / (TP + FN)\
\n	SPE (specificity) = TN / (TN + FP)\
\n	PPV (positive predictive value) = TP / (TP + FP)\
\n	NPV (negative predictive value) = TN / (TN + FN)\
\n	ACC (accuracy) = (TP + TN) / (TP + TN + FP + FN)\
\n	PRE (prevalence) = (TP + FN) / (TP + TN + FP + FN)\
\n	TPR (true positive rate) = TP / (TP + FN)\
\n	FPR (false positive rate) = FP / (TN + FP)\
\n";
const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***wmMasks1, ***gmMasks1, ***csfMasks1,
			***wmMasks2, ***gmMasks2, ***csfMasks2;
	char		*outputFilename = NULL,
			*wmFilename1 = NULL, *gmFilename1 = NULL, *csfFilename1 = NULL,
			*wmFilename2 = NULL, *gmFilename2 = NULL, *csfFilename2 = NULL;
	FILE		*outputHandle = stdout;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	float		***MATRIX, 	    /* true versus predicted allocations for each slice */
			TOTAL_MATRIX[3][3], /* true versus predicted allocations for all slices*/
			**TP, TOTAL_TP[3],
			**TN, TOTAL_TN[3],
			**FP, TOTAL_FP[3],
			**FN, TOTAL_FN[3],
			missedPixels[3], truePixels[3], predictedPixels[3],
			totalNumPixels, totalError;
	register int	i, j;
	int		wmW = -1, gmW = -1, otherW = -1,
			wmH = -1, gmH = -1, otherH = -1,
			actualNumSlices_wm = 0, actualNumSlices_gm = 0, actualNumSlices_other = 0;

	int		doOverallOnly = FALSE; /* e.g. report all slices not just totals */

	while( (optI=getopt(argc, argv, "o:es:w:h:x:y:W:G:C:m:g:c:a")) != EOF)
		switch( optI ){
			case 'o': outputFilename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'a': doOverallOnly = TRUE; break;
			case 'W': wmFilename2 = strdup(optarg); break; /* white matter */
			case 'G': gmFilename2 = strdup(optarg); break; /* grey  matter */
			case 'C': csfFilename2 = strdup(optarg); break; /* csf */
			case 'm': wmFilename1 = strdup(optarg); break; /* white matter */
			case 'g': gmFilename1 = strdup(optarg); break; /* grey  matter */
			case 'c': csfFilename1 = strdup(optarg); break; /* csf */
			
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( outputFilename != NULL ){
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing.\n", argv[0], outputFilename);
			exit(1);
		}
	}

	if( wmFilename1 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A first white matter filename must be specified.\n");
		exit(1);
	}
	if( gmFilename1 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A first grey matter filename must be specified.\n");
		exit(1);
	}
	if( csfFilename1 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A first csf filename must be specified.\n");
		exit(1);
	}
	if( wmFilename2 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second white matter filename must be specified.\n");
		exit(1);
	}
	if( gmFilename2 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second grey matter filename must be specified.\n");
		exit(1);
	}
	if( csfFilename2 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second csf filename must be specified.\n");
		exit(1);
	}
	if( (wmMasks1=getUNCSlices3D(wmFilename1, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmFilename1);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, wmFilename1);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, W, wmFilename1);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, H, wmFilename1);
		exit(1);
	}
	if( (gmMasks1=getUNCSlices3D(gmFilename1, 0, 0, &gmW, &gmH, NULL, &actualNumSlices_gm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], gmFilename1);
		exit(1);
	}
	if( (x+w) > gmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, gmW, gmFilename1);
		exit(1);
	}
	if( (y+h) > gmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, gmH, gmFilename1);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_gm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], gmFilename1, actualNumSlices_gm, wmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (csfMasks1=getUNCSlices3D(csfFilename1, 0, 0, &otherW, &otherH, NULL, &actualNumSlices_other, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], csfFilename1);
		exit(1);
	}
	if( (x+w) > otherW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, otherW, csfFilename1);
		exit(1);
	}
	if( (y+h) > otherH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, otherH, csfFilename1);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_other ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], csfFilename1, actualNumSlices_other, wmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (wmMasks2=getUNCSlices3D(wmFilename2, 0, 0, &wmW, &wmH, NULL, &actualNumSlices_wm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmFilename2);
		exit(1);
	}
	if( (x+w) > wmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, wmW, wmFilename2);
		exit(1);
	}
	if( (y+h) > wmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, wmH, wmFilename2);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_wm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], wmFilename2, actualNumSlices_wm, wmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (gmMasks2=getUNCSlices3D(gmFilename2, 0, 0, &gmW, &gmH, NULL, &actualNumSlices_gm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], gmFilename2);
		exit(1);
	}
	if( (x+w) > gmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, gmW, gmFilename2);
		exit(1);
	}
	if( (y+h) > gmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, gmH, gmFilename2);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_gm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], gmFilename2, actualNumSlices_gm, wmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (csfMasks2=getUNCSlices3D(csfFilename2, 0, 0, &otherW, &otherH, NULL, &actualNumSlices_other, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], csfFilename2);
		exit(1);
	}
	if( (x+w) > otherW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, otherW, csfFilename2);
		exit(1);
	}
	if( (y+h) > otherH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, otherH, csfFilename2);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_other ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], csfFilename2, actualNumSlices_other, wmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (MATRIX=callocFLOAT3D(numSlices, 3, 3)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x %d x %d (for MATRIX).\n", argv[0], numSlices, 3, 3);
		exit(1);
	}
	if( (TP=callocFLOAT2D(numSlices, 3)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x %d (for TP).\n", argv[0], numSlices, 3);
		exit(1);
	}
	if( (FP=callocFLOAT2D(numSlices, 3)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x %d (for FP).\n", argv[0], numSlices, 3);
		exit(1);
	}
	if( (TN=callocFLOAT2D(numSlices, 3)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x %d (for TN).\n", argv[0], numSlices, 3);
		exit(1);
	}
	if( (FN=callocFLOAT2D(numSlices, 3)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x %d (for FN).\n", argv[0], numSlices, 3);
		exit(1);
	}

	fprintf(stderr, "%s slice : ", argv[0]); fflush(stderr);
	for(i=0;i<3;i++) for(j=0;j<3;j++) TOTAL_MATRIX[i][j] = 0.0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		fprintf(stderr, "%d ", slice+1); fflush(stderr);
		for(i=0;i<3;i++) for(j=0;j<3;j++) MATRIX[s][i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( (wmMasks1[slice][i][j]>0) && (wmMasks2[slice][i][j]>0) ) MATRIX[s][mWM][mWM]++;
			if( (wmMasks1[slice][i][j]>0) && (gmMasks2[slice][i][j]>0) ) MATRIX[s][mWM][mGM]++;
			if( (wmMasks1[slice][i][j]>0) && (csfMasks2[slice][i][j]>0) ) MATRIX[s][mWM][mCSF]++;
			if( (gmMasks1[slice][i][j]>0) && (wmMasks2[slice][i][j]>0) ) MATRIX[s][mGM][mWM]++;
			if( (gmMasks1[slice][i][j]>0) && (gmMasks2[slice][i][j]>0) ) MATRIX[s][mGM][mGM]++;
			if( (gmMasks1[slice][i][j]>0) && (csfMasks2[slice][i][j]>0) ) MATRIX[s][mGM][mCSF]++;
			if( (csfMasks1[slice][i][j]>0) && (wmMasks2[slice][i][j]>0) ) MATRIX[s][mCSF][mWM]++;
			if( (csfMasks1[slice][i][j]>0) && (gmMasks2[slice][i][j]>0) ) MATRIX[s][mCSF][mGM]++;
			if( (csfMasks1[slice][i][j]>0) && (csfMasks2[slice][i][j]>0) ) MATRIX[s][mCSF][mCSF]++;
		}
	}
	fprintf(stderr, "\n");
	for(s=0;s<numSlices;s++){
		for(i=0;i<3;i++) for(j=0;j<3;j++) TOTAL_MATRIX[i][j] += MATRIX[s][i][j];
/* WM */	TP[s][mWM] = MATRIX[s][mWM][mWM];
		FN[s][mWM] = MATRIX[s][mGM][mWM] + MATRIX[s][mCSF][mWM];
		FP[s][mWM] = MATRIX[s][mWM][mGM] + MATRIX[s][mWM][mCSF];
		TN[s][mWM] = MATRIX[s][mGM][mGM] + MATRIX[s][mGM][mCSF] + MATRIX[s][mCSF][mGM] + MATRIX[s][mCSF][mCSF];

/* GM */	TP[s][mGM] = MATRIX[s][mGM][mGM];
		FN[s][mGM] = MATRIX[s][mWM][mGM] + MATRIX[s][mCSF][mGM];
		FP[s][mGM] = MATRIX[s][mGM][mWM] + MATRIX[s][mGM][mCSF];
		TN[s][mGM] = MATRIX[s][mWM][mWM] + MATRIX[s][mWM][mCSF] + MATRIX[s][mCSF][mWM] + MATRIX[s][mCSF][mCSF];

/*CSF */	TP[s][mCSF] = MATRIX[s][mCSF][mCSF];
		FN[s][mCSF] = MATRIX[s][mWM][mCSF] + MATRIX[s][mGM][mCSF];
		FP[s][mCSF] = MATRIX[s][mCSF][mWM] + MATRIX[s][mCSF][mGM];
		TN[s][mCSF] = MATRIX[s][mWM][mWM] + MATRIX[s][mWM][mGM] + MATRIX[s][mGM][mWM] + MATRIX[s][mGM][mGM];
	}

/* all slices, TOTAL */
/* WM */TOTAL_TP[mWM] = TOTAL_MATRIX[mWM][mWM];
	TOTAL_FN[mWM] = TOTAL_MATRIX[mGM][mWM] + TOTAL_MATRIX[mCSF][mWM];
	TOTAL_FP[mWM] = TOTAL_MATRIX[mWM][mGM] + TOTAL_MATRIX[mWM][mCSF];
	TOTAL_TN[mWM] = TOTAL_MATRIX[mGM][mGM] + TOTAL_MATRIX[mGM][mCSF] + TOTAL_MATRIX[mCSF][mGM] + TOTAL_MATRIX[mCSF][mCSF];

/* GM */TOTAL_TP[mGM] = TOTAL_MATRIX[mGM][mGM];
	TOTAL_FN[mGM] = TOTAL_MATRIX[mWM][mGM] + TOTAL_MATRIX[mCSF][mGM];
	TOTAL_FP[mGM] = TOTAL_MATRIX[mGM][mWM] + TOTAL_MATRIX[mGM][mCSF];
	TOTAL_TN[mGM] = TOTAL_MATRIX[mWM][mWM] + TOTAL_MATRIX[mWM][mCSF] + TOTAL_MATRIX[mCSF][mWM] + TOTAL_MATRIX[mCSF][mCSF];

/*CSF */TOTAL_TP[mCSF] = TOTAL_MATRIX[mCSF][mCSF];
	TOTAL_FN[mCSF] = TOTAL_MATRIX[mWM][mCSF] + TOTAL_MATRIX[mGM][mCSF];
	TOTAL_FP[mCSF] = TOTAL_MATRIX[mCSF][mWM] + TOTAL_MATRIX[mCSF][mGM];
	TOTAL_TN[mCSF] = TOTAL_MATRIX[mWM][mWM] + TOTAL_MATRIX[mWM][mGM] + TOTAL_MATRIX[mGM][mWM] + TOTAL_MATRIX[mGM][mGM];

	fprintf(outputHandle, "# TP: true positive, TN: true negative, FP: false positive, FN: false negative\n");
	fprintf(outputHandle, "# SEN (sensitivity) = TP / (TP + FN) \n");
	fprintf(outputHandle, "# SPE (specificity) = TN / (TN + FP)\n");
	fprintf(outputHandle, "# PPV (positive predictive value) = TP / (TP + FP)\n");
	fprintf(outputHandle, "# NPV (negative predictive value) = TN / (TN + FN)\n");
	fprintf(outputHandle, "# ACC (accuracy) = (TP + TN) / (TP + TN + FP + FN)\n");
	fprintf(outputHandle, "# PRE (prevalence) = (TP + FN) / (TP + TN + FP + FN)\n");
	fprintf(outputHandle, "# TPR (true positive rate) = TP / (TP + FN)\n");
	fprintf(outputHandle, "# FPR (false positive rate) = FP / (TN + FP)\n");

if( !doOverallOnly ){
	fprintf(outputHandle, "# Slice\tTP/WM\tTN/WM\tFP/WM\tFN/WM\tSEN/WM\tSPE/WM\tPPV/WM\tNPV/WM\tACC/WM\tPRE/WM\tTPR/WM\tFPR/WM\tTP/GM\tTN/GM\tFP/GM\tFN/GM\tSEN/GM\tSPE/GM\tPPV/GM\tNPV/GM\tACC/GM\tPRE/GM\tTPR/GM\tFPR/GM\tTP/CSF\tTN/CSF\tFP/CSF\tFN/CSF\tSEN/CSF\tSPE/CSF\tPPV/CSF\tNPV/CSF\tACC/CSF\tPRE/CSF\tTPR/CSF\tFPR/CSF\n");
	for(s=0;s<numSlices;s++){
		fprintf(outputHandle, "   %d\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n", s+1,
			TP[s][mWM], TN[s][mWM], FP[s][mWM], FN[s][mWM],
			TP[s][mWM] / (TP[s][mWM] + FN[s][mWM]),
			TN[s][mWM] / (TN[s][mWM] + FP[s][mWM]),
			TP[s][mWM] / (TP[s][mWM] + FP[s][mWM]),
			TN[s][mWM] / (TN[s][mWM] + FN[s][mWM]),
			(TP[s][mWM] + TN[s][mWM]) / (TP[s][mWM] + TN[s][mWM] + FP[s][mWM] + FN[s][mWM]),
			(TP[s][mWM] + FN[s][mWM]) / (TP[s][mWM] + TN[s][mWM] + FP[s][mWM] + FN[s][mWM]),
			TP[s][mWM] / (TP[s][mWM] + FN[s][mWM]),
			FP[s][mWM] / (TN[s][mWM] + FP[s][mWM]),

			TP[s][mGM], TN[s][mGM], FP[s][mGM], FN[s][mGM],
			TP[s][mGM] / (TP[s][mGM] + FN[s][mGM]),
			TN[s][mGM] / (TN[s][mGM] + FP[s][mGM]),
			TP[s][mGM] / (TP[s][mGM] + FP[s][mGM]),
			TN[s][mGM] / (TN[s][mGM] + FN[s][mGM]),
			(TP[s][mGM] + TN[s][mGM]) / (TP[s][mGM] + TN[s][mGM] + FP[s][mGM] + FN[s][mGM]),
			(TP[s][mGM] + FN[s][mGM]) / (TP[s][mGM] + TN[s][mGM] + FP[s][mGM] + FN[s][mGM]),
			TP[s][mGM] / (TP[s][mGM] + FN[s][mGM]),
			FP[s][mGM] / (TN[s][mGM] + FP[s][mGM]),

			TP[s][mCSF], TN[s][mCSF], FP[s][mCSF], FN[s][mCSF],
			TP[s][mCSF] / (TP[s][mCSF] + FN[s][mCSF]),
			TN[s][mCSF] / (TN[s][mCSF] + FP[s][mCSF]),
			TP[s][mCSF] / (TP[s][mCSF] + FP[s][mCSF]),
			TN[s][mCSF] / (TN[s][mCSF] + FN[s][mCSF]),
			(TP[s][mCSF] + TN[s][mCSF]) / (TP[s][mCSF] + TN[s][mCSF] + FP[s][mCSF] + FN[s][mCSF]),
			(TP[s][mCSF] + FN[s][mCSF]) / (TP[s][mCSF] + TN[s][mCSF] + FP[s][mCSF] + FN[s][mCSF]),
			TP[s][mCSF] / (TP[s][mCSF] + FN[s][mCSF]),
			FP[s][mCSF] / (TN[s][mCSF] + FP[s][mCSF]));
	}
}
	/* total */
if( (numSlices > 1) || doOverallOnly ){
	fprintf(outputHandle, "\n# ------------- total --------------\n");
	fprintf(outputHandle, "# Slice\tTP/WM\tTN/WM\tFP/WM\tFN/WM\tSEN/WM\tSPE/WM\tPPV/WM\tNPV/WM\tACC/WM\tPRE/WM\tTPR/WM\tFPR/WM\tTP/GM\tTN/GM\tFP/GM\tFN/GM\tSEN/GM\tSPE/GM\tPPV/GM\tNPV/GM\tACC/GM\tPRE/GM\tTPR/GM\tFPR/GM\tTP/CSF\tTN/CSF\tFP/CSF\tFN/CSF\tSEN/CSF\tSPE/CSF\tPPV/CSF\tNPV/CSF\tACC/CSF\tPRE/CSF\tTPR/CSF\tFPR/CSF\n");
	fprintf(outputHandle, "0\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mWM], TOTAL_TN[mWM], TOTAL_FP[mWM], TOTAL_FN[mWM],
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FN[mWM]),
		TOTAL_TN[mWM] / (TOTAL_TN[mWM] + TOTAL_FP[mWM]),
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FP[mWM]),
		TOTAL_TN[mWM] / (TOTAL_TN[mWM] + TOTAL_FN[mWM]),
		(TOTAL_TP[mWM] + TOTAL_TN[mWM]) / (TOTAL_TP[mWM] + TOTAL_TN[mWM] + TOTAL_FP[mWM] + TOTAL_FN[mWM]),
		(TOTAL_TP[mWM] + TOTAL_FN[mWM]) / (TOTAL_TP[mWM] + TOTAL_TN[mWM] + TOTAL_FP[mWM] + TOTAL_FN[mWM]),
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FN[mWM]),
		TOTAL_FP[mWM] / (TOTAL_TN[mWM] + TOTAL_FP[mWM]),

		TOTAL_TP[mGM], TOTAL_TN[mGM], TOTAL_FP[mGM], TOTAL_FN[mGM],
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FN[mGM]),
		TOTAL_TN[mGM] / (TOTAL_TN[mGM] + TOTAL_FP[mGM]),
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FP[mGM]),
		TOTAL_TN[mGM] / (TOTAL_TN[mGM] + TOTAL_FN[mGM]),
		(TOTAL_TP[mGM] + TOTAL_TN[mGM]) / (TOTAL_TP[mGM] + TOTAL_TN[mGM] + TOTAL_FP[mGM] + TOTAL_FN[mGM]),
		(TOTAL_TP[mGM] + TOTAL_FN[mGM]) / (TOTAL_TP[mGM] + TOTAL_TN[mGM] + TOTAL_FP[mGM] + TOTAL_FN[mGM]),
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FN[mGM]),
		TOTAL_FP[mGM] / (TOTAL_TN[mGM] + TOTAL_FP[mGM]),

		TOTAL_TP[mCSF], TOTAL_TN[mCSF], TOTAL_FP[mCSF], TOTAL_FN[mCSF],
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_TN[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FP[mCSF]),
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FP[mCSF]),
		TOTAL_TN[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FN[mCSF]),
		(TOTAL_TP[mCSF] + TOTAL_TN[mCSF]) / (TOTAL_TP[mCSF] + TOTAL_TN[mCSF] + TOTAL_FP[mCSF] + TOTAL_FN[mCSF]),
		(TOTAL_TP[mCSF] + TOTAL_FN[mCSF]) / (TOTAL_TP[mCSF] + TOTAL_TN[mCSF] + TOTAL_FP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_FP[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FP[mCSF]));
}

	/* print them in a matrix form as well */
	fprintf(outputHandle, "\n\n# ----- repeat of the above data but in a matrix form, rows are each tissue type\n");
if( !doOverallOnly ){
	for(s=0;s<numSlices;s++){
		fprintf(outputHandle, "# Sl:%d\tTP\tTN\tFP\tFN\tSEN\tSPE\tPPV\tNPV\tACC\tPRE\tTPR\tFPR\n", s+1);
		fprintf(outputHandle, "# WM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			TP[s][mWM], TN[s][mWM], FP[s][mWM], FN[s][mWM],
			TP[s][mWM] / (TP[s][mWM] + FN[s][mWM]),
			TN[s][mWM] / (TN[s][mWM] + FP[s][mWM]),
			TP[s][mWM] / (TP[s][mWM] + FP[s][mWM]),
			TN[s][mWM] / (TN[s][mWM] + FN[s][mWM]),
			(TP[s][mWM] + TN[s][mWM]) / (TP[s][mWM] + TN[s][mWM] + FP[s][mWM] + FN[s][mWM]),
			(TP[s][mWM] + FN[s][mWM]) / (TP[s][mWM] + TN[s][mWM] + FP[s][mWM] + FN[s][mWM]),
			TP[s][mWM] / (TP[s][mWM] + FN[s][mWM]),
			FP[s][mWM] / (TN[s][mWM] + FP[s][mWM]) );
		fprintf(outputHandle, "# GM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			TP[s][mGM], TN[s][mGM], FP[s][mGM], FN[s][mGM],
			TP[s][mGM] / (TP[s][mGM] + FN[s][mGM]),
			TN[s][mGM] / (TN[s][mGM] + FP[s][mGM]),
			TP[s][mGM] / (TP[s][mGM] + FP[s][mGM]),
			TN[s][mGM] / (TN[s][mGM] + FN[s][mGM]),
			(TP[s][mGM] + TN[s][mGM]) / (TP[s][mGM] + TN[s][mGM] + FP[s][mGM] + FN[s][mGM]),
			(TP[s][mGM] + FN[s][mGM]) / (TP[s][mGM] + TN[s][mGM] + FP[s][mGM] + FN[s][mGM]),
			TP[s][mGM] / (TP[s][mGM] + FN[s][mGM]),
			FP[s][mGM] / (TN[s][mGM] + FP[s][mGM]) );
		fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			TP[s][mCSF], TN[s][mCSF], FP[s][mCSF], FN[s][mCSF],
			TP[s][mCSF] / (TP[s][mCSF] + FN[s][mCSF]),
			TN[s][mCSF] / (TN[s][mCSF] + FP[s][mCSF]),
			TP[s][mCSF] / (TP[s][mCSF] + FP[s][mCSF]),
			TN[s][mCSF] / (TN[s][mCSF] + FN[s][mCSF]),
			(TP[s][mCSF] + TN[s][mCSF]) / (TP[s][mCSF] + TN[s][mCSF] + FP[s][mCSF] + FN[s][mCSF]),
			(TP[s][mCSF] + FN[s][mCSF]) / (TP[s][mCSF] + TN[s][mCSF] + FP[s][mCSF] + FN[s][mCSF]),
			TP[s][mCSF] / (TP[s][mCSF] + FN[s][mCSF]),
			FP[s][mCSF] / (TN[s][mCSF] + FP[s][mCSF]));
	}
}

	/* total */
if( (numSlices > 1) || doOverallOnly ){
	fprintf(outputHandle, "\n");
	fprintf(outputHandle, "# ------------- total of the above --------------\n");
	fprintf(outputHandle, "# \tTP\tTN\tFP\tFN\tSEN\tSPE\tPPV\tNPV\tACC\tPRE\tTPR\tFPR\n");
	fprintf(outputHandle, "# WM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mWM], TOTAL_TN[mWM], TOTAL_FP[mWM], TOTAL_FN[mWM],
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FN[mWM]),
		TOTAL_TN[mWM] / (TOTAL_TN[mWM] + TOTAL_FP[mWM]),
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FP[mWM]),
		TOTAL_TN[mWM] / (TOTAL_TN[mWM] + TOTAL_FN[mWM]),
		(TOTAL_TP[mWM] + TOTAL_TN[mWM]) / (TOTAL_TP[mWM] + TOTAL_TN[mWM] + TOTAL_FP[mWM] + TOTAL_FN[mWM]),
		(TOTAL_TP[mWM] + TOTAL_FN[mWM]) / (TOTAL_TP[mWM] + TOTAL_TN[mWM] + TOTAL_FP[mWM] + TOTAL_FN[mWM]),
		TOTAL_TP[mWM] / (TOTAL_TP[mWM] + TOTAL_FN[mWM]),
		TOTAL_FP[mWM] / (TOTAL_TN[mWM] + TOTAL_FP[mWM]) );
	fprintf(outputHandle, "# GM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mGM], TOTAL_TN[mGM], TOTAL_FP[mGM], TOTAL_FN[mGM],
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FN[mGM]),
		TOTAL_TN[mGM] / (TOTAL_TN[mGM] + TOTAL_FP[mGM]),
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FP[mGM]),
		TOTAL_TN[mGM] / (TOTAL_TN[mGM] + TOTAL_FN[mGM]),
		(TOTAL_TP[mGM] + TOTAL_TN[mGM]) / (TOTAL_TP[mGM] + TOTAL_TN[mGM] + TOTAL_FP[mGM] + TOTAL_FN[mGM]),
		(TOTAL_TP[mGM] + TOTAL_FN[mGM]) / (TOTAL_TP[mGM] + TOTAL_TN[mGM] + TOTAL_FP[mGM] + TOTAL_FN[mGM]),
		TOTAL_TP[mGM] / (TOTAL_TP[mGM] + TOTAL_FN[mGM]),
		TOTAL_FP[mGM] / (TOTAL_TN[mGM] + TOTAL_FP[mGM]) );

	fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mCSF], TOTAL_TN[mCSF], TOTAL_FP[mCSF], TOTAL_FN[mCSF],
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_TN[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FP[mCSF]),
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FP[mCSF]),
		TOTAL_TN[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FN[mCSF]),
		(TOTAL_TP[mCSF] + TOTAL_TN[mCSF]) / (TOTAL_TP[mCSF] + TOTAL_TN[mCSF] + TOTAL_FP[mCSF] + TOTAL_FN[mCSF]),
		(TOTAL_TP[mCSF] + TOTAL_FN[mCSF]) / (TOTAL_TP[mCSF] + TOTAL_TN[mCSF] + TOTAL_FP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_TP[mCSF] / (TOTAL_TP[mCSF] + TOTAL_FN[mCSF]),
		TOTAL_FP[mCSF] / (TOTAL_TN[mCSF] + TOTAL_FP[mCSF]) );
}
	/* print the matrix */
	fprintf(outputHandle, "\n\n# -- for each slice, the following matrices record the predicted and actual pixels for each tissue (WM/GM/CSF) --\n");
	fprintf(outputHandle, "# -- the number in the cell at the intersection of WM-row and WM-column denotes the number of pixels predicted to be WM and were actually WM (according to the gold-standard of the images in files defined using the -W -G and -C options --\n");
	fprintf(outputHandle, "# -- the number in the cell at the intersection of WM-row and GM-column denotes the number of pixels predicted to be WM but they were actually GM. --\n");
	fprintf(outputHandle, "# -- The errors at the last line are the percentages of abs(true-predicted)/true * 100 %%. The overall error of misclassification is the bottom right number -- \n");
if( !doOverallOnly ){
	for(s=0;s<numSlices;s++){
		fprintf(outputHandle, "# Sl:%d\tWM\tGM\tCSF\tPredicted number of Pixels\n", s+1);
		truePixels[mWM] = MATRIX[s][mWM][mWM]+MATRIX[s][mGM][mWM]+MATRIX[s][mCSF][mWM];
		truePixels[mGM] = MATRIX[s][mWM][mGM]+MATRIX[s][mGM][mGM]+MATRIX[s][mCSF][mGM];
		truePixels[mCSF]= MATRIX[s][mWM][mCSF]+MATRIX[s][mGM][mCSF]+MATRIX[s][mCSF][mCSF];
		predictedPixels[mWM] = MATRIX[s][mWM][mWM]+MATRIX[s][mWM][mGM]+MATRIX[s][mWM][mCSF];
		predictedPixels[mGM] = MATRIX[s][mGM][mWM]+MATRIX[s][mGM][mGM]+MATRIX[s][mGM][mCSF];
		predictedPixels[mCSF]= MATRIX[s][mCSF][mWM]+MATRIX[s][mCSF][mGM]+MATRIX[s][mCSF][mCSF];
		missedPixels[mWM] = 100.0 * ABS(truePixels[mWM] - predictedPixels[mWM]) / truePixels[mWM];
		missedPixels[mGM] = 100.0 * ABS(truePixels[mGM] - predictedPixels[mGM]) / truePixels[mGM];
		missedPixels[mCSF]= 100.0 * ABS(truePixels[mCSF] - predictedPixels[mCSF]) / truePixels[mCSF];
		totalNumPixels = truePixels[mWM]+truePixels[mGM]+truePixels[mCSF]; /* this is equal to the sum of the predicted too */
		totalError = 100.0 * (totalNumPixels - MATRIX[s][mWM][mWM] - MATRIX[s][mGM][mGM] - MATRIX[s][mCSF][mCSF]) / totalNumPixels;

		fprintf(outputHandle, "# WM\t%.0f\t%.0f\t%.0f\t%.0f\n", MATRIX[s][mWM][mWM], MATRIX[s][mWM][mGM], MATRIX[s][mWM][mCSF], predictedPixels[mWM]);
		fprintf(outputHandle, "# GM\t%.0f\t%.0f\t%.0f\t%.0f\n", MATRIX[s][mGM][mWM], MATRIX[s][mGM][mGM], MATRIX[s][mGM][mCSF], predictedPixels[mGM]);
		fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\t%.0f\n", MATRIX[s][mCSF][mWM], MATRIX[s][mCSF][mGM], MATRIX[s][mCSF][mCSF], predictedPixels[mCSF]);
		fprintf(outputHandle, "# -----------------------------------\n");
		fprintf(outputHandle, "# True\t%.0f\t%.0f\t%.0f\t%.0f\n", truePixels[mWM], truePixels[mGM], truePixels[mCSF], truePixels[mWM]+truePixels[mGM]+truePixels[mCSF]);
		fprintf(outputHandle, "# Error\t%.2f\t%.2f\t%.2f\t%.2f\n", missedPixels[mWM], missedPixels[mGM], missedPixels[mCSF], totalError);
		fprintf(outputHandle, "\n");
	}
}

if( (numSlices > 1) || doOverallOnly ){
	fprintf(outputHandle, "# --------------- total of the above -------------\n# Total\tWM\tGM\tCSF\tPredicted number of Pixels\n");
	truePixels[mWM] = TOTAL_MATRIX[mWM][mWM]+TOTAL_MATRIX[mGM][mWM]+TOTAL_MATRIX[mCSF][mWM];
	truePixels[mGM] = TOTAL_MATRIX[mWM][mGM]+TOTAL_MATRIX[mGM][mGM]+TOTAL_MATRIX[mCSF][mGM];
	truePixels[mCSF]= TOTAL_MATRIX[mWM][mCSF]+TOTAL_MATRIX[mGM][mCSF]+TOTAL_MATRIX[mCSF][mCSF];
	predictedPixels[mWM] = TOTAL_MATRIX[mWM][mWM]+TOTAL_MATRIX[mWM][mGM]+TOTAL_MATRIX[mWM][mCSF];
	predictedPixels[mGM] = TOTAL_MATRIX[mGM][mWM]+TOTAL_MATRIX[mGM][mGM]+TOTAL_MATRIX[mGM][mCSF];
	predictedPixels[mCSF]= TOTAL_MATRIX[mCSF][mWM]+TOTAL_MATRIX[mCSF][mGM]+TOTAL_MATRIX[mCSF][mCSF];
	missedPixels[mWM] = 100.0 * ABS(truePixels[mWM] - predictedPixels[mWM]) / truePixels[mWM];
	missedPixels[mGM] = 100.0 * ABS(truePixels[mGM] - predictedPixels[mGM]) / truePixels[mGM];
	missedPixels[mCSF]= 100.0 * ABS(truePixels[mCSF] - predictedPixels[mCSF]) / truePixels[mCSF];
	totalNumPixels = truePixels[mWM]+truePixels[mGM]+truePixels[mCSF]; /* this is equal to the sum of the predicted too */
	totalError = 100.0 * (totalNumPixels - TOTAL_MATRIX[mWM][mWM] - TOTAL_MATRIX[mGM][mGM] - TOTAL_MATRIX[mCSF][mCSF]) / totalNumPixels;

	fprintf(outputHandle, "# WM\t%.0f\t%.0f\t%.0f\t%.0f\n", TOTAL_MATRIX[mWM][mWM], TOTAL_MATRIX[mWM][mGM], TOTAL_MATRIX[mWM][mCSF], predictedPixels[mWM]);
	fprintf(outputHandle, "# GM\t%.0f\t%.0f\t%.0f\t%.0f\n", TOTAL_MATRIX[mGM][mWM], TOTAL_MATRIX[mGM][mGM], TOTAL_MATRIX[mGM][mCSF], predictedPixels[mGM]);
	fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\t%.0f\n", TOTAL_MATRIX[mCSF][mWM], TOTAL_MATRIX[mCSF][mGM], TOTAL_MATRIX[mCSF][mCSF], predictedPixels[mCSF]);
	fprintf(outputHandle, "# -----------------------------------\n");
	fprintf(outputHandle, "# True\t%.0f\t%.0f\t%.0f\t%.0f\n", truePixels[mWM], truePixels[mGM], truePixels[mCSF], totalNumPixels);
	fprintf(outputHandle, "# Error\t%.2f\t%.2f\t%.2f\t%.2f\n", missedPixels[mWM], missedPixels[mGM], missedPixels[mCSF], totalError);
	fprintf(outputHandle, "\n");
}

	freeDATATYPE3D(wmMasks1, actualNumSlices_wm, wmW); freeDATATYPE3D(gmMasks1, actualNumSlices_gm, gmW); freeDATATYPE3D(csfMasks1, actualNumSlices_other, otherW);
	free(wmFilename1); free(csfFilename1); free(gmFilename1);
	if( outputFilename != NULL ){
		fclose(outputHandle);
		free(outputFilename);
	}
	freeFLOAT3D(MATRIX, numSlices, 3);
	freeFLOAT2D(TP, numSlices);
	freeFLOAT2D(TN, numSlices);
	freeFLOAT2D(FP, numSlices);
	freeFLOAT2D(FN, numSlices);
	exit(0);
}
