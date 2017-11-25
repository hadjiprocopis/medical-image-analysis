#ifndef clustering_GUARDH
#define	clustering_GUARDH

typedef	struct _VECTOR	vector;
typedef struct _POINT	point;
typedef	struct _CL_UNC	clunc;
typedef struct _CLUSTER	cluster;
typedef	struct _CLENGINE clengine;
typedef struct _CLESTATS clestats;
typedef struct _ENTROPY	entropy;
typedef struct _CSTATS	cstats;
#ifndef _CLUSTERING_LIB_INTERNAL
/* these are not included during the compilation of the library, these are for users of the library
   AND after the library has been compiled and installed */
#include <vector.h>
#include <point.h>
#include <clunc.h>
#include <cstats.h>
#include <cluster.h>
#include <clestats.h>
#include <clengine.h>
#include <clio.h>
//#include <clsimulated_annealing.h>
#else
/* private includes */
#include "vector.h"
#include "point.h"
#include "clunc.h"
#include "cstats.h"
#include "cluster.h"
#include "clestats.h"
#include "clengine.h"
#include "clio.h"
//#include "clsimulated_annealing.h"
#endif

#ifndef	SQR
#define	SQR(a)	( (a)*(a) )
#endif

#endif // guard
