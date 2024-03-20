#define _POSIX_C_SOURCE 200809

#include "evaluate.h"

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"

int global_fd = 1;

static int read_file(FILE *file)
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

static int open_file(char *path, int mode)
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

static int eval_dot(struct ast *ast)
{
    char **argv = ast->data;
    int res = 0;
    res = open_file(argv[1], 0);
    return res;
}

static struct dico *new_dico(void)
{
    struct dico *d = malloc(sizeof(struct dico));
    d->entries = malloc(100);
    d->size_v = 0;
    d->size_f = 0;
    d->continuef = 0;
    d->breakf = 0;
    d->nb_arg = 1;
    d->func = malloc(100);
    return d;
}

static void free_dico(struct dico *dictionary)
{
    for (size_t i = 0; i < dictionary->size_v; i++)
    {
        if (dictionary->entries[i])
        {
            free(dictionary->entries[i]->key);
            if (dictionary->entries[i]->value)
                free(dictionary->entries[i]->value);
            free(dictionary->entries[i]);
        }
    }
    for (size_t j = 0; j < dictionary->size_f; j++)
    {
        if (dictionary->func[j])
        {
            free(dictionary->func[j]->key);
            free(dictionary->func[j]);
        }
    }
    free(dictionary->entries);
    free(dictionary->func);
    free(dictionary);
}

static int findvar(struct dico *d, char *key)
{
    for (size_t i = 0; i < d->size_v; i++)
    {
        if (d->entries[i] && strcmp(d->entries[i]->key, key) == 0)
            return i;
    }
    return -1;
}

static void modifyvalue(struct dico *d, int i, const char *value)
{
    char *tmp = strdup(value);
    char *tok = strtok(tmp, "=");
    tok = strtok(NULL, "='");
    d->entries[i]->value = realloc(d->entries[i]->value, strlen(tok) + 1);
    strcpy(d->entries[i]->value, tok);
    free(tmp);
}

static int findfunc(struct dico *d, char *key)
{
    for (size_t j = 0; j < d->size_f; j++)
    {
        if (d->func[j] && strcmp(d->func[j]->key, key) == 0)
            return j;
    }
    return -1;
}

static void Addfunc(struct dico *dico, struct ast *ast)
{
    int ind = findfunc(dico, ast->data[0]);
    if (ind < 0)
    {
        dico->func[dico->size_f] = malloc(sizeof(struct key_func));
        dico->func[dico->size_f]->key = calloc(1000, sizeof(char));
        strcpy(dico->func[dico->size_f]->key, ast->data[0]);
        dico->func[dico->size_f]->ast = ast->ast_list[0];
        dico->size_f++;
    }
    else
        dico->func[ind]->ast = ast->ast_list[0];
}

static void Addvalue(struct dico *dictionary, const char *keyValue)
{
    char *tmp = strdup(keyValue);
    char *token = strtok(tmp, "=");
    if (token == NULL)
        errx(1, "Invalid Format");
    int ind = findvar(dictionary, token);
    if (ind < 0)
    {
        dictionary->entries[dictionary->size_v] =
            malloc(sizeof(struct key_value));
        dictionary->entries[dictionary->size_v]->key =
            calloc(1000, sizeof(char));
        strcpy(dictionary->entries[dictionary->size_v]->key, token);
        token = strtok(NULL, "=");
        if (token == NULL
            && strcmp(dictionary->entries[dictionary->size_v]->key, "OLDPWD"))
        {
            errx(1, "Invalid key/value");
            free(dictionary->entries[dictionary->size_v]->key);
        }
        dictionary->entries[dictionary->size_v]->value =
            calloc(1000, sizeof(char));
        strcpy(dictionary->entries[dictionary->size_v]->value, token);
        dictionary->entries[dictionary->size_v]->arg = 0;
        dictionary->size_v++;
    }
    else
        modifyvalue(dictionary, ind, keyValue);
    free(tmp);
}

