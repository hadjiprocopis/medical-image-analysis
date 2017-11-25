#ifndef	_ROI_UTILS_VISIT

typedef	enum {
	ROI_FILTER_TYPE_IRREGULAR_NUM_POINTS = 0,

	ROI_FILTER_TYPE_RECTANGULAR_X0 = 10,
	ROI_FILTER_TYPE_RECTANGULAR_Y0 = 11,
	ROI_FILTER_TYPE_RECTANGULAR_WIDTH = 12,
	ROI_FILTER_TYPE_RECTANGULAR_HEIGHT = 13,

	ROI_FILTER_TYPE_ELLIPTICAL_EX0 = 20,
	ROI_FILTER_TYPE_ELLIPTICAL_EY0 = 21,
	ROI_FILTER_TYPE_ELLIPTICAL_EA = 22,
	ROI_FILTER_TYPE_ELLIPTICAL_EB = 23,
	ROI_FILTER_TYPE_ELLIPTICAL_ROT = 24,

	ROI_FILTER_TYPE_GENERAL_CENTROID_X = 31,
	ROI_FILTER_TYPE_GENERAL_CENTROID_Y = 32,
	ROI_FILTER_TYPE_GENERAL_SLICE_NUMBER = 33,
	ROI_FILTER_TYPE_GENERAL_NUM_POINTS_INSIDE = 34,
	ROI_FILTER_TYPE_GENERAL_X0 = 35,
	ROI_FILTER_TYPE_GENERAL_Y0 = 36,
	ROI_FILTER_TYPE_GENERAL_WIDTH = 37,
	ROI_FILTER_TYPE_GENERAL_HEIGHT = 38,
	ROI_FILTER_TYPE_GENERAL_MIN_PIXEL = 39,
	ROI_FILTER_TYPE_GENERAL_MAX_PIXEL = 40,
	ROI_FILTER_TYPE_GENERAL_MEAN_PIXEL = 41,
	ROI_FILTER_TYPE_GENERAL_STDEV_PIXEL = 42,
	ROI_FILTER_TYPE_GENERAL_TYPE = 43
} roiFilterType;

void	rois_associate_with_unc_volume(roi **/*a_r*/, int /*numRois*/, DATATYPE ***/*volume_data*/, float */*scratch_pad*/);
void	roi_associate_with_unc_volume(roi */*a_r*/, DATATYPE ***/*volume_data*/, float */*scratch_pad*/);

int	rois_calculate(roi **/*a_r*/, int /*numRois*/);
int	roi_calculate(roi */*a_r*/);
void	roi_calculate_irregular_region(roiRegionIrregular */*irreReg*/, roi */*a_roi*/ /*it is optional*/);
void	roi_calculate_rectangular_region(roiRegionRectangular */*rectReg*/, roi */*a_roi*/ /*it is optional*/);
void	roi_calculate_elliptical_region(roiRegionElliptical */*elliReg*/, roi */*a_roi*/ /*it is optional*/);
int	write_rois_stats_to_file(char */*filename*/, roi **/*myRois*/, int /*numRois*/, char /*writeInfoForEachROIPeripheralPoint*/);

roi	**rois_filter(roi **/*a_r*/, int /*numRois*/, int */*newNumRois*/, roiRegionType /*rType*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/, char /*reverse*/);
int	roi_filter(roi */*a_roi*/, roiRegionType /*rType*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/);
int	roi_region_general_filter(roi */*a_roi*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/);
int	roi_region_irregular_filter(roiRegionIrregular */*a_region*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/);
int	roi_region_rectangular_filter(roiRegionRectangular */*a_region*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/);
int	roi_region_elliptical_filter(roiRegionElliptical */*a_region*/, int /*numCriteria*/, roiFilterType */*fType*/, float */*rMin*/, float */*rMax*/);

int	roi_region_irregular_get_inside_points(roiRegionIrregular */*a_region*/, int /*x0*/, int /*y0*/, int /*h*/, float **/*_x*/, float **/*_y*/, int */*total_points*/);
int	roi_region_rectangular_get_inside_points(roiRegionRectangular */*a_region*/, float **/*_x*/, float **/*_y*/, int */*total_points*/);
int	roi_region_elliptical_get_inside_points(roiRegionElliptical */*a_region*/, int /*x0*/, int /*y0*/, int /*w*/, int /*h*/, float **/*_x*/, float **/*_y*/, int */*total_points*/);
int	roi_get_inside_points(roi */*a_roi*/);
int	rois_get_inside_points(roi **/*a_rois*/, int /*numRois*/);

roiPoint ***roi_select_groups_of_inside_points(roi */*a_roi*/, char /*shouldNotReusePoints*/, int */*numGroups*/, int /*numPointsPerGroup*/, int /*x*/, int /*y*/, int /*z*/, int /*W*/, int /*H*/, int /*D*/);
roiPoint ***rois_select_groups_of_inside_points(roi **/*a_rois*/, int /*numRois*/, char /*shouldNotReusePoints*/, int */*numGroups*/, int /*numPointsPerGroup*/, int /*x*/, int /*y*/, int /*z*/, int /*W*/, int /*H*/, int /*D*/);
int	 ***rois_select_groups_of_inside_points_negative(roi **/*a_rois*/, int /*numRois*/, char /*shouldNotReusePoints*/, int */*numGroups*/, int /*numPointsPerGroup*/, DATATYPE ****/*uncData*/, int /*numUNCVolumes*/, int /*x*/, int /*y*/, int /*z*/, int /*W*/, int /*H*/, int /*D*/, int /*numSlicesBelow*/, int /*numSlicesAbove*/);

char	rois_convert_millimetres_to_pixels(roi **/*a_rois*/, int /*numRois*/, float /*pixel_size_x*/, float /*pixel_size_y*/, float /*pixel_size_z*/);
char	rois_convert_pixels_to_millimetres(roi **/*a_rois*/, int /*numRois*/, float /*pixel_size_x*/, float /*pixel_size_y*/, float /*pixel_size_z*/);
char	rois_convert_pixel_size(roi **/*a_rois*/, int /*numRois*/, float /*old_pixel_size_x*/, float /*old_pixel_size_y*/, float /*old_pixel_size_z*/, float /*new_pixel_size_x*/, float /*new_pixel_size_y*/, float /*new_pixel_size_z*/);

char	roi_convert_millimetres_to_pixels(roi */*a_roi*/, float /*pixel_size_x*/, float /*pixel_size_y*/, float /*pixel_size_z*/);
char	roi_convert_pixels_to_millimetres(roi */*a_roi*/, float /*pixel_size_x*/, float /*pixel_size_y*/, float /*pixel_size_z*/);
char	roi_convert_pixel_size(roi */*a_roi*/, float /*old_pixel_size_x*/, float /*old_pixel_size_y*/, float /*old_pixel_size_z*/, float /*new_pixel_size_x*/, float /*new_pixel_size_y*/, float /*new_pixel_size_z*/);

void roiedge(int ix[], int iy[], int npt, int *uyp[], int np[], int ny, int fill_mode);
#define _ROI_UTILS_VISIT
#endif
