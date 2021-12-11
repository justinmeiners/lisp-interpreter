#include <stdlib.h>
#include <time.h>
#include <string.h>

// Disable asserts?
// #define NDEBUG

//#define LISP_DEBUG
#define LISP_IMPLEMENTATION
#include "lisp.h"

static Lisp sch_load(Lisp args, LispError* e, LispContext ctx)
{
    Lisp path = lisp_car(args);
    Lisp result = lisp_read_path(lisp_string(path), e, ctx);
    if (*e != LISP_ERROR_NONE) return lisp_make_null();
    return lisp_eval(result, e, ctx);
}

int main(int argc, const char* argv[])
{
    const char* file_path = NULL;
    int run_script = 0;
    int verbose = 0;
    
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--load") == 0)
        {
            file_path = argv[i + 1];
        }
        if (strcmp(argv[i], "--script") == 0)
        {
            file_path = argv[i + 1];
            run_script = 1;
        }
    }
    
    LispContext ctx = lisp_init();
    lisp_load_lib(ctx);
    lisp_env_define(
        lisp_cdr(lisp_env_global(ctx)),
        lisp_make_symbol("LOAD", ctx), 
        lisp_make_func(sch_load),
        ctx
    );

    clock_t start_time, end_time;
        
    if (file_path)
    {
        if (verbose)
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

        if (verbose)
            printf("read (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);

        start_time = clock();

        Lisp code = lisp_macroexpand(l, &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(stderr, "%s\n", lisp_error_string(error));
            exit(1);
        }


        end_time = clock();

        if (verbose)
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

        if (verbose)
            printf("eval (us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
    }

    if (!run_script)
    {
        // REPL
        while (!feof(stdin))
        {
            printf("> ");
            char line[LISP_FILE_CHUNK_SIZE];
            fgets(line, LISP_FILE_CHUNK_SIZE, stdin);

            clock_t start_time = clock();
            LispError error;
            Lisp code = lisp_read(line, &error, ctx);
            
            if (error != LISP_ERROR_NONE)
            {
                fprintf(stderr, "%s\n", lisp_error_string(error));
                continue;
            }

            Lisp l = lisp_eval(code, &error, ctx);
            clock_t end_time = clock();

            if (error != LISP_ERROR_NONE)
            {
                fprintf(stderr, "%s\n", lisp_error_string(error));
            }

            lisp_print(l);
            printf("\n");
            
            lisp_collect(lisp_make_null(), ctx);
            
            if (verbose)
                printf("(us): %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
       }
    }

    lisp_shutdown(ctx);

    return 0;
}

