OPTIONS_AUTOMAKE=gnu

AM_CPPFLAGS= -I$(top_srcdir) -I$(top_srcdir)/FILTERS_LIB -I$(top_srcdir)/COMMON -I$(top_srcdir)/IO_LIB -I$(top_srcdir)/IO_LIB/ANALYZE -I$(top_srcdir)/IO_LIB/MISC -I$(top_srcdir)/IO_LIB/UNC -I$(top_srcdir)/IO_LIB/ROI -I$(top_srcdir)/SPIRAL -I$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST -I$(top_srcdir)/libim -I$(top_srcdir)/ALLOC_LIB -I$(top_srcdir)/CLUSTERING_3D
AM_CFLAGS = -I$(top_srcdir)
AM_LDFLAGS = -L$(top_srcdir) -L$(top_srcdir)/FILTERS_LIB -L$(top_srcdir)/COMMON -L$(top_srcdir)/IO_LIB -L$(top_srcdir)/IO_LIB/ANALYZE -L$(top_srcdir)/IO_LIB/MISC -L$(top_srcdir)/IO_LIB/UNC -L$(top_srcdir)/IO_LIB/ROI -L$(top_srcdir)/SPIRAL -L$(top_srcdir)/DATA_STRUCTURES/LINKED_LIST -L$(top_srcdir)/libim -L$(top_srcdir)/ALLOC_LIB -L$(top_srcdir)/CLUSTERING_3D
AM_LDFLAGS += -lim -lmiscio -luncio -lanalyzeio -lalloc -lfilters -lcontour -ltiff -ljpeg

bin_PROGRAMS = ANALYZE2unc headconv headshow imgconv
ANALYZE2unc_SOURCES =  ANALYZE2unc.c
headconv_SOURCES = headconv.c
headshow_SOURCES = headshow.c
imgconv_SOURCES = imgconv.c
