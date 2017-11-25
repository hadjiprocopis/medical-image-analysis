#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

#define	mWMGM	0
#define	mCSF	1
 
const	char	Examples[] = "\
\n	-W actual_wm.unc -G actual_gm.unc -C actual_csf.unc -m predicted_wm.unc -g predicted_gm.unc -c predicted_csf.unc -o output\
\n";

const	char	Usage[] = "options as follows:\
\nGold standard files:\
\n\t -T tissueFilename\
\n	(UNC image file with one or more slices holding the *gold standard* WM&GM = tissue probability maps or just masks)\
\n\
\n\t -C csfFilename\
\n	(UNC image file with one or more slices holding the *gold standard* CSF probability maps or just masks)\
\n\
\nPredicted segmentation files:\
\n\t -t whiteMatterFilename\
\n	(UNC image file with one or more slices holding the *predicted* WM&GM = tissue probability maps or just masks)\
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
\nThis program will evaluate the results of a WMGM/CSF segmentation.\
\nThis is done by supplying the mask files of the predicted segmentations\
\nfor the three compartments (WMGM/CSF) AND the mask files of a\
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
\n    WM  CSF\
\nWM  A1  A2\
\nCSF B1  B2\
\n\
\nwhere A1 is the number of pixels predicted to be WM and\
\nwere actually WM, while A2 is the number of pixels predicted\
\nto be WM but were actually CSF. Similarly, B2 was the number\
\nof pixels predicted to be WN but actually were CSF (always\
\naccording to the gold standard files).\
\n\
\nThen for WMGM,\
\n	true positive, TP = A1\
\n	false negative, FN = B1\
\n	false positive, FP = A2\
\n	true negative, TN = B2\
\nFor CSF,\
\n	true positive, TP = B2\
\n	false negative, FN = A2\
\n	false positive, FP = B1\
\n	true negative, TN = A1\
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
	DATATYPE	***wmgmMasks1, ***csfMasks1,
			***wmgmMasks2, ***csfMasks2;
	char		*outputFilename = NULL,
			*wmgmFilename1 = NULL, *csfFilename1 = NULL,
			*wmgmFilename2 = NULL, *csfFilename2 = NULL;
	FILE		*outputHandle = stdout;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	float		***MATRIX, 	    /* true versus predicted allocations for each slice */
			TOTAL_MATRIX[2][2], /* true versus predicted allocations for all slices*/
			**TP, TOTAL_TP[2],
			**TN, TOTAL_TN[2],
			**FP, TOTAL_FP[2],
			**FN, TOTAL_FN[2],
			missedPixels[2], truePixels[2], predictedPixels[2],
			totalNumPixels, totalError;
	register int	i, j;
	int		wmgmW = -1, csfW = -1,
			wmgmH = -1, csfH = -1,
			actualNumSlices_wmgm = 0, actualNumSlices_csf = 0;

	int		doOverallOnly = FALSE; /* e.g. report all slices not just totals */

	while( (optI=getopt(argc, argv, "o:es:w:h:x:y:T:C:t:c:a")) != EOF)
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
			case 'T': wmgmFilename2 = strdup(optarg); break; /* white matter */
			case 'C': csfFilename2 = strdup(optarg); break; /* csf */
			case 't': wmgmFilename1 = strdup(optarg); break; /* white matter */
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

	if( wmgmFilename1 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A first white matter filename must be specified.\n");
		exit(1);
	}
	if( csfFilename1 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A first csf filename must be specified.\n");
		exit(1);
	}
	if( wmgmFilename2 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second white matter filename must be specified.\n");
		exit(1);
	}
	if( csfFilename2 == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A second csf filename must be specified.\n");
		exit(1);
	}
	if( (wmgmMasks1=getUNCSlices3D(wmgmFilename1, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmgmFilename1);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, wmgmFilename1);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, W, wmgmFilename1);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, H, wmgmFilename1);
		exit(1);
	}
	if( (csfMasks1=getUNCSlices3D(csfFilename1, 0, 0, &csfW, &csfH, NULL, &actualNumSlices_csf, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], csfFilename1);
		exit(1);
	}
	if( (x+w) > csfW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, csfW, csfFilename1);
		exit(1);
	}
	if( (y+h) > csfH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, csfH, csfFilename1);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_csf ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], csfFilename1, actualNumSlices_csf, wmgmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (wmgmMasks2=getUNCSlices3D(wmgmFilename2, 0, 0, &wmgmW, &wmgmH, NULL, &actualNumSlices_wmgm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmgmFilename2);
		exit(1);
	}
	if( (x+w) > wmgmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, wmgmW, wmgmFilename2);
		exit(1);
	}
	if( (y+h) > wmgmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, wmgmH, wmgmFilename2);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_wmgm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], wmgmFilename2, actualNumSlices_wmgm, wmgmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (csfMasks2=getUNCSlices3D(csfFilename2, 0, 0, &csfW, &csfH, NULL, &actualNumSlices_csf, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], csfFilename2);
		exit(1);
	}
	if( (x+w) > csfW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, csfW, csfFilename2);
		exit(1);
	}
	if( (y+h) > csfH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, csfH, csfFilename2);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_csf ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], csfFilename2, actualNumSlices_csf, wmgmFilename1, actualNumSlices);
			exit(1);
		}
	}
	if( (MATRIX=callocFLOAT3D(numSlices, 2, 2)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x 2 x 2 (for MATRIX).\n", argv[0], numSlices);
		exit(1);
	}
	if( (TP=callocFLOAT2D(numSlices, 2)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x 2 (for TP).\n", argv[0], numSlices);
		exit(1);
	}
	if( (FP=callocFLOAT2D(numSlices, 2)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x 2 (for FP).\n", argv[0], numSlices);
		exit(1);
	}
	if( (TN=callocFLOAT2D(numSlices, 2)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x 2 (for TN).\n", argv[0], numSlices);
		exit(1);
	}
	if( (FN=callocFLOAT2D(numSlices, 2)) == NULL ){
		fprintf(stderr, "%s : call to callocFLOAT3D has failed for %d x 2 (for FN).\n", argv[0], numSlices);
		exit(1);
	}

	fprintf(stderr, "%s slice : ", argv[0]); fflush(stderr);
	for(i=0;i<2;i++) for(j=0;j<2;j++) TOTAL_MATRIX[i][j] = 0.0;
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		fprintf(stderr, "%d ", slice+1); fflush(stderr);
		for(i=0;i<2;i++) for(j=0;j<2;j++) MATRIX[s][i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( (wmgmMasks1[slice][i][j]>0) && (wmgmMasks2[slice][i][j]>0) ) MATRIX[s][mWMGM][mWMGM]++;
			if( (wmgmMasks1[slice][i][j]>0) && (csfMasks2[slice][i][j]>0) ) MATRIX[s][mWMGM][mCSF]++;
			if( (csfMasks1[slice][i][j]>0) && (wmgmMasks2[slice][i][j]>0) ) MATRIX[s][mCSF][mWMGM]++;
			if( (csfMasks1[slice][i][j]>0) && (csfMasks2[slice][i][j]>0) ) MATRIX[s][mCSF][mCSF]++;
		}
	}
	fprintf(stderr, "\n");
	for(s=0;s<numSlices;s++){
		for(i=0;i<2;i++) for(j=0;j<2;j++) TOTAL_MATRIX[i][j] += MATRIX[s][i][j];
/* WMGM */	TP[s][mWMGM] = MATRIX[s][mWMGM][mWMGM];
		FN[s][mWMGM] = MATRIX[s][mCSF][mWMGM];
		FP[s][mWMGM] = MATRIX[s][mWMGM][mCSF];
		TN[s][mWMGM] = MATRIX[s][mCSF][mCSF];

/*CSF */	TP[s][mCSF] = MATRIX[s][mCSF][mCSF];
		FN[s][mCSF] = MATRIX[s][mWMGM][mCSF];
		FP[s][mCSF] = MATRIX[s][mCSF][mWMGM];
		TN[s][mCSF] = MATRIX[s][mWMGM][mWMGM];
	}

/* all slices, TOTAL */
/*WMGM*/TOTAL_TP[mWMGM] = TOTAL_MATRIX[mWMGM][mWMGM];
	TOTAL_FN[mWMGM] = TOTAL_MATRIX[mCSF][mWMGM];
	TOTAL_FP[mWMGM] = TOTAL_MATRIX[mWMGM][mCSF];
	TOTAL_TN[mWMGM] = TOTAL_MATRIX[mCSF][mCSF];

/*CSF */TOTAL_TP[mCSF] = TOTAL_MATRIX[mCSF][mCSF];
	TOTAL_FN[mCSF] = TOTAL_MATRIX[mWMGM][mCSF];
	TOTAL_FP[mCSF] = TOTAL_MATRIX[mCSF][mWMGM];
	TOTAL_TN[mCSF] = TOTAL_MATRIX[mWMGM][mWMGM];

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
	fprintf(outputHandle, "# Slice\tTP/WMGM\tTN/WMGM\tFP/WMGM\tFN/WMGM\tSEN/WMGM\tSPE/WMGM\tPPV/WMGM\tNPV/WMGM\tACC/WMGM\tPRE/WMGM\tTPR/WMGM\tFPR/WMGM\tTP/CSF\tTN/CSF\tFP/CSF\tFN/CSF\tSEN/CSF\tSPE/CSF\tPPV/CSF\tNPV/CSF\tACC/CSF\tPRE/CSF\tTPR/CSF\tFPR/CSF\n");
	for(s=0;s<numSlices;s++){
		fprintf(outputHandle, "   %d\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n", s+1,
			TP[s][mWMGM], TN[s][mWMGM], FP[s][mWMGM], FN[s][mWMGM],
			TP[s][mWMGM] / (TP[s][mWMGM] + FN[s][mWMGM]),
			TN[s][mWMGM] / (TN[s][mWMGM] + FP[s][mWMGM]),
			TP[s][mWMGM] / (TP[s][mWMGM] + FP[s][mWMGM]),
			TN[s][mWMGM] / (TN[s][mWMGM] + FN[s][mWMGM]),
			(TP[s][mWMGM] + TN[s][mWMGM]) / (TP[s][mWMGM] + TN[s][mWMGM] + FP[s][mWMGM] + FN[s][mWMGM]),
			(TP[s][mWMGM] + FN[s][mWMGM]) / (TP[s][mWMGM] + TN[s][mWMGM] + FP[s][mWMGM] + FN[s][mWMGM]),
			TP[s][mWMGM] / (TP[s][mWMGM] + FN[s][mWMGM]),
			FP[s][mWMGM] / (TN[s][mWMGM] + FP[s][mWMGM]),

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
	fprintf(outputHandle, "# Slice\tTP/WMGM\tTN/WMGM\tFP/WMGM\tFN/WMGM\tSEN/WMGM\tSPE/WMGM\tPPV/WMGM\tNPV/WMGM\tACC/WMGM\tPRE/WMGM\tTPR/WMGM\tFPR/WMGM\tTP/CSF\tTN/CSF\tFP/CSF\tFN/CSF\tSEN/CSF\tSPE/CSF\tPPV/CSF\tNPV/CSF\tACC/CSF\tPRE/CSF\tTPR/CSF\tFPR/CSF\n");
	fprintf(outputHandle, "0\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mWMGM], TOTAL_TN[mWMGM], TOTAL_FP[mWMGM], TOTAL_FN[mWMGM],
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_TN[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM]),
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FP[mWMGM]),
		TOTAL_TN[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FN[mWMGM]),
		(TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM]) / (TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM] + TOTAL_FN[mWMGM]),
		(TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]) / (TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_FP[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM]),

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
		fprintf(outputHandle, "# WMGM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
			TP[s][mWMGM], TN[s][mWMGM], FP[s][mWMGM], FN[s][mWMGM],
			TP[s][mWMGM] / (TP[s][mWMGM] + FN[s][mWMGM]),
			TN[s][mWMGM] / (TN[s][mWMGM] + FP[s][mWMGM]),
			TP[s][mWMGM] / (TP[s][mWMGM] + FP[s][mWMGM]),
			TN[s][mWMGM] / (TN[s][mWMGM] + FN[s][mWMGM]),
			(TP[s][mWMGM] + TN[s][mWMGM]) / (TP[s][mWMGM] + TN[s][mWMGM] + FP[s][mWMGM] + FN[s][mWMGM]),
			(TP[s][mWMGM] + FN[s][mWMGM]) / (TP[s][mWMGM] + TN[s][mWMGM] + FP[s][mWMGM] + FN[s][mWMGM]),
			TP[s][mWMGM] / (TP[s][mWMGM] + FN[s][mWMGM]),
			FP[s][mWMGM] / (TN[s][mWMGM] + FP[s][mWMGM]) );
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
	fprintf(outputHandle, "# WMGM\t%.0f\t%.0f\t%.0f\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		TOTAL_TP[mWMGM], TOTAL_TN[mWMGM], TOTAL_FP[mWMGM], TOTAL_FN[mWMGM],
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_TN[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM]),
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FP[mWMGM]),
		TOTAL_TN[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FN[mWMGM]),
		(TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM]) / (TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM] + TOTAL_FN[mWMGM]),
		(TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]) / (TOTAL_TP[mWMGM] + TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_TP[mWMGM] / (TOTAL_TP[mWMGM] + TOTAL_FN[mWMGM]),
		TOTAL_FP[mWMGM] / (TOTAL_TN[mWMGM] + TOTAL_FP[mWMGM]) );

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
	fprintf(outputHandle, "\n\n# -- for each slice, the following matrices record the predicted and actual pixels for each tissue (WMGM/CSF) --\n");
	fprintf(outputHandle, "# -- the number in the cell at the intersection of WM-row and WM-column denotes the number of pixels predicted to be WM and were actually WM (according to the gold-standard of the images in files defined using the -W -G and -C options --\n");
	fprintf(outputHandle, "# -- the number in the cell at the intersection of WM-row and GM-column denotes the number of pixels predicted to be WM but they were actually GM. --\n");
	fprintf(outputHandle, "# -- The errors at the last line are the percentages of abs(true-predicted)/true * 100 %%. The overall error of misclassification is the bottom right number -- \n");