static void Addarg(struct dico *d, struct ast *ast)
{
    if (ast->nb_data > 1)
    {
        char big[1000] = { "@=" };
        for (int i = 1; i < ast->nb_data; i++)
        {
            char buff[100];
            sprintf(buff, "%d=%s", d->nb_arg, ast->data[i]);
            Addvalue(d, buff);
            d->entries[d->size_v - 1]->arg = 1;
            d->nb_arg++;
            if (i == 1)
                strcat(big, ast->data[i]);
            else
                strcat(strcat(big, " "), ast->data[i]);
        }
        Addvalue(d, big);
        d->entries[d->size_v - 1]->arg = 1;
    }
}

static void add_init(struct dico *var)
{
    char buff[100];
    char *t = getcwd(buff, 100);
    char tmp[100];
    char tmp2[100];
    sprintf(tmp, "%s=%s", "PWD", t);
    sprintf(tmp2, "%s=%d", "?", 0);
    Addvalue(var, tmp);
    var->entries[1] = malloc(sizeof(struct key_value));
    var->entries[1]->key = calloc(1000, sizeof(char));
    strcpy(var->entries[1]->key, "OLDPWD");
    var->entries[1]->value = NULL;
    var->entries[1]->arg = 0;
    var->size_v++;
    Addvalue(var, tmp2);
}

static struct ast *expansion(struct ast *ast, struct dico *var)
{
    for (int i = 0; i < ast->nb_data; i++)
    {
        if (ast->data[i][0] == '$')
        {
            char off = 1;
            if (ast->data[i][1] == '{')
            {
                ast->data[i][strlen(ast->data[i]) - 1] = 0;
                off++;
            }
            int index = findvar(var, (ast->data[i] + off));
            if (index == -1)
            {
                free(ast->data[i]);
                ast->data[i] = NULL;
                continue;
            }
            free(ast->data[i]);
            if (strcmp(var->entries[index]->key, "OLDPWD") == 0
                && var->entries[index]->value == NULL)
            {
                ast->data[i] = calloc(1, 5);
                ast->data[i] = strcpy(ast->data[i], "");
            }
            else
            {
                ast->data[i] = calloc(strlen(var->entries[index]->value) + 1,
                                      sizeof(char));
                ast->data[i] = strcpy(ast->data[i], var->entries[index]->value);
            }
        }
    }
    return ast;
}

static int exec_c(struct ast *ast)
{
    ast->data = realloc(ast->data, (ast->nb_data + 1) * sizeof(char *));
    ast->data[ast->nb_data] = NULL;
    pid_t pid = fork();
    if (pid == -1)
        errx(1, "fork");
    else if (pid == 0)
    {
        if (execvp(ast->data[0], ast->data) == -1)
        {
            perror("Bad Exec");
            return 127;
        }
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) < 127 && WEXITSTATUS(status) != 0)
                return 2;
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            return WTERMSIG(status);
        }
    }
    else
        exit(EXIT_FAILURE);
    return 0;
}

static void echo_ex(int *i, size_t *j, struct ast *ast)
{
    if (ast->data[*i][*j] == '\\' && ast->data[*i][*j + 1])
    {
        *j += 1;
        switch (ast->data[*i][*j])
        {
        case 'n':
            dprintf(global_fd, "\n");
            break;
        case 't':
            dprintf(global_fd, "\t");
            break;
        default:
            dprintf(global_fd, "\\%c", ast->data[*i][*j]);
            break;
        }
        *j += 1;
    }
    else
    {
        dprintf(global_fd, "%c", ast->data[*i][*j]);
        *j += 1;
    }
}

