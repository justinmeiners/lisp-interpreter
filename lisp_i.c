#include "lisp.h"

int main(int argc, const char* argv[])
{   
    LispHeap heap;
    lisp_heap_init(&heap);

    LispEnv global;
    lisp_env_init_default(&global);


#define LINE_MAX 2048

    // REPL
    while (!feof(stdin))
    {
        printf("> ");
        char line[LINE_MAX];
        fgets(line, LINE_MAX, stdin);

        LispWord contents = lisp_read(line, &heap);
        lisp_print(stdout, lisp_eval(lisp_car(contents), &global, &heap));
        printf("\n");
    }

    return 1;
}

