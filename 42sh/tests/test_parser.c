#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "parser/parser.h"

TestSuite(Parser);

Test(Parser, parse_simple_command)
{
    struct lexer *lexer = lexer_new("echo a");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_command_list)
{
    struct lexer *lexer =
        lexer_new("echo a; echo b; echo c; ls /bin; cat Makefile; find t*;");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_if)
{
    struct lexer *lexer = lexer_new("if false; then echo bite; elif false; "
                                    "then echo fun; else echo dong; fi");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_single_quotes)
{
    struct lexer *lexer =
        lexer_new("echo 'a b ;d echo if then else ls;;'; a; ';;;;;'");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_EOF)
{
    struct lexer *lexer = lexer_new("");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_comments)
{
    struct lexer *lexer =
        lexer_new("echo \\#escaped \"#\"quoted not#first #commented");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_while)
{
    struct lexer *lexer = lexer_new("while true; do echo a; done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_until)
{
    struct lexer *lexer = lexer_new("until true; do echo a; done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_pipe)
{
    struct lexer *lexer = lexer_new("echo a | echo b");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_neg)
{
    struct lexer *lexer = lexer_new("if ! true; then echo a; else echo b; fi");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_and)
{
    struct lexer *lexer = lexer_new("true && false");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_or)
{
    struct lexer *lexer = lexer_new("true || false");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_and_or)
{
    struct lexer *lexer =
        lexer_new("true || false && false && true || false || true");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_redir)
{
    struct lexer *lexer = lexer_new("> test.txt echo a");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_for)
{
    struct lexer *lexer = lexer_new("for i in 1 2 3 4 5; do echo a; done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_expect_eq(status, PARSER_OK);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_wrong_grammar_01, .init = cr_redirect_stderr)
{
    struct lexer *lexer = lexer_new("echo ;;");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    fflush(stderr);
    cr_expect_stderr_eq_str("Error on parsing\n");
    cr_expect_eq(status, PARSER_UNEXPECTED_TOKEN);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_wrong_grammar_02, .init = cr_redirect_stderr)
{
    struct lexer *lexer = lexer_new("if true; then; fi");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    fflush(stderr);
    cr_expect_stderr_eq_str("Error on parsing\n");
    cr_expect_eq(status, PARSER_UNEXPECTED_TOKEN);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Parser, parse_wrong_grammar_03, .init = cr_redirect_stderr)
{
    struct lexer *lexer =
        lexer_new("if false; then echo a; elif false; echo b; else echo c; fi");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    fflush(stderr);
    cr_expect_stderr_eq_str("Error on parsing\n");
    cr_expect_eq(status, PARSER_UNEXPECTED_TOKEN);
    lexer_free(lexer);
    ast_free(ast);
}