static void builtinEcho(struct ast *ast)
{
    int flag_newline = 1;
    int flag_escape = 1;
    int i = 1;
    if (ast->nb_data > 1 && ast->data[1] && ast->data[1][0] == '-')
    {
        i = 2;
        int j = 1;
        switch (ast->data[1][j])
        {
        case 'n':
            flag_newline = 0;
            break;
        case 'e':
            break;
        case 'E':
            flag_escape = 0;
            break;
        default:
            if (ast->nb_data == 2)
                dprintf(1, "%s", ast->data[1]);
            else
                dprintf(1, "%s ", ast->data[1]);
        }
    }
    while (i < ast->nb_data)
    {
        if (ast->data[i])
        {
            if (flag_escape)
            {
                size_t j = 0;
                while (j < strlen(ast->data[i]))
                    echo_ex(&i, &j, ast);
            }
            else
            {
                dprintf(1, "%s", ast->data[i]);
            }
            i++;
            if (i < ast->nb_data)
                dprintf(global_fd, " ");
        }
        else
            i++;
    }
    if (flag_newline)
        dprintf(global_fd, "\n");
    fflush(stdout);
}

static int true_false(struct ast *ast)
{
    if (strcmp(ast->data[0], "true") == 0)
    {
        fflush(stdout);
        return 0;
    }
    if (strcmp(ast->data[0], "false") == 0)
    {
        fflush(stdout);
        return 1;
    }
    return 0;
}

static int mycd(struct ast *ast, struct dico *d)
{
    int res = 0;
    char buff[100];
    if (ast->nb_data == 1)
    {
        res = chdir("/root");
        if (!res)
        {
            char tmp[100];
            sprintf(tmp, "%s=%s", "OLDPWD", d->entries[0]->value);
            Addvalue(d, tmp);
            //==
            char yolo[100];
            sprintf(yolo, "%s=%s", "PWD", getcwd(buff, 100));
            modifyvalue(d, 0, yolo);
        }
    }
    else if (ast->data[1][0] == '-')
    {
        if (!d->entries[1]->value)
        {
            dprintf(2, "OLDPWD set to null\n");
            return 1;
        }
        res = chdir(d->entries[1]->value);
        if (!res)
        {
            char tmp[100];
            sprintf(tmp, "%s=%s", "OLDPWD", d->entries[0]->value);
            Addvalue(d, tmp);
            //==
            char yolo[100];
            sprintf(yolo, "%s=%s", "PWD", getcwd(buff, 100));
            modifyvalue(d, 0, yolo);
        }
    }
    else
    {
        res = chdir(ast->data[1]);
        if (!res)
        {
            char tmp[100];
            sprintf(tmp, "%s=%s", "OLDPWD", d->entries[0]->value);
            Addvalue(d, tmp);
            //==
            char yolo[100];
            sprintf(yolo, "%s=%s", "PWD", getcwd(buff, 100));
            modifyvalue(d, 0, yolo);
        }
    }
    if (res)
    {
        dprintf(2, "cd: %s: No such file or directory\n", ast->data[1]);
        return 1;
    }
    return res;
}

static char **deepcopy(char **original, size_t size)
{
    if (original == NULL || size == 0)
    {
        return NULL;
    }
    char **copy = malloc(size * sizeof(char *));
    if (copy == NULL)
    {
        return NULL;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (original[i] != NULL)
        {
            copy[i] = strdup(original[i]);
            if (copy[i] == NULL)
            {
                for (size_t j = 0; j < i; ++j)
                {
                    free(copy[j]);
                    free(original[j]);
                }
                free(copy);
                return NULL;
            }
        }
        else
        {
            copy[i] = NULL;
        }
    }
    return copy;
}

static int my_exit(unsigned int n)
{
    if (n <= 255)
        exit(n);
    else
    {
        fprintf(stderr, "Error: Exit status must be between 0 and 255.\n");
        exit(EXIT_FAILURE);
    }
}

static int my_unset(char *key, struct dico *var, int mode)
{
    if (mode == 1)
    {
        int i = findvar(var, key);
        if (i == -1)
            return i;
        else
        {
            free(var->entries[i]->key);
            if (var->entries[i]->value)
                free(var->entries[i]->value);
        }
        free(var->entries[i]);
        var->entries[i] = NULL;
    }
    else
    {
        int j = findfunc(var, key);
        if (j == -1)
            return -2;
        else
        {
            var->func[j]->ast = NULL;
        }
    }
    return 0;
}

