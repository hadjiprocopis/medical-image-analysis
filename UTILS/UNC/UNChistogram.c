#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Common_IMMA.h>
#include <Alloc.h>
#include <IO.h>
#include <filters.h>

/* change here the default bin size */
#define	DEFAULT_BINSIZE			1
/* the character to precede all comment lines - can be changed with the '-c' switch */
#define	DEFAULT_COMMENT_DESIGNATOR	"#"

const	char	Examples[] = "\
\n	-i input.unc -o hist\
\n\
\nwill calculate the histogram of each slice in the file\
\ninput.unc. As no bin size was specified (with the '-b'\
\noption) the default bin size of 1 pixel will be used.\
\nAssuming the 'input.unc' contained 3 slices,\
\nThe result will be placed in 3 different files:\
\n'hist_1', 'hist_2' and 'hist_3'\
\nIn addition, a whole brain histogram will be calculated\
\nand placed in a file called 'hist_whole'.\
\n(If there were more than 10 slices in the UNC file,\
\n the histogram files would be 'hist_01' 'hist_02' etc.)\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5\
\n\
\nThe same as above but the bin size will be 10 pixels.\
\nThen the histogram will be normalised and smoothed\
\nwith a window size of 5 pixels.\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -x 40 -y 50 -w 50 -h 50\
\n\
\nAs above but only the portion of the image defined by\
\nthe rectangle whose left-top corner is at (40,50)\
\nand with width and height of 50 pixels.\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -x 40 -y 50 -w 50 -h 50 -s 1 -s 5 -s 7\
\n\
\nAs above but only for the slices 1, 5 and 7.\
\n(So only 3 histogram files will be created\
\n named 'hist_1', 'hist_5' and 'hist_7', plus\
\n the whole brain histogram 'hist_whole' but\
\n with the difference that in this case\
\n 'whole' means just the selected slices 1, 5 and 7).\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -s 1 -s 5 -s 7\
\n\
\nAs above, only for the slices 1, 5 and 7 but for\
\nthe whole image in each slice - no region of interest specified.\
\n(So only 3 histogram files will be created\
\n named 'hist_1', 'hist_5' and 'hist_7')\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -a\
\n\
\nAs before, but only a whole brain histogram\
\nis calculated, histogram for individual slices are\
\nomitted.\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -A\
\n\
\nAs before, but only individual slices histograms\
\nare calculated, the whole brain histogram is omitted.\
\n\
\nanother example:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -z\
\n\
\nAs before, but in the output histogram file,\
\nthe frequency of occurence of the black pixel will be zero.\
\nNormalisation and smoothing are done taking into account\
\nthe real value of the frequency of occurence of the black pixel\
\nand not that it is written as zero just for decorative purposes\
\nin the output file.\
\n\
\nIf you want to set the black pixel frequency ot zero and\
\nthis be taken into account by normalisation and smoothing\
\nthen use the '-Z' option instead. In this case, the black\
\npixels are totally ignored. The following example will do\
\njust that:\
\n\
\n	-i input.unc -o hist -b 10 -n -m 5 -Z\
\n\
\nMany warnings as this will alter the shape of the histogram.\
\nOn the other hand, using '-Z' is probably correct when you\
\nwant to calculate the histogram of a brain area or the whole\
\nbrain which does not mean to calculate the histogram of the\
\nimage itself.";

