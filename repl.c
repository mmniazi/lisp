#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "builtins.c"

void repl(lenv *e) {
    puts("Lisp version 0.0.1");
    puts("Enter exit for closing repl\n");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        char *input = readline("lisp> ");
        add_history(input);

        ast *tree = parse(input);
        if (tree->type != AST_ERROR) {
            lval *x = lval_eval(e, lval_read(tree));
            lval_println(x);
            lval_del(x);
        } else {
            lval *err = lval_err(tree->context, ast_error_str(tree->context, tree->val));
            lval_println(err);
        }

        free_ast(tree);
        free(input);

    }
#pragma clang diagnostic pop
}

int main(int argc, char **argv) {
    lenv *global_env = lenv_new();
    lenv_add_builtins(global_env);
    load_input_files(argc, argv, global_env);

    repl(global_env);

    return 0;
}