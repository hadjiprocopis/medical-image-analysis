OPTIONS_AUTOMAKE=gnu

AM_CPPFLAGS=\
	-I$(top_srcdir)/COMMON\
	-I$(top_srcdir)/libim\
	-I$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
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

AM_LDFLAGS += -lim -luncio -lalloc -lroiio -lroi -lspiral -lfilters -lcontour -ltiff -ljpeg

bin_PROGRAMS = \
	ROIextract\
	ROIfilter\
	ROIsmoothSegmentation\
	ROIstats

ROIextract_SOURCES = ROIextract.c
ROIsmoothSegmentation_SOURCES = ROIsmoothSegmentation.c
ROIfilter_SOURCES = ROIfilter.c
ROIstats_SOURCES = ROIstats.c
