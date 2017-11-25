 #include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

const	char	Usage[] = "options as follows ([] means optional):\
\n\
\n\t -i inputFilename (UNC format)\
\n\t -o outputFilename (UNC format)\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\t[-j (this help message)]\
\n\t[-9\
\n        (tell the program to copy the header/title information\
\n        from the input file to the output files. If there is\
\n        more than 1 input file, then the information is copied\
\n        from the first file.)]\
\n\
\n\t[-v (verbose flag, it says what it does)]\
\n\t[-k (the output will be a binary file ->\
\n		(minOutputPixel:maxOutputPixel - see -r option)\
\n		regions with low color mean that\
\n		the image pixels did not meet the\
\n		criteria imposed. E.g. if you were\
\n		interested in pixels whose frequency\
\n		of occurence was between so and so.)]\
\n\t[-n (flag to include original image slice\
\n               at the BEGINING of all the maps for the\
\n               slice)]\
\n\t[-l (flag to indicate that when a ROI (region of\
\n               interest) is specified by -x, -y, -w\
\n               and -h the image outside the ROI will\
\n               be shown as it was. In the absence of\
\n               this flag, the image outside the ROI\
\n               is black. The image outside the ROI\
\n               can be removed by specifying the '-g'\
\n               option. In this case, the saved UNC\
\n               file will have the dimensions of the ROI.\
\n               When the '-l' option is present, the '-g'\
\n               option is AUTOMATICALLY present too.)]\
\n\t[-g (flag to indicate that in the case a ROI has been\
\n               specified, the saved file will have the\
\n               same dimensions as the original image.\
\n               The image falling outside the ROI will\
\n               be black or in the presence of the '-l'\
\n               option, the same as the original image.)]\
\n\t[-z n   (flag to indicate that in the case a ROI has been\
\n               specified and the image outside ROI is not\
\n               to be removed, a border of n pixels is to be\
\n               drawn around the ROI with the maximum pixel\
\n               intensity of the image.)]\
\n** at least one of the options below must be specified.\
\n   You can specify as many maps of the same type as you\
\n   want (e.g. -f 10:20 -f 20:30)\
\n\
\n\t[-r min:max (scale output map pixel values to be between\
\n               min and max - in case they are too small,\
\n               etc. If this option is not used DEFAULT\
\n               values will be used, so if you can not\
\n               see anything adjust this value. Also note\
\n               that in case a ROI has been specified and\
\n               the image outside ROI should be present,\
\n               it is a good idea NOT to use this option\
\n               so that the min and max pixel intensities\
\n               of the ROI are the same as those for the\
\n               whole image.)]\
\n\
\n** for the options below, a window size is calculated from\
\n   specified w and h with (2*w+1, 2*h+1) e.g. w is half the\
\n   window width plus the center pixel of the window\
\n\
\n** Mean of Pixel Values :\
\n\t[-m w:h[:lo:hi]\
\n              (mean pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel.)]\
\n\t[-M w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel\
\n               and the mean pixel value of the region\
\n               of interest)]\
\n\t[-b w:h:W:H[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel\
\n	       and a larger window's with size (2*W+1 x 2*H+1)\
\n	       centered around each pixel.)]\
\n\t[-B w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel\
\n               and that pixel's value.)]\
\n\t[-d lo:hi\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               pixel value of the region of interest and\
\n               each pixel's value. Display only\
\n               values between lo and hi.)]\
\n\
\n** Min/Max of Pixel Values:\
\n\t[-X w:h[:lo:hi]\
\n		(The MAX pixel value of a window of size\
\n		 (2*w+1 x 2*h+1) centered around each pixel\
\n		 - optionally within the range lo:hi)]\
\n\t[-I w:h[:lo:hi]\
\n		(The MIN pixel value of a window of size\
\n		 (2*w+1 x 2*h+1) centered around each pixel\
\n		 - optionally within the range lo:hi)]\
\n\
\n** Stdev of Pixel Values :\
\n\t[-t w:h[:lo:hi]\
\n              (stdev of the pixel values within a window of\
\n               size (2*w+1 x 2*h+1) centered around each pixel.)]\
\n\t[-T w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the stdev of the\
\n               pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel\
\n               and the stdev of pixel values of the\
\n               region of interest.)]\
\n\t[-e w:h:W:H[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the stdev of\
\n               pixel values within a window of size\
\n               (2*w+1 x 2*h+1) centered around each pixel\
\n	       and a larger window's with size (2*W+1 x 2*H+1)\
\n	       centered around each pixel.)]\
\n\
\n** Mean frequency of occurence of pixel values\
\n\t[-f lo:hi   (frequency of occurence of each pixel value,\
\n               display only values between lo(inclusive)\
\n               and hi(exclusive). If you want a single\
\n               frequency, say 10, do -f 10:11)]\
\n\t[-F lo:hi   (ABSOLUTE DIFFERENCE between\
\n               frequency of occurence of each pixel value,\
\n               and mean frequency of occurence in ROI.\
\n               Display only values between lo(inclusive)\
\n               and hi(exclusive). If you want a single\
\n               frequency, say 10, do -f 10:11)]\
\n\t[-q w:h:W:H[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean frequency\
\n               of occurence of pixel values of a sub-window\
\n               (2*w+1 x 2*h+1) from the mean frequency of\
\n               occurence of pixel values of a bigger window\
\n               (2*H+1 x 2*H+1) centred around it)]\
\n\t[-Q w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               frequency of occurence of pixels within\
\n               a window (2*w+1 x 2*h+1) and the\
\n               frequency of occurence of a single pixel\
\n               (the one the window is centered around.)]\
\n\t[-c w:h[:lo:hi]\
\n              (display the mean frequency of occurence\
\n               of pixels within a window (2*w+1 x 2*h+1))]\
\n\t[-C w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between the mean\
\n               frequency of occurence of pixels within\
\n               a window (2*w+1 x 2*h+1) and the mean\
\n               frequency of occurence for all the pixels\
\n               of the region of interest.)]\
\n\
\n** Min/Max of Frequency of Occurence\
\n\t[-O w:h[:lo:hi]\
\n		(The MAX frequency of occurence of the pixels\
\n		 of a window of size\
\n		 (2*w+1 x 2*h+1) centered around each pixel\
\n		 - optionally within the range lo:hi)]\
\n\t[-W w:h[:lo:hi]\
\n		(The MIN frequency of occurence of the pixels\
\n		 of a window of size\
\n		 (2*w+1 x 2*h+1) centered around each pixel\
\n		 - optionally within the range lo:hi)]\
\n\
\n** Stdev of the frequency of occurence of pixel values\
\n\t[-u w:h:W:H[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between frequency of\
\n               occurence of pixel values within a\
\n               window of size (2*w+1 x 2*h+1) and\
\n               a larger window's of size (2*W+1 x 2*H+1),\
\n               both centered around each pixel)]\
\n\t[-U w:h[:lo:hi]\
\n              (ABSOLUTE DIFFERENCE between stdev of\
\n               frequency of occurence of pixel values\
\n               within a window of size (2*w+1 x 2*h+1)\
\n               and the stdev of frequency of occurence\
\n               of pixel values within the region of\
\n               interest.)]\
\n\
\n** this is a shortcut for having all of the above options. Note\
\n   that when this option is specified, any other maps specified\
\n   will be ignored\
\n\
\n\t[-a w:h:W:H:lo:hi\
\n              (ALL of the above in this help file's order with\
\n                     w, h, W, H, lo and hi IN COMMON)]";

