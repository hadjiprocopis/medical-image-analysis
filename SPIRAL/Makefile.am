OPTIONS_AUTOMAKE=gnu
ACLOCAL_AMFLAGS=-I ./m4
AM_CPPFLAGS=\
        -I$(top_srcdir)/COMMON\
        -I$(top_srcdir)/libim\
        -I$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST\
        -I$(top_srcdir)/ALLOC_LIB\
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

lib_LIBRARIES = libspiral.a
libspiral_a_SOURCES =  spiral.c
libspiral_a_CFLAGS = $(AM_CFLAGS)

lib_LTLIBRARIES = libspiral.la
libspiral_la_SOURCES = $(libspiral_a_SOURCES)
libspiral_la_LDFLAGS = -export-dynamic $(LDFLAGS)
pkgincludedir = $(includedir)/$(pkgname)
pkginclude_HEADERS =  spiral.h
