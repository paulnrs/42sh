#include "parser.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum parser_status parse_list(struct ast **res, struct lexer *lexer);
static enum parser_status parse_and_or(struct ast **res, struct lexer *lexer);
static enum parser_status parse_pipeline(struct ast **res, struct lexer *lexer);
static enum parser_status parse_command(struct ast **res, struct lexer *lexer);
static enum parser_status parse_simple_command(struct ast **res,
                                               struct lexer *lexer);
static enum parser_status parse_shell_command(struct ast **res,
                                              struct lexer *lexer);
static enum parser_status parse_rule_if(struct ast **res, struct lexer *lexer);
static enum parser_status parse_else(struct ast **res, struct lexer *lexer);
static enum parser_status parse_compound_list(struct ast **res,
                                              struct lexer *lexer);
static enum parser_status parse_rule_while(struct ast **res,
                                           struct lexer *lexer);
static enum parser_status parse_rule_until(struct ast **res,
                                           struct lexer *lexer);
static enum parser_status parse_prefix(struct ast **res, struct lexer *lexer);
static enum parser_status parse_redirection(struct ast **res,
                                            struct lexer *lexer);
static enum parser_status parse_rule_for(struct ast **res, struct lexer *lexer);
static enum parser_status parse_functions(struct ast **res,
                                          struct lexer *lexer);

static struct ast *create_ast(enum ast_type type, char *data)
{
    struct ast *new = calloc(1, sizeof(struct ast));
    new->type = type;
    if (strcmp(data, ""))
    {
        new->data = calloc(1, sizeof(char *));
        new->data[0] = data;
        new->nb_data = 1;
    }
    return new;
}

static void free_pop(struct token token, struct lexer *lexer)
{
    token_free(token);
    lexer_pop(lexer);
}

static enum parser_status free_all(struct ast **res, struct lexer *lexer,
                                   struct token token)
{
    token_free(token);
    if (lexer->pos != 0 && *res)
        ast_free(*res);
    *res = NULL;
    return PARSER_UNEXPECTED_TOKEN;
}

struct ast *add_child(struct ast *parent, struct ast *child)
{
    parent->ast_list =
        realloc(parent->ast_list, (parent->nb_ast + 1) * sizeof(struct ast));
    parent->ast_list[parent->nb_ast] = child;
    parent->nb_ast++;
    return parent;
}

struct ast *add_data(struct ast *ast, char *data)
{
    ast->data = realloc(ast->data, (ast->nb_data + 1) * sizeof(char *));
    ast->data[ast->nb_data] = data;
    ast->nb_data++;
    return ast;
}

enum parser_status parse(struct ast **res, struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_EOF || token.type == TOKEN_BACKSLASH)
        return PARSER_OK;
    if (parse_list(res, lexer) != PARSER_OK)
    {
        fprintf(stderr, "Error on parsing\n");
        return free_all(res, lexer, token);
    }
    token_free(token);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_EOF && token.type != TOKEN_BACKSLASH)
    {
        fprintf(stderr, "Error on parsing\n");
        return free_all(res, lexer, token);
    }
    token_free(token);
    return PARSER_OK;
}

static enum parser_status parse_list(struct ast **res, struct lexer *lexer)
{
    if (parse_and_or(res, lexer) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_ERROR)
        return PARSER_UNEXPECTED_TOKEN;
    struct ast *ast = create_ast(AST_LIST, "");
    ast->ast_list =
        realloc(ast->ast_list, (ast->nb_ast + 1) * sizeof(struct ast));
    ast->ast_list[ast->nb_ast] = *res;
    ast->nb_ast++;
    while (token.type == TOKEN_SEMI_COLON)
    {
        token_free(token);
        lexer_pop(lexer);
        struct ast *new_ast = NULL;
        struct ast *keep = *res;
        if (parse_and_or(&new_ast, lexer) != PARSER_OK)
        {
            *res = keep;
            break;
        }
        ast->ast_list =
            realloc(ast->ast_list, (ast->nb_ast + 1) * sizeof(struct ast));
        ast->ast_list[ast->nb_ast] = new_ast;
        ast->nb_ast++;
        token = lexer_peek(lexer);
    }
    token_free(token);
    *res = ast;
    return PARSER_OK;
}