const	char	ShortUsage[] = "options as follows ([] means optional):\
\n\t -i inputFilename (UNC format)\
\n\t -o outputFilename (UNC format)\
\n\t[-w widthOfInterest]\
\n\t[-h heightOfInterest]\
\n\t[-x xCoordOfInterest]\
\n\t[-y yCoordOfInterest]\
\n\t[-s sliceNumber [-s s...]]\
\n\t[-v (verbose)]\
\n\t[-k (output binary image, 1 means satisfies criteria imposed, 0 otherwise)]\
\n\t[-n (include original image)]\
\n\t[-l (show image with ROI, makes -g=TRUE)]\
\n\t[-g (with ROI, size of output image = size of input)]\
\n\t[-z width (draw small border around ROI of width 'width')]\
\n\t[-r min:max (output color range, do not use with ROI)]\
\n\
\n** ONE or MORE of the options below must be specified.\
\n** small window size (2*w+1, 2*h+1), large (2*W+1, 2*H+1)\
\n\
\n** pixel values (PV), mean pixel values (MPV)\
\n\t[-m w:h[:lo:hi] (MPV of sub-window)]\
\n\t[-M w:h[:lo:hi] (MPV of sub-window - MPV of whole image)]\
\n\t[-b w:h:W:H[:lo:hi] (MPV of wxh - MPV of WxH)]\
\n\t[-B w:h[:lo:hi] (MPV of wxh - MPV of each pixel)]\
\n\t[-d lo:hi (PV for each pixel - MPV of whole image)]\
\n** min/max pixel values\
\n\t[-X w:h[:lo:hi] (MAX PV of sub-window)]\
\n\t[-I w:h[:lo:hi] (MIN PV of sub-window)]\
\n** pixel values (PV), stdev of pixel values (SPV)\
\n\t[-t w:h[:lo:hi] (SPV of sub-window wxh)]\
\n\t[-T w:h[:lo:hi] (SPV of wxh - SPV of whole image)]\
\n\t[-e w:h:W:H[:lo:hi] (SPV of wxh - SPV of WxH)]\
\n\
\n** Frequency of Occurence (FOC), mean FOC (MFOC)\
\n\t[-f lo:hi (FOC (lo,hi] for each pixel)]\
\n\t[-F lo:hi (FOC for each pixel - whole images MFOC (lo,hi])]\
\n\t[-q w:h:W:H[:lo:hi] (MFOC of wxh - MFOC of WxH)]\
\n\t[-Q w:h[:lo:hi] (MFOC of wxh - each pixel's FOC)]\
\n\t[-c w:h[:lo:hi] (MFOC of sub-window wxh)]\
\n\t[-C w:h[:lo:hi] (MFOC of wxh - whole image's MFOC)]\
\n** min/max Frequency of Occurence\
\n\t[-O w:h[:lo:hi] (MAX of wxh - each pixel's FOC)]\
\n\t[-W w:h[:lo:hi] (MIN of wxh - each pixel's FOC)]\
\n** Stdev of Frequency of Occurence (SFOC)\
\n\t[-u w:h:W:H[:lo:hi] (SFOC of wxh - SFOC of WxH)]\
\n\t[-U w:h[:lo:hi] (SFOC of wxh - SFOC of whole image)]\
\n\
\n\
\n** shortcut to include all of the above, use it as many times\
\n\t[-a w:h:W:H:lo:hi (options apply to all)]\
\n\
\n\t[-j (SHOW LONGER HELP PAGE WITH MORE DETAIL)]";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

/* the min and max output colors, numbers will be scaled to fit this range */
#define	DEFAULT_MIN_OUTPUT_COLOR	0
#define	DEFAULT_MAX_OUTPUT_COLOR	1000

/* if you are adding any more map types then add at the end its LETTER as in -'letter'
   and enum it equal to the last integer plus 1,
   for example, if -x invokes the new map type and 10 is the last integer in the enum
   then add ', X=11' at the end of this enum statement.
   *** DO NOT FORGET **** to increment the #define NUM_MAP_TYPES to plus 1 */
