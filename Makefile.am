ACLOCAL_AMFLAGS = -I m4
SUBDIRS = \
	COMMON\
	libim\
	DATA_STRUCTURES/LINKED_LIST\
	DATA_STRUCTURES/MATRIX\
	ALLOC_LIB\
	DATA_READERS/parser/histogram\
	DATA_READERS\
	IO_LIB/UNC\
	IO_LIB/ANALYZE\
	IO_LIB/MISC\
	IO_LIB/ROI\
	CONTOUR\
	FILTERS_LIB\
	SPIRAL\
	ROI\
	RING\
	SPLINE\
	CLUSTERING_3D\
	CELLULAR_AUTOMATA\
	REGISTRATION\
	UTILS/UNC\
	UTILS/ANALYZE\
	UTILS/VARIOUS\
	UTILS/ROI
	
#EXTRA_DIST = autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = IONtoolkit.pc
pkginclude_LIBS = libuncio liballoc libanalyzeio\
		libdata_readers libcontour libfilters\
		libspiral libcellular_automata libregistration libring\
		libroiio libroi libclustering\
		libspline liblinked_list libmatrix

nobase_pkginclude_HEADERS =\
	ALLOC_LIB/Alloc.h\
	CELLULAR_AUTOMATA/CAcell.h\
	CELLULAR_AUTOMATA/CAcellular_automaton.h\
	CELLULAR_AUTOMATA/CAIO.h\
	CELLULAR_AUTOMATA/CAsymbol.h\
	CELLULAR_AUTOMATA/cellular_automata.h\
	CLUSTERING_3D/clengine.h\
	CLUSTERING_3D/clestats.h\
	CLUSTERING_3D/clio.h\
	CLUSTERING_3D/clunc.h\
	CLUSTERING_3D/cluster.h\
	CLUSTERING_3D/clustering.h\
	CLUSTERING_3D/cstats.h\
	CLUSTERING_3D/point.h\
	CLUSTERING_3D/vector.h\
	COMMON/Common_IMMA.h\
	CONTOUR/connected.h\
	CONTOUR/contour.h\
	DATA_READERS/data_readers.h\
	DATA_READERS/parser/histogram/Grammar.tab.h\
	DATA_STRUCTURES/FIFO/fifo.h\
	DATA_STRUCTURES/LINKED_LIST/LinkedList.h\
	DATA_STRUCTURES/LINKED_LIST/LinkedListItem.h\
	DATA_STRUCTURES/LINKED_LIST/LinkedListIterator.h\
	DATA_STRUCTURES/MATRIX/matrix.h\
	FILTERS_LIB/combinatorics.h\
	FILTERS_LIB/erosion.h\
	FILTERS_LIB/filters.h\
	FILTERS_LIB/filters2D.h\
	FILTERS_LIB/filters3D.h\
	FILTERS_LIB/histogram.h\
	FILTERS_LIB/maps.h\
	FILTERS_LIB/normalise.h\
	FILTERS_LIB/otsu.h\
	FILTERS_LIB/random_numbers.h\
	FILTERS_LIB/statistics.h\
	FILTERS_LIB/threshold.h\
	FILTERS_LIB/timeseries.h\
	IO_LIB/ANALYZE/analyze_db.h\
	IO_LIB/ANALYZE/dbh.h\
	IO_LIB/ANALYZE/IO_analyze.h\
	IO_LIB/ANALYZE/IO_analyzeP.h\
	IO_LIB/IO.h\
	IO_LIB/IO_constants.h\
	IO_LIB/MISC/IO_misc.h\
	IO_LIB/ROI/IO_roi.h\
	IO_LIB/ROI/y.tab.h\
	IO_LIB/UNC/IO_jpeg.h\
	IO_LIB/UNC/IO_tiff.h\
	IO_LIB/UNC/IO_unc.h\
	libim/image.h\
	libim/iminfo.h\
	NEURAL_NETWORK/BPNetworkConstants.h\
	NEURAL_NETWORK/BPNetworkDeclarations.h\
	NEURAL_NETWORK/BPNetworkDefinitions.h\
	NEURAL_NETWORK/BPNetworkError.h\
	NEURAL_NETWORK/BPNetworkStandardInclude.h\
	NEURAL_NETWORK/neural_network.h\
	NEW_C/LIB/assign.h\
	NEW_C/LIB/calculate.h\
	NEW_C/LIB/derivatives_wrt_c.h\
	NEW_C/LIB/derivatives_wrt_w.h\
	NEW_C/LIB/do_clustering.h\
	NEW_C/LIB/entropy_clustering.h\
	NEW_C/LIB/initialise.h\
	NEW_C/LIB/io.h\
	NEW_C/LIB/memory.h\
	oingo/include/oingo/assignment.h\
	oingo/include/oingo/boolean.h\
	oingo/include/oingo/constants.h\
	oingo/include/oingo/designator.h\
	oingo/include/oingo/dimensions.h\
	oingo/include/oingo/expression.h\
	oingo/include/oingo/function.h\
	oingo/include/oingo/imagE.h\
	oingo/include/oingo/ImplementedFunctions.h\
	oingo/include/oingo/number.h\
	oingo/include/oingo/oingo.h\
	oingo/include/oingo/operator.h\
	oingo/include/oingo/parameter.h\
	oingo/include/oingo/point.h\
	oingo/include/oingo/region.h\
	oingo/include/oingo/strinG.h\
	oingo/include/oingo/supertype.h\
	oingo/include/oingo/symbol.h\
	oingo/include/oingo/symboltable.h\
	oingo/include/oingo/type.h\
	oingo/include/oingo/typedefs.h\
	REGISTRATION/dumpreslicer.h\
	REGISTRATION/native2registered.h\
	REGISTRATION/registration.h\
	RING/ring.h\
	ROI/roi_utils.h\
	SIGNAL/signal_handler.h\
	SPIRAL/spiral.h\
	SPLINE/spline.h
