#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.c"

enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_SYMBOL,
    TOKEN_RESERVED_SYMBOL
};

enum {
    TOKENIZER_TOKENS,
    TOKENIZER_ERROR
};

typedef struct code_context {
    int row;
    int col;
    char *trace;
} code_context;

typedef struct token {
    int type;
    char *val;
    code_context *context;
} token;

typedef struct error {
    char *val;
    code_context *context;
} error;

typedef struct tokens {
    int type;
    int count;
    token **items;
    error *err;
} tokens;


int is_empty(const char *input) {
    return *input == '\0';
}

int is_number(const char *input) {
    return (*input >= '0' && *input <= '9') || *input == '.';
}

int is_qoutes(const char *input) {
    return *input == '"';
}

int is_comment(const char *input) {
    return *input == ';';
}

int is_whitespace(const char *input) {
    return *input == ' ' || *input == '\t';
}

int is_newline(const char *input) {
    return *input == '\n';
}

int is_escape_char(const char *input) {
    return *input == '\\';
}

int is_reserved_symbol(const char *input) {
    return exists(input, "{}()");
}

int is_symbol(const char *input) {
    return !is_number(input) &&
           !is_qoutes(input) &&
           !is_comment(input) &&
           !is_whitespace(input) &&
           !is_newline(input) &&
           !is_escape_char(input) &&
           !is_reserved_symbol(input);
}

int column(const char *curr_loc, const char *row_start) {
    return (int) (curr_loc - row_start + 1);
}

code_context *create_context(int row, int col, const char *trace) {
    code_context *context = malloc(sizeof(code_context));
    context->row = row;
    context->col = col;
    context->trace = str_dup(trace);
    return context;
}

code_context *copy_context(code_context *c) {
    if (!c) return c;
    return create_context(c->row, c->col, c->trace);
}

token *create_token(const char *start, const char *curr_loc, int type,
                    int row_no, const char *row) {
    int col_no = column(start, row);
    token *t = malloc(sizeof(token));
    t->context = create_context(row_no, col_no, row);
    t->type = type;
    unsigned long len = curr_loc - start;
    t->val = str_dup_n(start, len);
    return t;
}

tokens *add_token(token *v, tokens *t) {
    t->count++;
    t->items = realloc(t->items, sizeof(token *) * t->count);
    t->items[t->count - 1] = v;
    return t;
}

tokens *init_tokens() {
    tokens *t = malloc(sizeof(tokens));
    t->type = TOKENIZER_TOKENS;
    t->count = 0;
    t->items = malloc(sizeof(token *));
    return t;
}

tokens *create_error(tokens *t, int row, int col, char *err_str, char *trace) {
    t->type = TOKENIZER_ERROR;
    error *err = malloc(sizeof(error));
    err->val = str_dup(err_str);
    err->context = create_context(row, col, trace);
    t->err = err;
    return t;
}


tokens *tokenize(char *input) {
    tokens *token_list = init_tokens();
    int row_no = 1;
    char *row_start = input;
    // NOTE: Make sure to update is_symbol definition when adding new types
    while (!is_empty(input)) {
        if (is_whitespace(input)) {
            input++;
        } else if (is_newline(input)) {
            input++;
            row_no++;
            row_start = input;
        } else if (is_comment(input)) {
            while (!is_newline(input) && !is_empty(input)) input++;
        } else if (is_number(input)) {
            char *start = input;
            while (is_number(input)) input++;
            token *number = create_token(start, input, TOKEN_NUMBER,
                                         row_no, row_start);
            token_list = add_token(number, token_list);
        } else if (is_qoutes(input)) {
            char *prev = input++;
            char *start = input;
            while (!is_qoutes(input) || is_escape_char(prev)) {
                if (is_empty(input)) {
                    int col_no = column(input, row_start);
                    char *err = "missing string delimiter, expected '\"'";
                    return create_error(token_list, row_no, col_no, err, row_start);
                }
                input++;
                prev++;
            }
            token *string = create_token(start, input, TOKEN_STRING,
                                         row_no, row_start);
            token_list = add_token(string, token_list);
            input++;
        } else if (is_reserved_symbol(input)) {
            char *start = input;
            ++input;
            token *res_sym = create_token(start, input, TOKEN_RESERVED_SYMBOL,
                                          row_no, row_start);
            token_list = add_token(res_sym, token_list);
        } else if (is_symbol(input)) {
            char *start = input;
            while (is_symbol(input) && !is_empty(input)) input++;
            if (input != start) {
                token *symbol = create_token(start, input, TOKEN_SYMBOL,
                                             row_no, row_start);
                token_list = add_token(symbol, token_list);
            }
        }
    }

    return token_list;
}

char *token_name(int type) {
    switch (type) {
        case 0:
            return "NUMBER";
        case 1:
            return "STRING";
        case 2:
            return "SYMBOL";
        case 3:
            return "RESERVED_SYMBOL";
        default:
            return "UNKNOWN";
    }
}

void free_context(code_context *c) {
    if (c == NULL) { return; }
    free(c->trace);
    free(c);
}

void free_token(token *t) {
    if (t == NULL) { return; }
    free(t->val);
    free_context(t->context);
    free(t);
}

void free_error(error *e) {
    if (e == NULL) { return; }
    free_context(e->context);
    free(e->val);
    free(e);
}

void free_tokens(tokens *t) {
    if (t == NULL) { return; }

    for (int i = 0; i < t->count; i++) {
        free_token(t->items[i]);
    }

    free(t->items);
    if (t->type == TOKENIZER_ERROR) free_error(t->err);
    free(t);
}