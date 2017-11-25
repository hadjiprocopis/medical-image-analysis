struct	_PARAMETER {
	char		*str; /* original string from parser */
	supertype	*c;
	parameter	*next;
	parameter	*previous;

	supertype	*root;
};

parameter	*new_parameter(char *, supertype *, parameter * /*parent, if any, else not part of any parameter list or just the parent */);
parameter	*clone_parameter(parameter *, parameter */*new parent, if any, else not part of any parameter list or just the parent */);
parameter	*clone_parameters(parameter *);
int		num_parameters(parameter */*parent*/);
parameter	**enumerate_parameters(parameter */*parent*/);
int		add_parameter(parameter *, parameter *);
void		destroy_parameter(parameter *);
void		destroy_parameters(parameter *);
char		*string_parameter(parameter *);
char		*string_parameters(parameter *);
image		*get_image_from_parameter(parameter *);
number		*get_number_from_parameter(parameter *);
string		*get_string_from_parameter(parameter *);
region		*get_region_from_parameter(parameter *);
dimensions	*get_dimensions_from_parameter(parameter *);
point		*get_point_from_parameter(parameter *);