static void imple(struct ast *ast, char **tmp)
{
    data_free(ast);
    ast->data = tmp;
}

static int handle_unset(struct ast *ast, struct dico *var)
{
    int res = 0;
    if (ast->nb_data == 2)
    {
        res = my_unset(ast->data[1], var, 1);
    }
    else if (ast->nb_data == 3 && strcmp(ast->data[1], "-v") == 0)
    {
        res = my_unset(ast->data[2], var, 1);
    }
    else if (ast->nb_data == 3 && strcmp(ast->data[1], "-f") == 0)
    {
        res = my_unset(ast->data[2], var, 2);
    }
    else
    {
        fprintf(stderr, "unset: invalid usage\n");
        res = -1;
    }
    return res;
}

static void delete_arg(struct dico *var)
{
    for (size_t i = 0; i < var->size_v; i++)
    {
        if (var->entries[i]->arg == 1)
        {
            my_unset(var->entries[i]->key, var, 1);
        }
    }
}

static int eval_func(struct ast *ast, int ind, struct dico *func)
{
    Addarg(func, ast);
    int res2 = ast_evaluate(func->func[ind]->ast, func);
    delete_arg(func);
    return res2;
}

static int command(struct ast *ast, struct dico *var)
{
    char **tmp = deepcopy(ast->data, ast->nb_data);
    char *endptr;
    int ind;
    ast = expansion(ast, var);
    if (strcmp(ast->data[0], "echo") == 0)
        builtinEcho(ast);
    else if (strcmp(ast->data[0], "true") == 0
             || strcmp(ast->data[0], "false") == 0)
    {
        imple(ast, tmp);
        return true_false(ast);
    }
    else if (!strcmp(ast->data[0], "cd"))
    {
        imple(ast, tmp);
        return mycd(ast, var);
    }
    else if (!strcmp(ast->data[0], "continue"))
        var->continuef =
            ast->nb_data == 2 ? strtol(ast->data[1], &endptr, 10) : 1;
    else if (!strcmp(ast->data[0], "break"))
        var->breakf =
            (ast->nb_data == 2) ? strtol(ast->data[1], &endptr, 10) : 1;
    else if (!strcmp(ast->data[0], "exit"))
    {
        char *inte;
        int code = (ast->nb_data == 2) ? strtol(ast->data[1], &inte, 10) : 0;
        return my_exit(code);
    }
    else if (!strcmp(ast->data[0], "."))
    {
        imple(ast, tmp);
        return eval_dot(ast);
    }
    else if (!strcmp(ast->data[0], "unset"))
    {
        int res1 = handle_unset(ast, var);
        imple(ast, tmp);
        return res1;
    }
    else if ((ind = findfunc(var, ast->data[0])) >= 0)
    {
        int res2 = eval_func(ast, ind, var);
        imple(ast, tmp);
        return res2;
    }
    else
    {
        int res = exec_c(ast);
        imple(ast, tmp);
        return res;
    }
    imple(ast, tmp);
    return 0;
}

static int mypipe(struct ast *ast, struct dico *var)
{
    int num_commands = ast->nb_ast;
    int status;
    int pipe_fd[2];
    int res = 0;
    int input_fd = STDIN_FILENO;
    int last = 0;
    for (int i = 0; i < num_commands; ++i)
    {
        if (i < num_commands - 1 && pipe(pipe_fd) == -1)
            return 1;
        pid_t child_pid = fork();
        if (child_pid == -1)
            return 1;
        else if (child_pid == 0)
        {
            if (i > 0)
            {
                dup2(input_fd, STDIN_FILENO);
            }
            if (i < num_commands - 1)
            {
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }
            exit(ast_evaluate(ast->ast_list[i], var));
        }
        else
        {
            if (i > 0)
                close(input_fd);
            if (i < num_commands - 1)
            {
                input_fd = pipe_fd[0];
                close(pipe_fd[1]);
            }
            waitpid(child_pid, &status, 0);
            if (WIFEXITED(status))
                last = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                last = WTERMSIG(status);
            else
                last = 1;
            if (i == num_commands - 1)
                res = last;
            else
                res = (res != 0) ? res : 0;
        }
    }
    return res;
}

