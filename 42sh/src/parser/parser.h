#ifndef PARSER_H
#define PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"

enum parser_status
{
    PARSER_OK,
    PARSER_UNEXPECTED_TOKEN,
};

struct ast *add_child(struct ast *parent, struct ast *child);
enum parser_status parse(struct ast **res, struct lexer *lexer);

#endif /* !PARSER_H */
