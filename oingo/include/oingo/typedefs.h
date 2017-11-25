typedef	struct	_OPERATOR	operator;
typedef	struct	_EXPRESSION	expression;
typedef	struct	_FUNCTION	function;
typedef	struct	_PARAMETER	parameter;
typedef	struct	_SUPERTYPE	supertype;
typedef	struct	_DESIGNATOR	designator;
typedef	struct	_NUMBER		number;
typedef	struct	_STRING		string;
typedef	struct	_DIMENSIONS	dimensions;
typedef	struct	_REGION		region;
typedef	struct	_POINT		point;
typedef	struct	_IMAGE		image;
typedef	struct	_BOOLEAN	boolean;

/* pointer to function which we can execute */
typedef	int(*execute)(parameter ** /*array of parameters*/, int /*num_parameters*/, supertype **/*the result is placed here*/);

/* symbol and symbol table typedefs */
typedef	struct	_SYMBOL		symbol;
typedef	struct	_SYMBOLTABLE	symboltable;
