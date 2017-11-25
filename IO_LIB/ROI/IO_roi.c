#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Common_IMMA.h>

#include "IO_roi.h"

char	*rois_toString(roi **a_rois, int numRois){
	int	i, n;
	char	*ret, **s, *p;

	if( (s=(char **)malloc(numRois*sizeof(char *))) == NULL ){
		fprintf(stderr, "rois_toString : could not allocate %zd bytes for s.\n", numRois*sizeof(char *));
		return NULL;
	}

	for(i=0,n=0;i<numRois;i++){
		if( (s[i]=roi_toString(a_rois[i])) == NULL ){
			fprintf(stderr, "rois_toString : call to roi_toString has failed for %d roi.\n", i);
			return NULL;
		}
		n += strlen(s[i]);
	}

	if( (ret=(char *)malloc(n*sizeof(char))) == NULL ){
		fprintf(stderr, "rois_toString : could not allocate %zd bytes for ret.\n", n*sizeof(char));
		for(i=0;i<numRois;i++) free(s[i]); free(s);
		return NULL;
	}

	for(i=0,p=&(ret[0]);i<numRois;i++){
		sprintf(p, "%s", s[i]);
		p += strlen(s[i]);
	}

	for(i=0;i<numRois;i++) free(s[i]); free(s);
	return ret;
}

