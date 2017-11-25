#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* in math.h */
/* M_SQRT1_2 = sqrt(2) / 2 */
/* M_SQRT2   = sqrt(2) */

/* sqrt(2*M_PI) */
#define	SQRT_2PI	2.50662827463100050241
/* 2*sqrt(2*M_PI)/(2-sqrt(2)) */
#define	SCALE1		8.55816425107303305938
/* sqrt(2) / (2-sqrt(2)) */
#define	SCALE2		2.41421356237309504879

double sigmoid(double Ymin, double Ymax, double C1, double stdev, double xoff, double x);

#define SIGMOID2(_A, _B, _mean, _minPixel, _X) ((_A)*(TANH((_B)*((_X)-(_mean)))+1)+(_minPixel))

#define	DEFAULT_ITERATIONS	100
#define	DEFAULT_SENSITIVITY	56.5
/* the constant of proportionality in Slope = C1 / stdev */
#define	DEFAULT_C1		1.0

const	char	Usage[] = "options as follows:\n\t -i inputFilename\n\t -o outputFilename\n\t -w windowWidth\n\t -h windowHeight\n\t -s sensitivity\n\t -M meanOfInterest\n\t[-S stdevOfInterest]\n\t[-m minPixel]\n\t[-x maxPixel]\n\t[-n iterations]\n\t[-r saveResultsEveryNIerations]\n\t[-v (verbose flag)]\n\t[-c C1]";
const	char	Author[] = "A.Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***dataIn, ***dataOut1, ***dataOut2,
			***pFrom, ***pTo, ***pSwap, minPixel, maxPixel;
	int		w = -1, h = -1, numSlices = -1, depth, format;
	char		inputFilename[1000], outputFilename[1000], AppName[1000],
			dummyS1[1000], dummyS2[1000], copyHeaderFlag = FALSE;
 	double		mean, stdev, diff;
	register int	i, j, s, iters;

	int	optI, numIterations = DEFAULT_ITERATIONS,
		windowWidth = -1, windowHeight = -1, verbose = 0,
		windowWidthx2 = -1, windowHeightx2 = -1,
		saveResultsEvery = 0,
		*oldMinPixel, *oldMaxPixel, minPixelRange=0, maxPixelRange=256*256-1;
	double	*oldMean, *oldStdev, C1 = DEFAULT_C1, sensitivity = DEFAULT_SENSITIVITY,
		meanOfInterest = 500.0, stdevOfInterest = 50.0, cutoff,
		lowColor, hiColor;
		
	strcpy(AppName, argv[0]);	


	while( (optI=getopt(argc, argv, "i:o:w:h:s:m:x:n:c:r:vM:S:u9")) != EOF)
		switch( optI ){
			case 'i': strcpy(inputFilename, optarg); break;
			case 'o': strcpy(outputFilename, optarg); break;
			case 'w': windowWidth = atoi(optarg);
				  windowWidthx2 = 2 * windowWidth + 1; break;
			case 'h': windowHeight = atoi(optarg);
				  windowHeightx2 = 2 * windowHeight + 1; break;
			case 's': sensitivity = atof(optarg); break;
			case 'm': minPixelRange = atoi(optarg); break;
			case 'x': maxPixelRange = atoi(optarg); break;
			case 'u': fprintf(stderr, "Usage : %s %s\n", AppName, Usage);
				  fprintf(stderr, "\n%s\n", Author);
				  exit(0);
			case 'n': numIterations = atoi(optarg); break;
			case 'r': saveResultsEvery = atoi(optarg); break;
			case 'c': C1 = atof(optarg); break;
			case 'v': verbose = 1; break;
			case 'M': meanOfInterest = atof(optarg); break;
			case 'S': stdevOfInterest = atof(optarg); break;

			case '9': copyHeaderFlag = TRUE; break;

			default:
				fprintf(stderr, "Usage : %s %s\n", AppName, Usage);
				fprintf(stderr, "Unknown option '-%c'.\n", optI);
				exit(1);
		}
	if( (inputFilename[0]=='\0') || (outputFilename[0]=='\0') ||
	    (windowWidth<=0) || (windowHeight<=0) || (minPixelRange>maxPixelRange) ||
	    (numIterations<=0) || (stdevOfInterest<=0.0) ){
	    	fprintf(stderr, "Usage : %s %s\n", AppName, Usage);
	    	exit(1);
	    }
	cutoff = M_SQRT1_2 / (stdevOfInterest * SQRT_2PI);
	lowColor = (meanOfInterest - stdevOfInterest);
	hiColor = (meanOfInterest + stdevOfInterest);

	if( (dataIn=getUNCSlices3D(inputFilename, 0, 0, &w, &h, NULL, &numSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices has failed for file '%s'.\n", AppName, inputFilename);
		exit(1);
	}
	if( (dataOut1=callocDATATYPE3D(numSlices, w, h)) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d x %d DATATYPEs of size %zd bytes each.\n", AppName, numSlices, w, h, sizeof(DATATYPE));
		freeDATATYPE3D(dataIn, numSlices, w);
		exit(1);
	}
	if( (dataOut2=callocDATATYPE3D(numSlices, w, h)) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d x %d DATATYPEs of size %zd bytes each.\n", AppName, numSlices, w, h, sizeof(DATATYPE));
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
  		exit(1);
	}
	if( (oldMean=(double *)calloc(numSlices, sizeof(double))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d doubles.\n", AppName, numSlices);
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
		freeDATATYPE3D(dataOut2, numSlices, w);
 		exit(1);
	}
	if( (oldStdev=(double *)calloc(numSlices, sizeof(double))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d doubles.\n", AppName, numSlices);
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
		freeDATATYPE3D(dataOut2, numSlices, w);
		free(oldMean);
		exit(1);
	}
	if( (oldMinPixel=(int *)calloc(numSlices, sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d integers.\n", AppName, numSlices);
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
		freeDATATYPE3D(dataOut2, numSlices, w);
		free(oldMean); free(oldStdev);
		exit(1);
	}
	if( (oldMaxPixel=(int *)calloc(numSlices, sizeof(int))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d integers.\n", AppName, numSlices);
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
		freeDATATYPE3D(dataOut2, numSlices, w);
		free(oldMean); free(oldStdev); free(oldMinPixel);
		exit(1);
	}
	for(s=0;s<numSlices;s++) oldMean[s] = oldStdev[s] = oldMinPixel[s] = oldMaxPixel[2] = 0.0;
		
	fprintf(stderr, "%s : Read image from '%s' %dx%d slices %d dep=%d for=%d\n", AppName, inputFilename, w, h, numSlices, depth, format);

/*
	       (Ymax - Ymin)           2.0 * Sensitivity
	f(x) =	----------- . ( tanh(---------------------- . (X - mean)) + 1)
		     2.0               (Ymax-Ymin)*stdev
*/
//	min_maxPixel2D(dataIn[s], 0, 0, w, h, &minPixel, &maxPixel);
//	printf("min pixel: %d, max pixel : %d\n", minPixel, maxPixel);
	pFrom = dataIn;
	pTo = dataOut1;
	for(iters=0;iters<numIterations;iters++){
		for(s=0;s<numSlices;s++){
			for(i=windowWidth;i<w-windowWidth;i++){
				for(j=windowHeight;j<h-windowHeight;j++){
					statistics2D(pFrom[s], i-windowWidth, j-windowHeight,windowWidthx2, windowHeightx2,&minPixel, &maxPixel, &mean, &stdev);
//lowColor = minPixel; hiColor = maxPixel;
//					gaussian = GAUSSIAN(mean, (stdev==0.0?0.5:stdev), pFrom[s][i][j]);
//					printf("FROM %d (gau=%lf, cut=%lf)\n", pFrom[s][i][j], gaussian, cutoff);
//					if( gaussian <= cutoff){
//					if( (pFrom[s][i][j]>lowColor) && (pFrom[s][i][j]<hiColor) ){

diff = SIGMOID(-sensitivity, sensitivity, 2.0*C1/(stdev==0?0.1:stdev), mean, pFrom[s][i][j]);
pTo[s][i][j] = pFrom[s][i][j] + (int )(MAX(0.0, diff));
//printf("%d %d mean=%.2lf stdev=%.2lf\n", minPixel, maxPixel, mean, stdev);
//printf("%d + %lf = %d\n", pFrom[s][i][j], diff, pTo[s][i][j]);

						//diff = pFrom[s][i][j]+ SIGMOID2(1.0, (0.5/(sensitivity*(stdev==0?0.1:stdev))), mean, 0, pFrom[s][i][j]);

//						printf("TO %d\n", pTo[s][i][j]);
//					} else {
//						pTo[s][i][j] = pFrom[s][i][j];
//					}



//					if( gaussian < cutoff ) continue;
//	diff = SIGMOID(lowColor, hiColor, C1, (stdev==0?0.1:stdev), mean, pFrom[s][i][j]);
//	diff = SIGMOID2(1.0, (0.5/(sensitivity*(stdev==0?0.1:stdev))), mean, 0, pFrom[s][i][j]);
//	pTo[s][i][j] = pFrom[s][i][j] + (int )diff;
//	printf("%lf %lf %d mean=%.2lf stdev=%.2lf diff=%lf -> %d\n",
//lowColor, hiColor, pFrom[s][i][j], mean, stdev, diff, pTo[s][i][j]);
				}
			}
		}
		/* print out some stats */
		for(s=0;verbose&&(s<numSlices);s++){
			statistics2D(pTo[s], windowWidth, windowHeight, w-windowWidthx2, h-windowHeightx2, &minPixel, &maxPixel, &mean, &stdev);
			printf("iter %d, slice %d:\n  pixel range:  %d %d (was %d %d, diff %d %d)\n  mean:\t\t%f (was %f, diff %f)\n  stdev:\t%f (was %f, diff %f)\n",
				iters, s, minPixel, maxPixel,
				oldMinPixel[s], oldMaxPixel[s],
				minPixel-oldMinPixel[s],
				maxPixel-oldMaxPixel[s], mean, oldMean[s],
				mean-oldMean[s], stdev, oldStdev[s],
				stdev-oldStdev[s]);
			oldMinPixel[s] = minPixel; oldMaxPixel[s] = maxPixel;
			oldMean[s] = mean; oldStdev[s] = stdev;
		}
		if( (saveResultsEvery>0) && ((iters % saveResultsEvery) == 0) && (iters>=saveResultsEvery) ){
			strcpy(dummyS1, outputFilename);
			dummyS1[strlen(dummyS1)-4] = '\0';
			sprintf(dummyS2, "%s_%06d.unc", dummyS1, iters);
			writeUNCSlices3D(dummyS2, pTo, w, h, 0, 0, w, h, NULL, numSlices, format, CREATE);
		}	
		pSwap = pFrom; pFrom = pTo; pTo = pSwap;
	}

	if( writeUNCSlices3D(outputFilename, pTo, w, h, 0, 0, w, h, NULL, numSlices, format, CREATE) == INVALID ){
		fprintf(stderr, "%s : call to writeUNCSlices3D has failed.\n", AppName);
		freeDATATYPE3D(dataIn, numSlices, w);
		freeDATATYPE3D(dataOut1, numSlices, w);
		freeDATATYPE3D(dataOut2, numSlices, w);
		free(oldMean); free(oldStdev); free(oldMinPixel); free(oldMaxPixel);
		exit(1);
	}
	freeDATATYPE3D(dataIn, numSlices, w);
	freeDATATYPE3D(dataOut1, numSlices, w);
	freeDATATYPE3D(dataOut2, numSlices, w);
	/* now copy the image info/title/header of source to destination */
	if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
		fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
		free(oldMean); free(oldStdev); free(oldMinPixel); free(oldMaxPixel);
		exit(1);
	}
	free(oldMean); free(oldStdev); free(oldMinPixel); free(oldMaxPixel);
	exit(0);
}