static enum parser_status parse_and_or(struct ast **res, struct lexer *lexer)
{
    struct ast *child = NULL;
    if (parse_pipeline(&child, lexer) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;
    struct token token = lexer_peek(lexer);
    struct ast *ast = NULL;
    if (token.type == TOKEN_AND)
    {
        ast = create_ast(AST_AND, "");
        ast = add_child(ast, child);
    }
    else if (token.type == TOKEN_OR)
    {
        ast = create_ast(AST_OR, "");
        ast = add_child(ast, child);
    }
    while (token.type == TOKEN_AND || token.type == TOKEN_OR)
    {
        if ((token.type == TOKEN_AND && ast->type == AST_OR)
            || (token.type == TOKEN_OR && ast->type == AST_AND))
        {
            child = NULL;
            child = create_ast(AST_LIST, "");
            child = add_child(child, ast);
            ast = child;
        }
        ast->type = token.type == TOKEN_AND ? AST_AND : AST_OR;
        lexer_pop(lexer);
        token = lexer_peek(lexer);
        while (token.type == TOKEN_BACKSLASH)
        {
            lexer_pop(lexer);
            token = lexer_peek(lexer);
        }
        token_free(token);
        child = NULL;
        if (parse_pipeline(&child, lexer) != PARSER_OK)
        {
            ast_free(ast);
            ast_free(child);
            return PARSER_UNEXPECTED_TOKEN;
        }
        ast = add_child(ast, child);
        token = lexer_peek(lexer);
    }
    token_free(token);
    if (!ast)
        ast = child;
    *res = ast;
    return PARSER_OK;
}

static enum parser_status parse_pipeline(struct ast **res, struct lexer *lexer)
{
    struct ast *ast = NULL;
    struct ast *neg = NULL;
    struct ast *child = NULL;
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_NEG)
    {
        neg = create_ast(AST_NEG, "");
        lexer_pop(lexer);
    }
    token_free(token);
    if (parse_command(&child, lexer) != PARSER_OK)
    {
        ast_free(neg);
        return PARSER_UNEXPECTED_TOKEN;
    }
    token = lexer_peek(lexer);
    if (token.type == TOKEN_PIPE)
    {
        ast = create_ast(AST_PIPE, "");
        ast = add_child(ast, child);
    }
    while (token.type == TOKEN_PIPE)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
        while (token.type == TOKEN_BACKSLASH)
        {
            lexer_pop(lexer);
            token = lexer_peek(lexer);
        }
        token_free(token);
        child = NULL;
        if (parse_command(&child, lexer) != PARSER_OK)
        {
            ast_free(ast);
            ast_free(child);
            return PARSER_UNEXPECTED_TOKEN;
        }
        ast = add_child(ast, child);
        token = lexer_peek(lexer);
    }
    token_free(token);
    if (!ast)
        ast = child;
    if (neg)
    {
        neg = add_child(neg, ast);
        *res = neg;
    }
    else
        *res = ast;
    return PARSER_OK;
}

static enum parser_status parse_command(struct ast **res, struct lexer *lexer)
{
    size_t keep_pos = lexer->pos;
    struct ast *shell = NULL;
    if (parse_shell_command(&shell, lexer) == PARSER_OK)
    {
        size_t keep_pos = lexer->pos;
        struct ast *child = NULL;
        while (parse_redirection(&child, lexer) == PARSER_OK)
        {
            child = add_child(child, shell);
            shell = child;
            child = NULL;
            keep_pos = lexer->pos;
        }
        lexer->pos = keep_pos;
        *res = shell;
        return PARSER_OK;
    }
    lexer->pos = keep_pos;
    if (parse_functions(&shell, lexer) == PARSER_OK)
    {
        size_t keep_pos = lexer->pos;
        struct ast *child = NULL;
        while (parse_redirection(&child, lexer) == PARSER_OK)
        {
            child = add_child(child, shell);
            shell = child;
            child = NULL;
            keep_pos = lexer->pos;
        }
        lexer->pos = keep_pos;
        *res = shell;
        return PARSER_OK;
    }
    lexer->pos = keep_pos;
    if (parse_simple_command(res, lexer) == PARSER_OK)
        return PARSER_OK;
    return PARSER_UNEXPECTED_TOKEN;
}

