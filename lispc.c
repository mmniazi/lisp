#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "libs/mpc.h"
#include "builtins.c"

mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispy;

lval *builtin_load_file(lenv *e, lval *a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    /* Parse File given by string name */
    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {

        /* Read contents */
        lval *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        /* Evaluate each Expression */
        while (expr->count) {
            lval *x = lval_eval(e, lval_pop(expr, 0));
            /* If Evaluation leads to error print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        /* Delete expressions and arguments */
        lval_del(expr);
        lval_del(a);

        /* Return empty list */
        return lval_sexpr();

    } else {
        /* Get Parse Error as String */
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        /* Create new error message using it */
        lval *err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        /* Cleanup and return error */
        return err;
    }
}

void lenv_add_builtins(lenv *e) {
    /* List Functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    /* Variable Functions */
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);


    /* Exit REPL */
    lenv_add_builtin(e, "exit", builtin_exit);

    /* User defined functions */
    lenv_add_builtin(e, "lambda", builtin_lambda);
    lenv_add_builtin(e, "fun", builtin_fun);

    /* Conditionals Functions */
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, "<", builtin_gt);
    lenv_add_builtin(e, "<=", builtin_ge);
    lenv_add_builtin(e, ">", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_le);
    lenv_add_builtin(e, "||", builtin_or);
    lenv_add_builtin(e, "&&", builtin_and);
    lenv_add_builtin(e, "!", builtin_not);
    lenv_put(e, lval_sym("true"), lval_num(1));
    lenv_put(e, lval_sym("false"), lval_num(0));

    /* Generic Functions */
    /* String Functions */
    lenv_add_builtin(e, "load",  builtin_load_file);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);
}

void repl(lenv *e) {
    puts("Lispy Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        char *input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval *x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);

    }
#pragma clang diagnostic pop
}

void define_grammer() {
    Number = mpc_new("number");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Symbol = mpc_new("symbol");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "number  : /-?[0-9]+/ ;                              "
              "string  : /\"(\\\\.|[^\"])*\"/ ;                    "
              "comment : /;[^\\r\\n]*/ ;                           "
              "symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        "
              "sexpr   : '(' <expr>* ')' ;                         "
              "qexpr   : '{' <expr>* '}' ;                         "
              "expr    : <number> | <string> | <symbol> |          "
              "          <comment> | <sexpr> | <qexpr> ;           "
              "lispy   : /^/ <expr>* /$/ ;                         ",
              Number, String, Comment, Symbol, Sexpr, Qexpr, Expr, Lispy);
}

void load_input_files(int argc, char **argv, lenv *e) {
    /* Supplied with list of files */
    if (argc >= 2) {

        /* loop over each supplied filename (starting from 1) */
        for (int i = 1; i < argc; i++) {

            /* Argument list with a single argument, the filename */
            lval *args = lval_add(lval_sexpr(), lval_str(argv[i]));

            /* Pass to builtin load and get the result */
            lval *x = builtin_load_file(e, args);

            /* If the result is an error be sure to print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }
    }
}

int main(int argc, char **argv) {
    define_grammer();

    lenv *global_env = lenv_new();
    lenv_add_builtins(global_env);

    load_input_files(argc, argv, global_env);

    repl(global_env);

    mpc_cleanup(7, Number, String, Comment, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}