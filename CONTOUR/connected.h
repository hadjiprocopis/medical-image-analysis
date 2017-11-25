#ifndef _CONNECTED_HEADER
#define _CONNECTED_HEADER

typedef struct _CONNECTED_OBJECT {
	int			num_points;	/* number of points belonging to this connected object */
	DATATYPE		id;		/* id of this object - the color of the object */
	int			*x, *y, *z;	/* list of points */
	DATATYPE		*v;		/* value of each point */
	int			x0, y0, w, h;	/* bounding box */
} connected_object;

typedef struct _CONNECTED_OBJECTS {
	int			num_connected_objects;
	connected_object	**objects;
} connected_objects;

connected_object	*connected_object_new(int /*num_points*/);
void			connected_object_destroy(connected_object *);
connected_object	*connected_object_copy(connected_object */*src*/);
connected_objects	*connected_objects_new(int /*num_connected_objects*/, int */*num_points*/); /* both params are optional */
void			connected_objects_destroy(connected_objects *);

/* from file connected.c */
int	find_connected_pixels2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*z*/, int /*w*/, int /*h*/, DATATYPE **/*result*/, neighboursType /*nType*/, DATATYPE /*minP*/, DATATYPE /*maxP*/, int */*num_connected_objects*/, int */*max_recursion_level*/, connected_objects **/*con_obj*/);
connected_object	*find_connected_object2D(DATATYPE **/*data*/, int /*seedX*/, int /*seedY*/, int /*x*/, int /*y*/, int /*z*/, int /*w*/, int /*h*/, DATATYPE **/*result*/, neighboursType /*nType*/, DATATYPE /*minP*/, DATATYPE /*maxP*/, int */*max_recursion_level*/);

#endif

