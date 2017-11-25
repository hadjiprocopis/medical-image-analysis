OPTIONS_AUTOMAKE=gnu
ACLOCAL_AMFLAGS=-I ./m4

AM_CPPFLAGS=\
        -I$(top_srcdir)/COMMON\
        -I$(top_srcdir)/libim\
        -I$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
        -I$(top_srcdir)/ALLOC_LIB\
        -I$(top_srcdir)/IO_LIB/UNC\
        -I$(top_srcdir)/IO_LIB/ANALYZE\
        -I$(top_srcdir)/IO_LIB/MISC\
        -I$(top_srcdir)/CONTOUR\
        -I$(top_srcdir)/FILTERS_LIB\
        -I$(top_srcdir)/SPIRAL\
        -I$(top_srcdir)/ROI\
        -I$(top_srcdir)/RING\
        -I$(top_srcdir)/SPLINE\
        -I$(top_srcdir)/CLUSTERING_3D\
        -I$(top_srcdir)/CELLULAR_AUTOMATA\
        -I$(top_srcdir)/UTILS/ANALYZE

AM_LDFLAGS=-fPIC\
        -L$(top_srcdir)/COMMON\
        -L$(top_srcdir)/libim\
        -L$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
        -L$(top_srcdir)/ALLOC_LIB\
        -L$(top_srcdir)/IO_LIB/UNC\
        -L$(top_srcdir)/IO_LIB/ANALYZE\
        -L$(top_srcdir)/IO_LIB/MISC\
        -L$(top_srcdir)/CONTOUR\
        -L$(top_srcdir)/FILTERS_LIB\
        -L$(top_srcdir)/SPIRAL\
        -L$(top_srcdir)/ROI\
        -L$(top_srcdir)/RING\
        -L$(top_srcdir)/SPLINE\
        -L$(top_srcdir)/CLUSTERING_3D\
        -L$(top_srcdir)/CELLULAR_AUTOMATA\
        -L$(top_srcdir)/UTILS/ANALYZE

lib_LIBRARIES = libdata_readers.a
libdata_readers_a_SOURCES = histogram.c
libdata_readers_a_CFLAGS = $(AM_CFLAGS)
#libdata_readers_a_LDFLAGS = $(AM_LDFLAGS) -fPIC
#libdata_readers_a_LIBADD = parser/histogram/libdata_readers_histogram_parser_a-Tokenizer.o
libdata_readers_a_LIBADD = parser/histogram/libdata_readers_histogram_parser.a

lib_LTLIBRARIES = libdata_readers.la
libdata_readers_la_SOURCES = $(libdata_readers_a_SOURCES)
libdata_readers_la_LDFLAGS = -export-dynamic $(LDFLAGS) -fPIC
#libdata_readers_la_LIBADD = parser/histogram/libdata_readers_histogram_parser_a-Tokenizer.o
libdata_readers_la_LIBADD = parser/histogram/libdata_readers_histogram_parser_a-Grammar.o \
				parser/histogram/libdata_readers_histogram_parser_a-Tokenizer.o

pkgincludedir = $(includedir)/$(pkgname)
pkginclude_HEADERS = data_readers.h
