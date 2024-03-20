#include "lexer.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lexer *lexer_new(const char *input)
{
    if (!input)
        return NULL;
    struct lexer *lexer = calloc(1, sizeof(struct lexer));
    if (!lexer)
        return NULL;
    lexer->input = input;
    return lexer;
}

void lexer_free(struct lexer *lexer)
{
    free(lexer);
}

static int cond(struct lexer *lexer, int i)
{
    if (i && lexer->input[lexer->pos] != ' ' && lexer->input[lexer->pos] != '\n'
        && lexer->input[lexer->pos] != ';' && lexer->input[lexer->pos] != '>'
        && lexer->input[lexer->pos] != '<' && lexer->input[lexer->pos] != ')'
        && lexer->input[lexer->pos] != '(')
        return 1;
    if (lexer->input[lexer->pos] != ' ' && lexer->input[lexer->pos] != '\n'
        && lexer->input[lexer->pos] != ';' && lexer->input[lexer->pos] != '>'
        && lexer->input[lexer->pos] != '<' && lexer->input[lexer->pos] != ')'
        && lexer->input[lexer->pos] != '(' && lexer->input[lexer->pos] != '{'
        && lexer->input[lexer->pos] != '}')
        return 1;
    return 0;
}

static char *to_str(struct lexer *lexer, size_t len)
{
    char *res = calloc(len + 1, sizeof(char));
    size_t size = 0;
    if (lexer->input[lexer->pos] == '\'')
    {
        lexer->pos++;
        while (lexer->pos < len && lexer->input[lexer->pos] != '\'')
            res[size++] = lexer->input[lexer->pos++];
        if (lexer->pos == len && lexer->input[lexer->pos - 1] != '\'')
            errx(2, "Error while lexing quotes");
        lexer->pos++;
    }
    else if (lexer->input[lexer->pos] == '"')
    {
        lexer->pos++;
        while (lexer->pos < len && lexer->input[lexer->pos] != '"')
        {
            if (lexer->input[lexer->pos] == '\\')
                lexer->pos++;
            res[size++] = lexer->input[lexer->pos++];
        }
        if (lexer->pos == len && lexer->input[lexer->pos - 1] != '"')
            errx(2, "Error while lexing quotes");
        lexer->pos++;
    }
    else if (lexer->input[lexer->pos] == '$')
    {
        while (lexer->pos < len && cond(lexer, 1))
            res[size++] = lexer->input[lexer->pos++];
    }
    while (lexer->pos < len && cond(lexer, 0))
    {
        if (lexer->input[lexer->pos] == '\\')
            lexer->pos++;
        res[size++] = lexer->input[lexer->pos++];
    }
    return res;
}

static void skip(struct lexer *lexer, size_t len, char c)
{
    if (c == '\n')
    {
        while (lexer->pos < len && lexer->input[lexer->pos] != c)
            lexer->pos++;
    }
    else
    {
        while (lexer->pos < len && lexer->input[lexer->pos] == c)
            lexer->pos++;
    }
}

static enum token_type word_care(char *str)
{
    if (!strcmp(str, "if"))
        return TOKEN_IF;
    if (!strcmp(str, "then"))
        return TOKEN_THEN;
    if (!strcmp(str, "elif"))
        return TOKEN_ELIF;
    if (!strcmp(str, "else"))
        return TOKEN_ELSE;
    if (!strcmp(str, "fi"))
        return TOKEN_FI;
    if (!strcmp(str, "while"))
        return TOKEN_WHILE;
    if (!strcmp(str, "do"))
        return TOKEN_DO;
    if (!strcmp(str, "done"))
        return TOKEN_DONE;
    if (!strcmp(str, "until"))
        return TOKEN_UNTIL;
    if (!strcmp(str, "for"))
        return TOKEN_FOR;
    if (!strcmp(str, "in"))
        return TOKEN_IN;
    return TOKEN_WORD;
}

static enum token_type symbol_care(char c)
{
    if (c == ';')
        return TOKEN_SEMI_COLON;
    if (c == '!')
        return TOKEN_NEG;
    if (c == '{')
        return TOKEN_LEFT_BRACKET;
    if (c == '}')
        return TOKEN_RIGHT_BRACKET;
    if (c == '(')
        return TOKEN_LEFT_PARENTHESIS;
    if (c == ')')
        return TOKEN_RIGHT_PARENTHESIS;
    return TOKEN_BACKSLASH;
}