if( !doOverallOnly ){
	for(s=0;s<numSlices;s++){
		fprintf(outputHandle, "# Sl:%d\tWMGM\tCSF\tPredicted number of Pixels\n", s+1);
		truePixels[mWMGM] = MATRIX[s][mWMGM][mWMGM]+MATRIX[s][mCSF][mWMGM];
		truePixels[mCSF]= MATRIX[s][mWMGM][mCSF]+MATRIX[s][mCSF][mCSF];
		predictedPixels[mWMGM] = MATRIX[s][mWMGM][mWMGM]+MATRIX[s][mWMGM][mCSF];
		predictedPixels[mCSF]= MATRIX[s][mCSF][mWMGM]+MATRIX[s][mCSF][mCSF];
		missedPixels[mWMGM] = 100.0 * ABS(truePixels[mWMGM] - predictedPixels[mWMGM]) / truePixels[mWMGM];
		missedPixels[mCSF]= 100.0 * ABS(truePixels[mCSF] - predictedPixels[mCSF]) / truePixels[mCSF];
		totalNumPixels = truePixels[mWMGM]+truePixels[mCSF]; /* this is equal to the sum of the predicted too */
		totalError = 100.0 * (totalNumPixels - MATRIX[s][mWMGM][mWMGM] - MATRIX[s][mCSF][mCSF]) / totalNumPixels;

		fprintf(outputHandle, "# WMGM\t%.0f\t%.0f\t%.0f\n", MATRIX[s][mWMGM][mWMGM], MATRIX[s][mWMGM][mCSF], predictedPixels[mWMGM]);
		fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\n", MATRIX[s][mCSF][mWMGM], MATRIX[s][mCSF][mCSF], predictedPixels[mCSF]);
		fprintf(outputHandle, "# -----------------------------------\n");
		fprintf(outputHandle, "# True\t%.0f\t%.0f\t%.0f\n", truePixels[mWMGM], truePixels[mCSF], truePixels[mWMGM]+truePixels[mCSF]);
		fprintf(outputHandle, "# Error\t%.2f\t%.2f\t%.2f\n", missedPixels[mWMGM], missedPixels[mCSF], totalError);
		fprintf(outputHandle, "\n");
	}
}

