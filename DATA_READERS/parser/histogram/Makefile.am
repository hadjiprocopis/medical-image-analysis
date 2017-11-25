OPTIONS_AUTOMAKE=gnu
SUFFIXES = .c .h .y .l
AM_YFLAGS = -d
ACLOCAL_AMFLAGS=-I ./m4
# when -Werror, it complains about Tokenizer.l, so remove temporarily and ignore studid bootstrap.sh warnings

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
        -I$(top_srcdir)/CELLULAR_AUTOMATA

AM_LDFLAGS=\
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
        -L$(top_srcdir)/CELLULAR_AUTOMATA

# for jpeglib and tiff.h
AM_CPPFLAGS += -I/sw/include
AM_LDFLAGS += -L/sw/lib -lCommon_IMMA -fPIC
AM_CFLAGS = -fPIC

lib_LIBRARIES = libdata_readers_histogram_parser.a
libdata_readers_histogram_parser_a_SOURCES = Tokenizer.l Grammar.y
libdata_readers_histogram_parser_a_CFLAGS = $(AM_CFLAGS)
#libdata_readers_histogram_parser_a_LDFLAGS = -fPIC

# extra clean target for yaccs
clean-local :
	rm -f Grammar.c Tokenizer.c t.tab.c

.PHONY: clean-yacc-extra
clean-local : clean-yacc-extra