static char *redir_care(struct lexer *lexer, size_t len)
{
    char *l = calloc(3, sizeof(char));
    l[0] = lexer->input[lexer->pos++];
    if (lexer->pos >= len)
        return l;
    if (!strcmp(l, ">"))
    {
        l[1] = lexer->input[lexer->pos++];
        if (!strcmp(l, ">>"))
            return l;
        if (!strcmp(l, ">&"))
            return l;
        if (!strcmp(l, ">|"))
            return l;
        lexer->pos--;
        l[1] = 0;
        return l;
    }
    l[1] = lexer->input[lexer->pos++];
    if (!strcmp(l, "<&"))
        return l;
    if (!strcmp(l, "<>"))
        return l;
    lexer->pos--;
    l[1] = 0;
    return l;
}

static int to_pipe(struct lexer *lexer, struct token *token)
{
    if (lexer->input[lexer->pos] == '|')
    {
        lexer->pos++;
        if (lexer->input[lexer->pos] == '|')
        {
            lexer->pos++;
            token->type = TOKEN_OR;
        }
        else
            token->type = TOKEN_PIPE;
        return 1;
    }
    return 0;
}

static int to_esp(struct lexer *lexer, struct token *token)
{
    if (lexer->input[lexer->pos] == '&')
    {
        lexer->pos++;
        if (lexer->input[lexer->pos] == '&')
        {
            lexer->pos++;
            token->type = TOKEN_AND;
        }
        else
            token->type = TOKEN_ESP;
        return 1;
    }
    return 0;
}

struct token assignment_care(struct token token)
{
    if (token.data[0] != '=' && (token.data[0] < '0' || token.data[0] > '9'))
        token.type = TOKEN_ASSIGNMENT_WORD;
    else
        token.type = word_care(token.data);
    return token;
}

struct token parse_input_for_tok(struct lexer *lexer)
{
    struct token token = { TOKEN_ERROR, "" };
    size_t len = strlen(lexer->input);
    int yes = 0;
    if (lexer->pos >= len)
        token.type = TOKEN_EOF;
    else if (lexer->input[lexer->pos] == ';' || lexer->input[lexer->pos] == '\n'
             || lexer->input[lexer->pos] == '!'
             || lexer->input[lexer->pos] == '{'
             || lexer->input[lexer->pos] == '}'
             || lexer->input[lexer->pos] == '('
             || lexer->input[lexer->pos] == ')')
    {
        token.type = symbol_care(lexer->input[lexer->pos]);
        lexer->pos++;
    }
    else if (to_pipe(lexer, &token))
        yes = 0;
    else if (to_esp(lexer, &token))
        yes = 0;
    else if (lexer->input[lexer->pos] == '<' || lexer->input[lexer->pos] == '>')
    {
        token.data = redir_care(lexer, len);
        token.type = TOKEN_REDIR;
    }
    else if (lexer->input[lexer->pos] != '#')
    {
        if (lexer->input[lexer->pos] == '\'')
            yes = 1;
        token.data = to_str(lexer, len);
        if (yes)
            token.type = TOKEN_WORD;
        else if (strstr(token.data, "="))
            token = assignment_care(token);
        else
            token.type = word_care(token.data);
    }
    else if (lexer->input[lexer->pos] == '#')
    {
        token.type = TOKEN_BACKSLASH;
        lexer->pos++;
        skip(lexer, len, '\n');
    }
    else
        fprintf(stderr, "parse_input_for_tok: token is not valid\n");
    skip(lexer, len, ' ');
    lexer->current_tok = token;
    return token;
}

struct token lexer_peek(struct lexer *lexer)
{
    int keep = lexer->pos;
    struct token token = parse_input_for_tok(lexer);
    lexer->pos = keep;
    return token;
}

void lexer_pop(struct lexer *lexer)
{
    struct token to_free = parse_input_for_tok(lexer);
    token_free(to_free);
}

void token_free(struct token token)
{
    if (token.type == TOKEN_WORD)
        free(token.data);
    else if (strcmp(token.data, ""))
        free(token.data);
}
