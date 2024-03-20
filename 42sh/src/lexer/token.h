#ifndef TOKEN_H
#define TOKEN_H

#include <unistd.h>

enum token_type
{
    TOKEN_IF, // if
    TOKEN_THEN, // then
    TOKEN_ELIF, // elif
    TOKEN_ELSE, // else
    TOKEN_FI, // fi
    TOKEN_SEMI_COLON, // ;
    TOKEN_ESP, // &
    TOKEN_BACKSLASH, // \n
    TOKEN_NEG, // !
    TOKEN_REDIR, // >, <, >>, >&, <&, >|, <>
    TOKEN_PIPE, // |
    TOKEN_WHILE, // while
    TOKEN_UNTIL, // until
    TOKEN_DO, // do
    TOKEN_DONE, // done
    TOKEN_AND, // &&
    TOKEN_OR, // ||
    TOKEN_FOR, // for
    TOKEN_IN, // in
    TOKEN_IONUMBER, // every word that is a number
    TOKEN_WORD, // every word inside the input
    TOKEN_ASSIGNMENT_WORD, // variable
    TOKEN_LEFT_BRACKET, // {
    TOKEN_RIGHT_BRACKET, // }
    TOKEN_LEFT_PARENTHESIS, // (
    TOKEN_RIGHT_PARENTHESIS, // )
    TOKEN_EOF, // end of input marker
    TOKEN_ERROR, // it is not a real token, invalid token
};

struct token
{
    enum token_type type; // The kind of token
    char *data; // If the token is a number, its value
};
#endif /* !TOKEN_H */
