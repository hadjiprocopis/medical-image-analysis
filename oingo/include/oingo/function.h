struct	_FUNCTION {
	char		*name;
	parameter	*parameters;
	int		num_parameters;
	execute		exec;		
	supertype	*result;

	supertype	*root;
};

function	*new_function(char *, parameter *, execute /*pointer to function to execute*/);
function	*clone_function(function *);
void		destroy_function(function *);
char		*string_function(function *);
parameter	*get_function_parameter(function *, int);
int		evaluate_function(function *);
