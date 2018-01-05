#include <stdlib.h>
#include "lisp.h"

#define LINE_MAX 2048

int main(int argc, const char* argv[])
{   
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

        lisp_print(list);
        lisp_eval(list, NULL, &ctx);
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

            LispWord contents = lisp_read(line, &ctx);
            lisp_print(lisp_eval(contents, NULL, &ctx));
            printf("\n");
            printf("collected: %lu size: %lu\n", lisp_collect(&ctx), ctx.heap.size);
        }
    }

    return 1;
}

