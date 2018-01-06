#include <stdlib.h>
#include "lisp.h"
#include <time.h>

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{
    printf("word-size: %lu\n", sizeof(LispWord));
    printf("block-size: %lu\n", sizeof(LispBlock));
    
    LispContext ctx;
    lisp_init(&ctx);

    if (argc > 1)
    {
        FILE* file = fopen(argv[1], "r");

        if (!file)
        {
            fprintf(stderr, "failed to open: %s", argv[1]);
            return 2;
        }

        fseek(file, 0, SEEK_END);
        size_t length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char* contents = malloc(length);
        
        if (!contents)
        {
            return 3;
        }

        fread(contents, 1, length, file);
        fclose(file);

        LispWord list = lisp_read(contents, &ctx);
        lisp_eval(list, ctx.env, &ctx);
        lisp_collect(&ctx); 
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
            LispWord contents = lisp_read(line, &ctx);
            LispWord result = lisp_eval(contents, ctx.env, &ctx);
            clock_t end_time = clock();
            lisp_print(result);
            printf("\n");
            
            if (ctx.debug)
                printf("us: %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);

            start_time = clock();
            lisp_collect(&ctx);
            end_time = clock();
            
            if (ctx.debug)
                printf("gc us: %lu\n", 1000000 * (end_time - start_time) / CLOCKS_PER_SEC);
        }
    }

    return 1;
}

