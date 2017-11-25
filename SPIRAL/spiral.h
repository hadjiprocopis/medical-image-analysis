#ifndef	_SPIRAL_VISIT

typedef	enum {_SPIRAL_LEFT=0, _SPIRAL_RIGHT=1, _SPIRAL_UP=2, _SPIRAL_DOWN=3} spiralDirection;


#ifdef	_SPIRAL_LIB_INTERNAL
/* private stuff */

/* when current direction is
	_SPIRAL_LEFT, _SPIRAL_RIGHT, _SPIRAL_UP, _SPIRAL_DOWN
   then next direction should be
   	_SPIRAL_UP,   _SPIRAL_DOWN,  _SPIRAL_RIGHT, _SPIRAL_LEFT
*/
const int	_spiral_next_direction[] = {_SPIRAL_UP, _SPIRAL_DOWN, _SPIRAL_RIGHT, _SPIRAL_LEFT};

/* these numbers tell us whether to add or subtract from X / Y
   when moving in a given direction.
   For example _spiral_direction_factors_x[_SPIRAL_LEFT] tell us that
   x should be subtracted by the step of the spiral */
const int	_spiral_direction_factors_x[] = {-1, 1, 0, 0};
const int	_spiral_direction_factors_y[] = {0, 0, -1, 1};
#endif


typedef	struct	_SPIRAL {
	int	x0, y0, z0,	/* epicentre of the spiral */
		w, h, d,	/* dimensions of the search space */
		x, y, z,	/* current spiral point */
		fromX, fromY, fromZ,	/* spiral is bounded by the box whose left bottom */
		toX, toY, toZ,	/* corner is 'from' and right top corent is 'to' */
		step[4],	/* step in each direction - usually 1 square */
		bl,		/* number of squares left before changing direction */
		l[4];		/* the number of squares in each direction we have to complete before turning  */

	char	hasMore,	/* flag to indicate i have more points waiting for you.
				   if FALSE, it means that we are out of bounds, no more
				   points can be given to you */
		_outOfBounds;	/* private flag to indicate that we are out of bounds */

	spiralDirection	current_direction; /* the direction to follow in order to get the next point */
} spiral;

spiral	*spiral_new(int /*x0*/, int /*y0*/, int /*z0*/, int /*fromX*/, int /*fromY*/, int /*fromZ*/, int /*toX*/, int /*toY*/, int /*toZ*/, int /*sL*/, int /*sR*/, int /*sU*/, int /*sD*/);
void	spiral_init(spiral */*a_spiral*/, int /*x0*/, int /*y0*/, int /*z0*/, int /*fromX*/, int /*fromY*/, int /*fromZ*/, int /*toX*/, int /*toY*/, int /*toZ*/, int /*sL*/, int /*sR*/, int /*sU*/, int /*sD*/);
void	spiral_reset(spiral */*a_spiral*/, int /*x0*/, int /*y0*/, int /*z0*/);
char	spiral_next_point(spiral */*a_spiral*/);
void	spiral_change_direction(spiral */*a_spiral*/);
void	spiral_destroy(spiral */*a_spiral*/);

#define	_SPIRAL_VISIT
#endif
