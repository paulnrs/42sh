#include <criterion/criterion.h>

#include "lexer/lexer.h"
#include "lexer/token.h"

TestSuite(Lexer);

Test(Lexer, test_new)
{
    struct lexer *lexer = lexer_new("help");
    cr_expect_str_eq(lexer->input, "help");
    lexer_free(lexer);
}

Test(Lexer, lexer_simple_command)
{
    struct lexer *lexer = lexer_new("echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "echo");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "a");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_command_list)
{
    struct lexer *lexer = lexer_new("echo a; echo b; echo c; ls /bin;");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "echo");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_SEMI_COLON);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    for (int i = 0; i < 8; i++)
        lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "/bin");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_if)
{
    struct lexer *lexer = lexer_new("if true; then echo a; else echo b; fi");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_IF);
    cr_expect_str_eq(token.data, "if");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_THEN);
    cr_expect_str_eq(token.data, "then");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_ELSE);
    cr_expect_str_eq(token.data, "else");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_FI);
    cr_expect_str_eq(token.data, "fi");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_elif)
{
    struct lexer *lexer = lexer_new(
        "if true; then echo a; elif echo b; then echo c; else echo d; fi");
    for (int i = 0; i < 7; i++)
        lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_ELIF);
    cr_expect_str_eq(token.data, "elif");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_backslash)
{
    struct lexer *lexer =
        lexer_new("if false\ntrue\nthen\necho a\echo b; echo c\nfi");
    lexer_pop(lexer);
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_BACKSLASH);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_single_quotes)
{
    struct lexer *lexer = lexer_new("echo 'a b ;if d' b");
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "a b ;if d");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "b");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_comments)
{
    struct lexer *lexer =
        lexer_new("echo \\#escaped \"#\"quoted not#first #commented");
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "#escaped");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "#quoted");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "not#first");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_while)
{
    struct lexer *lexer = lexer_new("while true; do echo a; done");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WHILE);
    cr_expect_str_eq(token.data, "while");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_DO);
    cr_expect_str_eq(token.data, "do");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_DONE);
    cr_expect_str_eq(token.data, "done");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_until)
{
    struct lexer *lexer = lexer_new("until true; do echo a; done");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_UNTIL);
    cr_expect_str_eq(token.data, "until");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_neg)
{
    struct lexer *lexer = lexer_new("! true; ! false");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_NEG);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_NEG);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_and)
{
    struct lexer *lexer = lexer_new("true && false");
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_AND);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_or)
{
    struct lexer *lexer = lexer_new("true || false");
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_OR);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_01)
{
    struct lexer *lexer = lexer_new("> test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, ">");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_02)
{
    struct lexer *lexer = lexer_new("< test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, "<");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_03)
{
    struct lexer *lexer = lexer_new(">> test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, ">>");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_04)
{
    struct lexer *lexer = lexer_new(">& test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, ">&");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_05)
{
    struct lexer *lexer = lexer_new("<& test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, "<&");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_06)
{
    struct lexer *lexer = lexer_new(">| test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, ">|");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_redir_07)
{
    struct lexer *lexer = lexer_new("<> test.txt echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_REDIR);
    cr_expect_str_eq(token.data, "<>");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_pipe)
{
    struct lexer *lexer = lexer_new("ls | echo");
    lexer_pop(lexer);
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_PIPE);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_command_block)
{
    struct lexer *lexer = lexer_new("{ echo a } | tr a h");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_LEFT_BRACKET);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_RIGHT_BRACKET);
    cr_expect_str_eq(token.data, "");
    lexer_free(lexer);
}

Test(Lexer, lexer_subshell)
{
    struct lexer *lexer = lexer_new("(a=42; echo $a); echo a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_LEFT_PARENTHESIS);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_ASSIGNMENT_WORD);
    cr_expect_str_eq(token.data, "a=42");
    token_free(token);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_RIGHT_PARENTHESIS);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_func)
{
    struct lexer *lexer = lexer_new("func(){ echo a; }; func 0;");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "func");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_LEFT_PARENTHESIS);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_RIGHT_PARENTHESIS);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_LEFT_BRACKET);
    cr_expect_str_eq(token.data, "");
    token_free(token);
    lexer_free(lexer);
}

Test(Lexer, lexer_variable)
{
    struct lexer *lexer = lexer_new("echo $a");
    struct token token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "echo");
    token_free(token);
    lexer_pop(lexer);
    token = lexer_peek(lexer);
    cr_expect_eq(token.type, TOKEN_WORD);
    cr_expect_str_eq(token.data, "$a");
    token_free(token);
    lexer_free(lexer);
}
