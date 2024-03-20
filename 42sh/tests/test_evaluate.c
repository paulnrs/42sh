#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "evaluate/evaluate.h"
#include "parser/parser.h"

TestSuite(Evaluate);

Test(Evaluate, evaluate_simple_command, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("echo a");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("a\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_command_list, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("echo a; echo b; echo c; echo d; echo e");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("a\nb\nc\nd\ne\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_if, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("if false; then echo bite; elif false; "
                                    "then echo fun; else echo dong; fi");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("dong\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_single_quote, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new(
        "echo 'a b ;d echo if then else ls;;'; echo -n a; echo ';;;;;'");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("a b ;d echo if then else ls;;\na;;;;;\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_EOF, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_comments, .init = cr_redirect_stdout)
{
    struct lexer *lexer =
        lexer_new("echo \\#escaped \"#\"quoted not#first #commented");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("#escaped #quoted not#first\n");
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_true, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("true");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_false, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("false");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("");
    cr_expect_eq(res, 1);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_echo_n, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("echo -n abc; echo -n def");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("abcdef");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_pipe, .init = cr_redirect_stdout)
{
    struct lexer *lexer =
        lexer_new("echo -n Hello, World | tr e a | tr a b | tr b e");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("Hello, World");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_for1, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new(
        "for i in 1 2 3 4; do if true; then echo ok; else false; fi;  done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("ok\nok\nok\nok\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_for2, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new(
        "for i in 1 2 3 4; do for i in 1 2 ;do echo ok; done ; echo tg; done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("ok\nok\ntg\nok\nok\ntg\nok\nok\ntg\nok\nok\ntg\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_for3, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("for i in 1 2 3 4; do for i in 1 2 ;do "
                                    "echo ok; done ; echo tg; done | wc -l");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("12\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_for4, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("for i in 'ok' 'ok2' 3 'cboneft'; do for i "
                                    "in 1 2 ;do echo ok; done ; echo tg; done");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("ok\nok\ntg\nok\nok\ntg\nok\nok\ntg\nok\nok\ntg\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_for5, .init = cr_redirect_stdout)
{
    struct lexer *lexer =
        lexer_new("for i in 1 2 3 4; do for i in 'tg' 'hg' ;do echo ok | wc -l "
                  "; done ; echo tg; done | wc -l");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_stdout_eq_str("12\n");
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_not_existant, .init = cr_redirect_stderr)
{
    struct lexer *lexer =
        lexer_new("for i in 1 2 3 4; do for i in 'tg' 'hg' ;do echo ok | wc -l "
                  "; done ; echo tg; done | c -l");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stderr);
    cr_expect_eq(res, 127);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_true_false1, .init = cr_redirect_stdout)
{
    struct lexer *lexer =
        lexer_new("true && false || false && true || true && false");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_eq(res, 1);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_true_false2, .init = cr_redirect_stdout)
{
    struct lexer *lexer = lexer_new("true && true || false");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stdout);
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_bad_command, .init = cr_redirect_stderr)
{
    struct lexer *lexer =
        lexer_new("for i in 1 2 3 4; do for i in 'tg' 'hg' ;do echo ok | wc -l "
                  "; done ; echo tg; done | cat t");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    int res = evaluate(ast);
    fflush(stderr);
    cr_expect_eq(res, 2);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_bad_command2, .init = cr_redirect_stderr)
{
    struct lexer *lexer = lexer_new(
        "a=2; b=txt; echo $a; unset a; echo $b; unset -f b; echo $a $b");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_redirect_stdout();
    int res = evaluate(ast);
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_continue, .init = cr_redirect_stderr)
{
    struct lexer *lexer =
        lexer_new("for i in 1 2 3 4 5 ; do if [ $i -eq 3 ]; then continue 3; "
                  "else echo $i $i;fi ;done ");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_redirect_stdout();
    int res = evaluate(ast);
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}

Test(Evaluate, evaluate_break, .init = cr_redirect_stderr)
{
    struct lexer *lexer = lexer_new("for i in 1 2 3 4 5 ; do if [ $i -eq 3 ]; "
                                    "then break; else echo $i $i;fi ;done ");
    struct ast *ast = NULL;
    enum parser_status status = parse(&ast, lexer);
    cr_redirect_stdout();
    int res = evaluate(ast);
    cr_expect_eq(res, 0);
    lexer_free(lexer);
    ast_free(ast);
}
