#include <stdlib.h>
#include <time.h>
#include <string.h>

#define LISP_IMPLEMENTATION
#define LISP_NO_LIB
#include "lisp.h"

int main(int argc, const char* argv[])
{
    LispContext ctx = lisp_init();
    LispError error;
    Lisp data = lisp_read_file(stdin, &error, ctx);

    if (error != LISP_ERROR_NONE)
    {
        fprintf(stderr, "error: %s\n", lisp_error_string(error));
    }
    data = lisp_collect(data, ctx);
    lisp_print(data);

    lisp_shutdown(ctx);
    return 0;
}

