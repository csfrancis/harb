%{
#include <stdio.h>
#include <cinttypes>

#include "nodes.h"
#include "fields.h"

using namespace harb;

#define YYSTYPE harb::node::base*

%}

%error-verbose
%define api.pure

%parse-param {node::block *root_node}
%parse-param {yyscan_t yyscanner}
%lex-param {yyscan_t yyscanner}

%token INVALID_CHARACTER
%token NUM
%token STRING
%token IDENT
%token EQ "=="
%token NEQ "!="
%token END 0

%token STATS
%token TOP
%token WHERE
%token BY
%token PRINT
%token ROOTPATH

%right '|'
%left AND
%left OR
%nonassoc EQ NEQ '<' '>' LTE GTE

%{
#include "lexer.h"

void
yyerror(node::block *, yyscan_t yyscanner, const char *err)
{
	fprintf(stderr, "error: %s\n", err);
}

%}
%%

start:
stmt END {
	printf("stmt: %p\n", $1);
	node::executable *node = dynamic_cast<node::executable *>($1);
	root_node->append_node(node);
}
;

stmt:
  WHERE cond { printf("where\n"); }
| STATS IDENT '(' IDENT ')' opt_by_arg { printf("stats\n"); }
| TOP opt_top_args { printf("top\n"); }
| PRINT opt_num_arg { printf("print\n"); }
| ROOTPATH opt_num_arg { printf("rootpath\n"); }
| cond {
	if (typeid(*$1) == typeid(node::number)) {
		node::field_base *addr_node = harb::find_field("address");
		node::binary_op *eq_node = new node::binary_op(addr_node, $1, "==");
		node::where *where_node = new node::where(eq_node);
		$$ = where_node;
	} else {
		$$ = NULL;
	}
}
| stmt '|' stmt { printf("multi funcall\n"); }
;

opt_top_args:
| NUM opt_by_arg { printf("top_args with num\n"); }
;

opt_by_arg:
| BY ident_args { printf("with by arg\n"); }
;

opt_num_arg:
| NUM { printf("with num\n"); }
;

ident_args:
| IDENT   { printf("ident_args 1\n"); }
| IDENT ',' IDENT { printf("ident_args 2\n"); }
;

cond:
cond AND cond {
  printf("and\n");
} |
cond OR cond {
  printf("or\n");
} |
cond EQ cond {

} |
cond NEQ cond {

} |
cond '>' cond {

} |
cond '<' cond {

} |
cond GTE cond {

} |
cond LTE cond {

} |
'(' cond ')' {

} |
primary
;

primary: NUM | STRING | IDENT;

%%

int
Harb::parse_command(const char *str, node::block *root)
{
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

  yylex_init(&scanner);
  buf = yy_scan_bytes(str, strlen(str), scanner);
  int ret = yyparse(root, scanner);
  yy_delete_buffer(buf, scanner);
  yylex_destroy(scanner);

  return ret;
}
