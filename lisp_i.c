#include "lisp.h"

int main(int argc, const char* argv[])
{   
    LispHeap heap;
    lisp_heap_init(&heap);

    LispEnv global;
    lisp_env_init_default(&global);

   // printf("%i\n", sizeof(LispWord));

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
        printf("collected: %lu size: %lu\n", lisp_gc(&heap, &global), heap.size);
    }

    return 1;
}

