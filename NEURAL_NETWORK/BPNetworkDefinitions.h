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
 *			FILE: BPNetworkDefinitions.h			*
 *	It contains all the actual DEFINITIONS of all the data 		*
 *	structures and function  prototypes used in the Neural Engine	*
 *	library. It is included only in the Main.c function		*
 *									*
 ************************************************************************/

/************************************************************************
 *      NOTE in a Two dimensional array of layers and nodes,            *
 *      the FIRST element is the LAYER and                              *
 *      the SECOND element is the NODE                                  *
 ************************************************************************/

/* An integer holding the number of bytes allocated by calloc */
int	TotalMemoryAllocated;

/************************************************************************
 *      This is the typedef of the kind of precision I want to give to  *
 *	the network arithmetic:						*
 *	double or float or ...						*
 ************************************************************************/
typedef	float	BPPrecisionType;
/*typedef	double	BPPrecisionType;*/


/************************************************************************
 *      Neuron: This is the structure which holds the data for the Neuron
 *	, the basic element of the nework.                              *
 ************************************************************************/

/*  ... Layer & Node refer to the position of the Neuron in the network 
    ... Inpunun & Outnum refer to the number of inputs and outputs of the
	Neuron
    ... *Weight refers to the One dimensional array of BPPrecisionTypes which holds
	weight values of the Neuron. Weights of the Neuron are those which are
	on the LEFT (Towards its input).
    ... *PreviousWeight refers to the One dimensional array of BPPrecisionTypes which holds
	the previous iteration weight values of the Neuron. Useful when we want to
	implement a momentum term in the new weight matrix calculation, or a safety
	measure for a sudden error increase.
    ... *WeightChange refers to how much the relevant weight must change when we do 
	the weights update.   
    ... Threshold is the threshold value of the neuron
    ... NetActivation is the Sum of the Products of all the Inputs times the
	corresponding weight value.
    ... Output is just the NeetActivation Sigmoided
    ... Delta is the Delta value for that Neuron during the Error Back Prop.
*/
typedef struct  _NEURON_STRUCT   Neuron;
struct  _NEURON_STRUCT {
		int		Layer, Node,
				Inpnum, Outnum;
		BPPrecisionType	*Weight, *PreviousWeight, *WeightChange,
				Threshold, ThresholdChange,
				NetActivation, Output, DiscreteOutput,
				Delta;
				} _SILLY_NEURON_STRUCT;

/************************************************************************
 *      BPNetworkType: This is the structure which holds the data for	*
 *	network itself							*
 ************************************************************************/
/*  ... *Layer is a One Dimensional array of integers which holds the number
	of nodes in the Nth layer ie Layer[N]=# of nodes.
    ... LayerNum refers to the total number of Layers of that Network
    ... Because we may have many networks (BP) at the same time we prefer
	to use an array of Networks ie *Network.
    ... **NetworkMap is a TWO Dimensional array which will hold the addresses
	in memory of all the neurons in the current network. It is of type
	Neuron because it will hold the pointer of the struct NEURON_STRUCT.
	For example the address of the Neuron in the third node of the fifth
	layer is given by (if this is the first BP network):
		Neuron Address = BPNetwork[0].NetworkMap[5][3]
    ... *IndividualError is a ONe-Dimensional array of BPPrecisionType which holds
    	the error of each OUTPUT node after each iteration.
    ... TotalError holds the average error of all output nodes, ie the
    	average of the contents of IndError[...].
    ... Beta is the rate of learning and is initialised at some default value at
    	beginning.
    ... Lamda is a momentum term, used when calculating the new weight matrix. The
    	convergence is better with that. If not desired then Lamda = 0.0.
    	Good value for Lamda is 0.9+ but less than 1.0.
    ... Propably we should add the type of the inputs/outputs by an enum
	typedef ie Discrete or Continuous ... */
enum	_BP_NETWORK_ENUM_TYPE	{
		Discrete = 0,
		Continuous = 1 };
typedef enum    _BP_NETWORK_ENUM_TYPE   BPNetworkEnumType;
enum	_BP_NETWORK_ENUM_OUTPUT_TYPE {
		Sigmoid	= 0,
		Linear = 1 };
typedef enum	_BP_NETWORK_ENUM_OUTPUT_TYPE   BPNetworkEnumOutputType;

typedef struct  _BACKPROP_NET_STRUCT BPNetwork;
struct  _BACKPROP_NET_STRUCT {
		int			LayersNum,
					Layer[10],
					NumberOfOutputClasses;
		Neuron			***NetworkMap;
		BPPrecisionType		*IndividualError, *Derivatives,
					TotalError, Beta, Lamda,
					ClassSeparation, FirstClassAt, LastClassAt;
		BPNetworkEnumType	NetworkType, TrainingType;
		BPNetworkEnumOutputType	OutputType;
		
} _SILLY_BACKPROP_NET_STRUCT;

/* A GLOBAL variable. The signal caught by the program. It is checked in every loop. */
int	SignalCaught;
char	SignalList[45][20];

/* This are my prototype functions */
ReturnCode	CreateBPNetwork(
	int	     layersnum,
	int	     *nodesnum,
	BPNetwork       *network );
ReturnCode      InitialiseBPNetwork(
	BPNetwork       *network );
ReturnCode      CreateNeuron(
	int     layer,
	int     node,
	int     inpnum,
	int     outnum,
	Neuron  *neuron );
ReturnCode      DestroyBPNetwork(
	BPNetwork       *network );
ReturnCode      BPNetworkFeedInputs(
	int	     	inpnum,
	BPPrecisionType	*inputs,
	BPNetwork       *network );
ReturnCode      BPNetworkForwardPropagate(
	BPNetwork       *network );
ReturnCode      BPNetworkBackPropagate(
	int	     	outnum,
	BPPrecisionType	*outputs,
	BPNetwork       *network );
ReturnCode      BPNetworkAdjustWeights(
	BPNetwork       *network,
	int		weight_change_average_factor );
void	RestoreWeightsToPreviousValues(
	BPNetwork	*network );
void    ShakeWeights(
	BPNetwork	*network,
	BPPrecisionType	max_noise );
ReturnCode      SaveWeightsInFile(
	BPNetwork       *network,
	char		*Filename,
	char		*Mode,
	FILE		*FileOfWeights,
	char		*comments /*if any else NULL*/);
ReturnCode      LoadWeightsFromFile(
	BPNetwork       *network,
	char		*Filename,
	FILE		*FileOfWeights);
ReturnCode	MonitorEW(
	BPNetwork	*network,
	FILE		*a_handle);
BPPrecisionType	RandomNumber(
	BPPrecisionType	max );
long		Seed(
	long		a_seed );
BPPrecisionType	DiscreteOutput(
	BPNetwork	*network,
	BPPrecisionType	x );
void		SignalHandler(
	int		signal_number );
void		ResetSignalHandler(
	void );
int		SetSignalHandler(
	void );
void		RemoveSignalHandler(
	void );
void		CatchSignal(
	int		signal_number );
ReturnCode	BPNetworkCalculateDerivative(
	BPNetwork	*network );
ReturnCode	BPNetworkCalculateDerivatives(
	BPNetwork	*network );
        
/* End of the BPNetworkDefinitions.h File */