const	char	Usage[] = "options as follows:\
\n -i inputFilename\
\n	(UNC image file with one or more slices)\
\n\
\n -o outputBaseName\
\n	(If **more than one slices** are present in the UNC file\
\n	 a histogram will be calculated for each slice - or\
\n	 for those slices defined with '-s' options (see below)\
\n	 - and saved in a separate file whose name will begin\
\n	 with the 'outputBaseName' string and the slice number\
\n	 appended to it.\
\n	 example, outputBaseName='BLAHBLAH', slices 1, 2 and 3:\
\n	 BLAHBLAH_01, BLAHBLAH_02 and BLAHBLAH_03 will be\
\n	 the produced histogram files\
\n\
\n	 On the other hand, if only one slice's histogram\
\n	 is requested, the histogram filename will be the\
\n	 *same* as that in the 'outputBaseName' string.)]\
\n\
\n[-b binSize\
\n	(The size of the bins in pixels - the default is 1)]\
\n\
\n[-n	(normalise flag - divides each histogram entry by the total\
\n	area under the histogram.)]\
\n\
\n[-m W\
\n	(smooths the histogram in a similar way as the SMOOTH function\
\n	in IDL. In effect, moving window averaging of a 1D series.\
\n\
\n	Here it is how it is done:\
\n	The smoothed value of the ith element of the histogram is\
\n	obtained by summing up the W before, the W after\
\n	entries (where '2*W+1' is the window size) plus the ith element,\
\n	and then divide this sum by (2*W+1).\
\n	Note that the first and last W histogram entries will not\
\n	be smoothed.\
\n\
\n	** A good window size (2*W+1) could be 11, WHICH MAKES\
\n	   W equal to 5 - but remember, smoothing\
\n	   produces a 'fake' histogram and the larger W is, the bigger\
\n	   the discrepancy between the initial and final histograms is.)]\
\n\
\n[-t	(use this flag if you want to print a title at the top of\
\n	the histogram file. The title will say which image file this\
\n	histogram represents, report all the options and switches used\
\n	and also report some statistics such as peak frequency and pixel\
\n	value corresponding to that peak, and 25th, 50th (median) and 75th\
\n	percentiles - all the pixels are sorted according to frequency\
\n	of occurence and then the pixels in the middle (median) and those\
\n	at 1/4 and 3/4 are selected.\
\n\
\n	The above title will also be reported at the standard output\
\n	without the need for any special switches.\
\n\
\n	To indicate that the title lines are comments and not actual data,\
\n	they will be preceded by a comment character. This may be specified\
\n	using the '-c' option. However the default comment character (#)\
\n	is understood by most sane UNIX plotters.\
\n\
\n	If your plotter does not like the title, try changing the comment character\
\n	to something that it will understand (e.g. a '\%' or a 'c'), otherwise\
\n	remove title support all-together by not using the '-t' switch at all.)]\
\n\
\n[-u\
\n	(print the cumulative frequencies instead.)]\
\n\
\n[-U\
\n	(print the reverse cumulative frequencies\
\n	 e.g. starting adding up frequencies from the highest\
\n	 pixel to the lowest.)]\
\n\
\n[-v\
\n	(be verbose - print various statistics etc.)]\
\n\
\n[-T	text\
\n	(use this flag to add additional lines of comments at the title section\
\n	 of the output histogram file. All specified 'text' will be preceded by\
\n	 a comment mark (#). This flag may be used many times to specify as\
\n	 many lines of comments in the order of appearance.)]\
\n\
\n[-c	commentDesignator\
\n	(specify the string/character which will precede all comment lines.\
\n	 Default is the hashmark '#' recognised by most unix plotters as comment.)]\
\n	 \
\n[-a	(will calculate ONLY a whole image histogram.\
\n	NO histograms for each slice individually will be calculated.\
\n	The default is that histograms for each slice AS WELL as\
\n	histogram for the whole brain will be calculated.)]\
\n\
\n[-A	(will calculate ONLY individual slices histograms.\
\n	NO whole image histogram will be calculated.\
\n	The default is that histograms for each slice AS WELL as\
\n	histogram for the whole brain will be calculated.)]\
\n\
\n[-z	(the frequency of occurence of the black pixel -\
\n	e.g. the pixel with the lowest intensity, the background -\
\n	will be set to zero when written in the output histogram\
\n	file. Calculations for normalising and smoothing - if any -\
\n	WILL NOT TAKE INTO ACCOUNT this change. This change is\
\n	just for decorative reasons. The '-Z' option, however\
\n	will do much more.)]\
\n\
\n[-Z	(the frequency of occurence of the black pixel -\
\n	e.g. the pixel with the lowest intensity, the background -\
\n	will be set to zero prior normalisation and smoothing - if\
\n	any. Thus, unlike the '-z' option, this option has\
\n	a real effect on the final histogram numbers.)]\
\n\
\n** Use this options to select a region of interest whose\
\n   histogram you need to calculate. You may use one or more\
\n   or all of '-w', '-h', '-x' and '-y' once.\
\n   You may use one or more '-s' options in order to specify\
\n   more slices. Slice numbers start from 1.\
\n   These parameters are optional, if not present then the\
\n   whole image, all slices will be used.\
\n\
\n[-w widthOfInterest]\
\n[-h heightOfInterest]\
\n[-x xCoordOfInterest]\
\n[-y yCoordOfInterest]\
\n[-s sliceNumber [-s s...]]\
\n\
\n** If both normalise (-n) and smooth (-m) options are present,\
\n   the histogram will:\
\n	**FIRST BE SMOOTHED** and **THEN BE NORMALISED**\
\n\
\n** This program will calculate histograms for each slice (if more than\
\n   one, that is) in the input UNC file. It will also calculate a\
\n   histogram for the whole image (whole brain image). The histogram\
\n   filenames will be constructed from the string supplied with the\
\n   '-o name' option. The histogram for the ith slice will be called\
\n  'name_i' where as the whole brain histogram will be called\
\n  'mame_whole'. If there is only 1 slice in the image file,\
\n  the only histogram calculated will be in file 'name'.\
\n\
\n** A few words about the statistics calculated and placed at the header\
\n   when the user chooses the '-t' option.\
\n   A typical header will look like this:\
\n1:	# out.unc, whole brain histogram, slices: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43, region (x=0,y=0,w=256,h=256), bin size 10, (min,max) = (0, 10000), normalised, black pixel ignored during normalisation.\
\n2:	# intensity stats: min(0, 0.000000), max(10000, 0.000000)\
\n3:	# frequency stats: min(0, 0.000000), max(1240, 0.396630)\
\n4:	# active pixel range (880 to 10000)\
\n5:	# histogram mean: 1435.799072\
\n6:	# peak=(1240, 0.396630)\
\n7:	# mean=0.000999, stdev=0.017802\
\n8:	# frequency percentiles: p25=(4470, 0.000003) p50=median=(3490, 0.000007) and p75=(4850, 0.000012)\
\n9:	# pixel percentiles: p25=3160 p50=median=5440 and p75=7720\
\n10:	# number of pixels counted: 577104\
\n\
\n   The first line tells us the name of the input UNC image file, the slices and\
\n   the region over which the histogram was calculated. Also, bin size and whether\
\n   the black pixel (pixel zero and negative pixels) participated in the\
\n   calculations (see options 'Z', 'z')\
\n\
\n   The second line tells us about the min and max intensity values (intensity\
\n   values are on the X-axis and frequency(short for frequency of occurence)\
\n   values on the Y-axis) and at which frequency these values occur.\
\n   e.g. min(A,B) max(C,D) 'A' and 'C' are min and max pixel values and\
\n   'B' is how many pixels have the value 'A' and 'D' how many pixels have the\
\n   value 'C'.\
\n\
\n   The third line tells us the same things but for frequency of occurence (the Y-axis\
\n   values).\
\n   so, min(A,B) max(C,D) says that the minimum frequency was 'B' and occured for\
\n   pixels of value 'A'. Also 'D' was the maximum frequency of occurence (also known\
\n   as peak value) and occured for pixel values of 'C'. That means that pixels with\
\n   intensity value 'C' were the most frequent, whereas those with value 'A' were\
\n   the least frequent.\
\n\
\n   The fourth line tells us which was the minimum and maximum pixel values excluding\
\n   the background pixel (pixel 0 or negative).\
\n\
\n   The fifth line is the 'histogram mean'. This value is calculated by\
\n   summing up all pixel intensity values in the image (or in any way those\
\n   in the slices and region considered and not background) and then dividing\
\n   by the number of those pixels. It can also be called, the mean intensity\
\n   value of the image.\
\n\
\n   The sixth line is the peak of the histogram, i.e. the most frequent pixel\
\n   value (or pixel range if the bin size is greater than 1), i.e. the highest point\
\n   on the histogram curve.\
\n   so peak=(1240, 0.396630) means that the most frequent pixel value is 1240\
\n   and the frequency of occurence is 0.39... (of course this is a normalised\
\n   value because the user must have selected the '-n' option).\
\n\
\n   Line 7 gives us the mean and stdev of frequencies. The mean is adding the\
\n   content of each bin and then dividing by the number of bins. Similarly for\
\n   stdev. This statistic is not very important and perhaps even silly.\
\n\
\n    the percentiles are defined as follows:\
\n    median is the T1 value of the middle voxel (ie that voxel that is 50% along the\
\n    line of voxels when all the voxels in the brain are ordered in a line from that\
\n    with the lowest T1 value to that with the highest). The 25th and 75th\
\n    percentiles are the T1 values for the voxel which is 1/4 and 3/4 of the way\
\n    along that line. (of course T1 values refer to pixel intensity values)\
\n                                                                                     \
\n   Line 8 : the frequency percentiles and at which intensity value they occur.\
\n   Line 9 : the pixel intensity values percentiles.\
\n   Finally, Line 10 gives a count of all the active pixels participated in\
\n   the calculations.";