char	*roi_toString(roi *a_roi){
	char	*ret, *p;
	int	n = 50, i;
	roiRegionRectangular	*rectReg;
	roiRegionElliptical	*elliReg;
	roiRegionIrregular	*irreReg;

	if( a_roi->name != NULL ) n += strlen(a_roi->name);
	if( a_roi->image != NULL ) n += strlen(a_roi->image) + 10;

	switch( a_roi->type ){
		case	RECTANGULAR_ROI_REGION:
			rectReg = (roiRegionRectangular *)(a_roi->roi_region);
			n += 200;
			if( (ret=(char *)malloc(n*sizeof(char))) == NULL ){
				fprintf(stderr, "roi_toString : could not allocate %zd bytes for ret (rectangular region).\n", n*sizeof(char));
				return NULL;
			}
			/* a_roi->maxPixel > 0.0 means an image is associated with this roi */
			if( a_roi->maxPixel > 0.0 )
				sprintf(ret, "Rectangular region (%f, %f, %f, %f) covering %d points\nname='%s', image='%s %d', slice %d, pixels=(%.0f, %.0f, %.2f, %.2f)\n", rectReg->x0, rectReg->y0, rectReg->width, rectReg->height, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice, a_roi->minPixel, a_roi->maxPixel, a_roi->meanPixel, a_roi->stdevPixel);
			else				
				sprintf(ret, "Rectangular region (%f, %f, %f, %f) covering %d points\nname='%s', image='%s %d', slice %d\n", rectReg->x0, rectReg->y0, rectReg->width, rectReg->height, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice);
			break;
		case	ELLIPTICAL_ROI_REGION:
			elliReg = (roiRegionElliptical *)(a_roi->roi_region);
			n += 200;
			if( (ret=(char *)malloc(n*sizeof(char))) == NULL ){
				fprintf(stderr, "roi_toString : could not allocate %zd bytes for ret (elliptical region).\n", n*sizeof(char));
				return NULL;
			}
			/* a_roi->maxPixel > 0.0 means an image is associated with this roi */
			if( a_roi->maxPixel > 0.0 )
				sprintf(ret, "Elliptical region (%f, %f, %f, %f, %f) covering %d points\nname='%s', image='%s %d', slice %d\nbounding rect leftop=(%.1f, %.1f) dim=(%.1f, %.1f) pixels=(%.0f, %.0f, %.2f, %.2f)\n", elliReg->ex0, elliReg->ey0, elliReg->ea, elliReg->eb, elliReg->rot, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice, a_roi->x0, a_roi->y0, a_roi->width, a_roi->height, a_roi->minPixel, a_roi->maxPixel, a_roi->meanPixel, a_roi->stdevPixel);
			else
				sprintf(ret, "Elliptical region (%f, %f, %f, %f, %f) covering %d points\nname='%s', image='%s %d', slice %d\nbounding rect leftop=(%.1f, %.1f) dim=(%.1f, %.1f)\n", elliReg->ex0, elliReg->ey0, elliReg->ea, elliReg->eb, elliReg->rot, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice, a_roi->x0, a_roi->y0, a_roi->width, a_roi->height);
			break;
		case	IRREGULAR_ROI_REGION:
			irreReg = (roiRegionIrregular *)(a_roi->roi_region);
			n += 200 + irreReg->num_points * 50;
			if( (ret=(char *)malloc(n*sizeof(char))) == NULL ){
				fprintf(stderr, "roi_toString : could not allocate %zd bytes for ret (irregular region).\n", n*sizeof(char));
				return NULL;
			}
			/* a_roi->maxPixel > 0.0 means an image is associated with this roi */
			if( a_roi->maxPixel > 0.0 )
				sprintf(ret, "Irregular region with %d peripheral points and covering %d points\nname='%s', image='%s %d', slice %d\ncentroid at (%f, %f)\nbounding rect leftop=(%.1f, %.1f) dim=(%.1f, %.1f) pixels=(%.0f, %.0f, %.2f, %.2f)", irreReg->num_points, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice, a_roi->centroid_x, a_roi->centroid_y, a_roi->x0, a_roi->y0, a_roi->width, a_roi->height, a_roi->minPixel, a_roi->maxPixel, a_roi->meanPixel, a_roi->stdevPixel);
			else
				sprintf(ret, "Irregular region with %d peripheral points and covering %d points\nname='%s', image='%s %d', slice %d\ncentroid at (%f, %f)\nbounding rect leftop=(%.1f, %.1f) dim=(%.1f, %.1f)", irreReg->num_points, a_roi->num_points_inside, a_roi->name, a_roi->image, a_roi->slice+1, a_roi->slice, a_roi->centroid_x, a_roi->centroid_y, a_roi->x0, a_roi->y0, a_roi->width, a_roi->height);
			for(i=0;i<irreReg->num_points;i++){
				p = &(ret[strlen(ret)]);
				sprintf(p, "  (%f/%d, %f/%d) (%f, %f) (%f, %f, %f) (%f, %f)\n", irreReg->points[i]->x, irreReg->points[i]->X, irreReg->points[i]->y, irreReg->points[i]->Y, irreReg->points[i]->r, irreReg->points[i]->theta, irreReg->points[i]->dx, irreReg->points[i]->dy, irreReg->points[i]->slope, irreReg->points[i]->dr, irreReg->points[i]->dtheta);
			}
			break;
		default:
			fprintf(stderr, "roi_toString : type %d not yet implemented.\n", a_roi->type);
			return NULL;
	}

	n = strlen(ret);
	if( (ret=(char *)realloc(ret, n * sizeof(char))) == NULL ){
		fprintf(stderr, "roi_toString : could not reallocate %zd bytes for ret.\n", n * sizeof(char));
		return NULL;
	}
	
	return ret;
}