static int my_for(struct ast *ast, struct dico *var)
{
    int res = 0;
    char *tmp = malloc(10000);
    char *key = strdup(ast->data[0]);
    for (int i = 1; i < ast->nb_data; i++)
    {
        snprintf(tmp, 10000, "%s=%s", key, ast->data[i]);
        Addvalue(var, tmp);
        res = ast_evaluate(ast->ast_list[0], var);
        if (var->continuef)
        {
            var->continuef = 0;
            continue;
        }
        if (var->breakf)
        {
            var->breakf = 0;
            break;
        }
    }
    free(key);
    free(tmp);
    return res;
}

static int my_while(struct ast *ast, struct dico *var)
{
    int res = 0;
    while (ast_evaluate(ast->ast_list[0], var) == 0)
    {
        res = ast_evaluate(ast->ast_list[1], var);
        if (var->continuef)
        {
            var->continuef = 0;
            continue;
        }
        if (var->breakf)
        {
            var->breakf = 0;
            break;
        }
    }
    return res;
}

static int my_until(struct ast *ast, struct dico *var)
{
    int res = 0;
    while (ast_evaluate(ast->ast_list[0], var) != 0)
    {
        res = ast_evaluate(ast->ast_list[1], var);
        if (var->continuef)
        {
            var->continuef = 0;
            continue;
        }
        if (var->breakf)
        {
            var->breakf = 0;
            break;
        }
    }
    return res;
}

static int fd_update(struct ast *ast, int flags, int stream)
{
    int res = dup(stream);
    if (global_fd < 2)
    {
        global_fd = open(ast->data[1], flags, 0644);
        if (global_fd == -1)
            errx(1, "invalid file");
        fcntl(global_fd, F_SETFD, FD_CLOEXEC);
        dup2(global_fd, stream);
    }
    else
    {
        int n = open(ast->data[1], flags, 0644);
        close(n);
    }
    return res;
}

static int eval_or(struct ast *ast, struct dico *var)
{
    int res = ast_evaluate(ast->ast_list[0], var);
    for (int i = 1; i < ast->nb_ast; i++)
        res = res && ast_evaluate(ast->ast_list[i], var);
    return res;
}

static int eval_and(struct ast *ast, struct dico *var)
{
    int res = ast_evaluate(ast->ast_list[0], var);
    for (int i = 1; i < ast->nb_ast; i++)
        res = res || ast_evaluate(ast->ast_list[i], var);
    return res;
}

static int eval_if(struct ast *ast, struct dico *var)
{
    int res = 0;
    if (!ast_evaluate(ast->ast_list[0], var))
        res = ast_evaluate(ast->ast_list[1], var);
    else if (ast->nb_ast == 3)
        res = ast_evaluate(ast->ast_list[2], var);
    return res;
}

static int step(struct ast *ast, int flags, int stream, struct dico *var)
{
    int safe = fd_update(ast, flags, stream);
    int res = 0;
    if (ast->nb_ast && ast->ast_list[0]->data)
        res = ast_evaluate(ast->ast_list[0], var);
    fflush(NULL);
    dup2(safe, 1);
    close(safe);
    close(global_fd);
    global_fd = 1;
    return res;
}

static int ETSI(struct ast *ast, int flags, int stream, struct dico *var)
{
    int safe = fd_update(ast, flags, stream);
    int safe2 = dup(2);
    dup2(global_fd, 2);
    int res = 0;
    if (ast->nb_ast && ast->ast_list[0]->data)
        res = ast_evaluate(ast->ast_list[0], var);
    fflush(NULL);
    dup2(safe, 1);
    dup2(safe2, 2);
    close(safe);
    close(safe2);
    close(global_fd);
    global_fd = 1;
    return res;
}

