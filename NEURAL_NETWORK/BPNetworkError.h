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
 *			FILE: BPNetworkError.h				*
 *	It contains the definition of all the possible return values	*
 *	of the functions in the Neural Engine Library			*
 *									*
 ************************************************************************/


/* Our definition of the ReturnCode is a short int */
typedef	short	int	ReturnCode;


/* Here are all the possible Errors and their respective value */
#define	SUCCESS				 0
#define	MEMORY_ALLOCATION_FAILED	-1
#define	ILLEGAL_PARAMETERS		-2
#define	FILE_OPEN_ERROR			-3