roi	**rois_copy(roi **a_rois, int numRois){
	int	i, j;
	roi	**new_rois;

	if( (new_rois=(roi **)malloc(numRois*sizeof(roi *))) == NULL ){
		fprintf(stderr, "roi_copy : could not allocate %zd bytes for %d rois.\n", numRois*sizeof(roi *), numRois);
		return NULL;
	}
	for(i=0;i<numRois;i++)
		if( (new_rois[i]=roi_copy(a_rois[i])) == NULL ){
			fprintf(stderr, "roi_copy : call to roi_copy has failed for %d roi.\n", i);
			for(j=0;j<i;j++) roi_destroy(new_rois[i]); free(new_rois);
			return NULL;
		}

	return new_rois;
}
roiPoint *roi_point_copy(roiPoint *a_point){
	roiPoint	*new_point;

	if( (new_point=roi_point_new(a_point->p)) == NULL ){
		fprintf(stderr, "roi_point_copy : call to point_new has failed.\n");
		return NULL;
	}

	new_point->x = a_point->x;
	new_point->y = a_point->y;
	new_point->z = a_point->z;

	new_point->X = a_point->X;
	new_point->Y = a_point->Y;
	new_point->Z = a_point->Z;

	new_point->r = a_point->r;
	new_point->theta = a_point->theta;

	new_point->dx = a_point->dx;
	new_point->dy = a_point->dy;
	new_point->slope = a_point->slope;
	new_point->dr = a_point->dr;
	new_point->dtheta = a_point->dtheta;

	new_point->v = a_point->v;

	return new_point;
}
roiPoint **roi_points_copy(roiPoint **a_points, int numPoints){
	int		i;
	roiPoint	**new_points;

	if( (new_points=(roiPoint **)malloc(numPoints*sizeof(roiPoint *))) == NULL ){
		fprintf(stderr, "roi_points_copy : could not allocate %zd bytes for new_points.\n", numPoints*sizeof(roiPoint *));
		return NULL;
	}
	for(i=0;i<numPoints;i++)
		if( (new_points[i]=roi_point_copy(a_points[i])) == NULL ){
			fprintf(stderr, "roi_points_copy : call to roi_point_copy has failed for %d roi.\n", i);
			free(new_points);
			return NULL;
		}
	return new_points;
}

roi	*roi_copy(roi *a_roi){
	roi			*new_roi;

	if( (new_roi=roi_new()) == NULL ){
		fprintf(stderr, "roi_copy : call to roi_new has failed.\n");
		return NULL;
	}
	if( (new_roi->roi_region=roi_region_copy(a_roi->roi_region, a_roi->type)) == NULL ){
		fprintf(stderr, "roi_copy : call to roi_region_copy has failed.\n");
		free(new_roi);
		return NULL;
	}
	new_roi->name  = strdup(a_roi->name);
	new_roi->image = strdup(a_roi->image);
	new_roi->slice = a_roi->slice;	
	new_roi->type  = a_roi->type;
	new_roi->centroid_x = a_roi->centroid_x;
	new_roi->centroid_y = a_roi->centroid_y;
	new_roi->x0	    = a_roi->x0;
	new_roi->y0	    = a_roi->y0;
	new_roi->width	    = a_roi->width;
	new_roi->height	    = a_roi->height;
	new_roi->minPixel   = a_roi->minPixel;
	new_roi->maxPixel   = a_roi->maxPixel;
	new_roi->meanPixel  = a_roi->meanPixel;
	new_roi->stdevPixel = a_roi->stdevPixel;

	if( (new_roi->num_points_inside=a_roi->num_points_inside) > 0 )
		if( (new_roi->points_inside=roi_points_copy(a_roi->points_inside, a_roi->num_points_inside)) == NULL ){
			fprintf(stderr, "roi_copy : call to roi_points_copy has failed.\n");
			free(new_roi);
			return NULL;
		}

	return new_roi;
}
void	*roi_region_copy(void *a_region, roiRegionType rType){
	void			*new_region = NULL;
	roiRegionIrregular	*irreReg, *new_irreReg;
	roiRegionElliptical	*elliReg, *new_elliReg;
	roiRegionRectangular	*rectReg, *new_rectReg;
	int			i;

	switch( rType ){
		case RECTANGULAR_ROI_REGION:
			rectReg = (roiRegionRectangular *)a_region;
			if( (new_region=roi_region_new(rType, rectReg->p, 0)) == NULL ){
				fprintf(stderr, "roi_region_copy : call to roi_region_new (1,2) has failed for %d.\n", rType);
				return NULL;
			}
			new_rectReg = (roiRegionRectangular *)new_region;
			new_rectReg->x0 = rectReg->x0;
			new_rectReg->y0 = rectReg->y0;
			new_rectReg->width = rectReg->width;
			new_rectReg->height = rectReg->height;
			break;
		case ELLIPTICAL_ROI_REGION:
			elliReg = (roiRegionElliptical *)a_region;
			if( (new_region=roi_region_new(rType, elliReg->p, 0)) == NULL ){
				fprintf(stderr, "roi_region_copy : call to roi_region_new (1,2) has failed for %d.\n", rType);
				return NULL;
			}
			new_elliReg = (roiRegionElliptical *)new_region;
			new_elliReg->ex0 = elliReg->ex0;
			new_elliReg->ey0 = elliReg->ey0;
			new_elliReg->ea = elliReg->ea;
			new_elliReg->eb = elliReg->eb;
			new_elliReg->rot = elliReg->rot;
			break;
		case IRREGULAR_ROI_REGION:
			irreReg = (roiRegionIrregular *)a_region;
			if( (new_region=roi_region_new(rType, irreReg->p, irreReg->num_points)) == NULL ){
				fprintf(stderr, "roi_region_copy : call to roi_region_new (3) has failed for %d.\n", rType);
				return NULL;
			}
			new_irreReg = (roiRegionIrregular *)new_region;
			for(i=0;i<irreReg->num_points;i++){
				new_irreReg->points[i]->x = irreReg->points[i]->x;
				new_irreReg->points[i]->y = irreReg->points[i]->y;
				new_irreReg->points[i]->z = irreReg->points[i]->z;

				new_irreReg->points[i]->X = irreReg->points[i]->X;
				new_irreReg->points[i]->Y = irreReg->points[i]->Y;
				new_irreReg->points[i]->Z = irreReg->points[i]->Z;

				new_irreReg->points[i]->r = irreReg->points[i]->r;
				new_irreReg->points[i]->theta = irreReg->points[i]->theta;

				new_irreReg->points[i]->dx = irreReg->points[i]->dx;
				new_irreReg->points[i]->dy = irreReg->points[i]->dy;
				new_irreReg->points[i]->slope = irreReg->points[i]->slope;
				new_irreReg->points[i]->dr = irreReg->points[i]->dr;
				new_irreReg->points[i]->dtheta = irreReg->points[i]->dtheta;

				new_irreReg->points[i]->p = irreReg->points[i]->p;
			}
			break;
		default:
			fprintf(stderr, "roi_region_copy : region type '%d' not implemented yet.\n", rType);
			break;
	} /* switch */

	return new_region;
}

