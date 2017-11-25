typedef	enum {
	opAND	=	0,
	opOR	=	1,
	opNOT	=	2,
	opXOR	=	3,
	opPLUS	=	4,
	opMINUS	=	5,
	opMULTIPLY=	6,
	opDIVIDE=	7,
	opEQ	=	8,
	opGT	=	9,
	opGE	=	10,
	opLT	=	11,
	opLE	=	12,
	opNOP	=	13,
	unknownOperator	=	20
} operators;

struct _OPERATOR {
	operators	c;
	char		*str;

	supertype	*root;
};

operator	*new_operator(operators);
operator	*clone_operator(operator *);
char		*string_operator(operator *);
char		*string_operators(operators);
void		destroy_operator(operator *);
operators	operators_string(char *);
