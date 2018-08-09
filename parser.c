#include <stdbool.h>
#include "tokenizer.c"

int ERROR_STR_SIZE = 500;

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

ast *parse_children(ast *tree, tokens *t, int start, int end);

char *ast_error_str(code_context *c, const char *e) {
    char *str = malloc(sizeof(char) * ERROR_STR_SIZE);
    snprintf(str, ERROR_STR_SIZE,
             "error on row %d column %d: %s\n"
             "Stack Trace:\n"
             "%s\n",
             c->row, c->col, e, c->trace);
    return str;
}

ast *create_ast_val(int type, const char *val, code_context *c) {
    ast *ast_val = malloc(sizeof(ast));
    ast_val->type = type;
    ast_val->val = str_dup(val);
    ast_val->child_count = 0;
    ast_val->children = NULL;
    ast_val->context = copy_context(c);
    return ast_val;
}

ast *create_ast_expr(int type, code_context *c) {
    ast *ast_expr = malloc(sizeof(ast));
    ast_expr->type = type;
    ast_expr->val = NULL;
    ast_expr->child_count = 0;
    ast_expr->children = NULL;
    if (c == NULL) ast_expr->context = c;
    else ast_expr->context = copy_context(c);
    return ast_expr;
}

bool is_sexpr_start(token *t) {
    return t->type == TOKEN_RESERVED_SYMBOL && !strcmp(t->val, "(");
}

bool is_qexpr_start(token *t) {
    return t->type == TOKEN_RESERVED_SYMBOL && !strcmp(t->val, "{");
}

bool is_sexpr_end(token *t) {
    return t->type == TOKEN_RESERVED_SYMBOL && !strcmp(t->val, ")");
}

bool is_qexpr_end(token *t) {
    return t->type == TOKEN_RESERVED_SYMBOL && !strcmp(t->val, "}");
}

ast *add_child(ast *tree, ast *child) {
    tree->child_count++;
    tree->children = realloc(tree->children, sizeof(ast *) * tree->child_count);
    tree->children[tree->child_count - 1] = child;
    return tree;
}

ast *parse_expr(tokens *t, int *start, int end, int type) {
    int token_no = *start;
    int same_expr_nesting = 1;
    char *err;
    bool (*is_expr_end)(token *);
    bool (*is_expr_start)(token *);
    if (type == AST_SEXPR) {
        is_expr_start = &is_sexpr_start;
        is_expr_end = &is_sexpr_end;
        err = "missing s-expression closing brace, expected ')'";
    } else {
        is_expr_start = &is_qexpr_start;
        is_expr_end = &is_qexpr_end;
        err = "missing q-expression closing brace, expected '}'";
    }

    while (same_expr_nesting) {
        token_no++;
        if (token_no == end)
            return create_ast_val(AST_ERROR, err, t->items[token_no - 1]->context);
        if (is_expr_start(t->items[token_no]))
            same_expr_nesting++;
        if (is_expr_end(t->items[token_no]))
            same_expr_nesting--;
    }

    ast *tree = create_ast_expr(type, t->items[*start]->context);
    tree = parse_children(tree, t, *start + 1, token_no);
    *start = token_no;
    return tree;
}

ast *parse_children(ast *tree, tokens *t, int start, int end) {
    for (int token_no = start; token_no < end; token_no++) {
        ast *child;
        token *curr_t = t->items[token_no];

        if (curr_t->type == TOKEN_NUMBER) {
            child = create_ast_val(AST_NUMBER, curr_t->val, curr_t->context);
        } else if (curr_t->type == TOKEN_STRING) {
            child = create_ast_val(AST_STRING, curr_t->val, curr_t->context);
        } else if (curr_t->type == TOKEN_SYMBOL) {
            child = create_ast_val(AST_SYMBOL, curr_t->val, curr_t->context);
        } else if (is_sexpr_start(curr_t)) {
            child = parse_expr(t, &token_no, end, AST_SEXPR);
        } else if (is_qexpr_start(curr_t)) {
            child = parse_expr(t, &token_no, end, AST_QEXPR);
        } else if (is_sexpr_end(curr_t)) {
            char *err = "encountered extra ')'";
            return create_ast_val(AST_ERROR, err, t->items[token_no]->context);
        } else if (is_qexpr_end(curr_t)) {
            char *err = "encountered extra '}'";
            return create_ast_val(AST_ERROR, err, t->items[token_no]->context);
        } else {
            char *err = "unable to parse, unknown syntax";
            return create_ast_val(AST_ERROR, err, t->items[token_no]->context);
        }

        if (child->type == AST_ERROR) return child;

        tree = add_child(tree, child);
    }
    return tree;
}

ast *create_root_ast(tokens *t) {
    code_context *c = (t->count == 0) ? NULL : t->items[0]->context;
    ast *root = create_ast_expr(AST_SEXPR, c);
    return parse_children(root, t, 0, t->count);
}

ast *parse(char *input) {
    tokens *t = tokenize(input);

    if (t->type == TOKENIZER_ERROR) {
        return create_ast_val(AST_ERROR, t->err->val, t->err->context);
    }

    ast *root = create_root_ast(t);

    free_tokens(t);

    return root;
}


void free_ast(ast *tree) {
    if (tree == NULL) { return; }

    for (int i = 0; i < tree->child_count; i++) {
        free_ast(tree->children[i]);
    }

    if (tree->child_count > 0) free(tree->children);
    else free(tree->val);
    free_context(tree->context);
    free(tree);
}