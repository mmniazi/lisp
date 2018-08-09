#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "parser.c"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR
};

typedef lval *(*lbuiltin)(lenv *, lval *);

void lval_print(lval *v);

lenv *lenv_new(void);

void lenv_del(lenv *e);

lenv *lenv_copy(lenv *e);

void lenv_put(lenv *e, lval *k, lval *v);

struct lval {
    int type;

    /* Basic */
    long num;
    char *err;
    char *sym;
    char *str;
    code_context *context;

    /* Function */
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

    /* Expression */
    int count;
    lval **cell;
};

struct lenv {
    lenv *par;
    int count;
    char **syms;
    lval **vals;
};

lval *lval_num(long x, code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    v->context = copy_context(c);
    return v;
}

lval *lval_str(char *x, code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(x) + 1);
    strcpy(v->str, x);
    v->context = copy_context(c);
    return v;
}

lval *lval_err(code_context *c, char *fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->context = copy_context(c);

    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    /* Allocate 512 bytes of space */
    v->err = malloc(512);

    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);

    /* Reallocate to number of bytes actually used */
    v->err = realloc(v->err, strlen(v->err) + 1);

    /* Cleanup our va list */
    va_end(va);

    return v;
}

lval *lval_sym(char *s, code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    v->context = copy_context(c);
    strcpy(v->sym, s);
    return v;
}

lval *lval_sexpr(code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    v->context = copy_context(c);
    return v;
}

lval *lval_qexpr(code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    v->context = copy_context(c);
    return v;
}

lval *lval_func(lbuiltin func) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    v->context = NULL;
    return v;
}

lval *lval_lambda(lval *formals, lval *body, code_context *c) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    /* Set Builtin to Null */
    v->builtin = NULL;

    /* Build new environment */
    v->env = lenv_new();

    /* Set Formals and Body */
    v->formals = formals;
    v->body = body;
    v->context = copy_context(c);
    return v;
}

void lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_STR:
            free(v->str);
            break;
        default:
            break;
    }
    free_context(v->context);
    free(v);
}

lval *lval_read_num(ast *t) {
    errno = 0;
    long x = strtol(t->val, NULL, 10);
    return errno != ERANGE ?
           lval_num(x, t->context) : lval_err(t->context, "invalid number");
}

lval *lval_add(lval *v, lval *x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

lval *lval_read(ast *t) {
    if (t->type == AST_NUMBER) { return lval_read_num(t); }
    if (t->type == AST_STRING) { return lval_str(t->val, t->context); }
    if (t->type == AST_SYMBOL) { return lval_sym(t->val, t->context); }

    lval *v = NULL;
    if (t->type == AST_SEXPR) { v = lval_sexpr(t->context); }
    else if (t->type == AST_QEXPR) { v = lval_qexpr(t->context); }

    for (int i = 0; i < t->child_count; i++)
        v = lval_add(v, lval_read(t->children[i]));

    return v;
}

lval *lval_copy(lval *v) {

    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {

        /* Copy Functions and Numbers Directly */
        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;

        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

            /* Copy Lists by copying each sub-expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval *) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;
        default:
            break;
    }

    return x;
}

int lval_eq(lval *x, lval *y) {

    /* Different Types are always unequal */
    if (x->type != y->type) { return 0; }

    /* Compare Based upon type */
    switch (x->type) {
        /* Compare Number Value */
        case LVAL_NUM:
            return (x->num == y->num);

            /* Compare String Values */
        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);

            /* If builtin compare, otherwise compare formals and body */
        case LVAL_FUN:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals)
                       && lval_eq(x->body, y->body);
            }

            /* If list compare every individual element */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                /* If any element not equal then whole list not equal */
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }
            /* Otherwise lists must be equal */
            return 1;
        case LVAL_STR:
            return (strcmp(x->str, y->str) == 0);
        default:
            return 0;
    }
}

lval *lval_pop(lval *v, int i) {
    /* Find the item at "i" */
    lval *x = v->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i + 1],
            sizeof(lval *) * (v->count - i - 1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    return x;
}

lval *lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval *lval_join(lval *x, lval *y) {

    /* For each cell in 'y' add it to 'x' */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    /* Delete the empty 'y' and return 'x' */
    lval_del(y);
    return x;
}

char *ltype_name(int t) {
    switch (t) {
        case LVAL_FUN:
            return "Function";
        case LVAL_NUM:
            return "Number";
        case LVAL_ERR:
            return "Error";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        case LVAL_STR:
            return "String";
        default:
            return "Unknown";
    }
}

void lval_expr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        /* Print Value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->num);
            break;
        case LVAL_ERR:
            printf("Error on row %d column %d: %s\n"
                   "Stack Trace:\n%s\n",
                   v->context->row, v->context->col, v->err, v->context->trace);
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(lambda ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_STR:
            printf("\"%s\"", v->str);
            break;
        default:
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

lenv *lenv_new(void) {
    lenv *e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval *lenv_get(lenv *e, lval *k) {

    /* Iterate over all items in environment */
    for (int i = 0; i < e->count; i++) {
        /* Check if the stored string matches the symbol string */
        /* If it does, return a copy of the value */
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    /* If no symbol check in parent otherwise error */
    if (e->par) {
        return lenv_get(e->par, k);
    } else {
        return lval_err(k->context, "Unbound Symbol '%s'", k->sym);
    }
}

void lenv_put(lenv *e, lval *k, lval *v) {

    /* Iterate over all items in environment */
    /* This is to see if variable already exists */
    for (int i = 0; i < e->count; i++) {

        /* If variable is found delete item at that position */
        /* And replace with variable supplied by user */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* If no existing entry found allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);
    e->syms = realloc(e->syms, sizeof(char *) * e->count);

    /* Copy contents of lval and symbol string into new location */
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

lenv *lenv_copy(lenv *e) {
    lenv *n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char *) * n->count);
    n->vals = malloc(sizeof(lval *) * n->count);
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

void lenv_def(lenv *e, lval *k, lval *v) {
    /* Iterate till e has no parent */
    while (e->par) { e = e->par; }
    /* Put value in e */
    lenv_put(e, k, v);
}