#include "ast.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

struct ast *ast_new(enum ast_type type)
{
    struct ast *new = calloc(1, sizeof(struct ast));
    if (!new)
        return NULL;
    new->type = type;
    return new;
}

void data_free(struct ast *ast)
{
    for (int i = 0; i < ast->nb_data; i++)
        if (ast->data[i])
            free(ast->data[i]);
    free(ast->data);
}

void ast_free(struct ast *ast)
{
    if (!ast)
        return;
    for (int i = 0; i < ast->nb_ast; i++)
        ast_free(ast->ast_list[i]);
    data_free(ast);
    free(ast->ast_list);
    free(ast);
}
