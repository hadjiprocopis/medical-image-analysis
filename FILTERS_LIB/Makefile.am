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
        -L$(top_srcdir)/CELLULAR_AUTOMATA\
        -L$(top_srcdir)/UTILS/ANALYZE

lib_LIBRARIES = libfilters.a
libfilters_a_SOURCES = normalise.c otsu.c histogram.c filters2D.c filters3D.c statistics_float.c statistics1D.c statistics2D.c statistics3D.c maps.c threshold.c erosion.c random_numbers.c combinatorics.c
libfilters_a_CFLAGS = $(AM_CFLAGS)

lib_LTLIBRARIES = libfilters.la
libfilters_la_SOURCES = $(libfilters_a_SOURCES)
libfilters_la_LDFLAGS = -export-dynamic $(LDFLAGS)

pkgincludedir = $(includedir)/$(pkgname)
pkginclude_HEADERS = normalise.h otsu.h filters.h threshold.h statistics.h maps.h erosion.h histogram.h filters3D.h filters2D.h random_numbers.h combinatorics.h private_filters2D.h private_filters3D.h threshold_private.h
