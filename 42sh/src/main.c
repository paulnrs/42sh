#define _POSIX_C_SOURCE 200809L

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"
#include "evaluate/evaluate.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int read_file(FILE *file)
{
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    char *ptr = calloc(size + 1, sizeof(char));
    fseek(file, 0, SEEK_SET);
    size_t reaad = fread(ptr, sizeof(char), size, file);
    if (!reaad)
    {
        free(ptr);
        ptr = "";
    }
    struct lexer *lexer = lexer_new(ptr);
    struct ast *ast = NULL;
    lexer->pos = 0;
    enum parser_status status = parse(&ast, lexer);
    struct token token = lexer_peek(lexer);
    while (status == PARSER_OK && token.type != TOKEN_EOF)
    {
        while (token.type == TOKEN_BACKSLASH)
        {
            lexer_pop(lexer);
            token = lexer_peek(lexer);
        }
        token_free(token);
        struct ast *new = NULL;
        status = parse(&new, lexer);
        if (new)
            ast = add_child(ast, new);
        token = lexer_peek(lexer);
    }
    token_free(token);
    int res = 0;
    if (ast && status == PARSER_OK)
    {
        res = evaluate(ast);
        ast_free(ast);
    }
    else
        res = reaad ? 2 : 0;
    lexer_free(lexer);
    if (reaad)
        free(ptr);
    return res;
}

int open_file(char *path, int mode)
{
    FILE *file = NULL;
    if (mode)
        file = fmemopen(path, strlen(path), "r");
    else
        file = fopen(path, "r");
    if (!file)
        errx(1, "Could not open file");
    int res = read_file(file);
    fclose(file);
    return res;
}

int main(int argc, char **argv)
{
    int res = 0;
    if (argc == 1)
    {
        off_t size = lseek(STDIN_FILENO, 0, SEEK_END);
        char *buff = calloc(size + 1, sizeof(char));
        lseek(STDIN_FILENO, 0, SEEK_SET);
        ssize_t reaad = read(STDIN_FILENO, buff, size);
        if (reaad)
            res = open_file(buff, 1);
        free(buff);
    }
    else if (argc > 2 && !strcmp(argv[1], "-c"))
    {
        if (argc != 3)
            errx(1, "Not enough or too many arguments after -c");
        res = open_file(argv[2], 1);
    }
    else
    {
        if (argc > 1)
            res = open_file(argv[1], 0);
    }
    return res;
}
