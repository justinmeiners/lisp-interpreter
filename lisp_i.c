#include <stdlib.h>
#include "lisp.h"
#include <time.h>

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{
    //printf("word-size: %lu\n", sizeof(LispWord));
    //printf("block-size: %lu\n", sizeof(LispBlock));
    
    LispContextRef ctx = lisp_init(20971520);
    Lisp env = lisp_make_default_env(ctx);

    if (argc > 1)
    {
        FILE* file = fopen(argv[1], "r");

        if (!file)
        {
            fprintf(stderr, "failed to open: %s", argv[1]);
            return 2;
        }

        Lisp list = lisp_parse_file(file, ctx);

       // Lisp list = lisp_read_file(file, ctx);

        fclose(file);

        lisp_print(list);
    
        //lisp_eval(list, env, ctx);
        //lisp_collect(env, ctx);
    }
    else
    {
        // REPL
        while (!feof(stdin))
        {
            printf("> ");
            char line[LINE_MAX];
            fgets(line, LINE_MAX, stdin);

            clock_t start_time = clock();
            Lisp contents = lisp_read(line, ctx);
            Lisp result = lisp_eval(contents, env, ctx);
            clock_t end_time = clock();
            lisp_print(result);
            printf("\n");
            
            if (LISP_DEBUG)
                printf("us: %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);

            start_time = clock();
            env = lisp_collect(env, ctx);
            end_time = clock();
            
            if (LISP_DEBUG)
                printf("gc us: %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
        }
    }

    return 1;
}

