#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

#define	mWM 0
#define	mGM 1
#define mOTHER 2

const	char	Examples[] = "\
\n	-i input.unc -W wm.unc -G gm.unc -O other.unc -o output -a 75.0:100.0\
\n\
\n	voxels will be allocated to the WM class only if their probability\
\n	of being WM is 75 to 100 %%.\
\n";

const	char	Usage[] = "options as follows:\
\n\t -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n\t -W whiteMatterFilename\
\n	(UNC image file with one or more slices holding the WM probability maps)\
\n\
\n\t -G greyMatterFilename\
\n	(UNC image file with one or more slices holding the GM probability maps)\
\n\
\n\t -O otherMatterFilename\
\n	(UNC image file with one or more slices holding 'other' probability maps)\
\n\
\n\t[-a minPWM:maxPWM\
\n\t[-b minPGM:maxPGM\
\n\t[-c monPOTHER:maxPOTHER\
\n	(Specify percentage ranges (min-max) for each tissue type which if\
\n	 the probability of each voxel falls within this range, it will be\
\n	 included, otherwise it will be discarded.\
\n	 Note that minP and maxP are percentages - should be between 0 and 100 %%.)]\
\n\t -o outputBasename\
\n	(Three output files will be produced one for each\
\n	 of white, grey and other matter. Their names will\
\n	 be formed from the specified 'outputBasename' as follows\
\n	 wm file:    'outputBasename'_wm.unc\
\n	 gm file:    'outputBasename'_gm.unc\
\n	 other file: 'outputBasename'_other.unc\
\n	 NOTE: do not use any UNC extension for the basename,\
\n	 for example 'outputBasename' can be 'the_results' etc.)\
\n\t[-r pMin:pMax\
\n	(The range of the probabilities - e.g. pixel values of the\
\n	 images for WM, GM and Other - If you get no images and a\
\n	 lot of NaN in the results, then definetely the range of\
\n	 the probabilities are wrong. open your images and check\
\n	 their min and max intensities - then set pMin and pMax.)]\
\n\t[-S\
\n	(flag to do simple operation)]\
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
\n\t[-s sliceNumber [-s s...]]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut, ***wmProbs, ***gmProbs, ***otherProbs;
	char		*inputFilename = NULL, *outputFilename = NULL, *outputBasename = NULL,
			*wmFilename = NULL, *gmFilename = NULL, *otherFilename = NULL, copyHeaderFlag = FALSE;
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0,
			probabilitiesRangeMin = 0 , probabilitiesRangeMax = 32768;
	register int	i, j;
	int		wmW = -1, gmW = -1, otherW = -1,
			wmH = -1, gmH = -1, otherH = -1,
			actualNumSlices_wm = 0, actualNumSlices_gm = 0, actualNumSlices_other = 0;

	int		threshold,
			brainTotalArea = 0, *brainAreas,
			wmTotalArea = 0, gmTotalArea = 0, otherTotalArea = 0,
			*wmAreas, *gmAreas, *otherAreas;
	int		doSimpleFlag = FALSE;
	float		minProbabilityThreshold[3], maxProbabilityThreshold[3];

	for(i=0;i<3;i++){
		/* default probability thresholds */
		minProbabilityThreshold[i] = 0.0;
		maxProbabilityThreshold[i] = 100.0;
	}

	while( (optI=getopt(argc, argv, "i:o:es:w:h:x:y:W:G:O:Sr:9a:b:c:")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'W': wmFilename = strdup(optarg); break; /* white matter */
			case 'G': gmFilename = strdup(optarg); break; /* grey  matter */
			case 'O': otherFilename = strdup(optarg); break; /* other */
			case 'S': doSimpleFlag = TRUE; break;
			case 'a': sscanf(optarg, "%f:%f", &minProbabilityThreshold[mWM], &maxProbabilityThreshold[mWM]); break;
			case 'b': sscanf(optarg, "%f:%f", &minProbabilityThreshold[mGM], &maxProbabilityThreshold[mGM]); break;
			case 'c': sscanf(optarg, "%f:%f", &minProbabilityThreshold[mOTHER], &maxProbabilityThreshold[mOTHER]); break;
			case 'r': sscanf(optarg, "%d:%d", &probabilitiesRangeMin, &probabilitiesRangeMax); break;

			case '9': copyHeaderFlag = TRUE; break;
			
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	for(i=0;i<3;i++){
		/* convert the percentages to pixel values */
		maxProbabilityThreshold[i] *= 32767.0 / 100.0;
		minProbabilityThreshold[i] *= 32767.0 / 100.0;
	}
	
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		if( outputBasename != NULL ) free(outputBasename);
		if( wmFilename != NULL ) free(wmFilename);
		if( gmFilename != NULL ) free(gmFilename);
		if( otherFilename != NULL ) free(otherFilename);
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename);
		if( wmFilename != NULL ) free(wmFilename);
		if( gmFilename != NULL ) free(gmFilename);
		if( otherFilename != NULL ) free(otherFilename);
		exit(1);
	}
	if( wmFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename); free(outputBasename);
		if( gmFilename != NULL ) free(gmFilename);
		if( otherFilename != NULL ) free(otherFilename);
		exit(1);
	}
	if( gmFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename); free(outputBasename); free(wmFilename);
		if( otherFilename != NULL ) free(otherFilename);
		exit(1);
	}
	if( otherFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(inputFilename); free(outputBasename); free(wmFilename); free(gmFilename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, W, inputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, H, inputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (wmProbs=getUNCSlices3D(wmFilename, 0, 0, &wmW, &wmH, NULL, &actualNumSlices_wm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		exit(1);
	}
	if( (x+w) > wmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, wmW, wmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
		exit(1);
	}
	if( (y+h) > wmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, wmH, wmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_wm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], wmFilename, actualNumSlices_wm, inputFilename, actualNumSlices);
			free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
			exit(1);
		}
	}
	if( (gmProbs=getUNCSlices3D(gmFilename, 0, 0, &gmW, &gmH, NULL, &actualNumSlices_gm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], gmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); 
		exit(1);
	}
	if( (x+w) > gmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, gmW, gmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW);
		exit(1);
	}
	if( (y+h) > gmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, gmH, gmFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_gm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], gmFilename, actualNumSlices_gm, inputFilename, actualNumSlices);
			free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW);
			exit(1);
		}
	}
	if( (otherProbs=getUNCSlices3D(otherFilename, 0, 0, &otherW, &otherH, NULL, &actualNumSlices_other, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], otherFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW);
		exit(1);
	}
	if( (x+w) > otherW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, otherW, otherFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}
	if( (y+h) > otherH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, otherH, otherFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_other ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], otherFilename, actualNumSlices_other, inputFilename, actualNumSlices);
			free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
			freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
			exit(1);
		}
	}
	/* finished with validations */

	if( (dataOut=callocDATATYPE3D(numSlices, W, H)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for output data.\n", argv[0], numSlices * W * H * sizeof(DATATYPE));
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}

	if( (outputFilename=(char *)malloc(strlen(outputBasename)+50)) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for filename.\n", argv[0], strlen(outputBasename)+50);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}		

	if( (wmAreas=(int *)malloc(numSlices*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for wmAreas.\n", argv[0], numSlices*sizeof(int));
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}		
	if( (gmAreas=(int *)malloc(numSlices*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for gmAreas.\n", argv[0], numSlices*sizeof(int));
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}		
	if( (otherAreas=(int *)malloc(numSlices*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for otherAreas.\n", argv[0], numSlices*sizeof(int));
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}		
	if( (brainAreas=(int *)malloc(numSlices*sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %zd bytes for brainAreas.\n", argv[0], numSlices*sizeof(int));
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas); free(otherAreas); 
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}		

	for(s=0;s<numSlices;s++) wmAreas[s] = gmAreas[s] = otherAreas[s] = brainAreas[s] = 0;

	printf("%s, doing white matter segmentation, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<W;i++) for(j=0;j<H;j++) dataOut[s][i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doSimpleFlag ) threshold = 0;
			else threshold = probabilitiesRangeMax - wmProbs[slice][i][j] - gmProbs[slice][i][j] - otherProbs[slice][i][j];
			if( (wmProbs[slice][i][j] < minProbabilityThreshold[mWM]) || (wmProbs[slice][i][j] >= maxProbabilityThreshold[mWM]) ) continue;
			if( (wmProbs[slice][i][j] > gmProbs[slice][i][j])    &&
			    (wmProbs[slice][i][j] > otherProbs[slice][i][j]) &&
			    (wmProbs[slice][i][j] > threshold) ){
				dataOut[s][i][j] = data[slice][i][j];
				wmTotalArea++; brainTotalArea++;
				wmAreas[s]++;
				brainAreas[s]++;
			    }
		}
	}
	sprintf(outputFilename, "%s_wm.unc", outputBasename);
	printf(" (result in file '%s')\n", outputFilename);
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas); free(otherAreas); free(brainAreas);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}

	printf("%s, doing grey matter segmentation, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<W;i++) for(j=0;j<H;j++) dataOut[s][i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doSimpleFlag ) threshold = 0;
			else threshold = probabilitiesRangeMax - wmProbs[slice][i][j] - gmProbs[slice][i][j] - otherProbs[slice][i][j];
			if( (gmProbs[slice][i][j] < minProbabilityThreshold[mGM]) || (gmProbs[slice][i][j] >= maxProbabilityThreshold[mGM]) ) continue;
			if( (gmProbs[slice][i][j] > wmProbs[slice][i][j])    &&
			    (gmProbs[slice][i][j] > otherProbs[slice][i][j]) &&
			    (gmProbs[slice][i][j] > threshold) ){
			    	dataOut[s][i][j] = data[slice][i][j];
				dataOut[s][i][j] = data[slice][i][j];
				gmTotalArea++; brainTotalArea++;
				gmAreas[s]++;
				brainAreas[s]++;
			    }
		}
	}
	sprintf(outputFilename, "%s_gm.unc", outputBasename);
	printf(" (result in file '%s')\n", outputFilename);
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas); free(otherAreas); free(brainAreas);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}

	printf("%s, doing 'other' segmentation, slice : ", inputFilename); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);
		for(i=0;i<W;i++) for(j=0;j<H;j++) dataOut[s][i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( doSimpleFlag ) threshold = 0;
			else threshold = probabilitiesRangeMax - wmProbs[slice][i][j] - gmProbs[slice][i][j] - otherProbs[slice][i][j];
			if( (otherProbs[slice][i][j] < minProbabilityThreshold[mOTHER]) || (otherProbs[slice][i][j] >= maxProbabilityThreshold[mOTHER]) ) continue;
			if( (otherProbs[slice][i][j] > wmProbs[slice][i][j]) &&
			    (otherProbs[slice][i][j] > gmProbs[slice][i][j]) &&
			    (otherProbs[slice][i][j] > threshold) ){
			    	dataOut[s][i][j] = data[slice][i][j];
				otherTotalArea++; brainTotalArea++;
				otherAreas[s]++;
				brainAreas[s]++;
			    }
		}
	}

	sprintf(outputFilename, "%s_other.unc", outputBasename);
	printf(" (result in file '%s')\n", outputFilename);
	if( !writeUNCSlices3D(outputFilename, dataOut, W, H, 0, 0, W, H, NULL, numSlices, format, OVERWRITE) ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
		free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas); free(otherAreas); free(brainAreas);
		freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
		exit(1);
	}
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(inputFilename); free(outputFilename);
		exit(1);
	}


	printf("WM:\tnumber of white matter pixels in a particular slice\n");
	printf("GM:\tnumber of grey  matter pixels in a particular slice\n");
	printf("OM:\tnumber of 'other' (e.g. not background and not GM or WM) matter pixels in a particular slice\n");
	printf("TB:\tsum of WM + GM + OM for a particular slice\n");
	printf("'total' figures refer to the sum of each quantity over the range of *selected* slices.\n\n");
	printf("\nPixel counts and calculations follow:\n");

	printf("  Slice\tWM (%%WM/TB)\tGM (%%GM/TB)\tOM (%%OM/TB)\tTB\t%%WM/GM\t%%WM/OM\t%%GM/OM\n--------------------------------------------------------------------------------------\n");
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("  %d\t%d(%.2f)\t%d(%.2f)\t%d(%.2f)\t%d\t%.2f\t%.2f\t%.2f\n",
			slice+1,
			wmAreas[s], 100.0 * (((float )wmAreas[s]) / ((float )brainAreas[s])),
			gmAreas[s], 100.0 * (((float )gmAreas[s]) / ((float )brainAreas[s])),
			otherAreas[s], 100.0 * (((float )otherAreas[s]) / ((float )brainAreas[s])),
			brainAreas[s],
			100.0 * (((float )wmAreas[s]) / ((float )gmAreas[s])),
			100.0 * (((float )wmAreas[s]) / ((float )otherAreas[s])),
			100.0 * (((float )gmAreas[s]) / ((float )otherAreas[s]))
		);
	}

	if( numSlices > 1 ){
		printf("--------------------------------------------------------------------------------------\n");
		printf(" total\t%d(%.2f)\t%d(%.2f)\t%d(%.2f)\t%d\t%.2f\t%.2f\t%.2f\n",
			wmTotalArea, 100.0 * (((float )wmTotalArea) / ((float )brainTotalArea)),
			gmTotalArea, 100.0 * (((float )gmTotalArea) / ((float )brainTotalArea)),
			otherTotalArea, 100.0 * (((float )otherTotalArea) / ((float )brainTotalArea)),
			brainTotalArea,
			100.0 * (((float )wmTotalArea) / ((float )gmTotalArea)),
			100.0 * (((float )wmTotalArea) / ((float )otherTotalArea)),
			100.0 * (((float )gmTotalArea) / ((float )otherTotalArea))
		);
		printf("\n");
	}

	if( numSlices != actualNumSlices )
		printf("*** Warning Warning Warning Warning Warning ***\nTotal pixel count figures, above, refer to the range of *selected* slices and not to *all* the slices of the image files.\n");

	freeDATATYPE3D(data, actualNumSlices, W); freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(otherProbs, actualNumSlices_other, otherW);
	free(inputFilename); free(wmFilename); free(otherFilename); free(gmFilename); free(outputBasename); free(outputFilename); free(wmAreas); free(gmAreas); free(otherAreas); free(brainAreas);
	exit(0);
}
