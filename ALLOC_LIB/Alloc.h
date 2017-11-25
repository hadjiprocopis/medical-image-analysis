#ifndef _ALLOC_HEADER
#define _ALLOC_HEADER

EXTERNAL_LINKAGE	void		freeDATATYPE2D(DATATYPE **data, int d1);
EXTERNAL_LINKAGE	void		freeDATATYPE3D(DATATYPE ***data, int d1, int d2);
EXTERNAL_LINKAGE	DATATYPE	**callocDATATYPE2D(int d1, int d2);
EXTERNAL_LINKAGE	DATATYPE	***callocDATATYPE3D(int d1, int d2, int d3);

EXTERNAL_LINKAGE	void		free2D(void **data, int d1);
EXTERNAL_LINKAGE	void		free3D(void ***data, int d1, int d2);
EXTERNAL_LINKAGE	void		**calloc2D(int d1, int d2, int size);
EXTERNAL_LINKAGE	void		***calloc3D(int d1, int d2, int d3, int size);

EXTERNAL_LINKAGE	void		freeFLOAT2D(float **data, int d1);
EXTERNAL_LINKAGE	void		freeFLOAT3D(float ***data, int d1, int d2);
EXTERNAL_LINKAGE	float		**callocFLOAT2D(int d1, int d2);
EXTERNAL_LINKAGE	float		***callocFLOAT3D(int d1, int d2, int d3);

EXTERNAL_LINKAGE	void		freeDOUBLE2D(double **data, int d1);
EXTERNAL_LINKAGE	void		freeDOUBLE3D(double ***data, int d1, int d2);
EXTERNAL_LINKAGE	double		**callocDOUBLE2D(int d1, int d2);
EXTERNAL_LINKAGE	double		***callocDOUBLE3D(int d1, int d2, int d3);

EXTERNAL_LINKAGE	void		freeINT2D(int **data, int d1);
EXTERNAL_LINKAGE	void		freeINT3D(int ***data, int d1, int d2);
EXTERNAL_LINKAGE	int		**callocINT2D(int d1, int d2);
EXTERNAL_LINKAGE	int		***callocINT3D(int d1, int d2, int d3);

EXTERNAL_LINKAGE	void		freeCHAR2D(char **data, int d1);
EXTERNAL_LINKAGE	void		freeCHAR3D(char ***data, int d1, int d2);
EXTERNAL_LINKAGE	char		**callocCHAR2D(int d1, int d2);
EXTERNAL_LINKAGE	char		***callocCHAR3D(int d1, int d2, int d3);

#endif
