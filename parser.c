// number  : /-?[0-9]+[\.0-9]?/ ;
// string  : /\"(\\\\.|[^\"])*\"/ ;
// comment : /;[^\\r\\n]*/ ;
// symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;
// sexpr   : '(' <expr>* ')' ;
// qexpr   : '{' <expr>* '}' ;
// expr    : <number> | <string> | <symbol> |
//           <comment> | <sexpr> | <qexpr> ;
// lispy   : /^/ <expr>* /$/ ;
#include "tokenizer.c"

enum {
    AST_NUMBER,
    AST_STRING,
    AST_SYMBOL,
    AST_SEXPR,
    AST_QEXPR
};

typedef struct ast {
    int type;
    char *val;
    struct ast **children;
    int child_count;
} ast;

// (def {nil} {})
// {1 * 2.0}
// [ ( def { nil } { } ) { 1 * 2.0 } ]
ast *parse(tokens *t) {
    if (t->type == TOKENIZER_ERROR) {

    }
    ast *root = malloc(sizeof(ast));
    root->type = AST_SEXPR;

}