const	char	Author[] = "Andreas Hadjiprocopis, NMR, ION, 2001";

int	main(int argc, char **argv){
	FILE		*outputHandle;
	DATATYPE	***data, ***data2;
	histogram	*hist = NULL;
	char		*inputFilename = NULL, *outputBasename = NULL,
			outputFilename[1000], *commentDesignator = strdup(DEFAULT_COMMENT_DESIGNATOR),
			*additionalComments[5000];
	int		x = 0, y = 0, w = -1, h = -1, W = -1, H = -1, numSlices = 0,
			depth, format, title = 0, smoothWindowSize = -1,
			normaliseFlag = FALSE, actualNumSlices = 0, wholeImageFlag = TRUE,
			histogramsForEachSliceFlag = TRUE,
			optI, slices[1000], allSlices = 0, binSize = DEFAULT_BINSIZE,
			blackPixelNotCountedFlag = FALSE,
			blackPixelNotCountedNormalisationFlag = FALSE,
			doNormalisationFirst = FALSE, verboseFlag = FALSE,
			numAdditionalComments = 0, doCumulativeFlag = FALSE,
			pixelOfMinFrequency, pixelOfMaxFrequency,
			doReverseCumulativeFlag = FALSE, sub,
			*numPixels;
	float		peakValue, p25Value, p50Value,
			p75Value, meanFrequency, stdevFrequency,
			frequencyOfMinPixel, frequencyOfMaxPixel,
			minFrequency, maxFrequency, cum;
	DATATYPE	peak, p25, p50, p75,
			minActivePixel, maxActivePixel;
	register int	i, j, ii, jj, s, slice;
	
	while( (optI=getopt(argc, argv, "i:o:s:w:h:x:y:b:tT:c:m:neaAzZuUrv")) != EOF)
		switch( optI ){
			case 'i': inputFilename = strdup(optarg); break;
			case 'o': outputBasename = strdup(optarg); break;
			case 's': slices[numSlices++] = atoi(optarg) - 1; break;
			case 'b': binSize = atoi(optarg); break;
			case 'z': blackPixelNotCountedFlag = TRUE; break;
			case 'Z': blackPixelNotCountedNormalisationFlag = TRUE; break;
			case 'r': doNormalisationFirst = TRUE; break;
			case 'a': histogramsForEachSliceFlag = FALSE;
				  wholeImageFlag = TRUE; break;
			case 'A': histogramsForEachSliceFlag = TRUE;
				  wholeImageFlag = FALSE; break;
			case 'w': w = atoi(optarg); break;
			case 'h': h = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'y': y = atoi(optarg); break;
			case 'm': smoothWindowSize = atoi(optarg); break;
			case 'e': fprintf(stderr, "Here are some examples:\n\n%s\n\n%s\n\n", Examples, Author);
				  exit(0);
			case 'n': normaliseFlag = TRUE; break;
			case 'u': doCumulativeFlag = TRUE; break;
			case 'U': doCumulativeFlag = TRUE; doReverseCumulativeFlag = TRUE; break;
			case 'v': verboseFlag = TRUE; break;
			case 't': title = 1; break;
			case 'c': free(commentDesignator); commentDesignator = strdup(optarg); break;
			case 'T': additionalComments[numAdditionalComments++] = strdup(optarg); break;

			default:  fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
				  fprintf(stderr, "Unknown option '-%c'.\n", optI);
				  exit(1);
		}
	if( inputFilename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An input filename must be specified.\n");
		exit(1);
	}
	if( outputBasename == NULL ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "An output basename must be specified.\n");
		exit(1);
	}
	if( binSize <= 0 ){
		fprintf(stderr, "Usage : %s %s\n%s\n", argv[0], Usage, Author);
		fprintf(stderr, "A positive integer for bin size must be specified.\n");
		free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(outputBasename);
		exit(1);
	}
	if( (data=getUNCSlices3D(inputFilename, 0, 0, &W, &H, NULL, &actualNumSlices, &depth, &format)) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
		exit(1);
	}
	if( numSlices == 0 ){ numSlices = actualNumSlices; allSlices = 1; }
	else {
		for(i=0;i<numSlices;i++){
			if( slices[i] >= actualNumSlices ){
				fprintf(stderr, "%s : slice numbers must not exceed %d, the total number of slices in file '%s'.\n", argv[0], actualNumSlices, inputFilename);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
				exit(1);
			} else if( slices[i] < 0 ){
				fprintf(stderr, "%s : slice numbers must start from 1.\n", argv[0]);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
				exit(1);
			}
		}
	}

	if( w <= 0 ) w = W; if( h <= 0 ) h = H;
	if( (x+w) > W ){
		fprintf(stderr, "%s : region of interest falls outside image width (%d).\n", argv[0], W);
		free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}
	if( (y+h) > H ){
		fprintf(stderr, "%s : region of interest falls outside image height (%d).\n", argv[0], H);
		free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
		freeDATATYPE3D(data, actualNumSlices, W);
		exit(1);
	}

	/* number of pixels per slice and numPixels[numSlices] has overall number of pixels over all slices */
	if( (numPixels=(int *)malloc((numSlices+1) * sizeof(int))) == NULL ){
		fprintf(stderr, "%s : call to getUNCSlices3D has failed for file '%s'.\n", argv[0], inputFilename);
		free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename);
		exit(1);
	}		

	if( verboseFlag )
		printf("%s : calculating histogram with region (x=%d,y=%d,w=%d,h=%d), bin size %d%s%s%s%s\n", inputFilename, x, y, w, h, binSize, normaliseFlag?strdup(", normalised"):"", (smoothWindowSize>0)?strdup(", smoothed"):"", blackPixelNotCountedNormalisationFlag?strdup(", black pixel ignored during normalisation"):"", blackPixelNotCountedFlag?strdup(", histogram data does not contain the black pixel."):strdup("."));

	if( histogramsForEachSliceFlag || (actualNumSlices==1) ){
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			if( (hist=histogram2D(data[slice], x, y, w, h, binSize)) == NULL ){
				fprintf(stderr, "%s : call to histogram2D has failed for file '%s', slice was '%d', region of interest was (%d,%d,%d,%d).\n", argv[0], inputFilename, slice+1, x, y, w, h);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
				freeDATATYPE3D(data, actualNumSlices, W);
				exit(1);
			}
			if( blackPixelNotCountedNormalisationFlag ){
				hist->bins[0] = 0;
				for(i=hist->minPixel,j=0;i<=0;i++,j++) hist->bins[j] = 0;
			}
			
			numPixels[s] = 0;
			for(i=0;i<hist->numBins;i++) numPixels[s] += hist->bins[i];

			if( smoothWindowSize > 0 )
				if( histogram_smooth(hist, smoothWindowSize, FALSE) == FALSE ){
					fprintf(stderr, "%s : call to histogram_smooth has failed.\n", argv[0]);
					free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
					freeDATATYPE3D(data, actualNumSlices, W);
					destroy_histogram(hist);
					exit(1);
				}
			if( normaliseFlag == TRUE )
				if( histogram_normalise(hist, (smoothWindowSize>0) ) == FALSE ){
					fprintf(stderr, "%s : call to histogram_normalise has failed.\n", argv[0]);
					free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
					freeDATATYPE3D(data, actualNumSlices, W);
					destroy_histogram(hist);
					exit(1);
				}
			calculate_histogram(hist);

			if( normaliseFlag == TRUE ){
				peak = hist->normalised_peak;
				p25 = hist->normalised_p25;
				p50 = hist->normalised_p50;
				p75 = hist->normalised_p75;
				peakValue = hist->normalised[peak-hist->minPixel]; p25Value = hist->normalised[p25-hist->minPixel]; p50Value = hist->normalised[p50-hist->minPixel]; p75Value = hist->normalised[p75-hist->minPixel];
				meanFrequency = hist->normalised_meanFrequency; stdevFrequency = hist->normalised_stdevFrequency;
				minFrequency = hist->normalised_minFrequency; maxFrequency = hist->normalised_maxFrequency;
				frequencyOfMinPixel = hist->normalised[0]; frequencyOfMaxPixel = hist->normalised[hist->maxPixel-hist->minPixel];
				pixelOfMinFrequency = hist->pixel_normalised_minFrequency; pixelOfMaxFrequency = hist->pixel_normalised_maxFrequency;
				minActivePixel = hist->normalised_minActivePixel; maxActivePixel = hist->normalised_maxActivePixel;
			} else {
				if( smoothWindowSize > 0 ){
					peak = hist->smooth_peak;
					p25 = hist->smooth_p25;
					p50 = hist->smooth_p50;
					p75 = hist->smooth_p75;
					peakValue = hist->smooth[peak-hist->minPixel]; p25Value = hist->smooth[p25-hist->minPixel]; p50Value = hist->smooth[p50-hist->minPixel]; p75Value = hist->smooth[p75-hist->minPixel];
					meanFrequency = hist->smooth_meanFrequency; stdevFrequency = hist->smooth_stdevFrequency;
					minFrequency = hist->smooth_minFrequency; maxFrequency = hist->smooth_maxFrequency;
					frequencyOfMinPixel = hist->smooth[0]; frequencyOfMaxPixel = hist->smooth[hist->maxPixel-hist->minPixel];
					pixelOfMinFrequency = hist->pixel_smooth_minFrequency; pixelOfMaxFrequency = hist->pixel_smooth_maxFrequency;
					minActivePixel = hist->smooth_minActivePixel; maxActivePixel = hist->smooth_maxActivePixel;
				} else {
					peak = hist->peak;
					p25 = hist->p25;
					p50 = hist->p50;
					p75 = hist->p75;
					peakValue = (float )(hist->bins[peak-hist->minPixel]); p25Value = (float )(hist->bins[p25-hist->minPixel]); p50Value = (float )(hist->bins[p50-hist->minPixel]); p75Value = (float )(hist->bins[p75-hist->minPixel]);
					meanFrequency = hist->meanFrequency; stdevFrequency = hist->stdevFrequency;
					minFrequency = (float )(hist->minFrequency); maxFrequency = (float )(hist->maxFrequency);
					frequencyOfMinPixel = (float )(hist->bins[0]); frequencyOfMaxPixel = (float )(hist->bins[hist->maxPixel-hist->minPixel]);
					pixelOfMinFrequency = hist->pixel_minFrequency; pixelOfMaxFrequency = hist->pixel_maxFrequency;
					minActivePixel = hist->minActivePixel; maxActivePixel = hist->maxActivePixel;
				}
			}
			/* pad 0 to complete 1,2,3 digit - but only if more than 1 slice requested */
			if( actualNumSlices > 1 ){
/* no no do not do padding if you want padding, uncomment the following 5 lines */
/*				if( numSlices > 100 )
					sprintf(outputFilename, "%s_%03d", outputBasename, slice+1);
				else if( numSlices > 10 )
					sprintf(outputFilename, "%s_%02d", outputBasename, slice+1);
				else */sprintf(outputFilename, "%s_%d", outputBasename, slice+1);
			} else strcpy(outputFilename, outputBasename);

			if( verboseFlag )
				printf("%s : slice %d, histogram file '%s'\n", inputFilename, slice+1, outputFilename);
			if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
				fprintf(stderr, "%s : could not open file '%s' for writing for slice '%d'.\n", argv[0], outputFilename, slice+1);
				freeDATATYPE3D(data, actualNumSlices, W);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
				exit(1);
			}
			if( blackPixelNotCountedFlag ) hist->bins[0] = 0;

			/* put a comment on top which is also the title */
			/* if your plotter does not understand comments do not use -t */
			if( title ){
				fprintf(outputHandle, "%s %s, slice %d, region (x=%d,y=%d,w=%d,h=%d), bin size %d, (min,max) = (%d, %d)%s%s%s%s\n", commentDesignator, inputFilename, slice+1, x, y, w, h, binSize, hist->minPixel, hist->maxPixel, normaliseFlag?strdup(", normalised"):"", (smoothWindowSize>0)?strdup(", smoothed"):"", blackPixelNotCountedNormalisationFlag?strdup(", black pixel ignored during normalisation"):"", blackPixelNotCountedFlag?strdup(", histogram data does not contain the black pixel."):strdup("."));
				fprintf(outputHandle, "%s intensity stats: min(%d, %f), max(%d, %f)\n", commentDesignator, hist->minPixel, frequencyOfMinPixel, hist->maxPixel, frequencyOfMaxPixel);
				fprintf(outputHandle, "%s frequency stats: min(%d, %f), max(%d, %f)\n", commentDesignator, pixelOfMinFrequency, minFrequency, pixelOfMaxFrequency*binSize, maxFrequency);
				fprintf(outputHandle, "%s active pixel range (%d to %d)\n", commentDesignator, minActivePixel*binSize, maxActivePixel*binSize);
				fprintf(outputHandle, "%s peak=(%d, %f)\n", commentDesignator, peak * binSize, peakValue);
				fprintf(outputHandle, "%s histogram mean: %f\n", commentDesignator, hist->histogram_mean);
				fprintf(outputHandle, "%s mean=%f, stdev=%f\n", commentDesignator, meanFrequency, stdevFrequency);
				fprintf(outputHandle, "%s frequency percentiles: p25=(%d, %f) p50=median=(%d, %f) and p75=(%d, %f)\n", commentDesignator, p25*binSize, p25Value, p50*binSize, p50Value, p75*binSize, p75Value);
				fprintf(outputHandle, "%s pixel percentiles: p25=%d p50=median=%d and p75=%d\n", commentDesignator, hist->pixel_p25*binSize, hist->pixel_p50*binSize, hist->pixel_p75*binSize);
				fprintf(outputHandle, "%s number of pixels counted: %d\n", commentDesignator, numPixels[s]);
				fprintf(outputHandle, "%s 'HIST.MEAN' represents the average pixel intensity value in the image - e.g. sum up all the pixel values and then divide by the number of pixels. It is an intensity value and therefore is placed on the x-axis.\n", commentDesignator);
				fprintf(outputHandle, "%s plain 'MEAN' represents the sum of all the frequencies (e.g. the contents of each bin) divided by the number of bins. Now, the sum of all frequencies is equal to the number of pixels in the image. Therefore this value represents the value that each bin should contain if the histogram was flat (e.g. all bins contain the same number of pixels).\n", commentDesignator);

				for(i=0;i<numAdditionalComments;i++)
					fprintf(outputHandle, "%s %s\n", commentDesignator, additionalComments[i]);
			}
			if( doCumulativeFlag ){
				cum = 0.0;
				sub = doReverseCumulativeFlag == TRUE ? hist->numBins : 0;
				if( normaliseFlag )
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((ABS(sub-i)*binSize + hist->minPixel)<0) ) continue;
						cum += hist->normalised[ABS(sub-i)];
						fprintf(outputHandle, "%d\t%f\n", ABS(sub-i)*binSize + hist->minPixel, cum);
					}
				else {
					if( smoothWindowSize > 0 )
						for(i=0;i<hist->numBins;i++){
							if( blackPixelNotCountedNormalisationFlag && ((ABS(sub-i)*binSize + hist->minPixel)<0) ) continue;
							cum += hist->smooth[ABS(sub-i)];
							fprintf(outputHandle, "%d\t%f\n", ABS(sub-i)*binSize + hist->minPixel, cum);
						}
					else
						for(i=0;i<hist->numBins;i++){
							if( blackPixelNotCountedNormalisationFlag && ((ABS(sub-i)*binSize + hist->minPixel)<0) ) continue;
							cum += (float )(hist->bins[ABS(sub-i)]);
							fprintf(outputHandle, "%d\t%f\n", ABS(sub-i)*binSize + hist->minPixel, cum);
						}
				}
			} else {
				if( normaliseFlag )
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
						fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, hist->normalised[i]);
					}
				else {
					if( smoothWindowSize > 0 )
						for(i=0;i<hist->numBins;i++){
							if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
							fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, hist->smooth[i]);
						}
					else
						for(i=0;i<hist->numBins;i++){
							if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
							fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, (float )(hist->bins[i]));
						}
				}
			}

			if( verboseFlag ){
				printf("  histogram mean: %f\n", hist->histogram_mean);
				printf("  peak=(pixel value=%d, frequency=%f)\n", peak*binSize, peakValue);
				printf("  active pixel range (%d to %d)\n", minActivePixel*binSize, maxActivePixel*binSize);
				printf("  mean frequency=%f, stdev of frequency=%f\n", meanFrequency, stdevFrequency);
				printf("  frequency percentiles p25=(pv=%d, fr=%f) p50=median=(pv=%d, fr=%f) and p75=(pv=%d, fr=%f)\n", p25*binSize, p25Value, p50*binSize, p50Value, p75*binSize, p75Value);
				printf("  pixel percentiles: p25=%d p50=median=%d and p75=%d\n", hist->pixel_p25*binSize, hist->pixel_p50*binSize, hist->pixel_p75*binSize);
				printf("  number of pixels counted: %d\n", numPixels[s]);
			}
			destroy_histogram(hist);
			fclose(outputHandle);
		}
	} /* histograms for each slice */

	if( (numSlices > 1) && wholeImageFlag ){
		printf("%s, collective histogram for slice : ", inputFilename);
		if( (data2=callocDATATYPE3D(numSlices, w, h)) == NULL ){
			fprintf(stderr, "%s : could not allocate %d x %d x %d bytes.\n", argv[0], numSlices, w, h);
			free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
			freeDATATYPE3D(data, actualNumSlices, W);
			exit(1);
		}
		for(s=0;s<numSlices;s++){
			slice = (allSlices==0) ? slices[s] : s;
			printf("%d ", slice+1); fflush(stdout);
			for(i=x,ii=0;i<x+w;i++,ii++) for(j=y,jj=0;j<y+h;j++,jj++) data2[s][ii][jj] = data[slice][i][j];
		}
		if( (hist=histogram3D(data2, 0, 0, 0, w, h, numSlices, binSize)) == NULL ){
			fprintf(stderr, "%s : call to histogram3D has failed for file '%s', whole brain histogram, region of interest was (%d,%d,%d,%d).\n", argv[0], inputFilename, x, y, w, h);
			free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
			freeDATATYPE3D(data, actualNumSlices, W);
			freeDATATYPE3D(data2, numSlices, w);
			exit(1);
		}
		printf("\n");
		freeDATATYPE3D(data2, numSlices, w);
		if( blackPixelNotCountedNormalisationFlag ){
			hist->bins[0] = 0;
			for(i=hist->minPixel,j=0;i<=0;i++,j++) hist->bins[j] = 0;
		}
		numPixels[numSlices] = 0;
		for(i=0;i<hist->numBins;i++) numPixels[numSlices] += hist->bins[i];

		if( smoothWindowSize > 0 )
			if( histogram_smooth(hist, smoothWindowSize, FALSE) == FALSE ){
				fprintf(stderr, "%s : call to histogram_smooth has failed.\n", argv[0]);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
				freeDATATYPE3D(data, actualNumSlices, W);
				destroy_histogram(hist);
				exit(1);
			}
		if( normaliseFlag == TRUE )
			if( histogram_normalise(hist, (smoothWindowSize>0) ) == FALSE ){
				fprintf(stderr, "%s : call to histogram_normalise has failed.\n", argv[0]);
				free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
				freeDATATYPE3D(data, actualNumSlices, W);
				destroy_histogram(hist);
				exit(1);
			}
		calculate_histogram(hist);
		if( normaliseFlag == TRUE ){
			peak = hist->normalised_peak; p25 = hist->normalised_p25; p50 = hist->normalised_p50; p75 = hist->normalised_p75;
			peakValue = hist->normalised[peak-hist->minPixel]; p25Value = hist->normalised[p25-hist->minPixel]; p50Value = hist->normalised[p50-hist->minPixel]; p75Value = hist->normalised[p75-hist->minPixel];
			meanFrequency = hist->normalised_meanFrequency; stdevFrequency = hist->normalised_stdevFrequency;
			minFrequency = hist->normalised_minFrequency; maxFrequency = hist->normalised_maxFrequency;
			frequencyOfMinPixel = hist->normalised[0]; frequencyOfMaxPixel = hist->normalised[hist->maxPixel-hist->minPixel];
			pixelOfMinFrequency = hist->pixel_normalised_minFrequency; pixelOfMaxFrequency = hist->pixel_normalised_maxFrequency;
			minActivePixel = hist->normalised_minActivePixel; maxActivePixel = hist->normalised_maxActivePixel;
		} else {
			if( smoothWindowSize > 0 ){
				peak = hist->smooth_peak; p25 = hist->smooth_p25; p50 = hist->smooth_p50; p75 = hist->smooth_p75;
				peakValue = hist->smooth[peak-hist->minPixel]; p25Value = hist->smooth[p25-hist->minPixel]; p50Value = hist->smooth[p50-hist->minPixel]; p75Value = hist->smooth[p75-hist->minPixel];
				meanFrequency = hist->smooth_meanFrequency; stdevFrequency = hist->smooth_stdevFrequency;
				minFrequency = hist->smooth_minFrequency; maxFrequency = hist->smooth_maxFrequency;
				frequencyOfMinPixel = hist->smooth[0]; frequencyOfMaxPixel = hist->smooth[hist->maxPixel-hist->minPixel];
				pixelOfMinFrequency = hist->pixel_smooth_minFrequency; pixelOfMaxFrequency = hist->pixel_smooth_maxFrequency;
				minActivePixel = hist->smooth_minActivePixel; maxActivePixel = hist->smooth_maxActivePixel;
			} else {
				peak = hist->peak; p25 = hist->p25; p50 = hist->p50; p75 = hist->p75;
				peakValue = (float )(hist->bins[peak-hist->minPixel]); p25Value = (float )(hist->bins[p25-hist->minPixel]); p50Value = (float )(hist->bins[p50-hist->minPixel]); p75Value = (float )(hist->bins[p75-hist->minPixel]);
				meanFrequency = hist->meanFrequency; stdevFrequency = hist->stdevFrequency;
				minFrequency = hist->minFrequency; maxFrequency = hist->maxFrequency;
				frequencyOfMinPixel = (float )(hist->bins[0]); frequencyOfMaxPixel = (float )(hist->bins[hist->maxPixel-hist->minPixel]);
				pixelOfMinFrequency = hist->pixel_minFrequency; pixelOfMaxFrequency = hist->pixel_maxFrequency;
				minActivePixel = hist->minActivePixel; maxActivePixel = hist->maxActivePixel;
			}
		}

		sprintf(outputFilename, "%s_whole", outputBasename);

		if( verboseFlag )
			printf("%s : whole brain, histogram file '%s'\n", inputFilename, outputFilename);
		if( (outputHandle=fopen(outputFilename, "w")) == NULL ){
			fprintf(stderr, "%s : could not open file '%s' for writing for whole image histogram.\n", argv[0], outputFilename);
			freeDATATYPE3D(data, actualNumSlices, W);
			free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
			exit(1);
		}
		/* put a comment on top which is also the title */
		/* if your plotter does not understand comments do not use -t */
		if( title ){
			fprintf(outputHandle, "%s %s, whole brain histogram, slices:", commentDesignator, inputFilename);
			for(s=0;s<numSlices;s++) fprintf(outputHandle, " %d", (allSlices==0) ? slices[s] : s);
			fprintf(outputHandle, ", region (x=%d,y=%d,w=%d,h=%d), bin size %d, (min,max) = (%d, %d)%s%s%s%s\n", x, y, w, h, binSize, hist->minPixel, hist->maxPixel, normaliseFlag?strdup(", normalised"):"", (smoothWindowSize>0)?strdup(", smoothed"):"", blackPixelNotCountedNormalisationFlag?strdup(", black pixel ignored during normalisation"):"", blackPixelNotCountedFlag?strdup(", histogram data does not contain the black pixel."):strdup("."));
			fprintf(outputHandle, "%s intensity stats: min(%d, %f), max(%d, %f)\n", commentDesignator, hist->minPixel, frequencyOfMinPixel, hist->maxPixel, frequencyOfMaxPixel);
			fprintf(outputHandle, "%s frequency stats: min(%d, %f), max(%d, %f)\n", commentDesignator, pixelOfMinFrequency, minFrequency, pixelOfMaxFrequency*binSize, maxFrequency);
			fprintf(outputHandle, "%s active pixel range (%d to %d)\n", commentDesignator, minActivePixel*binSize, maxActivePixel*binSize);
			fprintf(outputHandle, "%s histogram mean: %f\n", commentDesignator, hist->histogram_mean);
			fprintf(outputHandle, "%s peak=(%d, %f)\n", commentDesignator, peak*binSize, peakValue);
			fprintf(outputHandle, "%s mean=%f, stdev=%f\n", commentDesignator, meanFrequency, stdevFrequency);
			fprintf(outputHandle, "%s frequency percentiles: p25=(%d, %f) p50=median=(%d, %f) and p75=(%d, %f)\n", commentDesignator, p25*binSize, p25Value, p50*binSize, p50Value, p75*binSize, p75Value);
			fprintf(outputHandle, "%s pixel percentiles: p25=%d p50=median=%d and p75=%d\n", commentDesignator, hist->pixel_p25*binSize, hist->pixel_p50*binSize, hist->pixel_p75*binSize);
			fprintf(outputHandle, "%s number of pixels counted: %d\n", commentDesignator, numPixels[numSlices]);
			fprintf(outputHandle, "%s 'HIST.MEAN' represents the average pixel intensity value in the image - e.g. sum up all the pixel values and then divide by the number of pixels. It is an intensity value and therefore is placed on the x-axis.\n", commentDesignator);
			fprintf(outputHandle, "%s plain 'MEAN' represents the sum of all the frequencies (e.g. the contents of each bin) divided by the number of bins. Now, the sum of all frequencies is equal to the number of pixels in the image. Therefore this value represents the value that each bin should contain if the histogram was flat (e.g. all bins contain the same number of pixels).\n", commentDesignator);
			for(i=0;i<numAdditionalComments;i++)
				fprintf(outputHandle, "%s %s\n", commentDesignator, additionalComments[i]);
		}
		if( blackPixelNotCountedFlag ) hist->bins[0] = 0;

		/* count number of active pixels */
		
		if( doCumulativeFlag ){
			cum = 0.0;
			if( normaliseFlag )
				for(i=0;i<hist->numBins;i++){
					if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
					cum += hist->normalised[i];
					fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, cum);
				}
			else {
				if( smoothWindowSize > 0 )
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
						cum += hist->smooth[i];
						fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, cum);
					}
				else
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
						cum += (float )(hist->bins[i]);
						fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, cum);
					}
			}
		} else {
			if( normaliseFlag )
				for(i=0;i<hist->numBins;i++){
					if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
					fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, hist->normalised[i]);
				}
			else {
				if( smoothWindowSize > 0 )
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
						fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, hist->smooth[i]);
					}
				else
					for(i=0;i<hist->numBins;i++){
						if( blackPixelNotCountedNormalisationFlag && ((i*binSize + hist->minPixel)<0) ) continue;
						fprintf(outputHandle, "%d\t%f\n", i*binSize + hist->minPixel, (float )(hist->bins[i]));
					}
			}
		}
		if( verboseFlag ){
			printf("  intensity stats: min(%d, %f), max(%d, %f)\n", hist->minPixel, frequencyOfMinPixel, hist->maxPixel, frequencyOfMaxPixel);
			printf("  frequency stats: min(%d, %f), max(%d, %f)\n", pixelOfMinFrequency, minFrequency, pixelOfMaxFrequency*binSize, maxFrequency);
			printf("  active pixel range (%d to %d)\n", minActivePixel*binSize, maxActivePixel*binSize);
			printf("  histogram mean: %f\n", hist->histogram_mean);
			printf("  peak=(pixel value=%d, frequency=%f)\n", peak*binSize, peakValue);
			printf("  mean frequency=%f, stdev of frequency=%f\n", meanFrequency, stdevFrequency);
			printf("  frequency percentiles p25=(pv=%d, fr=%f) p50=median=(pv=%d, fr=%f) and p75=(pv=%d, fr=%f)\n", p25*binSize, p25Value, p50*binSize, p50Value, p75*binSize, p75Value);
			printf("  pixel percentiles: p25=%d p50=median=%d and p75=%d\n", hist->pixel_p25*binSize, hist->pixel_p50*binSize, hist->pixel_p75*binSize);
			printf("  number of pixels counted: %d\n", numPixels[numSlices]);
		}
		destroy_histogram(hist);
		fclose(outputHandle);
	} /* histogram for whole brain */

	free(inputFilename); for(i=0;i<numAdditionalComments;i++) free(additionalComments[i]); free(commentDesignator); free(outputBasename); free(numPixels);
	freeDATATYPE3D(data, actualNumSlices, W);
	exit(0);
}
