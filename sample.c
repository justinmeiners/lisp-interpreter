#include <stdlib.h>
#include <time.h>
#include <string.h>

#define LISP_IMPLEMENTATION
#include "lisp.h"

Lisp integer_range(Lisp args, LispError* e, LispContext ctx)
{
    // first argument
    LispInt start = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    // second argument
    LispInt end = lisp_int(lisp_car(args));

    if (end < start)
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_make_null();
    }

    LispInt n = end - start;
    Lisp numbers = lisp_make_vector_uninitialized(n, ctx);

    for (LispInt i = 0; i < n; ++i)
        lisp_vector_set(numbers, i, lisp_make_int(start + i));

    return numbers;
}

int main(int argc, const char* argv[])
{
    LispContext ctx = lisp_init();
    lisp_load_lib(ctx);

    // wrap in Lisp object
    Lisp func = lisp_make_func(integer_range);

    // add to enviornment with symbol INTEGER-RANGE
    Lisp env = lisp_env_global(ctx);
    lisp_env_define(env, lisp_make_symbol("INTEGER-RANGE", ctx), func, ctx);
    Lisp pi = lisp_make_real(3.141592);
    lisp_env_define(env, lisp_make_symbol("PI", ctx), pi, ctx);

    LispError e;
    Lisp result = lisp_eval(lisp_read("(INTEGER-RANGE 5 15)", &e, ctx), &e, ctx);
    lisp_print(result);

    if (e != LISP_ERROR_NONE) fprintf(stderr, "error: %s\n", lisp_error_string(e));

    lisp_shutdown(ctx);
    return 0;
}

