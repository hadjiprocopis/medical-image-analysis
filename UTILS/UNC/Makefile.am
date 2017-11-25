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
	-L$(top_srcdir)/CELLULAR_AUTOMATA
# for jpeglib
AM_CFLAGS = -I/sw/include

# -lfl is for lexer yacc library (flex)
AM_LDFLAGS += -lim -luncio -lalloc -ldata_readers -lcontour -lfilters -lspiral -ljpeg -ltiff -lfl -ly -lm

bin_PROGRAMS = \
	UNC2analyze\
	UNC2jpeg\
	UNC2raw\
	UNC2tiff\
	UNCascii\
	UNCblow_pixels\
	UNCboundingBox\
	UNCcalculateAccuracyOfTissueSegmentation\
	UNCcalculateAccuracyOfTissueSegmentation2\
	UNCcellular_automaton\
	UNCclear_cut_clusters\
	UNCclustering3\
	UNCcompare_prediction_with_standard\
	UNCconnected\
	UNCcontour\
	UNCcopy_header_info\
	UNCcrap\
	UNCcrop\
	UNCdiff\
	UNCdilate\
	UNCdiscrete\
	UNCdistance_from_brain_boundaries\
	UNCdump_header\
	UNCenhanceB0\
	UNCentropyFromProbabilityMaps\
	UNCerode\
	UNCextractSlices\
	UNCfilter2D\
	UNCfilter3D\
	UNCgradient\
	UNChistogram\
	UNCmake\
	UNCmap\
	UNCmask\
	UNCmatchHistogram\
	UNCmaxLikelihood\
	UNCnseg\
	UNCop\
	UNCotsu\
	UNCoutline\
	UNCprobabilityMapsToRGB\
	UNCpv2ma\
	UNCpv2mb\
	UNCremove_header\
	UNCring\
	UNCscale_pixels\
	UNCset_header\
	UNCsimulatedAnnealing\
	UNCsmoothSegmentation\
	UNCspiral\
	UNCstatistics\
	UNCstatistics_extended\
	UNCtest\
	UNCthreshold

#UNCclustering2
#UNCclustering
#UNCmarkov

UNC2analyze_SOURCES = UNC2analyze.c
UNC2analyze_LDADD = -lanalyzeio -lmiscio -lfilters

UNC2jpeg_SOURCES = UNC2jpeg.c
UNC2jpeg_LDADD = -ljpeg
UNC2raw_SOURCES = UNC2raw.c
UNC2tiff_SOURCES = UNC2tiff.c
UNCascii_SOURCES = UNCascii.c
UNCblow_pixels_SOURCES = UNCblow_pixels.c
UNCboundingBox_SOURCES = UNCboundingBox.c
UNCcalculateAccuracyOfTissueSegmentation_SOURCES = UNCcalculateAccuracyOfTissueSegmentation.c
UNCcalculateAccuracyOfTissueSegmentation2_SOURCES = UNCcalculateAccuracyOfTissueSegmentation2.c
UNCcellular_automaton_SOURCES = UNCcellular_automaton.c
UNCcellular_automaton_LDADD = -lcellular_automata -lclustering
UNCclear_cut_clusters_SOURCES = UNCclear_cut_clusters.c
#UNCclustering_SOURCES = UNCclustering.c
#UNCclustering2_SOURCES = UNCclustering2.c
UNCclustering3_SOURCES = UNCclustering3.c
UNCclustering3_LDADD = -lclustering
UNCcompare_prediction_with_standard_SOURCES = UNCcompare_prediction_with_standard.c
UNCconnected_SOURCES = UNCconnected.c
UNCconnected_LDADD = -lroiio -lroi -lcontour
UNCcontour_SOURCES = UNCcontour.c
UNCcopy_header_info_SOURCES = UNCcopy_header_info.c
UNCcrap_SOURCES = UNCcrap.c
UNCcrap_LDADD = -lclustering
UNCcrop_SOURCES = UNCcrop.c
UNCcrop_LDADD = -lfilters
UNCdiff_SOURCES = UNCdiff.c
UNCdilate_SOURCES = UNCdilate.c
UNCdilate_LDADD = -lfilters
UNCdiscrete_SOURCES = UNCdiscrete.c
UNCdistance_from_brain_boundaries_SOURCES = UNCdistance_from_brain_boundaries.c
UNCdump_header_SOURCES = UNCdump_header.c
UNCenhanceB0_SOURCES = UNCenhanceB0.c
UNCentropyFromProbabilityMaps_SOURCES = UNCentropyFromProbabilityMaps.c
UNCerode_SOURCES = UNCerode.c
UNCerode_LDADD = -lfilters
UNCextractSlices_SOURCES = UNCextractSlices.c
UNCfilter2D_SOURCES = UNCfilter2D.c
UNCfilter2D_LDADD = -lfilters
UNCfilter3D_SOURCES = UNCfilter3D.c
UNCfilter3D_LDADD = -lfilters
UNCgradient_SOURCES = UNCgradient.c
UNChistogram_SOURCES = UNChistogram.c
UNChistogram_LDADD = -ldata_readers -lfilters
UNCmake_SOURCES = UNCmake.c
UNCmap_SOURCES = UNCmap.c
UNCmap_LDADD = -lfilters
#UNCmarkov_SOURCES = UNCmarkov.c
UNCmask_SOURCES = UNCmask.c
UNCmatchHistogram_SOURCES = UNCmatchHistogram.c
UNCmatchHistogram_LDADD = -lfilters
UNCmaxLikelihood_SOURCES = UNCmaxLikelihood.c
UNCnseg_SOURCES = UNCnseg.c
UNCnseg_LDADD = -lfilters
UNCop_SOURCES = UNCop.c
UNCotsu_SOURCES = UNCotsu.c
UNCotsu_LDADD = -lfilters
UNCoutline_SOURCES = UNCoutline.c
UNCoutline_LDADD = -lcontour
UNCprobabilityMapsToRGB_SOURCES = UNCprobabilityMapsToRGB.c
UNCpv2ma_SOURCES = UNCpv2ma.c
UNCpv2mb_SOURCES = UNCpv2mb.c
UNCremove_header_SOURCES = UNCremove_header.c
UNCring_SOURCES = UNCring.c
UNCring_LDADD = -lfilters -lroiio -lring -lroi
UNCscale_pixels_SOURCES = UNCscale_pixels.c
UNCset_header_SOURCES = UNCset_header.c
UNCsimulatedAnnealing_SOURCES = UNCsimulatedAnnealing.c
UNCsimulatedAnnealing_LDADD = -lclustering
UNCsmoothSegmentation_SOURCES = UNCsmoothSegmentation.c
UNCsmoothSegmentation_LDADD = -lcontour
UNCspiral_SOURCES = UNCspiral.c
UNCstatistics_SOURCES = UNCstatistics.c
UNCstatistics_LDADD = -lfilters
UNCstatistics_extended_SOURCES = UNCstatistics_extended.c
UNCtest_SOURCES = UNCtest.c
UNCthreshold_SOURCES = UNCthreshold.c
UNCthreshold_LDADD = -lfilters

