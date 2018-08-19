#include <stdlib.h>
#include "lval.c"

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(args->context, fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, (args)->cell[index]->type == (expect), \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name((args)->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, (args)->count == (num), \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, (args)->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, (args)->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

lval *lval_eval(lenv *e, lval *v);

lval *lval_call(lenv *e, lval *f, lval *a);

lval *builtin_head(lenv *e, lval *a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval *v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval *builtin_tail(lenv *e, lval *a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval *v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval *builtin_list(lenv *e, lval *a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a) {

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval *x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval *builtin_op(lenv *e, lval *a, char *op) {

    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

    /* Pop the first element */
    lval *x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* While there are still elements remaining */
    while (a->count > 0) {

        /* Pop the next element */
        lval *y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                x = lval_err(y->context, "Division By Zero!");
                lval_del(y);
                break;
            }
            x->num /= y->num;
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}

lval *builtin_add(lenv *e, lval *a) {
    return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a) {
    return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a) {
    return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a) {
    return builtin_op(e, a, "/");
}

lval *builtin_var(lenv *e, lval *a, char *func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    /* First argument is symbol list */
    lval *syms = a->cell[0];

    /* Ensure all elements of first list are symbols */
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "Function %s cannot define non-symbol. "
                "Got %s, Expected %s.",
                func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    /* Check correct number of symbols and values */
    LASSERT(a, syms->count == a->count - 1,
            "Function %s should define equal no of values and symbols. "
            "%i symbols, %i values.",
            func, syms->count, a->count - 1);

    /* Assign copies of values to symbols */
    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        }

        if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }

    lval *empty_res = lval_sexpr(a->context);
    lval_del(a);
    return empty_res;
}

lval *builtin_def(lenv *e, lval *a) {
    return builtin_var(e, a, "def");
}

lval *builtin_put(lenv *e, lval *a) {
    return builtin_var(e, a, "=");
}

lval *builtin_exit(lenv *e, lval *a) {
    lval_del(a);
    exit(0);
}

lval *builtin_lambda(lenv *e, lval *a) {
    /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("lambda", a, 2);
    LASSERT_TYPE("lambda", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("lambda", a, 1, LVAL_QEXPR);

    /* Check first Q-Expression contains only Symbols */
    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, Expected %s.",
                ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    /* Pop first two arguments and pass them to lval_lambda */
    lval *formals = lval_pop(a, 0);
    lval *body = lval_pop(a, 0);

    lval *res = lval_lambda(formals, body, a->context);
    lval_del(a);
    return res;
}

lval *builtin_fun(lenv *e, lval *a) {
    /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("fun", a, 2);
    LASSERT_TYPE("fun", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("fun", a, 1, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("fun", a, 0);

    /* Check first Q-Expression contains only Symbols */
    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, Expected %s.",
                ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    lval *formals = lval_pop(a, 0);
    lval *name = lval_pop(formals, 0);
    lval *body = lval_pop(a, 0);

    lval *params = lval_add(lval_qexpr(name->context), lval_add(lval_qexpr(name->context), name));
    lval_add(params, lval_lambda(formals, body, a->context));

    lval_del(a);

    return builtin_def(e, params);
}

lval *builtin_ord(lenv *e, lval *a, char *op) {
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if (strcmp(op, ">") == 0) {
        r = (a->cell[0]->num > a->cell[1]->num);
    }
    if (strcmp(op, "<") == 0) {
        r = (a->cell[0]->num < a->cell[1]->num);
    }
    if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->num >= a->cell[1]->num);
    }
    if (strcmp(op, "<=") == 0) {
        r = (a->cell[0]->num <= a->cell[1]->num);
    }
    lval *num = lval_num(r, a->context);
    lval_del(a);
    return num;
}

lval *builtin_gt(lenv *e, lval *a) {
    return builtin_ord(e, a, ">");
}

lval *builtin_lt(lenv *e, lval *a) {
    return builtin_ord(e, a, "<");
}

lval *builtin_ge(lenv *e, lval *a) {
    return builtin_ord(e, a, ">=");
}

lval *builtin_le(lenv *e, lval *a) {
    return builtin_ord(e, a, "<=");
}

lval *builtin_cmp(lenv *e, lval *a, char *op) {
    LASSERT_NUM(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0) {
        r = lval_eq(a->cell[0], a->cell[1]);
    }
    if (strcmp(op, "!=") == 0) {
        r = !lval_eq(a->cell[0], a->cell[1]);
    }
    lval *num = lval_num(r, a->context);
    lval_del(a);
    return num;
}

lval *builtin_eq(lenv *e, lval *a) {
    return builtin_cmp(e, a, "==");
}

lval *builtin_ne(lenv *e, lval *a) {
    return builtin_cmp(e, a, "!=");
}

lval *builtin_if(lenv *e, lval *a) {
    /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    /* Pop first two arguments and pass them to lval_lambda */
    lval *cond = lval_pop(a, 0);
    lval *if_body = lval_pop(a, 0);
    lval *else_body = lval_pop(a, 0);
    if_body->type = LVAL_SEXPR;
    else_body->type = LVAL_SEXPR;
    lval_del(a);

    if (cond->num != 0) {
        return lval_eval(e, if_body);
    } else {
        return lval_eval(e, else_body);
    }
}

lval *builtin_or(lenv *e, lval *a) {
    LASSERT_NUM("or", a, 2);
    LASSERT_TYPE("or", a, 0, LVAL_NUM);
    LASSERT_TYPE("or", a, 1, LVAL_NUM);

    lval *num = lval_num(a->cell[0]->num || a->cell[1]->num, a->context);

    lval_del(a);

    return num;
}

lval *builtin_and(lenv *e, lval *a) {
    LASSERT_NUM("and", a, 2);
    LASSERT_TYPE("and", a, 0, LVAL_NUM);
    LASSERT_TYPE("and", a, 1, LVAL_NUM);

    lval *num = lval_num(a->cell[0]->num && a->cell[1]->num, a->context);
    lval_del(a);
    return num;
}

lval *builtin_not(lenv *e, lval *a) {
    LASSERT_NUM("or", a, 1);
    LASSERT_TYPE("or", a, 0, LVAL_NUM);

    lval *num = lval_num(!a->cell[0]->num, a->context);

    lval_del(a);

    return num;
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *k = lval_sym(name, NULL);
    lval *v = lval_func(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

lval *builtin_print(lenv *e, lval *a) {

    /* Print each argument followed by a space */
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]);
        putchar(' ');
    }

    /* Print a newline and delete arguments */
    putchar('\n');

    lval *empty_res = lval_sexpr(a->context);
    lval_del(a);
    return empty_res;
}

lval *builtin_error(lenv *e, lval *a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    /* Construct Error from first argument */
    lval *err = lval_err(a->context, a->cell[0]->str);

    /* Delete arguments and return */
    lval_del(a);
    return err;
}

lval *lval_eval_sexpr(lenv *e, lval *v) {

    /* Evaluate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error Checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty Expression */
    if (v->count == 0) { return v; }

    /* Single Expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure First Element is Function */
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval *err = lval_err(
                f->context,
                "S-Expression starts with incorrect type. "
                "Got %s, Expected %s.",
                ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }

    lval *result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

lval *lval_eval(lenv *e, lval *v) {
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}


lval *lval_call(lenv *e, lval *f, lval *a) {

    /* If Builtin then simply apply that */
    if (f->builtin) { return f->builtin(e, a); }

    /* Record Argument Counts */
    int given = a->count;
    int total = f->formals->count;

    /* While arguments still remain to be processed */
    while (a->count) {

        /* If we've ran out of formal arguments to bind */
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err(f->context,
                            "Function passed too many arguments. "
                            "Got %i, Expected %i.", given, total);
        }

        /* Pop the first symbol from the formals */
        lval *sym = lval_pop(f->formals, 0);

        /* Special Case to deal with '&' (varargs) */
        if (strcmp(sym->sym, "&") == 0) {

            /* Ensure '&' is followed by another symbol */
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err(sym->context,
                                "Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }

            /* Next formal should be bound to remaining arguments */
            lval *nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        /* Pop the next argument from the list */
        lval *val = lval_pop(a, 0);

        /* Bind a copy into the function's environment */
        lenv_put(f->env, sym, val);

        /* Delete symbol and value */
        lval_del(sym);
        lval_del(val);
    }

    /* Argument list is now bound so can be cleaned up */
    lval_del(a);

    /* If '&' remains in formal list bind to empty list */
    if (f->formals->count > 0 &&
        strcmp(f->formals->cell[0]->sym, "&") == 0) {

        /* Check to ensure that & is not passed invalidly. */
        if (f->formals->count != 2) {
            return lval_err(f->context,
                            "Function format invalid. "
                            "Symbol '&' not followed by single symbol.");
        }

        /* Pop and delete '&' symbol */
        lval_del(lval_pop(f->formals, 0));

        /* Pop next symbol and create empty list */
        lval *sym = lval_pop(f->formals, 0);
        lval *val = lval_qexpr(sym->context);

        /* Bind to environment and delete */
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    /* If all formals have been bound evaluate */
    if (f->formals->count == 0) {

        /* Set environment parent to evaluation environment */
        f->env->parent = e;

        lval *body = lval_add(lval_sexpr(f->body->context), lval_copy(f->body));

        /* Evaluate and return */
        return builtin_eval(f->env, body);
    } else {
        /* Otherwise return partially evaluated function */
        return lval_copy(f);
    }

}

lval *builtin_load_file(lenv *e, char *file, code_context *c) {
    char *file_content = load_file(file);
    if (file_content == NULL)
        return lval_err(c, "Could not load '%s': Failed to load file", file);

    ast *tree = parse(file_content);

    if (tree->type != AST_ERROR) {
        lval *expr = lval_read(tree);

        /* Evaluate each Expression */
        while (expr->count) {
            lval *x = lval_eval(e, lval_pop(expr, 0));
            /* If Evaluation leads to error print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        free_ast(tree);
        lval_del(expr);

        return lval_sexpr(c);
    } else {
        lval *err = lval_err(tree->context, "Could not load %s: \n%s", file, tree->val);

        free_ast(tree);

        return err;
    }
}

lval *builtin_load_file_lval(lenv *e, lval *file) {
    LASSERT_NUM("load", file, 1);
    LASSERT_TYPE("load", file, 0, LVAL_STR);

    return builtin_load_file(e, file->cell[0]->str, file->context);
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
    lenv_put(e, lval_sym("true", NULL), lval_num(1, NULL));
    lenv_put(e, lval_sym("false", NULL), lval_num(0, NULL));

    /* Generic Functions */
    /* String Functions */
    lenv_add_builtin(e, "load", builtin_load_file_lval);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);
}

void load_input_files(int argc, char **argv, lenv *e) {
    /* Supplied with list of files */
    if (argc >= 2) {

        /* loop over each supplied filename */
        for (int i = 1; i < argc; i++) {

            /* Pass to builtin load and get the result */
            lval *x = builtin_load_file(e, argv[i], NULL);

            /* If the result is an error be sure to print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }
    }
}