enum	{A=0, M=1, M_CAPS=2, B=3, B_CAPS=4, D=5, T=6, T_CAPS=7, E=8, F=9, F_CAPS=10,
	 Q=11, Q_CAPS=12, C=13, C_CAPS=14, U=15, U_CAPS=16,
	 X_CAPS=17, N_CAPS=18, I_CAPS=19, O_CAPS=20, W_CAPS=21 };

/* how many different maps we have ? e.g. -f, -q etc. count also those that they do not need spec, if any */
/* change this constant if you add or remove map types (last enum+1)*/
#define	NUM_MAP_TYPES			(21+1)
/* foreach map type, how many different specs can the user give? e.g. how many -f options at the same command line */
#define	MAX_SPECS_PER_MAP		10

int	main(int argc, char **argv){
	DATATYPE	***data, ***dataOut,
			minOutputColor = DEFAULT_MIN_OUTPUT_COLOR, maxOutputColor = DEFAULT_MAX_OUTPUT_COLOR,
			borderColor = DEFAULT_MIN_OUTPUT_COLOR,
			globalMinPixel, globalMaxPixel;
	char		inputFilename[1000] = {"\0"}, outputFilename[1000] = {"\0"},
			includeOriginalImage = FALSE, verbose = FALSE,
			showImageOutsideROI = FALSE, retainImageDimensions = FALSE,
			outputColorRangeSpecified = FALSE, copyHeaderFlag = FALSE;
	int		numSlices = 0, X, Y, W, H, inX, inY, inW, inH,
			depth, format, s, n, borderThickness = 0,
			optI, slices[1000], allSlices = 0, numSlicesOut = 0,
			nSpecs[NUM_MAP_TYPES], nSpecs_dup[NUM_MAP_TYPES],
			mapOrder[(NUM_MAP_TYPES-1) * MAX_SPECS_PER_MAP],
			sOut, numMaps, shouldDoNeighbours = 0,
			largeWindowSize, smallWindowSize, currentMap, m,
			d1, d2,
			xoffset[] = {-1, 0, 1, 1, 1, 0, -1, -1},
			yoffset[] = {-1, -1, -1, 0, 1, 1, 1, 0};
	histogram	*pixelFrequencies = NULL;
	register int	i, j, k, smallWindowW, smallWindowH,
			largeWindowW, largeWindowH, slice, param1, param2, param3, param4;
	double		globalMean, globalStdev, lo, hi, d3, d4;
	float		**tempData;
	area		SPECS[NUM_MAP_TYPES][MAX_SPECS_PER_MAP], ROI;
	char		**changeMap;

	/* initialise */
	for(i=0;i<NUM_MAP_TYPES;i++) for(j=0;j<MAX_SPECS_PER_MAP;j++) {
		nSpecs[i] = 0;
		SPECS[i][j].a = SPECS[i][j].b = SPECS[i][j].c = SPECS[i][j].d = 0;
		SPECS[i][j].lo = -1000000.0; SPECS[i][j].hi = 10000000;
	}
	numMaps = 0;
	ROI.a = ROI.b = 0; ROI.c = ROI.d = -1; /* region of interest init */

	/* read & process args */
	opterr = 0;
	while( (optI=getopt(argc, argv, "i:o:w:h:x:y:s:vknlgz:r:m:M:b:B:d:t:T:e:f:F:q:Q:c:C:u:U:a:NX:I:O:W:j?9")) != EOF)
		switch( optI ){
			case 'i': strcpy(inputFilename, optarg); break;
			case 'o': strcpy(outputFilename, optarg); break;
			case 'x': ROI.a = atoi(optarg); break;
			case 'y': ROI.b = atoi(optarg); break;
			case 'w': ROI.c = atoi(optarg); break;
			case 'h': ROI.d = atoi(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg); break;
			case 'v': verbose = TRUE; break;
			case 'k': break;
			case 'n': includeOriginalImage = TRUE; break;
			case 'l': showImageOutsideROI = TRUE; retainImageDimensions = TRUE; break;
			case 'g': retainImageDimensions = TRUE; break;
			case 'z': borderColor = maxOutputColor; borderThickness = atoi(optarg); break;
			case 'r': sscanf(optarg, "%d:%d", &(minOutputColor), &(maxOutputColor)); outputColorRangeSpecified = TRUE; break;

			case 'm': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[M][nSpecs[M]].a), &(SPECS[M][nSpecs[M]].b), &(SPECS[M][nSpecs[M]].lo), &(SPECS[M][nSpecs[M]].hi)); nSpecs[M]++; mapOrder[numMaps++] = M; break;
			case 'M': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[M_CAPS][nSpecs[M_CAPS]].a), &(SPECS[M_CAPS][nSpecs[M_CAPS]].b), &(SPECS[M_CAPS][nSpecs[M_CAPS]].lo), &(SPECS[M_CAPS][nSpecs[M_CAPS]].hi)); nSpecs[M_CAPS]++; mapOrder[numMaps++] = M_CAPS; break;
			case 'b': sscanf(optarg, "%d:%d:%d:%d:%lf:%lf", &(SPECS[B][nSpecs[B]].a), &(SPECS[B][nSpecs[B]].b), &(SPECS[B][nSpecs[B]].c), &(SPECS[B][nSpecs[B]].d), &(SPECS[B][nSpecs[B]].lo), &(SPECS[B][nSpecs[B]].hi)); nSpecs[B]++; mapOrder[numMaps++] = B; break;
			case 'B': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[B_CAPS][nSpecs[B_CAPS]].a), &(SPECS[B_CAPS][nSpecs[B_CAPS]].b), &(SPECS[B_CAPS][nSpecs[B_CAPS]].lo), &(SPECS[B_CAPS][nSpecs[B_CAPS]].hi)); nSpecs[B_CAPS]++; mapOrder[numMaps++] = B_CAPS; break;
			case 'd': sscanf(optarg, "%lf:%lf", &(SPECS[D][nSpecs[D]].lo), &(SPECS[D][nSpecs[D]].hi)); mapOrder[numMaps++] = D; nSpecs[D]++; break;
			case 't': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[T][nSpecs[T]].a), &(SPECS[T][nSpecs[T]].b), &(SPECS[T][nSpecs[T]].lo), &(SPECS[T][nSpecs[T]].hi)); nSpecs[T]++; mapOrder[numMaps++] = T; break;
			case 'T': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[T_CAPS][nSpecs[T_CAPS]].a), &(SPECS[T_CAPS][nSpecs[T_CAPS]].b), &(SPECS[T_CAPS][nSpecs[T_CAPS]].lo), &(SPECS[T_CAPS][nSpecs[T_CAPS]].hi)); nSpecs[T_CAPS]++; mapOrder[numMaps++] = T_CAPS; break;
			case 'e': sscanf(optarg, "%d:%d:%d:%d:%lf:%lf", &(SPECS[E][nSpecs[E]].a), &(SPECS[E][nSpecs[E]].b), &(SPECS[E][nSpecs[E]].c), &(SPECS[E][nSpecs[E]].d), &(SPECS[E][nSpecs[E]].lo), &(SPECS[E][nSpecs[E]].hi)); nSpecs[E]++; mapOrder[numMaps++] = E; break;
			case 'f': sscanf(optarg, "%lf:%lf", &(SPECS[F][nSpecs[F]].lo), &(SPECS[F][nSpecs[F]].hi)); nSpecs[F]++; mapOrder[numMaps++] = F; break;
			case 'F': sscanf(optarg, "%lf:%lf", &(SPECS[F_CAPS][nSpecs[F_CAPS]].lo), &(SPECS[F_CAPS][nSpecs[F_CAPS]].hi)); nSpecs[F_CAPS]++; mapOrder[numMaps++] = F_CAPS; break;
			case 'q': sscanf(optarg, "%d:%d:%d:%d:%lf:%lf", &(SPECS[Q][nSpecs[Q]].a), &(SPECS[Q][nSpecs[Q]].b), &(SPECS[Q][nSpecs[Q]].c), &(SPECS[Q][nSpecs[Q]].d), &(SPECS[Q][nSpecs[Q]].lo), &(SPECS[Q][nSpecs[Q]].hi)); nSpecs[Q]++; mapOrder[numMaps++] = Q; break;
			case 'Q': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[Q_CAPS][nSpecs[Q_CAPS]].a), &(SPECS[Q_CAPS][nSpecs[Q_CAPS]].b), &(SPECS[Q_CAPS][nSpecs[Q_CAPS]].lo), &(SPECS[Q_CAPS][nSpecs[Q_CAPS]].hi)); nSpecs[Q_CAPS]++; mapOrder[numMaps++] = Q_CAPS; break;
			case 'c': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[C][nSpecs[C]].a), &(SPECS[C][nSpecs[C]].b), &(SPECS[C][nSpecs[C]].lo), &(SPECS[C][nSpecs[C]].hi)); nSpecs[C]++; mapOrder[numMaps++] = C; break;
			case 'C': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[C_CAPS][nSpecs[C_CAPS]].a), &(SPECS[C_CAPS][nSpecs[C_CAPS]].b), &(SPECS[C_CAPS][nSpecs[C_CAPS]].lo), &(SPECS[C_CAPS][nSpecs[C_CAPS]].hi)); nSpecs[C_CAPS]++; mapOrder[numMaps++] = C_CAPS; break;
			case 'u': sscanf(optarg, "%d:%d:%d:%d:%lf:%lf", &(SPECS[U][nSpecs[U]].a), &(SPECS[U][nSpecs[U]].b), &(SPECS[U][nSpecs[U]].c), &(SPECS[U][nSpecs[U]].d), &(SPECS[U][nSpecs[U]].lo), &(SPECS[U][nSpecs[U]].hi)); nSpecs[U]++; mapOrder[numMaps++] = U; break;
			case 'U': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[U_CAPS][nSpecs[U_CAPS]].a), &(SPECS[U_CAPS][nSpecs[U_CAPS]].b), &(SPECS[U_CAPS][nSpecs[U_CAPS]].lo), &(SPECS[U_CAPS][nSpecs[U_CAPS]].hi)); nSpecs[U_CAPS]++; mapOrder[numMaps++] = U_CAPS; break;
			case 'X': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[X_CAPS][nSpecs[X_CAPS]].a), &(SPECS[X_CAPS][nSpecs[X_CAPS]].b), &(SPECS[X_CAPS][nSpecs[X_CAPS]].lo), &(SPECS[X_CAPS][nSpecs[X_CAPS]].hi)); nSpecs[X_CAPS]++; mapOrder[numMaps++] = X_CAPS; break;
			case 'I': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[I_CAPS][nSpecs[I_CAPS]].a), &(SPECS[I_CAPS][nSpecs[I_CAPS]].b), &(SPECS[I_CAPS][nSpecs[I_CAPS]].lo), &(SPECS[I_CAPS][nSpecs[I_CAPS]].hi)); nSpecs[I_CAPS]++; mapOrder[numMaps++] = I_CAPS; break;
			case 'O': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[O_CAPS][nSpecs[O_CAPS]].a), &(SPECS[O_CAPS][nSpecs[O_CAPS]].b), &(SPECS[O_CAPS][nSpecs[O_CAPS]].lo), &(SPECS[O_CAPS][nSpecs[O_CAPS]].hi)); nSpecs[O_CAPS]++; mapOrder[numMaps++] = O_CAPS; break;
			case 'W': sscanf(optarg, "%d:%d:%lf:%lf", &(SPECS[W_CAPS][nSpecs[W_CAPS]].a), &(SPECS[W_CAPS][nSpecs[W_CAPS]].b), &(SPECS[W_CAPS][nSpecs[W_CAPS]].lo), &(SPECS[W_CAPS][nSpecs[W_CAPS]].hi)); nSpecs[W_CAPS]++; mapOrder[numMaps++] = W_CAPS; break;
			case 'N': nSpecs[N_CAPS]++; mapOrder[numMaps++] = N_CAPS; shouldDoNeighbours=7; break;

			case 'a': sscanf(optarg, "%d:%d:%d:%d:%lf:%lf", &(SPECS[A][nSpecs[A]].a), &(SPECS[A][nSpecs[A]].b), &(SPECS[A][nSpecs[A]].c), &(SPECS[A][nSpecs[A]].d), &(SPECS[A][nSpecs[A]].lo), &(SPECS[A][nSpecs[A]].hi)); nSpecs[A]++; shouldDoNeighbours=7; break;

			case 'j': fprintf(stderr, "Usage : %s %s\n%s\n\n", argv[0], Usage, Author);
				  exit(0);

			case '9': copyHeaderFlag = TRUE; break;

			default:
			case '?': fprintf(stderr, "Usage : %s %s\n%s\n\n", argv[0], ShortUsage, Author);
				  fprintf(stderr, "Unknown option '-%c' or option requires argument(s) to follow.\n", optopt);
				  exit(1);
		}
	if( inputFilename[0]=='\0' ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		exit(1);
	}
	if( outputFilename[0]=='\0' ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "An output filename must be specified.\n");
		exit(1);
	}
	if( nSpecs[A] > 0 ){
		/* the -a option was selected, cancel all other map specs and copy the a specs to them */
		/* do it for as many -a options were specified */
		for(i=0;i<NUM_MAP_TYPES;i++){
			if( i == A ) continue;
			for(n=0;n<nSpecs[A];n++){
				SPECS[i][n].a = SPECS[A][n].a;
				SPECS[i][n].b = SPECS[A][n].b;
				SPECS[i][n].c = SPECS[A][n].c;
				SPECS[i][n].d = SPECS[A][n].d;
				SPECS[i][n].lo =SPECS[A][n].lo;
				SPECS[i][n].hi =SPECS[A][n].hi;
				mapOrder[n*(NUM_MAP_TYPES-1) + i] = i;
			}
			nSpecs_dup[i] = nSpecs[i] = nSpecs[A];
		}
		numMaps = (NUM_MAP_TYPES-1) * nSpecs[A];
	} else {
		/* how many map specs in total ? */
		for(i=0;i<NUM_MAP_TYPES;i++) nSpecs_dup[i] = nSpecs[i];
	}

	/* image dimensions */
	inX = (retainImageDimensions==TRUE ? 0:ROI.a); inY = (retainImageDimensions==TRUE ? 0:ROI.b);
	X = (retainImageDimensions==TRUE ? ROI.a:0); Y = (retainImageDimensions==TRUE ? ROI.b:0);
	inW = (retainImageDimensions==TRUE ? -1:ROI.c); inH = (retainImageDimensions==TRUE ? -1:ROI.d);
	/* inW and inH contain the whole image dimensions, W and H contain the ROI dimensions
	   if we only have to save the ROI (e.g. retainImageDimensions==FALSE) W=inW, H=inH and X=Y=0
	   and we will not load the whole image but just the ROI part, otherwise W<inW and H<inH.
	   So, use inW and inH for allocating data and initialising the output data but use W and H
	   for operating within the ROI */

	if( numSlices == 0 ){
		data = getUNCSlices3D(inputFilename, inX, inY, &inW, &inH, NULL, &numSlices, &depth, &format);
		allSlices = 1;
	} else data = getUNCSlices3D(inputFilename, inX, inY, &inW, &inH, slices, &numSlices, &depth, &format);
	if( data == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for input file '%s'.\n", argv[0], inputFilename);
		exit(1);
	}

	W = ROI.c<=0 ? inW:ROI.c; H = ROI.d<=0 ? inH:ROI.d;
	if( (numSlicesOut=(numMaps+shouldDoNeighbours+((includeOriginalImage==TRUE)?1:0))*numSlices) < numSlices ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], ShortUsage, Author);
		fprintf(stderr, "At least one map specification must be given - if you are unsure use '-a w:h:W:H' for all map types.\n");
		freeDATATYPE3D(data, numSlices, inW);
		exit(1);
	}
	if( (dataOut=callocDATATYPE3D(numSlicesOut, inW, inH)) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d x %d(slices) of DATATYPEs, %zd bytes each.\n", argv[0], inW, inH, numSlicesOut, sizeof(DATATYPE));
		freeDATATYPE3D(data, numSlices, inW);
		exit(1);
	}
	if( (tempData=(float **)calloc2D(W, H, sizeof(float))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d floats.\n", argv[0], W, H);
		freeDATATYPE3D(data, numSlices, inW);
		freeDATATYPE3D(dataOut, numSlices, inW);
		exit(1);
	}
	if( (changeMap=(char **)calloc2D(inW, inH, sizeof(char))) == NULL ){
		fprintf(stderr, "%s : could not allocate %d x %d floats.\n", argv[0], W, H);
		freeDATATYPE3D(data, numSlices, inW);
		freeDATATYPE3D(dataOut, numSlices, inW);
		free2D((void **)tempData, numSlices);
		exit(1);
	}
	printf("%s (%s) slice: ", argv[0], inputFilename); fflush(stdout);
	for(s=0,sOut=0;s<numSlices;s++){
		slice = (allSlices==0) ? slices[s] : s;
		printf(" %d", slice+1); fflush(stdout);
		/* do some global stats for this slice but only inside ROI */
		if( (pixelFrequencies=histogram2D(data[slice], X, Y, W, H, 1)) == NULL ){
			fprintf(stderr, "%s : call to histogram2D has failed for file '%s', slice was '%d', region of interest was (%d,%d,%d,%d).\n", argv[0], inputFilename, slice, X, Y, W, H);
			freeDATATYPE3D(data, numSlices, inW);
			freeDATATYPE3D(dataOut, numSlicesOut, inW); free2D((void **)changeMap, inW); free2D((void **)tempData, W);
			exit(1);
		}			
		mean_stdev2D(data[slice], X, Y, W, H, &globalMean, &globalStdev);
		/* if we show the whole image, and output color ranges not specified, used those of
		   image AND NOT defaults */
		if( showImageOutsideROI && !outputColorRangeSpecified ){ maxOutputColor = pixelFrequencies->maxPixel; minOutputColor = pixelFrequencies->minPixel; }

		min_maxPixel2D(data[slice], X, Y, W, H, &globalMinPixel, &globalMaxPixel);

		/* write the original slice first, if requested */
		if( includeOriginalImage ){
			/* map the original image to min/max colors */
			for(i=X;i<X+W;i++) for(j=Y;j<Y+H;j++)
				dataOut[sOut][i][j] = ROUND(SCALE_OUTPUT((float )(data[slice][i][j]), (float )minOutputColor, (float )maxOutputColor, (float )globalMinPixel, (float )globalMaxPixel));

			/* draw a border around ROI -- for options that do not involve subwindows */
			if( showImageOutsideROI && (borderThickness>0) ){
				for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
				for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
			}
			sOut++;
		}

		for(m=0;m<numMaps;m++){
			/* initialise the output data, black outside ROI or original image outside ROI, depending on flags */
			if( retainImageDimensions && showImageOutsideROI )
				for(i=inX;i<inW;i++) for(j=inY;j<inH;j++) dataOut[sOut][i][j] = data[slice][i][j];
			/* so that when we have a succession of maps, the changeMap which is common
			   does not affect the next map */
			for(i=0;i<inW;i++) for(j=0;j<inH;j++) changeMap[i][j] = changed;
			currentMap = mapOrder[m];
			n = nSpecs_dup[currentMap]-nSpecs[currentMap]; nSpecs[currentMap]--;
			param1 = SPECS[currentMap][n].a; param2 = SPECS[currentMap][n].b;
			param3 = SPECS[currentMap][n].c; param4 = SPECS[currentMap][n].d;
			    lo = SPECS[currentMap][n].lo;    hi = SPECS[currentMap][n].hi;
			smallWindowW = 2*param1+1; smallWindowH = 2*param2+1; /* param1, 2 refer to w and h */
			largeWindowW = 2*param3+1; largeWindowH = 2*param4+1; /* param3, 4 refer to W and H */
			smallWindowSize = smallWindowW * smallWindowH; /* refer to the window defined by w and h */
			largeWindowSize = largeWindowW * largeWindowH; /* refer to the larger window defined by W and H */
			switch( mapOrder[m] ){
				case N_CAPS: /* produce the 8 neighbours of each pixel starting from the top left and then clockwise */
					if( verbose ) printf("file '%s', input slice %d, -N , output slice %d to %d, ROI(%d,%d,%d,%d), image size %d x %d :\n", inputFilename, slice+1, sOut+1, sOut+1+8, X, Y, W, H, inW, inH);
					for(k=0;k<8;k++,sOut++){
						for(i=X+1;i<X+W-1;i++) for(j=Y+1;j<Y+H-1;j++) dataOut[sOut][i][j] = ROUND(SCALE_OUTPUT((float )(data[slice][i+xoffset[k]][j+yoffset[k]]), (float )minOutputColor, (float )maxOutputColor, (float )globalMinPixel, (float )globalMaxPixel));

						if( verbose ){ statistics2D(dataOut[sOut], X, Y, X+W, Y+H, &d1, &d2, &d3, &d4); printf("\tslice %d, stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", sOut+1, d1, d2, d3, d4); }
						/* draw a border around ROI -- for options that do not involve subwindows */
						if( showImageOutsideROI && (borderThickness>0) ){
							for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
							for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
						}
					}
					if( verbose ) printf("end -N info\n");
					break;
				case F: /* [-f lo:hi (FOC (lo,hi] for single pixels)] */
					if( verbose ){ printf("file '%s', input slice %d, -f %d:%d, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, (int )lo, (int )hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( lo > hi ){ fprintf(stderr, "-f option (%d) : invalid spec - low frequency must be LESS than high frequency, skipping this map spec...\n", n+1); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, 0, 0, 0, 0, lo, hi, NOP, SINGLE_PIXEL, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-f %d:%d) has failed.\n", argv[0], (int )lo, (int )hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* draw a border around ROI -- for options that do not involve subwindows */
					if( showImageOutsideROI && (borderThickness>0) ){
						for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
						for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					}
					if( verbose ){ statistics2D(dataOut[sOut], X, Y, X+W, Y+H, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case F_CAPS: /* [-F lo:hi (FOC for single pixel - whole images MFOC (lo,hi])] */
					if( verbose ){ printf("file '%s', input slice %d, -F %d:%d, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, (int )lo, (int )hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( lo > hi ){ fprintf(stderr, "-F option (%d) : invalid spec - low frequency must be LESS than high frequency, skipping this map spec...\n", n+1); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, 0, 0, 0, 0, lo, hi, NOP, COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-F %d:%d) has failed.\n", argv[0], (int )lo, (int )hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* draw a border around ROI -- for options that do not involve subwindows */
					if( showImageOutsideROI && (borderThickness>0) ){
						for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
						for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					}
					if( verbose ){ statistics2D(dataOut[sOut], X, Y, X+W, Y+H, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				/* min/max of frequency of occurence */
				case O_CAPS: /* [-m w:h[:lo:hi] (Max FOC of sub-window)] */
					if( verbose ){ printf("file '%s', input slice %d, -X %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-X option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_max2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_max2D (-X %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case W_CAPS: /* [-m w:h[:lo:hi] (Min FOC of sub-window)] */
					if( verbose ){ printf("file '%s', input slice %d, -X %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-X option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_min2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_min2D (-X %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case Q: /* [-q w:h:W:H[:lo:hi] (MFOC of wxh - MFOC of WxH)] */
					if( verbose ){ printf("file '%s', input slice %d, -q %d:%d:%d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, param3, param4, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>=param3)||(param2>=param4)||(param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-q option (%d) : invalid spec w must be less than W, h must be less than H and each of them to fit the image (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, param3, param4, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-q %d:%d:%d:%d:%f:%f) has failed.\n", argv[0], param1, param2, param3, param4, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param3 = param4 = borderThickness;
					for(j=Y+param4;j<Y+H-param4;j++){ for(i=X;i<X+param3;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param3;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param4;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param4;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param3, Y+param4, X+W-param3, Y+H-param4, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case C: /* [-c w:h[:lo:hi] (MFOC of sub-window wxh)] */
					if( verbose ){ printf("file '%s', input slice %d, -c %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-c option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-c %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case C_CAPS: /* [-C w:h[:lo:hi] (MFOC of wxh - whole image's MFOC)] */
					if( verbose ){ printf("file '%s', input slice %d, -C %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-C option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-C %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case Q_CAPS: /* [-Q w:h[:lo:hi] (single pixel's FOC - MFOC of wxh)] */
					if( verbose ){ printf("file '%s', input slice %d, -Q %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-Q option (%d) : invalid spec w and h must be small enough to fit the image (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_frequency_mean2D (-Q %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param3 = param4 = borderThickness;
					for(j=Y+param4;j<Y+H-param4;j++){ for(i=X;i<X+param3;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param3;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param4;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param4;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				/* min/max pixel values */
				case X_CAPS: /* [-m w:h[:lo:hi] (Max PV of sub-window)] */
					if( verbose ){ printf("file '%s', input slice %d, -X %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-X option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_maxPixel2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_maxPixel2D (-X %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case I_CAPS: /* [-m w:h[:lo:hi] (Min PV of sub-window)] */
					if( verbose ){ printf("file '%s', input slice %d, -X %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-X option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_minPixel2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_minPixel2D (-X %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				/* mean pixel values */
				case M: /* [-m w:h[:lo:hi] (MPV of sub-window)] */
					if( verbose ){ printf("file '%s', input slice %d, -m %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-m option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-m %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case M_CAPS: /* [-M w:h[:lo:hi] (MPV of sub-window - MPV of whole image)] */
					if( verbose ){ printf("file '%s', input slice %d, -M %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-M option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE, globalMean) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-M %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case B: /* [-b w:h:W:H[:lo:hi] (MPV of wxh - MPV of WxH)] */
					if( verbose ){ printf("file '%s', input slice %d, -b %d:%d:%d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, param3, param4, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>=param3)||(param2>=param4)||(param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-e option (%d) : invalid spec w must be less than W, h must be less than H and each of them to fit the image (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, param3, param4, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-b %d:%d:%d:%d:%f:%f) has failed.\n", argv[0], param1, param2, param3, param4, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param3 = param4 = borderThickness;
					for(j=Y+param4;j<Y+H-param4;j++){ for(i=X;i<X+param3;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param3;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param4;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param4;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param3, Y+param4, X+W-param3, Y+H-param4, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case B_CAPS: /* [-B w:h[:lo:hi] (MPV of wxh - PV of each pixel)] */
					if( verbose ){ printf("file '%s', input slice %d, -B %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-B option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SINGLE_PIXEL_TO_SMALL_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-B %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case D: /* [-d [lo:hi] (PV for each pixel - MPV of whole image)] */
					if( verbose ){ printf("file '%s', input slice %d, -d %f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( map_mean2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, 0, 0, 0, 0, lo, hi, NOP, COMPARE_SINGLE_PIXEL_TO_WHOLE_IMAGE, globalMean) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-d) has failed.\n", argv[0]);
						if( verbose ) printf(", failure\n"); break;
					}
					/* draw a border around ROI -- for options that do not involve subwindows */
					if( showImageOutsideROI && (borderThickness>0) ){
						for(i=X;i<X+W;i++){ for(j=Y;j<Y+borderThickness;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-borderThickness;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
						for(j=Y+borderThickness;j<Y+H-borderThickness;j++){ for(i=X;i<X+borderThickness;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-borderThickness;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					}
					if( verbose ){ statistics2D(dataOut[sOut], X, Y, X+W, Y+H, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				/* stdev of pixel values */
				case T: /* [-t w:h[:lo:hi] (SPV of sub-window wxh)] */
					if( verbose ){ printf("file '%s', input slice %d, -t %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-t option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_stdev2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, SMALL_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-t %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case T_CAPS: /* [-T w:h[:lo:hi] (SPV of wxh - SPV of whole image)] */
					if( verbose ){ printf("file '%s', input slice %d, -T %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-T option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_stdev2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE, globalStdev) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-T %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case E: /* [-e w:h:W:H[:lo:hi] (SPV of wxh - SPV of WxH)] */
					if( verbose ){ printf("file '%s', input slice %d, -e %d:%d:%d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, param3, param4, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>=param3)||(param2>=param4)||(param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-e option (%d) : invalid spec w must be less than W, h must be less than H and each of them to fit the image (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_stdev2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, param3, param4, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW, 0.0) == FALSE ){
						fprintf(stderr, "%s : call to map_stdev2D (-e %d:%d:%d:%d:%f:%f) has failed.\n", argv[0], param1, param2, param3, param4, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param3 = param4 = borderThickness;
					for(j=Y+param4;j<Y+H-param4;j++){ for(i=X;i<X+param3;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param3;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param4;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param4;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param3, Y+param4, X+W-param3, Y+H-param4, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case U: /* [-u w:h:W:H[:lo:hi] (SFOC of wxh - SFOC of WxH)] */
					if( verbose ){ printf("file '%s', input slice %d, -u %d:%d:%d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, param3, param4, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>=param3)||(param2>=param4)||(param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-u option (%d) : invalid spec w must be less than W, h must be less than H and each of them to fit the image (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_stdev2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, param3, param4, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_LARGER_WINDOW, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_stdev2D (-u %d:%d:%d:%d:%f:%f) has failed.\n", argv[0], param1, param2, param3, param4, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param3 = param4 = borderThickness;
					for(j=Y+param4;j<Y+H-param4;j++){ for(i=X;i<X+param3;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param3;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param4;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param4;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param3, Y+param4, X+W-param3, Y+H-param4, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case U_CAPS: /* [-U w:h[:lo:hi] (SFOC of wxh - SFOC of whole image)] */
					if( verbose ){ printf("file '%s', input slice %d, -U %d:%d:%f:%f, output slice %d, ROI(%d,%d,%d,%d), image size %d x %d", inputFilename, slice+1, param1, param2, lo, hi, sOut+1, X, Y, W, H, inW, inH); fflush(stdout); }
					if( (param1>W)||(param2>H)||(lo>hi) ){ fprintf(stderr, "-U option (%d) : invalid spec - w and h must fit image size (%dx%d). Also low range must be less than high range - skipping this map spec...\n", n+1, W, H); if( verbose ) printf(", failed\n"); continue; }
					if( map_frequency_stdev2D(data[slice], changeMap, dataOut[sOut], minOutputColor, maxOutputColor, X, Y, W, H, param1, param2, 0, 0, lo, hi, NOP, COMPARE_SMALL_WINDOW_TO_WHOLE_IMAGE, pixelFrequencies) == FALSE ){
						fprintf(stderr, "%s : call to map_mean2D (-U %d:%d:%f:%f) has failed.\n", argv[0], param1, param2, lo, hi);
						if( verbose ) printf(", failure\n"); break;
					}
					/* clear the edges of image we could not fill because of window sizes */
					if( showImageOutsideROI ) param1 = param2 = borderThickness;
					for(i=X;i<X+W;i++){ for(j=Y;j<Y+param2;j++) dataOut[sOut][i][j] = borderColor; for(j=Y+H-param2;j<Y+H;j++) dataOut[sOut][i][j] = borderColor; }
					for(j=Y+param2;j<Y+H-param2;j++){ for(i=X;i<X+param1;i++) dataOut[sOut][i][j] = borderColor; for(i=X+W-param1;i<X+W;i++) dataOut[sOut][i][j] = borderColor; }
					if( verbose ){ statistics2D(dataOut[sOut], X+param1, Y+param2, X+W-param1, Y+H-param2, &d1, &d2, &d3, &d4); printf(", stats(minPixel=%d, maxPixel=%d, mean=%f, stdev=%f), success\n", d1, d2, d3, d4); }
					sOut++; break;
				case A: break; /* nothing */
				default:
					fprintf(stderr, "%s : map %d not implemented yet...\n", argv[0], mapOrder[m]);
					break;
			} /* end switch */
		} /* for(m=0;m<numMaps;m++) */

		/* initialise specs again, for second slice etc. */
		for(m=0;m<numMaps;m++) nSpecs[mapOrder[m]] = nSpecs_dup[mapOrder[m]];

		free(pixelFrequencies->bins); /* free the histogram, we will do another for next slice */
	} /* for(s=0,sOut=0;s<numSlices;s++) */
	printf("\n");

	/* done with original data and histograms */
	freeDATATYPE3D(data, numSlices, inW);
	if( pixelFrequencies != NULL ) destroy_histogram(pixelFrequencies);

	/* write images out */
	if( sOut > 0 ){
		if( !writeUNCSlices3D(outputFilename, dataOut, inW, inH, 0, 0, inW, inH, NULL, sOut, format, OVERWRITE) ){
			fprintf(stderr, "%s : call to writeUNCSlices3D has failed for output file '%s'.\n", argv[0], outputFilename);
			freeDATATYPE3D(dataOut, numSlicesOut, inW); free2D((void **)changeMap, inW); free2D((void **)tempData, W);
			exit(1);
		}
		/* now copy the image info/title/header of source to destination */
		if( copyHeaderFlag ) if( !copyUNCInfo(inputFilename, outputFilename, IO_INFO_COPY|IO_TITLE_COPY, NULL, 0) ){
			fprintf(stderr, "%s : call to copyUNCInfo has failed while copying header/title info from file '%s' to '%s'.\n", argv[0], inputFilename, outputFilename);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s : no output was produced! Check your maps' specs.\n", argv[0]);
		freeDATATYPE3D(dataOut, numSlicesOut, inW); free2D((void **)changeMap, inW); free2D((void **)tempData, W);
		exit(1);
	}
	freeDATATYPE3D(dataOut, numSlicesOut, inW); free2D((void **)changeMap, inW); free2D((void **)tempData, W);

	exit(0);
}
