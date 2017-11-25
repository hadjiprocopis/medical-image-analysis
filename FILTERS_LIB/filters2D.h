/* the offsets to add to a point (x,y) in order to get the set of its 8 neighbours */

void	sharpen2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	laplacian2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	sobel2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	average2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	median2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	min2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	max2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);
void	prewitt2D(DATATYPE **/*data*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/, DATATYPE **/*dataOut*/, float /*level*/);

