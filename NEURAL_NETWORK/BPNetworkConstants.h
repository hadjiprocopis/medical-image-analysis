/************************************************************************
 *									*
 *		ARTIFICIAL NEURAL NETWORKS SOFTWARE			*
 *									*
 *   An Error Back Propagation Neural Network Engine for Feed-Forward	*
 *			Multi-Layer Neural Networks			*
 *									*
 *			by Andrea HadjiProcopis				*
 *		    (livantes@barney.cs.city.ac.uk)			*
 *		Copyright Andrea HadjiProcopis, 1994			*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 *		FILE: BPNetworkConstants.h				*
 *	It contains all the constants of the Engine functions.		*
 *									*
 ************************************************************************/
 
#define	MAX_WEIGHT_VALUE		0.96
#define	MAX_THRESHOLD_VALUE		0.96
#define	DEFAULT_BETA_VALUE		0.09
#define	DEFAULT_LAMDA_VALUE		0.0

#ifndef TRUE
#define	TRUE				1
#endif
#ifndef FALSE
#define	FALSE				0
#endif

#define	EXEMPLAR_WEIGHT_UPDATE_METHOD	0x10
#define EPOCH_WEIGHT_UPDATE_METHOD	0x11

#define	SIGNAL_QUEUE_IS_EMPTY		-1

/*By 5%*/
#define	CHANGE_BETA_STEP		0.05

#define	MAX_CHARS_PER_LINE		25000

#ifndef	ABS
#define	ABS(a)				( ((a)<0) ? (-(a)):(a) )
#endif

#ifndef	ROUND
#define	ROUND(a)			( (((int )(a+0.5))!=((int )(a))) ? ((int )(a+1)):((int )(a)) )
#endif

#ifndef LIMIT
#define	LIMIT(first, last, x)		( ((x)<(first)) ? (first) : (((x)>(last))?(last):(x)) )
#endif

#ifndef DISCRETE_OUTPUT
#define DISCRETE_OUTPUT(first, last, step, x)	( (first) + ((step)*((BPPrecisionType )ROUND( ABS((first)-(LIMIT(first,last,x)))/(step)))) )
#endif

#ifndef MAX
#define	MAX(a, b)			( ((a)<(b)) ? (b):(a) )
#endif

#ifndef MIN
#define	MIN(a, b)			( ((a)<(b)) ? (a):(b) )
#endif
