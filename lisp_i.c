#include "lisp.h"

int main(int argc, const char* argv[])
{   
    LispEnv global;
    lisp_env_init_default(&global);


#define LINE_MAX 2048

    // REPL
    while (!feof(stdin))
    {
        printf("> ");
        char line[LINE_MAX];
        fgets(line, LINE_MAX, stdin);

        LispCell contents = lisp_read(line);
        lisp_print(stdout, lisp_eval(lisp_car(contents), &global));
        puts("");
    }

    return 1;
}