static struct token pop_peek(struct token token, struct lexer *lexer,
                             size_t *lexer_pos)
{
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    if (lexer_pos)
    {
        *lexer_pos = lexer->pos;
    }
    return token;
}

static int cond(struct token token)
{
    return ((strcmp(token.data, "") || token.type == TOKEN_WORD)
            && token.type != TOKEN_REDIR);
}

static enum parser_status parse_simple_command(struct ast **res,
                                               struct lexer *lexer)
{
    int prefix = 0;
    struct ast *ast_pre = NULL;
    struct ast *child = NULL;
    size_t keep_pos = lexer->pos;
    while (parse_prefix(&child, lexer) == PARSER_OK)
    {
        if (!ast_pre)
            ast_pre = child;
        else
        {
            child = add_child(child, ast_pre);
            ast_pre = child;
        }
        child = NULL;
        prefix = 1;
        keep_pos = lexer->pos;
    }
    lexer->pos = keep_pos;
    struct token token = lexer_peek(lexer);
    if (token.type != TOKEN_WORD)
    {
        if (prefix)
        {
            *res = ast_pre;
            return PARSER_OK;
        }
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *ast = create_ast(AST_COMMAND, token.data);
    token = pop_peek(token, lexer, &keep_pos);
    child = NULL;
    keep_pos = lexer->pos;
    while (cond(token) || parse_redirection(&child, lexer) == PARSER_OK)
    {
        if (child)
        {
            child = add_child(child, ast);
            ast = child;
            child = NULL;
        }
        else
        {
            ast = add_data(ast, token.data);
            token = pop_peek(token, lexer, NULL);
        }
        keep_pos = lexer->pos;
    }
    if (ast_pre)
        ast = add_child(ast_pre, ast);
    lexer->pos = keep_pos;
    token_free(token);
    *res = ast;
    return PARSER_OK;
}

static enum parser_status parse_prefix(struct ast **res, struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_ASSIGNMENT_WORD)
    {
        *res = create_ast(AST_ASSIGNMENT_WORD, token.data);
        lexer_pop(lexer);
        return PARSER_OK;
    }
    token_free(token);
    if (parse_redirection(res, lexer) == PARSER_OK)
        return PARSER_OK;
    return PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status parse_redirection(struct ast **res,
                                            struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_IONUMBER)
    {
        token_free(token);
        lexer_pop(lexer);
        token = lexer_peek(lexer);
        // ast node IONUMBER i guess
    }
    if (token.type != TOKEN_REDIR)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *ast = create_ast(AST_REDIR, token.data);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_WORD)
    {
        ast_free(ast);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    ast = add_data(ast, token.data);
    lexer_pop(lexer);
    *res = ast;
    return PARSER_OK;
}

