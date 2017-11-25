int	read_image(parameter **/*array of parameters*/, int /*num_parameters*/, supertype **/*result to be returned*/);
int	close_image(parameter **/*array of parameters*/, int /*num_parameters*/, supertype **/*result to be returned*/);
int	image_region(parameter **/*array of parameters*/, int /*num_parameters*/, supertype **/*result to be returned*/);
int	save_image(parameter **/*array of parameters*/, int /*num_parameters*/, supertype **/*result to be returned*/);

int	compare_mean_pixel_values(parameter **/*array of parameters*/, int /*num_parameters*/, supertype **/*result to be returned*/);


/* these can not be used directly in oingo scripts but will be called whenever
   you have something like image1 AND image2 expressions
*/
image	*image_logical_operations(image *A, image *B, operator *op);
image	*image_arithmetic_operations(image *A, image *B, operator *op);
