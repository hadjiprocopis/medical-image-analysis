#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Examples[] = "\
\n	-i input.unc -W wm.unc -G gm.unc -O other.unc -o output\
\n";

const	char	Usage[] = "options as follows:\
\n\t -W whiteMatterFilename\
\n	(UNC image file with one or more slices containing WM probability maps)\
\n\
\n\t -G greyMatterFilename\
\n	(UNC image file with one or more slices containing GM probability maps)\
\n\
\n\t -C csfFilename\
\n	(UNC image file with one or more slices containing CSF probability maps)\
\n\
\n\t -o outputFilename\
\n	(Filename of the output UNC file)\
\n\
\n\t[-t\
\n	(produce a TIFF file rather than a JPEG which is the default.)]\
\n\t[-q Q\
\n	(Q is an integer from 1 to 100 representing the quality of the JPEG,\
\n	 more quality means less compression means bigger file size.\
\n	 The default is quality of 100, the maximum)]	\
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
\nThis program will take the probability maps for WM, GM and CSF\
\nwhich are the results of tissue segmentation (by SPM or other)\
\nand will produce a JPEG or TIFF file where each color (RGB) represents:\
\nR -> prob of WM\
\nG -> prob of GM\
\nB -> prob of CSF";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	DATATYPE	***wmProbs, ***gmProbs, ***csfProbs, **R, **G, **B;
	char		outputFilename[2000], *outputBasename = NULL,
			*wmFilename = NULL, *gmFilename = NULL, *csfFilename = NULL, copyHeaderFlag = FALSE,
			extension[] = {"jpg"};
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, s, slice, actualNumSlices = 0,
			optI, slices[1000], allSlices = 0;
	register int	i, j;
	int		wmW = -1, gmW = -1, csfW = -1,
			wmH = -1, gmH = -1, csfH = -1,
			actualNumSlices_wm = 0, actualNumSlices_gm = 0, actualNumSlices_csf = 0,
			produceTIFF = FALSE, quality = 100;
	double		sum;

	while( (optI=getopt(argc, argv, "o:es:w:h:x:y:W:G:C:aSr:9tq:")) != EOF)
		switch( optI ){
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
			case 'C': csfFilename = strdup(optarg); break; /* csf */
			case 't': produceTIFF = TRUE; strcpy(extension, "tif"); break;
			case 'q': quality = atoi(optarg); break;

			case '9': copyHeaderFlag = TRUE; break;
			
			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		if( wmFilename != NULL ) free(wmFilename);
		if( gmFilename != NULL ) free(gmFilename);
		if( csfFilename != NULL ) free(csfFilename);
		exit(1);
	}
	if( wmFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(outputBasename);
		if( gmFilename != NULL ) free(gmFilename);
		if( csfFilename != NULL ) free(csfFilename);
		exit(1);
	}
	if( gmFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(outputBasename); free(wmFilename);
		if( csfFilename != NULL ) free(csfFilename);
		exit(1);
	}
	if( csfFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		free(outputBasename); free(wmFilename); free(gmFilename);
		exit(1);
	}
	if( (gmProbs=getUNCSlices3D(gmFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], gmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		exit(1);
	}
	gmW = W; gmH = H; actualNumSlices_gm = actualNumSlices;
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(s=0;s<numSlices;s++){
			if( slices[s] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, gmFilename);
				free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
				exit(1);
			} else if( slices[s] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
				exit(1);
			}
		}
	}
	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, W, gmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, H, gmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		
		exit(1);
	}
	if( (wmProbs=getUNCSlices3D(wmFilename, 0, 0, &wmW, &wmH, NULL, &actualNumSlices_wm, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], wmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		exit(1);
	}
	if( (x+w) > wmW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, wmW, wmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
		exit(1);
	}
	if( (y+h) > wmH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, wmH, wmFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_wm ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], wmFilename, actualNumSlices_wm, gmFilename, actualNumSlices);
			free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
			freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW);
			exit(1);
		}
	}
	if( (csfProbs=getUNCSlices3D(csfFilename, 0, 0, &csfW, &csfH, NULL, &actualNumSlices_csf, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], csfFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW);
		exit(1);
	}
	if( (x+w) > csfW ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d + %d > %d) for image in '%s'.\n", argv[0], x, w, csfW, csfFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
		exit(1);
	}
	if( (y+h) > csfH ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d + %d > %d) for image in '%s'.\n", argv[0], y, h, csfH, csfFilename);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
		exit(1);
	}
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		if( slice > actualNumSlices_csf ){
			fprintf(stderr, "%s : image in file '%s' has less slices (%d) than input image in '%s' (%d).\n", argv[0], csfFilename, actualNumSlices_csf, gmFilename, actualNumSlices);
			free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
			freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
			exit(1);
		}
	}
	/* finished with validations */
	if( (R=callocDATATYPE2D(W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE2D has failed for %d x %d\n", argv[0], W, H);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
		exit(1);
	}
	if( (G=callocDATATYPE2D(W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE2D has failed for %d x %d\n", argv[0], W, H);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
		exit(1);
	}
	if( (B=callocDATATYPE2D(W, H)) == NULL ){
		fprintf(stderr, "%s : call to callocDATATYPE2D has failed for %d x %d\n", argv[0], W, H);
		free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
		freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
		exit(1);
	}
		
	printf("%s, doing slice : ", argv[0]); fflush(stdout);
	for(s=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf("%d ", slice+1); fflush(stdout);

		for(i=0;i<W;i++) for(j=0;j<H;j++) R[i][j] = G[i][j] = B[i][j] = 0;
		for(i=x;i<x+w;i++) for(j=y;j<y+h;j++){
			if( (sum=wmProbs[slice][i][j] + gmProbs[slice][i][j] + csfProbs[slice][i][j]) == 0 ) continue;
			R[i][j] = (DATATYPE )ROUND(wmProbs[slice][i][j]  * 256.0 / sum);
			G[i][j] = (DATATYPE )ROUND(gmProbs[slice][i][j]  * 256.0 / sum);
			B[i][j] = (DATATYPE )ROUND(csfProbs[slice][i][j] * 256.0 / sum);
		}
		if( numSlices > 1 ){
			if( slice < 9 ) sprintf(outputFilename, "%s_00%d.%s", outputBasename, s+1, extension);
			else if( slice < 99 ) sprintf(outputFilename, "%s_0%d.%s", outputBasename, s+1, extension);
			else sprintf(outputFilename, "%s_%d.%s", outputBasename, s+1, extension);
		} else sprintf(outputFilename, "%s.%s", outputBasename, extension);

		if( produceTIFF ){
			if( write_RGB_to_RGB_tiff_file(R, G, B, W, H, outputFilename) == FALSE ){
				fprintf(stderr, "%s : call to write_RGB_to_RGB_tiff_file has failed for output file '%s' (image dims %d x %d).\n", argv[0], outputFilename, W, H);
				free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
				freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
				freeDATATYPE2D(R, W); freeDATATYPE2D(G, W); freeDATATYPE2D(B, W);
				exit(1);
			}
		} else {
			if( write_RGB_to_RGB_jpeg_file(R, G, B, W, H, quality, outputFilename) == FALSE ){
				fprintf(stderr, "%s : call to write_RGB_to_RGB_tiff_file has failed for output file '%s' (image dims %d x %d).\n", argv[0], outputFilename, W, H);
				free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
				freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
				freeDATATYPE2D(R, W); freeDATATYPE2D(G, W); freeDATATYPE2D(B, W);
				exit(1);
			}
		}
	}
	printf("\n");

	freeDATATYPE3D(wmProbs, actualNumSlices_wm, wmW); freeDATATYPE3D(gmProbs, actualNumSlices_gm, gmW); freeDATATYPE3D(csfProbs, actualNumSlices_csf, csfW);
	free(wmFilename); free(csfFilename); free(gmFilename); free(outputBasename);
	freeDATATYPE2D(R, W);
	freeDATATYPE2D(G, W);
	freeDATATYPE2D(B, W);
	exit(0);
}
