#define BIL_IMPLEMENTATION
#include "bil.h"

#define CC "gcc"
#define TAR "bin/lambda"
#define SRC "src/lambda.c", "src/arena.c", "src/lexer.c", "src/sv.c", "src/parser.c", "src/types.c"
#define CFLAGS "-Wall", "-Wextra", "-flto", "-O2"
#define DEBUG_FLAGS "-Wall", "-Wextra", "-g3"

static int debug_status;

void cmd_flags(int *argc, char ***argv)
{
    bil_shift_args(argc, argv); // skip program
    while (*argc > 0) {
        char *flag = bil_shift_args(argc, argv);
        if (!strcmp(flag, "debug")) {
            debug_status = 1;
        }
    }
}

int main(int argc, char **argv)
{
    BIL_REBUILD(argc, argv, "bin");
    int status = BIL_EXIT_SUCCESS;
    Bil_Cmd cmd = {0};

bil_workflow_begin();

    if (argc > 0)
        cmd_flags(&argc, &argv);

    bil_cmd_append(&cmd, CC, "-ledit");
    if (debug_status) bil_cmd_append(&cmd, DEBUG_FLAGS);
    else bil_cmd_append(&cmd, CFLAGS);
    bil_cmd_append(&cmd, SRC);
    bil_cmd_append(&cmd, "-o", TAR);

    if (!bil_cmd_run_sync(&cmd))
        status = BIL_EXIT_FAILURE;

bil_workflow_end();

    return status;
}
