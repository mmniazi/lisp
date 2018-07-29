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

typedef struct token {
    int type;
    int row;
    int col;
    char *val;
    char *context;
} token;

typedef struct error {
    int row;
    int col;
    char *val;
    char *context;
} error;

typedef struct tokens {
    int type;
    int count;
    token **items;
    error *err;
} tokens;

typedef struct rows {
    int count;
    char **items;
} rows;


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

int is_space(const char *input) {
    return *input == ' ';
}

int is_escape_char(const char *input) {
    return *input == '\\';
}

int is_reserved_symbol(const char *input) {
    return exists(input, "{}()");
}

int is_symbol(const char *input) {
    return !is_space(input) && !is_reserved_symbol(input);
}

int col_number(const char *curr_loc, const char *row_start) {
    return (int) (curr_loc - row_start + 1);
}

token *create_token(const char *start, const char *curr_loc, int type,
                    int row_no, const char *row) {
    int col = col_number(start, row);
    token *t = malloc(sizeof(token));
    t->row = row_no;
    t->col = col;
    t->context = malloc(strlen(row) + 1);
    strcpy(t->context, row);
    t->type = type;
    unsigned long len = curr_loc - start;
    t->val = malloc(len + 1);
    strncpy(t->val, start, len);
    t->val[len] = '\0';
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

tokens *create_error(tokens *t, int row, int col, char *err_str, char *context) {
    t->type = TOKENIZER_ERROR;
    error *err = malloc(sizeof(error));
    err->row = row;
    err->col = col;
    err->val = malloc(strlen(err_str) + 1);
    strcpy(err->val, err_str);
    err->context = malloc(strlen(context) + 1);
    strcpy(err->context, context);
    t->err = err;
    return t;
}

rows *to_rows(char *string) {
    rows *r = malloc(sizeof(r));
    r->items = NULL;
    r->count = 0;
    char *p = strtok(string, "\n");

    while (p) {
        r->items = realloc(r->items, sizeof(char *) * ++r->count);
        r->items[r->count - 1] = p;
        p = strtok(NULL, "\n");
    }

    return r;
}

tokens *tokenize_row(char *input, int row_no, tokens *token_list) {
    char *row = input;
    while (!is_empty(input)) {
        if (is_number(input)) {
            char *start = input;
            while (is_number(input)) input++;
            token *number = create_token(start, input, TOKEN_NUMBER,
                                         row_no, row);
            token_list = add_token(number, token_list);
        } else if (is_qoutes(input)) {
            char *prev = input++;
            char *start = input;
            while (!is_qoutes(input) || is_escape_char(prev)) {
                if (is_empty(input)) {
                    int col_no = col_number(input, row);
                    char *err = "missing string delimiter, expected \"";
                    return create_error(token_list, row_no, col_no, err, row);
                }
                input++;
                prev++;
            }
            token *string = create_token(start, input, TOKEN_STRING,
                                         row_no, row);
            token_list = add_token(string, token_list);
            input++;
        } else if (is_comment(input)) {
            while (!is_empty(input)) input++;
        } else if (is_symbol(input)) {
            char *start = input;
            while (is_symbol(input) && !is_empty(input)) input++;
            if (input != start) {
                token *symbol = create_token(start, input, TOKEN_SYMBOL,
                                             row_no, row);
                token_list = add_token(symbol, token_list);
            }
        } else if (is_reserved_symbol(input)) {
            token *res_sym = create_token(input, ++input, TOKEN_RESERVED_SYMBOL,
                                          row_no, row);
            token_list = add_token(res_sym, token_list);
        } else if (is_space(input)) {
            input++;
        } else {
            int col_no = col_number(input, row);
            char *err = "Failed to tokenize";
            return create_error(token_list, row_no, col_no, err, row);
        }
    }

    return token_list;
}


tokens *tokenize(char *input) {
    tokens *token_list = init_tokens();
    rows *r = to_rows(input);

    for (int row_no = 0; row_no < r->count; row_no++) {
        char *row = r->items[row_no];
        token_list = tokenize_row(row, row_no + 1, token_list);
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

void debug_tokenizer(tokens *t) {
    if (t->type == TOKENIZER_TOKENS) {
        for (int token_no = 0; token_no < t->count; token_no++) {
            char *type = token_name(t->items[token_no]->type);
            char *value = t->items[token_no]->val;
            int row_no = t->items[token_no]->row;
            int col_no = t->items[token_no]->col;
            printf("row %d column %d: %s(%s)\n", row_no, col_no, type, value);
            printf("Context:\n");
            printf("%s\n", t->items[token_no]->context);
            for (int i = 1; i < t->items[token_no]->col; i++) printf(" ");
            puts("^\n");
        }
    } else if (t->type == TOKENIZER_ERROR) {
        printf("row %d column %d: %s\n", t->err->row, t->err->col, t->err->val);
        printf("Context:\n");
        printf("%s\n", t->err->context);
        for (int i = 1; i < t->err->col; i++) printf(" ");
        puts("^\n");
    }
}