static int eval_redir(struct ast *ast, struct dico *var)
{
    int res = 0;
    if (ast->nb_data != 2)
    {
        dprintf(2, "Incorrect redirection end\n");
        return 1;
    }
    switch (ast->data[0][0])
    {
    case '>':
        if (strlen(ast->data[0]) == 1 || ast->data[0][1] == '|')
        {
            res = step(ast, O_CREAT | O_WRONLY | O_TRUNC, 1, var);
        }
        else if (ast->data[0][1] == '>')
        {
            res = step(ast, O_CREAT | O_WRONLY | O_APPEND, 1, var);
        }
        else if (ast->data[0][1] == '&')
        {
            res = ETSI(ast, O_CREAT | O_WRONLY | O_TRUNC, 1, var);
        }
        break;
    case '<':
        if (strlen(ast->data[0]) == 1)
        {
            res = step(ast, O_RDONLY, 0, var);
        }
        else if (ast->data[0][1] == '&')
        {
            res = ETSI(ast, O_RDONLY, 0, var);
        }
        else if (ast->data[0][1] == '>')
        {
            int n = open(ast->data[1], O_CREAT | O_RDWR | O_TRUNC, 0644);
            int j = open(ast->ast_list[0]->data[0], O_RDWR | O_TRUNC, 0644);
            dup2(j, n);
            res = ast_evaluate(ast->ast_list[0]->ast_list[0], var);
        }
        break;
    default:
        break;
    }
    return res;
}

static int subshell(struct ast *ast, struct dico *var)
{
    pid_t pid = fork();
    if (pid == -1)
        errx(1, "fork");
    if (pid == 0)
    {
        int res = ast_evaluate(ast->ast_list[0], var);
        exit(res);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) < 127 && WEXITSTATUS(status) != 0)
                return 2;
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            return WTERMSIG(status);
        }
    }
    else
        exit(EXIT_FAILURE);
    return 0;
}

static int ast_evaluate_bis(struct ast *ast, struct dico *var, int res)
{
    switch (ast->type)
    {
    case AST_UNTIL:
        res = my_until(ast, var);
        break;
    case AST_FOR:
        res = my_for(ast, var);
        break;
    case AST_NEG:
        res = !(ast_evaluate(ast->ast_list[0], var));
        break;
    case AST_ASSIGNMENT_WORD:
        Addvalue(var, ast->data[0]);
        break;
    case AST_SUBSHELL:
        res = subshell(ast, var);
        break;
    case AST_COMMAND_BLOCK:
        res = ast_evaluate(ast->ast_list[0], var);
        break;
    case AST_FUNCTION:
        Addfunc(var, ast);
        break;
    default:
        errx(1, "WTF THIS IS NOT SUPPOSED TO HAPPEN");
    }
    return res;
}

int ast_evaluate(struct ast *ast, struct dico *var)
{
    if (!ast)
        return 0;
    int res = 0;
    switch (ast->type)
    {
    case AST_COMMAND:
        res = command(ast, var);
        break;
    case AST_LIST:
        for (int i = 0; i < ast->nb_ast; i++)
        {
            res = ast_evaluate(ast->ast_list[i], var);
            if (var->breakf > 0 || var->continuef > 0)
                break;
        }
        break;
    case AST_IF:
        res = eval_if(ast, var);
        break;
    case AST_PIPE:
        res = mypipe(ast, var);
        break;
    case AST_AND:
        res = eval_and(ast, var);
        break;
    case AST_OR:
        res = eval_or(ast, var);
        break;
    case AST_REDIR:
        res = eval_redir(ast, var);
        break;
    case AST_WHILE:
        res = my_while(ast, var);
        break;
    default:
        return ast_evaluate_bis(ast, var, res);
    }
    char res1[100];
    sprintf(res1, "?=%d", res);
    Addvalue(var, res1);
    return res;
}

int evaluate(struct ast *ast)
{
    struct dico *variables = new_dico();
    add_init(variables);
    int res = ast_evaluate(ast, variables);
    free_dico(variables);
    return res;
}
