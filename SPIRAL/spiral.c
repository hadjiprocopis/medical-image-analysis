#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Common_IMMA.h"
#include "Alloc.h"
#include "IO.h"

#define _SPIRAL_LIB_INTERNAL

#include "spiral.h"

/* create and return a new spiral centred at (x0,y0,z0) within
   the bounding box with left bottom corner at (fromX, fromY, fromZ)
   and right top corner at (toX, toY, toZ),
   step in each direction is sL, sR, sU, sD
*/
spiral	*spiral_new(int x0, int y0, int z0, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int sL, int sR, int sU, int sD){
	spiral	*ret;

	if( (ret=(spiral *)malloc(sizeof(spiral))) == NULL ){
		fprintf(stderr, "spiral_new : could not allocate %zd bytes for ret.\n", sizeof(spiral));
		return NULL;
	}

	spiral_init(ret, x0, y0, z0, fromX, fromY, fromZ, toX, toY, toZ, sL, sR, sU, sD);

	return ret;
}
void	spiral_init(spiral *a_spiral, int x0, int y0, int z0, int fromX, int fromY, int fromZ, int toX, int toY, int toZ, int sL, int sR, int sU, int sD){
	/* dimensions of the space we are allowed to move */
	a_spiral->fromX = fromX;
	a_spiral->fromY = fromY;
	a_spiral->fromZ = fromZ;
	a_spiral->toX = toX;
	a_spiral->toY = toY;
	a_spiral->toZ = toZ;

	a_spiral->step[_SPIRAL_LEFT]  = sL;
	a_spiral->step[_SPIRAL_RIGHT] = sR;
	a_spiral->step[_SPIRAL_UP]    = sU;
	a_spiral->step[_SPIRAL_DOWN]  = sD;

	spiral_reset(a_spiral, x0, y0, z0);
}

/* spiral may be reused after calling this. different epicentre may be defined - bounds remain the same */
void	spiral_reset(spiral *a_spiral, int x0, int y0, int z0){
	/* epicentre and current point */
	a_spiral->x = a_spiral->x0 = x0;
	a_spiral->y = a_spiral->y0 = y0;
	a_spiral->z = a_spiral->z0 = z0;

	/* the number of squares in each direction we have to complete before turning */
	/* this will change */
	a_spiral->l[_SPIRAL_DOWN]	= 1; /* first go down by 1 */
	a_spiral->l[_SPIRAL_LEFT]	= 1; /* then left by 1 */
	a_spiral->l[_SPIRAL_UP]		= 2; /* then up by 2 (so as to pass the epicentre) */
	a_spiral->l[_SPIRAL_RIGHT]	= 2; /* then right by 2 */

	a_spiral->current_direction = _SPIRAL_DOWN; /* start by going down */
	a_spiral->bl = a_spiral->l[a_spiral->current_direction];   /* 1 square and then you have to change direction */

	a_spiral->hasMore = TRUE;
	a_spiral->_outOfBounds = FALSE;
}

char	spiral_next_point(spiral *a_spiral){	
	int	new_x, new_y;

	if( a_spiral->bl-- <= 0 ){
		if( a_spiral->_outOfBounds && (a_spiral->current_direction == _SPIRAL_DOWN) ){
			a_spiral->hasMore = FALSE;
			return FALSE;
		}
		spiral_change_direction(a_spiral);
		return spiral_next_point(a_spiral);
	}

	new_x = a_spiral->x +_spiral_direction_factors_x[a_spiral->current_direction] *
			a_spiral->step[a_spiral->current_direction];
	new_y = a_spiral->y + _spiral_direction_factors_y[a_spiral->current_direction] *
			a_spiral->step[a_spiral->current_direction];

	if( (new_x<a_spiral->fromX) || (new_x>=a_spiral->toX) ||
	    (new_y<a_spiral->fromY) || (new_y>=a_spiral->toY) ){
		a_spiral->_outOfBounds = TRUE;		
		return spiral_next_point(a_spiral);
	}

	a_spiral->_outOfBounds = FALSE;
	a_spiral->x = new_x; a_spiral->y = new_y;
	
	return TRUE;
}

/* change direction */
void	spiral_change_direction(spiral *a_spiral){
	int	i;

	if( a_spiral->current_direction == _SPIRAL_RIGHT ) for(i=0;i<4;i++) a_spiral->l[i] += 2;

	a_spiral->current_direction = _spiral_next_direction[a_spiral->current_direction];
	a_spiral->bl = a_spiral->l[a_spiral->current_direction];
}

void	spiral_destroy(spiral *a_spiral){
	free(a_spiral);
}