roi	**rois_new(int numRois){
	int	i, j;
	roi	**a_rois;

	if( (a_rois=(roi **)malloc(numRois*sizeof(roi *))) == NULL ){
		fprintf(stderr, "roi_new : could not allocate %zd bytes for %d rois.\n", numRois*sizeof(roi *), numRois);
		return NULL;
	}
	for(i=0;i<numRois;i++)
		if( (a_rois[i]=roi_new()) == NULL ){
			fprintf(stderr, "roi_new : call to roi_new has failed for %d roi.\n", i);
			for(j=0;j<i;j++) roi_destroy(a_rois[i]); free(a_rois);
			return NULL;
		}

	return a_rois;
}
roi	*roi_new(void){
	roi	*a_roi;

	if( (a_roi=(roi *)malloc(sizeof(roi))) == NULL ){
		fprintf(stderr, "roi_new : could not allocate %zd bytes for roi.\n", sizeof(roi));
		return NULL;
	}
	a_roi->type = UNKNOWN_ROI_REGION;
	a_roi->name = a_roi->image = NULL;
	a_roi->slice = -1;
	a_roi->roi_region = NULL;
	a_roi->centroid_x = a_roi->centroid_y =
	a_roi->x0 = a_roi->y0 = a_roi->width = a_roi->height =

	/* a roi may or may not be associated with a unc volume. A test to
	   find out is when a_roi->minPixel >= 0.
	   You can associate a roi with a unc volume using:
	   rois_associate_with_unc_volume */
	a_roi->minPixel = a_roi->maxPixel = -1.0;
	a_roi->meanPixel = a_roi->stdevPixel = 0.0;

	a_roi->num_points_inside = 0;
	a_roi->points_inside = NULL;

	return a_roi;
}
void	roi_region_destroy(void *a_region, roiRegionType a_type){
	roiRegionIrregular	*irreReg;

	switch( a_type ){
		case RECTANGULAR_ROI_REGION:
			free( a_region );
			return;
		case ELLIPTICAL_ROI_REGION:
			free( a_region );
			return;
		case IRREGULAR_ROI_REGION:
			irreReg = (roiRegionIrregular *)a_region;
			if( irreReg->num_points > 0 ) roi_points_destroy(irreReg->points, irreReg->num_points);
			free(irreReg);
			return;
		default:
			fprintf(stderr, "roi_region_destroy : type %d not yet implemented.\n", a_type);
			return;
	}
}

