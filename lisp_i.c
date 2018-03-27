#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lisp.h"

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{
    const char* file_path = NULL;
    
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--load") == 0)
        {
            file_path = argv[i + 1];
        }
    }
    
    LispContextRef ctx = lisp_init_default(20000000);

    clock_t start_time, end_time;
        
    if (file_path)
    {
        printf("loading: %s\n", file_path);

        start_time = clock();        
        FILE* file = fopen(file_path, "r");
        
        if (!file)
        {
            fprintf(stderr, "failed to open: %s", argv[1]);
            return 2;
        }
     
        Lisp l = lisp_read_file(file, ctx);
        fclose(file);
        end_time = clock();

        if (LISP_DEBUG)
            printf("read (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);

        start_time = clock();
        Lisp code = lisp_expand(l, ctx);
        end_time = clock();

        if (LISP_DEBUG)
            printf("expand (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);


        start_time = clock(); 
        lisp_eval(code, lisp_get_global_env(ctx), ctx);  
        end_time = clock();

        if (LISP_DEBUG)
            printf("eval (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
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
            Lisp code = lisp_expand(lisp_read(line, ctx), ctx);
            Lisp l = lisp_eval(code, lisp_get_global_env(ctx), ctx);
            clock_t end_time = clock();
            lisp_print(l);
            printf("\n");
            
            if (LISP_DEBUG)
                printf("(us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
       }
    }

    return 1;
}