static enum parser_status parse_shell_command2(struct ast **res,
                                               struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    struct ast *child = NULL;
    if (token.type == TOKEN_LEFT_BRACKET)
    {
        lexer_pop(lexer);
        if (parse_compound_list(&child, lexer) == PARSER_OK)
        {
            token = lexer_peek(lexer);
            if (token.type == TOKEN_RIGHT_BRACKET)
            {
                struct ast *ast = create_ast(AST_COMMAND_BLOCK, "");
                ast = add_child(ast, child);
                token_free(token);
                lexer_pop(lexer);
                *res = ast;
                return PARSER_OK;
            }
            token_free(token);
            lexer_pop(lexer);
        }
        ast_free(child);
        child = NULL;
    }
    else if (token.type == TOKEN_LEFT_PARENTHESIS)
    {
        lexer_pop(lexer);
        if (parse_compound_list(&child, lexer) == PARSER_OK)
        {
            token = lexer_peek(lexer);
            if (token.type == TOKEN_RIGHT_PARENTHESIS)
            {
                struct ast *ast = create_ast(AST_SUBSHELL, "");
                ast = add_child(ast, child);
                token_free(token);
                lexer_pop(lexer);
                *res = ast;
                return PARSER_OK;
            }
            token_free(token);
            lexer_pop(lexer);
        }
        ast_free(child);
        token_free(token);
    }
    else
        token_free(token);
    return PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status parse_shell_command(struct ast **res,
                                              struct lexer *lexer)
{
    struct ast *keep = *res;
    size_t keep_pos = lexer->pos;
    if (parse_rule_if(res, lexer) == PARSER_OK)
        return PARSER_OK;
    *res = keep;
    lexer->pos = keep_pos;
    if (parse_rule_while(res, lexer) == PARSER_OK)
        return PARSER_OK;
    *res = keep;
    lexer->pos = keep_pos;
    if (parse_rule_until(res, lexer) == PARSER_OK)
        return PARSER_OK;
    *res = keep;
    lexer->pos = keep_pos;
    if (parse_rule_for(res, lexer) == PARSER_OK)
        return PARSER_OK;
    *res = keep;
    lexer->pos = keep_pos;
    return parse_shell_command2(res, lexer);
}

static enum parser_status parse_rule_while(struct ast **res,
                                           struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_ERROR)
        return PARSER_UNEXPECTED_TOKEN;
    if (token.type != TOKEN_WHILE)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *child = NULL;
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *new_ast = create_ast(AST_WHILE, "");
    new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    child = NULL;
    if (token.type != TOKEN_DO)
    {
        token_free(token);
        ast_free(new_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_DONE)
    {
        ast_free(new_ast);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    *res = new_ast;
    return PARSER_OK;
}

static enum parser_status parse_rule_until(struct ast **res,
                                           struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_ERROR)
        return PARSER_UNEXPECTED_TOKEN;
    if (token.type != TOKEN_UNTIL)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *child = NULL;
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *new_ast = create_ast(AST_UNTIL, "");
    new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    child = NULL;
    if (token.type != TOKEN_DO)
    {
        token_free(token);
        ast_free(new_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_DONE)
    {
        ast_free(new_ast);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    *res = new_ast;
    return PARSER_OK;
}

static enum parser_status parse_rule_if(struct ast **res, struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type != TOKEN_IF)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *child = NULL;
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *new_ast = create_ast(AST_IF, "");
    new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_THEN)
    {
        token_free(token);
        ast_free(new_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    child = NULL;
    free_pop(token, lexer);
    if (parse_compound_list(&child, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        ast_free(child);
        return PARSER_UNEXPECTED_TOKEN;
    }
    new_ast = add_child(new_ast, child);
    child = NULL;
    size_t keep_pos = lexer->pos;
    if (parse_else(&child, lexer) != PARSER_OK)
    {
        ast_free(child);
        lexer->pos = keep_pos;
    }
    else
        new_ast = add_child(new_ast, child);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_FI)
    {
        token_free(token);
        ast_free(new_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    *res = new_ast;
    return PARSER_OK;
}

static enum parser_status parse_for_in_sequence(struct ast **ast_for,
                                                struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    while (token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    if (token.type != TOKEN_IN)
    {
        token_free(token);
        ast_free(*ast_for);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    token = lexer_peek(lexer);
    while (token.type == TOKEN_WORD)
    {
        *ast_for = add_data(*ast_for, token.data);
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    if (token.type != TOKEN_SEMI_COLON && token.type != TOKEN_BACKSLASH)
    {
        token_free(token);
        ast_free(*ast_for);
        return PARSER_UNEXPECTED_TOKEN;
    }
    lexer_pop(lexer);
    token_free(token);
    return PARSER_OK;
}

static enum parser_status parse_compound_list_or_free(struct ast **child,
                                                      struct ast **parent,
                                                      struct lexer *lexer)
{
    if (parse_compound_list(child, lexer) != PARSER_OK)
    {
        ast_free(*child);
        ast_free(*parent);
        return PARSER_UNEXPECTED_TOKEN;
    }
    return PARSER_OK;
}

static enum parser_status
handle_done_error(struct ast **ast_for, struct ast **child, struct token *token)
{
    ast_free(*ast_for);
    ast_free(*child);
    token_free(*token);
    return PARSER_UNEXPECTED_TOKEN;
}

static enum parser_status parse_rule_for(struct ast **res, struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type != TOKEN_FOR)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_WORD)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *ast_for = create_ast(AST_FOR, token.data);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    if (token.type == TOKEN_SEMI_COLON)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    else
    {
        token_free(token);
        if (parse_for_in_sequence(&ast_for, lexer) != PARSER_OK)
            return PARSER_UNEXPECTED_TOKEN;
        token = lexer_peek(lexer);
    }
    while (token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    if (token.type != TOKEN_DO)
    {
        token_free(token);
        ast_free(ast_for);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    struct ast *child = NULL;
    if (parse_compound_list_or_free(&child, &ast_for, lexer) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;
    ast_for = add_child(ast_for, child);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_DONE)
        return handle_done_error(&ast_for, &child, &token);
    free_pop(token, lexer);
    *res = ast_for;
    return PARSER_OK;
}

static enum parser_status parse_else(struct ast **res, struct lexer *lexer)
{
    int is_else = 0;
    struct token token = lexer_peek(lexer);
    if (token.type != TOKEN_ELSE && token.type != TOKEN_ELIF)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    if (token.type == TOKEN_ELSE)
        is_else = 1;
    free_pop(token, lexer);
    struct ast *new_ast = NULL;
    if (parse_compound_list(&new_ast, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    if (is_else)
    {
        *res = new_ast;
        return PARSER_OK;
    }
    token = lexer_peek(lexer);
    if (token.type != TOKEN_THEN)
    {
        ast_free(new_ast);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    free_pop(token, lexer);
    struct ast *if_ast = create_ast(AST_IF, "");
    if_ast = add_child(if_ast, new_ast);
    new_ast = NULL;
    if (parse_compound_list(&new_ast, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        ast_free(if_ast);
        return PARSER_UNEXPECTED_TOKEN;
    }
    if_ast = add_child(if_ast, new_ast);
    size_t keep_pos = lexer->pos;
    new_ast = NULL;
    if (parse_else(&new_ast, lexer) != PARSER_OK)
    {
        ast_free(new_ast);
        lexer->pos = keep_pos;
    }
    else
        if_ast = add_child(if_ast, new_ast);
    *res = if_ast;
    return PARSER_OK;
}

static enum parser_status parse_compound_list(struct ast **res,
                                              struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type == TOKEN_ERROR)
        return PARSER_UNEXPECTED_TOKEN;
    while (token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    token_free(token);
    struct ast *child = NULL;
    if (parse_and_or(&child, lexer) != PARSER_OK)
        return PARSER_UNEXPECTED_TOKEN;
    token = lexer_peek(lexer);
    struct ast *ast_list = create_ast(AST_LIST, "");
    ast_list = add_child(ast_list, child);
    size_t keep_pos = lexer->pos;
    while (token.type == TOKEN_SEMI_COLON || token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
        while (token.type == TOKEN_BACKSLASH)
        {
            lexer_pop(lexer);
            token = lexer_peek(lexer);
        }
        token_free(token);
        child = NULL;
        if (parse_and_or(&child, lexer) != PARSER_OK)
        {
            lexer->pos = keep_pos;
            break;
        }
        ast_list = add_child(ast_list, child);
        token = lexer_peek(lexer);
        keep_pos = lexer->pos;
    }
    token = lexer_peek(lexer);
    if (token.type == TOKEN_SEMI_COLON)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    while (token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    *res = ast_list;
    token_free(token);
    return PARSER_OK;
}

static enum parser_status parse_functions(struct ast **res, struct lexer *lexer)
{
    struct token token = lexer_peek(lexer);
    if (token.type != TOKEN_WORD)
    {
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    struct ast *func = create_ast(AST_FUNCTION, token.data);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_LEFT_PARENTHESIS)
    {
        ast_free(func);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    if (token.type != TOKEN_RIGHT_PARENTHESIS)
    {
        ast_free(func);
        token_free(token);
        return PARSER_UNEXPECTED_TOKEN;
    }
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    while (token.type == TOKEN_BACKSLASH)
    {
        lexer_pop(lexer);
        token = lexer_peek(lexer);
    }
    token_free(token);
    if (parse_shell_command(res, lexer) != PARSER_OK)
    {
        ast_free(func);
        return PARSER_UNEXPECTED_TOKEN;
    }
    func = add_child(func, (*res));
    *res = func;
    return PARSER_OK;
}
