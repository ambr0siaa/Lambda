#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "arena.h"
#include "types.h"
#include "lexer.h"
#include "parser.h"

#define LAM_PROMPT "> "

#define line_end(l)     free((l)->data)
#define lamrepl_usage   printf("Lambda REPL mode. To exit type \"quit\".\n")

static char *hs = ".lambda_history";

LAM_FUNC char *shift_args(int *argc, char ***argv)
{
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

LAM_FUNC void usage(const char *program)
{
    printf("\nLambda Programming Language\n");
    printf("    By default starting REPL mode.\n\n");
    printf("Usage: %s [options] <file.lam>\n", program);
    printf("Options:\n");
    printf("    -h    shows this usage\n");
}


LAM_FUNC int cmdargs(int *argc, char ***argv)
{
    int status = 1;
    const char *program = shift_args(argc, argv);

    while (*argc > 0) {
        char *flag = shift_args(argc, argv);
        if (flag[0] == '-') {
            switch (flag[1]) {
                case 'h': {
                    usage(program);
                    defer_status(1);
                }
                default: {
                    report("Unknown option `%c`", flag[1]);
                    defer_status(0);
                }
            }
        } else {
            report("Unknown flag `%s`", flag);
            defer_status(0);
        }
    }

defer:
    return status;
}

LAM_FUNC String_View slurp_line(const char *prompt)
{
    static int isread;
    if (!isread) {
        read_history(hs);
        isread = 1;
    }

    char *line = readline(prompt);
    if (!line) return (String_View) {0};
    
    add_history(line);
    append_history(1, hs);
    return sv_from_cstr(line);
}

LAM_FUNC int print_obj(LObject *o)
{
    switch (o->t) {
        case OBJ_TYPE_NIL:
            printf("nil");
            break;

        case OBJ_TYPE_INT:
            printf("%lli", o->v.i);
            break;

        case OBJ_TYPE_FLT:
            printf("%lf", o->v.f);
            break;

        case OBJ_TYPE_BOOLEAN:
            printf("%s", o->v.b == 1 ? "True" : "False");
            break;

        case OBJ_TYPE_STR:
            printf(SV_Fmt, (int)o->v.s->count, o->v.s->data);
            break;

        default:
            report("Unknown object type %u\n", o->t);
            return 0;          
    }

    printf("\n");
    return 1;
}

LAM_FUNC void repl(String_View line)
{
    Arena a = {0};
    Lexer lex = lexer_new(NULL, line);
    Statement s = parse_statement(&a, &lex);

    if (s.t != STATEMENT_NONE) {
        Expr expr = stateval(&a, &s);
        LObject o = obj_from_atom(&a, expr.v.a);
        print_obj(&o);
    }
    
    arena_free(&a);
}

int main(int argc, char **argv)
{
    if (!cmdargs(&argc, &argv))
        return EXIT_FAILURE;

    lamrepl_usage;

    while (1) {
        String_View line = slurp_line(LAM_PROMPT);
        if (!line.data) return EXIT_FAILURE;

        if (sv_cmp(line, sv_from_cstr("quit"))) {
            line_end(&line);
            break;
        }

        repl(line);
        line_end(&line);
    }

    return EXIT_SUCCESS;
}
