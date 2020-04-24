#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "lisp.h"

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{
    const char* file_path = NULL;
    size_t page_size = 8192;
    
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--load") == 0)
        {
            file_path = argv[i + 1];
        }
        else if (strcmp(argv[i], "--page-size") == 0)
        {
            page_size = atoi(argv[i + 1]);
        }
    }
    
    LispContext ctx = lisp_init_lib_opt(512, 1024, page_size);

    clock_t start_time, end_time;
        
    if (file_path)
    {
        if (LISP_DEBUG)
        {
            printf("loading: %s\n", file_path);
        }

        start_time = clock();        
        FILE* file = fopen(file_path, "r");
        
        if (!file)
        {
            fprintf(stderr, "failed to open: %s", argv[1]);
            return 2;
        }
     
        LispError error;
        Lisp l = lisp_read_file(file, &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(stderr, "%s\n", lisp_error_string(error));
        }


        fclose(file);
        end_time = clock();

        if (LISP_DEBUG)
            printf("read (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);

        start_time = clock();

        Lisp code = lisp_expand(l, &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(stderr, "%s\n", lisp_error_string(error));
            exit(1);
        }


        end_time = clock();

        if (LISP_DEBUG)
            printf("expand (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);


        start_time = clock(); 
        lisp_eval(code, &error, ctx);  
        end_time = clock();

        if (error != LISP_ERROR_NONE)
        {
            fprintf(stderr, "%s\n", lisp_error_string(error));
            exit(1);
        }

        lisp_collect(lisp_make_null(), ctx);

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
            LispError error;
            Lisp code = lisp_read(line, &error, ctx);
            
            if (error != LISP_ERROR_NONE)
            {
                fprintf(stderr, "%s\n", lisp_error_string(error));
                exit(1);
            }

            Lisp l = lisp_eval(code, &error, ctx);
            clock_t end_time = clock();

            if (error != LISP_ERROR_NONE)
            {
                fprintf(stderr, "%s\n", lisp_error_string(error));
                exit(1);
            }

            lisp_print(l);
            printf("\n");
            
            lisp_collect(lisp_make_null(), ctx);
            
            if (LISP_DEBUG)
                printf("(us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
       }
    }

    return 0;
}

