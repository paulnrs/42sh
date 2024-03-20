#ifndef AST_H
#define AST_H

#include <unistd.h>

enum ast_type
{
    AST_IF,
    AST_COMMAND,
    AST_LIST,
    AST_REDIR,
    AST_PIPE,
    AST_AND,
    AST_OR,
    AST_WHILE,
    AST_UNTIL,
    AST_FOR,
    AST_NEG,
    AST_ASSIGNMENT_WORD,
    AST_COMMAND_BLOCK,
    AST_SUBSHELL,
    AST_FUNCTION
};

/**
 * This very simple AST structure should be sufficient for such a simple AST.
 * It is however, NOT GOOD ENOUGH for more complicated projects, such as a
 * shell. Please read the project guide for some insights about other kinds of
 * ASTs.
 */
struct ast
{
    enum ast_type type; ///< The kind of node we're dealing with
    char **data; ///< If the node is a command, stores its words
    int nb_data; /// number of words in the data
    struct ast **ast_list; /// general tree
    int nb_ast; /// number of children
};

/**
 ** \brief Allocate a new ast with the given type
 */
struct ast *ast_new(enum ast_type type);
void generate_dot(struct ast *node);
void ast_free(struct ast *ast);
void data_free(struct ast *ast);

#endif /* !AST_H */
