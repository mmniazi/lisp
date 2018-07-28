#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// (def {nil} {})
// (def {true} 1)
// (def {false} 0)
//
// number  : /-?[0-9]+[\.0-9]?/ ;
// string  : /\"(\\\\.|[^\"])*\"/ ;
// comment : /;[^\\r\\n]*/ ;
// symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;
// sexpr   : '(' <expr>* ')' ;
// qexpr   : '{' <expr>* '}' ;
// expr    : <number> | <string> | <symbol> |
//           <comment> | <sexpr> | <qexpr> ;
// lispy   : /^/ <expr>* /$/ ;
enum {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_SYMBOL,
    TYPE_COMMENT
};

typedef struct token {
    int type;
    char *val;
} token;

typedef struct tokens {
    int count;
    token **items;
} tokens;


int is_number(const char *input) {
    return (*input >= '0' && *input <= '9') || *input == '.';
}

token *create_token(char *input, char *start, int type) {
    token *t = malloc(sizeof(token));
    t->type = type;
    t->val = malloc(input - start + 1);
    strncpy(t->val, start, input - start);
    return t;
}

tokens *add_token(token *v, tokens *t) {
    t->count++;
    t->items = realloc(t->items, sizeof(token *) * t->count);
    t->items[t->count - 1] = v;
    return t;
}

void parse(char *input) {
    tokens *token_list = malloc(sizeof(tokens));
    token_list->count = 0;
    token_list->items = malloc(sizeof(token *));

    for (int index = 0; *input != '\0'; input++) {
        if (is_number(input)) {
            char *start = input;
            while (is_number(input))
                input++;
            token *number = create_token(input, start, TYPE_NUMBER);
            token_list = add_token(number, token_list);
        }
    }

    for (int i = 0; i < token_list->count; i++) {
        printf("%s\n", token_list->items[i]->val);
    }
}
