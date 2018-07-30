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
    AST_QEXPR,
    AST_ERROR
};

typedef struct ast {
    int type;
    char *val;
    struct ast **children;
    int child_count;
    code_context *context;
} ast;


ast *create_ast_error(const char *error, code_context *context) {
    ast *ast_err = malloc(sizeof(ast));
    ast_err->type = AST_ERROR;
    ast_err->val = str_dup(error);
    ast_err->context = context;
    return ast_err;
}

// (def {nil} {})
// {1 * 2.0}
// [ ( def { nil } { } ) { 1 * 2.0 } ]
ast *parse(tokens *t) {
    if (t->type == TOKENIZER_ERROR) {
        return create_ast_error(t->err->val, t->err->context);
    }
    ast *root = malloc(sizeof(ast));
    root->type = AST_SEXPR;
    // todo: parse tokens to create an ast
}