/* *p is the pointer to the roi holding this region. use NULL if not appropriate.
   numPoints is only relevant if a_type is IRREGULAR_ROI_REGION */
void	*roi_region_new(roiRegionType a_type, roi *p, int numPoints){
	roiRegionRectangular	*rectReg;
	roiRegionElliptical	*elliReg;
	roiRegionIrregular	*irreReg;

	switch( a_type ){
		case RECTANGULAR_ROI_REGION:
			if( (rectReg=(roiRegionRectangular *)malloc(sizeof(roiRegionRectangular))) == NULL ){
				fprintf(stderr, "roi_region_new : could not allocate %zd bytes for rectReg (rectangular).\n", sizeof(roiRegionRectangular));
				return NULL;
			}
			rectReg->x0 = rectReg->y0 =
			rectReg->width = rectReg->height = 0.0;
			return (void *)rectReg;
		case ELLIPTICAL_ROI_REGION:
			if( (elliReg=(roiRegionElliptical *)malloc(sizeof(roiRegionElliptical))) == NULL ){
				fprintf(stderr, "roi_region_new : could not allocate %zd bytes for elliReg (elliptical).\n", sizeof(roiRegionElliptical));
				return NULL;
			}
			elliReg->ex0 = elliReg->ey0 =
			elliReg->ea = elliReg->eb = elliReg->rot = 0.0;
			return (void *)elliReg;
		case IRREGULAR_ROI_REGION:
			if( (irreReg=(roiRegionIrregular *)malloc(sizeof(roiRegionIrregular))) == NULL ){
				fprintf(stderr, "roi_region_new : could not allocate %zd bytes for irreReg (irregular).\n", sizeof(roiRegionElliptical));
				return NULL;
			}
			irreReg->num_points = numPoints;
			if( numPoints == 0 ){
				irreReg->points = NULL;
				return irreReg;
			}
			if( (irreReg->points=roi_points_new(numPoints, p)) == NULL ){
				fprintf(stderr, "roi_region_new : call to roi_points_new has failed for %d points.\n", numPoints);
				free(irreReg);
				return NULL;
			}
			return (void *)irreReg;
		default:
			fprintf(stderr, "roi_region_new : type %d not yet implemented.\n", a_type);
			/* and go to the null below */
			break;
	}

	return NULL;
}
void	roi_points_destroy(roiPoint **a_roi_points, int numPoints){
	int	i;
	for(i=0;i<numPoints;i++)
		roi_point_destroy(a_roi_points[i]);
	free(a_roi_points);
}
void	roi_point_destroy(roiPoint *a_roi_point){ free(a_roi_point); }
roiPoint **roi_points_new(int numPoints, roi *p){
	int		i, j;
	roiPoint	**a_roi_points;

	if( (a_roi_points=(roiPoint **)malloc(numPoints*sizeof(roiPoint *))) == NULL ){
		fprintf(stderr, "roi_points_new : could not allocate %zd bytes for a_roi_points (%d points).\n", numPoints*sizeof(roiPoint *), numPoints);
		return NULL;
	}
	for(i=0;i<numPoints;i++)
		if( (a_roi_points[i]=roi_point_new(p)) == NULL ){
			fprintf(stderr, "roi_points_new : call to a_roi_points has failed for %d point (of %d).\n", i, numPoints);
			for(j=0;j<i;j++) free(a_roi_points[i]); free(a_roi_points);
		}

	return a_roi_points;
}
roiPoint *roi_point_new(roi *p){
	roiPoint	*a_roi_point;

	if( (a_roi_point=(roiPoint *)malloc(sizeof(roiPoint))) == NULL ){
		fprintf(stderr, "roi_point_new : could not allocate %zd bytes for a_roi_point.\n", sizeof(roiPoint));
		return NULL;
	}
	a_roi_point->X = a_roi_point->Y = a_roi_point->Z = 0;

	a_roi_point->x = a_roi_point->y = a_roi_point->z =
	a_roi_point->v =
	a_roi_point->dx = a_roi_point->dy = a_roi_point->slope =
	a_roi_point->dr = a_roi_point->dtheta =
	a_roi_point->r = a_roi_point->theta = 0.0;

	a_roi_point->p = p;

	return a_roi_point;
}
void	roi_destroy(roi *a_roi){
	roi_region_destroy(a_roi->roi_region, a_roi->type);
	free(a_roi->name);
	free(a_roi->image);
	if( (a_roi->num_points_inside>0) &&
	    (a_roi->points_inside!=NULL) )
		roi_points_destroy(a_roi->points_inside, a_roi->num_points_inside);
	free(a_roi);
}
void	rois_destroy(roi **a_rois, int numRois){
	int	i;
	for(i=0;i<numRois;i++) roi_destroy(a_rois[i]);
	free(a_rois);
}
int	write_rois_to_file(char *filename, roi **myRois, int numRois, float pixel_size_x, float pixel_size_y){
	FILE	*file;
	int	i, j;

	roiRegionRectangular	*rectReg;
	roiRegionElliptical	*elliReg;
	roiRegionIrregular	*irreReg;

	if( (file=fopen(filename, "w")) == NULL ){
		fprintf(stderr, "write_rois_to_file : could not open file '%s' for writing.\n", filename);
		return FALSE;
	}

	for(i=0;i<numRois;i++)
		switch( myRois[i]->type ){
			case RECTANGULAR_ROI_REGION:
				rectReg = (roiRegionRectangular *)(myRois[i]->roi_region);
				fprintf(file, "Rectangular_region Name=\"%s\" Image=\"%s %d\"\nX0=%.2f Y0=%.2f Width=%.2f Height=%.2f\nEnd_region\n", myRois[i]->name, myRois[i]->image, myRois[i]->slice+1, rectReg->x0, rectReg->y0, rectReg->width, rectReg->height);
				break;
			case ELLIPTICAL_ROI_REGION:
				elliReg = (roiRegionElliptical *)(myRois[i]->roi_region);
				fprintf(file, "Elliptical_region Name=\"%s\" Image=\"%s %d\"\nEX0=%.2f EY0=%.2f EA=%.2f EB=%.2f ROT=%.2f\nEnd_region\n", myRois[i]->name, myRois[i]->image, myRois[i]->slice+1, elliReg->ex0, elliReg->ey0, elliReg->ea, elliReg->eb, elliReg->rot);
				break;
			case IRREGULAR_ROI_REGION:
				irreReg = (roiRegionIrregular *)(myRois[i]->roi_region);
				fprintf(file, "Irregular_region Name=\"%s\" Image=\"%s %d\"\nPoints=%d\n", myRois[i]->name, myRois[i]->image, myRois[i]->slice+1, irreReg->num_points);
				for(j=0;j<irreReg->num_points;j++)
					fprintf(file, "%.2f %.2f\n", irreReg->points[j]->x, irreReg->points[j]->y);
				fprintf(file, "End_region\n");
				break;
			default:
				fprintf(stderr, "write_rois_to_file : warning, roi %d has type %d which is not yet implemented.\n", i, myRois[i]->type);
				break;
		}

	fclose(file);
	return TRUE;
}
