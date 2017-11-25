OPTIONS_AUTOMAKE=gnu

AM_CPPFLAGS=\
	-I$(top_srcdir)/COMMON\
	-I$(top_srcdir)/libim\
	-I$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
	-I$(top_srcdir)/DATA_STRUCTURES/MATRIX\
	-I$(top_srcdir)/ALLOC_LIB\
	-I$(top_srcdir)/DATA_READERS\
	-I$(top_srcdir)/IO_LIB\
	-I$(top_srcdir)/IO_LIB/UNC\
	-I$(top_srcdir)/IO_LIB/ANALYZE\
	-I$(top_srcdir)/IO_LIB/MISC\
	-I$(top_srcdir)/IO_LIB/ROI\
	-I$(top_srcdir)/CONTOUR\
	-I$(top_srcdir)/FILTERS_LIB\
	-I$(top_srcdir)/SPIRAL\
	-I$(top_srcdir)/ROI\
	-I$(top_srcdir)/RING\
	-I$(top_srcdir)/SPLINE\
	-I$(top_srcdir)/CLUSTERING_3D\
	-I$(top_srcdir)/REGISTRATION\
	-I$(top_srcdir)/CELLULAR_AUTOMATA

AM_LDFLAGS=\
	-L$(top_srcdir)/COMMON\
	-L$(top_srcdir)/libim\
	-L$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
	-L$(top_srcdir)/DATA_STRUCTURES/MATRIX\
	-L$(top_srcdir)/ALLOC_LIB\
	-L$(top_srcdir)/DATA_READERS\
	-L$(top_srcdir)/IO_LIB/UNC\
	-L$(top_srcdir)/IO_LIB/ANALYZE\
	-L$(top_srcdir)/IO_LIB/MISC\
	-L$(top_srcdir)/IO_LIB/ROI\
	-L$(top_srcdir)/CONTOUR\
	-L$(top_srcdir)/FILTERS_LIB\
	-L$(top_srcdir)/SPIRAL\
	-L$(top_srcdir)/ROI\
	-L$(top_srcdir)/RING\
	-L$(top_srcdir)/SPLINE\
	-L$(top_srcdir)/CLUSTERING_3D\
	-L$(top_srcdir)/REGISTRATION\
	-L$(top_srcdir)/CELLULAR_AUTOMATA
# for jpeglib
AM_CFLAGS = -I/sw/include

AM_LDFLAGS += -lim -luncio -lalloc -lfilters -llinked_list -lcontour -lm -ltiff -ljpeg

bin_PROGRAMS = \
	ASCIIclustering\
	Combinations\
	Merge\
	Normalise\
	Permutations\
	ProduceRandomFilename\
	ascii2unc\
	fft\
	nativeSpace2reslicedSpace\
	ppm2tiff\
	raw2unc\
	shuffle

ASCIIclustering_SOURCES = ASCIIclustering.c
ASCIIclustering_LDADD = -lclustering
Combinations_SOURCES = Combinations.c
Merge_SOURCES = Merge.c
Normalise_SOURCES = Normalise.c
Normalise_LDADD = -lfilters
Permutations_SOURCES = Permutations.c
ProduceRandomFilename_SOURCES = ProduceRandomFilename.c
ascii2unc_SOURCES = ascii2unc.c
fft_SOURCES = fft.c
fft_LDADD = ${FFTW3_LIBS}
fft_CFLAGS = ${FFTW3_CFLAGS}
nativeSpace2reslicedSpace_SOURCES = nativeSpace2reslicedSpace.c
nativeSpace2reslicedSpace_LDADD = -lregistration
ppm2tiff_SOURCES = ppm2tiff.c
ppm2tiff_LDADD = -ltiff
raw2unc_SOURCES = raw2unc.c
shuffle_SOURCES = shuffle.c
	