if( (numSlices > 1) || doOverallOnly ){
	fprintf(outputHandle, "# --------------- total of the above -------------\n# Total\tWMGM\tCSF\tPredicted number of Pixels\n");
	truePixels[mWMGM] = TOTAL_MATRIX[mWMGM][mWMGM]+TOTAL_MATRIX[mCSF][mWMGM];
	truePixels[mCSF]= TOTAL_MATRIX[mWMGM][mCSF]+TOTAL_MATRIX[mCSF][mCSF];
	predictedPixels[mWMGM] = TOTAL_MATRIX[mWMGM][mWMGM]+TOTAL_MATRIX[mWMGM][mCSF];
	predictedPixels[mCSF]= TOTAL_MATRIX[mCSF][mWMGM]+TOTAL_MATRIX[mCSF][mCSF];
	missedPixels[mWMGM] = 100.0 * ABS(truePixels[mWMGM] - predictedPixels[mWMGM]) / truePixels[mWMGM];
	missedPixels[mCSF]= 100.0 * ABS(truePixels[mCSF] - predictedPixels[mCSF]) / truePixels[mCSF];
	totalNumPixels = truePixels[mWMGM]+truePixels[mCSF]; /* this is equal to the sum of the predicted too */
	totalError = 100.0 * (totalNumPixels - TOTAL_MATRIX[mWMGM][mWMGM] - TOTAL_MATRIX[mCSF][mCSF]) / totalNumPixels;

	fprintf(outputHandle, "# WMGM\t%.0f\t%.0f\t%.0f\n", TOTAL_MATRIX[mWMGM][mWMGM], TOTAL_MATRIX[mWMGM][mCSF], predictedPixels[mWMGM]);
	fprintf(outputHandle, "# CSF\t%.0f\t%.0f\t%.0f\n", TOTAL_MATRIX[mCSF][mWMGM], TOTAL_MATRIX[mCSF][mCSF], predictedPixels[mCSF]);
	fprintf(outputHandle, "# -----------------------------------\n");
	fprintf(outputHandle, "# True\t%.0f\t%.0f\t%.0f\n", truePixels[mWMGM], truePixels[mCSF], totalNumPixels);
	fprintf(outputHandle, "# Error\t%.2f\t%.2f\t%.2f\n", missedPixels[mWMGM], missedPixels[mCSF], totalError);
	fprintf(outputHandle, "\n");
}

	freeDATATYPE3D(wmgmMasks1, actualNumSlices_wmgm, wmgmW); freeDATATYPE3D(csfMasks1, actualNumSlices_csf, csfW);
	free(wmgmFilename1); free(csfFilename1);
	if( outputFilename != NULL ){
		fclose(outputHandle);
		free(outputFilename);
	}
	freeFLOAT3D(MATRIX, numSlices, 2);
	freeFLOAT2D(TP, numSlices);
	freeFLOAT2D(TN, numSlices);
	freeFLOAT2D(FP, numSlices);
	freeFLOAT2D(FN, numSlices);
	exit(0);
}
