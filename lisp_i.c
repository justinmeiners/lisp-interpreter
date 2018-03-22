#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lisp.h"

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{
    int quote = 0;
    const char* file_path = NULL;
    
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--quote") == 0)
        {
            quote = 1;
        }
        else if (strcmp(argv[i], "--file") == 0)
        {
            file_path = argv[i + 1];
        }
    }
    
    LispContextRef ctx = lisp_init(20971520);
    Lisp env = lisp_make_default_env(ctx);
    
    if (file_path)
    {
        FILE* file = fopen(file_path, "r");
        
        if (!file)
        {
            fprintf(stderr, "failed to open: %s", argv[1]);
            return 2;
        }
        
        clock_t start_time = clock();
        
        Lisp l = lisp_null();
        if (quote)
        {
            l = lisp_parse_file(file, ctx);
        }
        else
        {
            l = lisp_read_file(file, ctx);
        }
        clock_t end_time = clock();
        
        fclose(file);
        
        if (LISP_DEBUG)
            printf("us: %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
        
        lisp_collect(l, ctx);
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

