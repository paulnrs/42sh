#ifndef EVALUATE_H
#define EVALUATE_H

#include "../ast/ast.h"

struct key_value
{
    char *key;
    char *value;
    int arg;
};

struct key_func
{
    char *key;
    struct ast *ast;
};

struct dico
{
    struct key_value **entries;
    struct key_func **func;
    size_t size_v;
    size_t size_f;
    int continuef;
    int breakf;
    int nb_arg;
};

int evaluate(struct ast *ast);

int ast_evaluate(struct ast *ast, struct dico *d);

#endif /* !EVALUATE_H */
