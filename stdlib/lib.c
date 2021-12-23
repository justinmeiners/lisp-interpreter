#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include <stddef.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>

#define ARITY_CHECK(min_, max_) do { \
  int args_length_ = lisp_list_length(args); \
  if (args_length_ < min_) { *e = LISP_ERROR_TOO_FEW_ARGS; return lisp_null(); } \
  if (args_length_ > max_) { *e = LISP_ERROR_TOO_MANY_ARGS; return lisp_null(); } \
} while (0);

static Lisp sch_cons(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp x = lisp_car(args);
    args = lisp_cdr(args);
    Lisp y = lisp_car(args);
    return lisp_cons(x, y, ctx);
}

static Lisp sch_car(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_car(lisp_car(args));
}

static Lisp sch_cdr(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_cdr(lisp_car(args));
}

static Lisp sch_set_car(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    lisp_set_car(a, b);
    return lisp_null();
}

static Lisp sch_set_cdr(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    lisp_set_cdr(a, b);
    return lisp_null();
}

static Lisp sch_exact_eq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_eq(a, b));
}

static Lisp sch_equal(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_equal(a, b));
}

static Lisp sch_equal_r(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_equal_r(a, b));
}

static Lisp sch_is_null(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_is_null(lisp_car(args)));
}

static Lisp sch_is_pair(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_PAIR);
}

static Lisp sch_write(Lisp args, LispError* e, LispContext ctx)
{
    lisp_printf(ctx.p->out_port, lisp_car(args));
    return lisp_null();
}

static Lisp sch_display(Lisp args, LispError* e, LispContext ctx)
{
    Lisp x = lisp_car(args);
    lisp_displayf(ctx.p->out_port, x);
    return x;
}

static Lisp sch_write_char(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    fputc(lisp_char(lisp_car(args)), ctx.p->out_port);
    return lisp_null();
}

static Lisp sch_flush(Lisp args, LispError* e, LispContext ctx)
{
    fflush(ctx.p->out_port); return lisp_null();
}

static Lisp sch_read(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_read_file(ctx.p->in_port, e, ctx);
}

static Lisp sch_is_eof(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_equal(lisp_car(args), lisp_eof()));
}

static Lisp sch_error(Lisp args, LispError* e, LispContext ctx)
{
   if (lisp_is_pair(args))
   {
       Lisp l = lisp_car(args);
       fputs(lisp_string(l), ctx.p->err_port);
       args = lisp_cdr(args);
   }
   while (lisp_is_pair(args))
   {
       fputs(" ", ctx.p->err_port);
       lisp_printf(ctx.p->err_port, lisp_car(args));
       args = lisp_cdr(args);
   }

   *e = LISP_ERROR_RUNTIME;
   return lisp_null();
}

static Lisp sch_syntax_error(Lisp args, LispError* e, LispContext ctx)
{
    fprintf(ctx.p->err_port, "expand error: %s ", lisp_string(lisp_car(args)));
    args = lisp_cdr(args);
    if (!lisp_is_null(args))
    {
        lisp_printf(stderr, lisp_car(args));
    }
    fprintf(ctx.p->err_port, "\n");

    *e = LISP_ERROR_FORM_SYNTAX;
    return lisp_null();
}

static Lisp sch_equals(Lisp args, LispError* e, LispContext ctx)
{
    Lisp to_check  = lisp_car(args);
    if (lisp_is_null(to_check)) return lisp_true();
    args = lisp_cdr(args);

    while (lisp_is_pair(args))
    {
        if (lisp_bool(lisp_car(args)) != lisp_bool(to_check)) return lisp_false();
        args = lisp_cdr(args);
    }
    return lisp_true();
}

static Lisp sch_list(Lisp args, LispError* e, LispContext ctx) { return args; }

static Lisp sch_is_list(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_is_list(lisp_car(args)));
}

static Lisp sch_make_list(Lisp args, LispError* e, LispContext ctx)
{
    Lisp length = lisp_car(args);
    if (lisp_type(length) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    Lisp next = lisp_cdr(args);
    Lisp x;
    if (lisp_is_null(next))
    {
        x = lisp_null();
    }
    else
    {
        x = lisp_car(lisp_cdr(args));
    }
    return lisp_make_list(x, lisp_int(length), ctx);
}

static Lisp sch_list_copy(Lisp args, LispError* e, LispContext ctx) {
    return lisp_list_copy(lisp_car(args), ctx);
}

static Lisp sch_append(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_null();  

    while (lisp_is_pair(args))
    {
        l = lisp_list_append(l, lisp_car(args), ctx);
        args = lisp_cdr(args);
    }
    return l;
}

static Lisp sch_append_reverse(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);

    Lisp l = lisp_car(args);
    args = lisp_cdr(args);
    Lisp tail = lisp_car(args);

    return lisp_list_reverse2(l, tail);
}

static Lisp sch_list_ref(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp list = lisp_car(args);
    args = lisp_cdr(args);
    Lisp index = lisp_car(args);
    return lisp_list_ref(list, lisp_int(index));
}

static Lisp sch_length(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_int(lisp_list_length(lisp_car(args)));
}

static Lisp sch_list_advance(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp x = lisp_car(args);
    args = lisp_cdr(args);
    Lisp count = lisp_car(args);
    return lisp_list_advance(x, lisp_int(count));
}

static Lisp sch_add(Lisp args, LispError* e, LispContext ctx)
{
    LispInt exact = 0;
    LispReal inexact = 0; 

    while (lisp_is_pair(args))
    {
        Lisp x = lisp_car(args);
        args = lisp_cdr(args);
        switch (lisp_type(x))
        {
            case LISP_INT:
                exact += lisp_int(x);
                break;
            case LISP_REAL:
                inexact += lisp_real(x);
                break;
            default:
                *e = LISP_ERROR_ARG_TYPE;
                return lisp_null();
        }
    }
    
    return inexact == 0
        ? lisp_make_int(exact)
        : lisp_make_real(inexact + (LispReal)exact);
}

static Lisp sch_mult(Lisp args, LispError* e, LispContext ctx)
{
    LispInt exact = 1;
    LispReal inexact = 1; 

    while (lisp_is_pair(args))
    {
        Lisp x = lisp_car(args);
        args = lisp_cdr(args);
        switch (lisp_type(x))
        {
            case LISP_INT:
                exact *= lisp_int(x);
                break;
            case LISP_REAL:
                inexact *= lisp_real(x);
                break;
            default:
                *e = LISP_ERROR_ARG_TYPE;
                return lisp_null();
        }
    }
    
    return inexact == 1
        ? lisp_make_int(exact)
        : lisp_make_real(inexact * (LispReal)exact);
}

static Lisp sch_sub(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 2);
    Lisp x = lisp_car(args);
    args = lisp_cdr(args);

    Lisp y;
    if (lisp_is_null(args))
    {
        y = x;
        x = lisp_make_int(0);
    }
    else
    {
        y = lisp_car(args);
    }
    switch (lisp_type(x))
    {
        case LISP_REAL:
            return lisp_make_real(lisp_real(x) - lisp_number_to_real(y));
        case LISP_INT:
            switch (lisp_type(y))
            {
                case LISP_REAL:
                    return lisp_make_real(lisp_number_to_real(x) - lisp_real(y));
                case LISP_INT:
                    return lisp_make_int(lisp_int(x) - lisp_int(y));
                default:
                    *e = LISP_ERROR_ARG_TYPE;
                    return lisp_null();
            }
            break;
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }
}

static Lisp sch_divide(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp x = lisp_car(args);
    args = lisp_cdr(args);
    Lisp y = lisp_car(args);

    switch (lisp_type(x))
    {
        case LISP_REAL:
            return lisp_make_real(lisp_real(x) / lisp_number_to_real(y));
        case LISP_INT:
            switch (lisp_type(y))
            {
                case LISP_REAL:
                    return lisp_make_real(lisp_number_to_real(x) / lisp_real(y));
                case LISP_INT:
                    // TODO: divide by zero check?
                    return lisp_make_int(lisp_int(x) / lisp_int(y));
                default:
                    *e = LISP_ERROR_ARG_TYPE;
                    return lisp_null();
            }
            break;
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }
}

static Lisp sch_less(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp x = lisp_car(args);
    args = lisp_cdr(args);
    Lisp y = lisp_car(args);

    switch (lisp_type(x))
    {
        case LISP_REAL:
            return lisp_make_bool(lisp_real(x) < lisp_number_to_real(y));
        case LISP_INT:
            switch (lisp_type(y))
            {
                case LISP_REAL:
                    return lisp_make_bool(lisp_number_to_real(x) < lisp_real(y));
                case LISP_INT:
                    return lisp_make_bool(lisp_int(x) < lisp_int(y));
                default:
                    *e = LISP_ERROR_ARG_TYPE;
                    return lisp_null();
            }
            break;
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }
}

static Lisp sch_to_exact(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_int(lisp_number_to_int(lisp_car(args)));
}

static Lisp sch_to_inexact(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real(lisp_number_to_real(lisp_car(args)));
}

static Lisp sch_symbol_to_string(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp val = lisp_car(args);
    if (lisp_type(val) != LISP_SYMBOL)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }
    else
    {
        return lisp_make_string2(lisp_symbol_string(val), ctx);
    }
}

static Lisp sch_is_symbol(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_SYMBOL);
}

static Lisp sch_symbol_less(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    int result = strncmp(lisp_symbol_string(a), lisp_symbol_string(b), LISP_IDENTIFIER_MAX) < 0;
    return lisp_make_bool(result);
}

static Lisp sch_string_to_symbol(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp val = lisp_car(args);
    if (lisp_type(val) != LISP_STRING)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }
    else
    {
        return lisp_make_symbol(lisp_string(val), ctx);
    }
}

static Lisp sch_gensym(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(0, 0);
    return lisp_gen_symbol(ctx);
}

static Lisp sch_is_string(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_STRING);
}

static Lisp sch_string_is_null(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp a = lisp_car(args);
    return lisp_make_bool(lisp_string(a)[0] == '\0');
}

static Lisp sch_make_string(Lisp args, LispError* e, LispContext ctx)
{
    Lisp length = lisp_car(args);

    if (lisp_type(length) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    Lisp s = lisp_make_string(lisp_int(length), ctx);
    args = lisp_cdr(args);

    if (!lisp_is_null(args))
        lisp_buffer_fill(s, 0, lisp_int(length), lisp_char(lisp_car(args)));
    return s;
}

static Lisp sch_string_less(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    int result = strcmp(lisp_string(a), lisp_string(b)) < 0;
    return lisp_make_bool(result);
}

static Lisp sch_substring(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(3, 3);
    Lisp s = lisp_car(args);
    args = lisp_cdr(args);
    Lisp start = lisp_car(args);
    args = lisp_cdr(args);
    Lisp end = lisp_car(args);
    return lisp_substring(s, lisp_int(start), lisp_int(end), ctx);
}

static Lisp sch_string_length(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    int n = lisp_string_length(lisp_car(args));
    return lisp_make_int(n);
}

static Lisp sch_string_ref(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    Lisp str = lisp_car(args);
    Lisp index = lisp_car(lisp_cdr(args));
    if (lisp_type(str) != LISP_STRING || lisp_type(index) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    return lisp_make_char((int)lisp_string_ref(str, lisp_int(index)));
}

static Lisp sch_string_set(Lisp args, LispError* e, LispContext ctx)
{
    Lisp str = lisp_car(args);
    args = lisp_cdr(args);
    Lisp index = lisp_car(args);
    args = lisp_cdr(args);
    Lisp val = lisp_car(args); 
    if (lisp_type(str) != LISP_STRING || lisp_type(index) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    lisp_string_set(str, lisp_int(index), (char)lisp_int(val));
    return lisp_null();
}

static Lisp sch_string_upcase(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp r = lisp_buffer_copy(lisp_car(args), ctx);
    
    char* c = lisp_buffer(r);
    while (*c) { *c = toupper(*c); ++c; }
    return r;
}

static Lisp sch_string_downcase(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp r = lisp_buffer_copy(lisp_car(args), ctx);
    
    char* c = lisp_buffer(r);
    while (*c) { *c = tolower(*c); ++c; }
    return r;
}

static Lisp sch_string_append(Lisp args, LispError* e, LispContext ctx)
{
    int count = 0;
    Lisp it = args;
    while (lisp_is_pair(it))
    {
        Lisp x = lisp_car(it);
        count += strlen(lisp_string(x));
        it = lisp_cdr(it);
    }

    Lisp result = lisp_make_string(count, ctx);
    char* c = lisp_buffer(result);

    it = args;
    while (lisp_is_pair(it))
    {
        Lisp x = lisp_car(it);
        int n = (LispInt)strlen(lisp_string(x));
        memcpy(c, lisp_string(x), n);
        c += n;
        it = lisp_cdr(it);
    }
    return result;
}

static Lisp sch_string_to_list(Lisp args, LispError* e, LispContext ctx)
{
    const char* c = lisp_string(lisp_car(args));
    Lisp tail = lisp_null();
    while (*c)
    {
        tail = lisp_cons(lisp_make_char(*c), tail, ctx);
        ++c;
    }
    return lisp_list_reverse(tail);
}

static Lisp sch_list_to_string(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    Lisp result = lisp_make_string(lisp_list_length(l), ctx);
    char* s = lisp_buffer(result);
    
    while (lisp_is_pair(l))
    {
        *s = (char)lisp_char(lisp_car(l));
        ++s;
        l = lisp_cdr(l);
    }
    return result;
}

static Lisp sch_string_to_number(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    const char* string = lisp_string(lisp_car(args));
    if (strchr(string, '.'))
    {
        return lisp_parse_real(string);
    }
    else
    {
        return lisp_parse_int(string);
    }
}

static Lisp sch_number_to_string(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    char scratch[64];
    Lisp val = lisp_car(args);
    switch (lisp_type(val))
    {
        case LISP_REAL:
            snprintf(scratch, 64, "%f", lisp_real(val));
            break;
        case LISP_INT:
            snprintf(scratch, 64, "%lli", lisp_int(val));
            break;
        default:
        {
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
        }
    }
    return lisp_make_string2(scratch, ctx);
}

static Lisp sch_char_less(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_char(a) < lisp_char(b));
}

static Lisp sch_is_char(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_CHAR);
}

static Lisp sch_char_upcase(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_char(toupper(c));
}

static Lisp sch_char_downcase(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_char(tolower(c));
}

static Lisp sch_char_is_alphanum(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isalnum(c));
}

static Lisp sch_char_is_alpha(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isalpha(c));
}

static Lisp sch_char_is_number(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isdigit(c));
}

static Lisp sch_char_is_white(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isblank(c));
}

static Lisp sch_char_to_int(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_int(lisp_char(lisp_car(args)));
}

static Lisp sch_is_int(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_INT);
}

static Lisp sch_is_real(Lisp args, LispError* e, LispContext ctx)
{
    LispType t = lisp_type(lisp_car(args));
    return lisp_make_bool(t == LISP_INT || t == LISP_REAL);
}

static Lisp sch_is_boolean(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_BOOL);
}

static Lisp sch_not(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(!lisp_is_true(lisp_car(args)));
}

static Lisp sch_is_even(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool( (lisp_int(lisp_car(args)) & 1) == 0);
}

static Lisp sch_exp(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( exp(lisp_number_to_real(lisp_car(args))) );
}

static int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

static Lisp sch_power(Lisp args, LispError* e, LispContext ctx)
{
    Lisp base = lisp_car(args);
    args = lisp_cdr(args);
    Lisp power = lisp_car(args);

    if (lisp_type(base) == LISP_INT && lisp_type(power) == LISP_INT)
    {
        return lisp_make_int( ipow(lisp_int(base), lisp_int(power)) );
    }
    else
    {
        return lisp_make_real( pow(lisp_number_to_real(base), lisp_number_to_real(power)) );
    }
}

static Lisp sch_log(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( log(lisp_number_to_real(lisp_car(args))) );
}

static Lisp sch_sin(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( sin(lisp_number_to_real(lisp_car(args))) );
}

static Lisp sch_cos(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( cos(lisp_number_to_real(lisp_car(args))) );
}

static Lisp sch_tan(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( tan(lisp_number_to_real(lisp_car(args))) );
}

static Lisp sch_sqrt(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_real( sqrt(lisp_number_to_real(lisp_car(args))) );
}

static Lisp sch_atan(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 2);
    LispReal y = lisp_number_to_real(lisp_car(args));
    args = lisp_cdr(args);

    if (lisp_is_null(args))
    {
        return lisp_make_real( atan(y) );
    }
    else
    {
        LispReal x = lisp_number_to_real(lisp_car(args));
        return lisp_make_real( atan2(y, x) );
    }
}

static Lisp sch_round(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp x = lisp_car(args);
    switch (lisp_type(x))
    {
        case LISP_INT:
            return lisp_make_int((LispInt)round(lisp_int(x)));
        case LISP_REAL:
            return lisp_make_real(round(lisp_real(x)));
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }

}

static Lisp sch_floor(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp x = lisp_car(args);
    switch (lisp_type(x))
    {
        case LISP_INT:
            return lisp_make_int((LispInt)floor(lisp_int(x)));
        case LISP_REAL:
            return lisp_make_real(floor(lisp_real(x)));
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }

}

static Lisp sch_ceiling(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp x = lisp_car(args);
    switch (lisp_type(x))
    {
        case LISP_INT:
            return lisp_make_int((LispInt)ceil(lisp_int(x)));
        case LISP_REAL:
            return lisp_make_real(ceil(lisp_real(x)));
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }
}

static Lisp sch_quotient(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    LispInt a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    LispInt b = lisp_int(lisp_car(args));
    return lisp_make_int(a / b);
}

static Lisp sch_remainder(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    int a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    int b = lisp_int(lisp_car(args));
    return lisp_make_int(a % b);
}

static Lisp sch_modulo(Lisp args, LispError* e, LispContext ctx)
{
    int m = lisp_int(sch_remainder(args, e, ctx));
    if (m < 0) {
        int b = lisp_int(lisp_car(lisp_cdr(args)));
        m = (b < 0) ? m - b : m + b;
    }
    return lisp_make_int(m);
}

static Lisp sch_abs(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    Lisp x = lisp_car(args);
    switch (lisp_type(x))
    {
        case LISP_INT:
            return lisp_make_int(llabs(lisp_int(x)));
        case LISP_REAL:
            return lisp_make_real(fabs(lisp_real(x)));
        default:
            *e = LISP_ERROR_ARG_TYPE;
            return lisp_null();
    }
}

static Lisp sch_is_vector(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool( lisp_type(lisp_car(args)) == LISP_VECTOR );
}

static Lisp sch_make_vector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp length = lisp_car(args);

    if (lisp_type(length) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    Lisp v = lisp_make_vector(lisp_int(length), ctx);
    args = lisp_cdr(args);

    if (!lisp_is_null(args))
        lisp_vector_fill(v, lisp_car(args));
    return v;
}

static Lisp sch_vector_grow(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    Lisp length = lisp_car(lisp_cdr(args));

    if (lisp_type(length) != LISP_INT || lisp_type(v) != LISP_VECTOR)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    if (lisp_int(length) < lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_null();
    }

    return lisp_vector_grow(v, lisp_int(length), ctx);
}

static Lisp sch_vector_length(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    if (lisp_type(v) != LISP_VECTOR)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    return lisp_make_int(lisp_vector_length(v));
}

static Lisp sch_vector_ref(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    Lisp i = lisp_car(lisp_cdr(args));

    if (lisp_type(v) != LISP_VECTOR || lisp_type(i) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    if (lisp_int(i) >= lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_null();
    }

    return lisp_vector_ref(v, lisp_int(i));
}

static Lisp sch_vector_set(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_list_ref(args, 0);
    Lisp i = lisp_list_ref(args, 1);
    Lisp x = lisp_list_ref(args, 2);

    if (lisp_type(v) != LISP_VECTOR || lisp_type(i) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }

    if (lisp_int(i) >= lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_null();
    }

    lisp_vector_set(v, lisp_int(i), x);
    return lisp_null();
}

static Lisp sch_vector_swap(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    args = lisp_cdr(args);
    Lisp i = lisp_car(args);
    args = lisp_cdr(args);
    Lisp j = lisp_car(args);

    if (lisp_type(v) != LISP_VECTOR || lisp_type(i) != LISP_INT || lisp_type(j) != LISP_INT)
    {
        *e = LISP_ERROR_ARG_TYPE;
        return lisp_null();
    }
    if (lisp_int(i) >= lisp_vector_length(v) || lisp_int(j) >= lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_null();
    }
    return lisp_vector_swap(v, lisp_int(i), lisp_int(j));
}

static Lisp sch_vector_fill(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    args = lisp_cdr(args);
    lisp_vector_fill(v, lisp_car(args));
    return lisp_null();
}

static Lisp sch_vector_assq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp k = lisp_car(args);
    args = lisp_cdr(args);
    Lisp v = lisp_car(args);
    return lisp_avector_ref(v, k); 
}

static Lisp sch_subvector(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(3, 3);
    Lisp v = lisp_car(args);
    args = lisp_cdr(args);
    Lisp start = lisp_car(args);
    args = lisp_cdr(args);
    Lisp end = lisp_car(args);
    return lisp_subvector(v, lisp_int(start), lisp_int(end), ctx);
}

static Lisp sch_list_to_vector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    int n = lisp_list_length(l);
    Lisp v = lisp_make_vector(n, ctx);

    for (int i = 0; i < n; ++i)
    {
        lisp_vector_set(v, i, lisp_car(l));
        l = lisp_cdr(l);
    }
    return v;
}

static Lisp sch_vector_to_list(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    int n = lisp_vector_length(v);

    Lisp tail = lisp_null();
    for (int i = 0; i < n; ++i)
        tail = lisp_cons(lisp_vector_ref(v, i), tail, ctx);
    return lisp_list_reverse(tail);
}

static Lisp sch_pseudo_seed(Lisp args, LispError* e, LispContext ctx)
{
    Lisp seed = lisp_car(args);
    srand((unsigned int)lisp_int(seed));
    return lisp_null();
}

static Lisp sch_pseudo_rand(Lisp args, LispError* e, LispContext ctx)
{
    Lisp n = lisp_car(args);
    return lisp_make_int(rand() % lisp_int(n));
}

static Lisp sch_univeral_time(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_int((LispInt)time(NULL));
}

static Lisp sch_is_table(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_TABLE);
}

static Lisp sch_table_make(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_table(ctx);
}

static Lisp sch_table_get(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 3);
    Lisp table = lisp_car(args);
    args = lisp_cdr(args);
    Lisp key = lisp_car(args);
    args = lisp_cdr(args);

    Lisp def;
    if (lisp_is_pair(args))
    {
        def = lisp_car(args);
    }
    else
    {
        def = lisp_null();
    }

    int present;
    Lisp result = lisp_table_get(table, key, &present);
    return present ? result : def;
}

static Lisp sch_table_set(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(3, 3);
    Lisp table = lisp_car(args);
    args = lisp_cdr(args);
    Lisp key = lisp_car(args);
    args = lisp_cdr(args);
    Lisp x = lisp_car(args);
    lisp_table_set(table, key, x, ctx);
    return lisp_null();
}

static Lisp sch_table_size(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    return lisp_make_int(lisp_table_size(table));
}

static Lisp sch_table_to_alist(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    return lisp_table_to_alist(table, ctx);
}

static Lisp sch_is_promise(Lisp args, LispError* e, LispContext ctx)
{
  return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_PROMISE);
}

static Lisp sch_make_promise(Lisp args, LispError* e, LispContext ctx)
{
    Lisp proc = lisp_car(args);
    return lisp_make_promise(proc, ctx);
}

static Lisp sch_promise_store(Lisp args, LispError* e, LispContext ctx)
{
    Lisp promise = lisp_car(args);
    args = lisp_cdr(args);
    Lisp val = lisp_car(args);
    lisp_promise_store(promise, val);
    return lisp_null();
}

static Lisp sch_promise_proc(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_promise_proc(lisp_car(args));
}

static Lisp sch_promise_val(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_promise_val(lisp_car(args));
}

static Lisp sch_promise_forced(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_promise_forced(lisp_car(args)));
}

static Lisp sch_apply(Lisp args, LispError* e, LispContext ctx)
{
    Lisp operator = lisp_car(args);
    args = lisp_cdr(args);
    Lisp op_args = lisp_car(args);
    return lisp_apply(operator, op_args, e, ctx);
}

static Lisp sch_is_lambda(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_LAMBDA);
}

static Lisp sch_lambda_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_lambda_env(lisp_car(args));
}

static Lisp sch_is_func(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    int type = lisp_type(lisp_car(args));
    return lisp_make_bool(type == LISP_FUNC);
}

static Lisp sch_lambda_body(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_lambda_body(lisp_car(args));
}

static Lisp sch_macroexpand(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_macroexpand(lisp_car(args), e, ctx);
}

static Lisp sch_eval(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(2, 2);
    return lisp_eval2(lisp_car(args), lisp_car(lisp_cdr(args)), e, ctx);
}

static Lisp sch_system_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_cdr(lisp_env_global(ctx));
}

static Lisp sch_user_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_env_global(ctx);
}

static Lisp sch_gc_flip(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(0, 0);
    lisp_collect(lisp_null(), ctx);
    return lisp_false();
}

static Lisp sch_print_gc_stats(Lisp args, LispError* e, LispContext ctx)
{
    lisp_print_collect_stats(ctx);
    return lisp_false();
}

static Lisp sch_call_cc(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_call_cc(lisp_car(args), e, ctx);
}

static Lisp sch_is_cont(Lisp args, LispError* e, LispContext ctx)
{
    ARITY_CHECK(1, 1);
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_JUMP);
}

#undef ARITY_CHECK

static const LispFuncDef lib_cfunc_defs[] = {
    
    { "ERROR", sch_error },
    { "SYNTAX-ERROR", sch_syntax_error },

    // Output Procedures https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Output-Procedures.html
    { "WRITE", sch_write },
    { "DISPLAY", sch_display },
    { "WRITE-CHAR", sch_write_char },
    { "FLUSH-OUTPUT-PORT", sch_flush },
    { "READ", sch_read },
    { "EOF-OBJECT?", sch_is_eof },
    
    // Universal Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
    { "GET-UNIVERSAL-TIME", sch_univeral_time },
    { "PRINT-GC-STATISTICS", sch_print_gc_stats },
    
    { "MACROEXPAND", sch_macroexpand },
    
    // Equivalence Predicates https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Equivalence-Predicates.html
    { "EQ?", sch_exact_eq },
    { "EQV?", sch_equal },
    { "EQUAL?", sch_equal_r },
    
    // Booleans https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Booleans.html
    { "BOOLEAN?", sch_is_boolean },
    { "NOT", sch_not },

    // PAIRS
    { "CONS", sch_cons },
    { "CAR", sch_car },
    { "CDR", sch_cdr },
    { "SET-CAR!", sch_set_car },
    { "SET-CDR!", sch_set_cdr },
    { "NULL?", sch_is_null },
    { "PAIR?", sch_is_pair },

    // Lists https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_8.html
    { "LIST", sch_list },
    { "LIST?", sch_is_list },
    { "MAKE-LIST", sch_make_list },
    { "LIST-COPY", sch_list_copy },
    { "LENGTH", sch_length },
    { "APPEND", sch_append },
    { "APPEND-REVERSE!", sch_append_reverse },
    { "LIST-REF", sch_list_ref },
    { "NTHCDR", sch_list_advance },

    // Vectors https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_9.html#SEC82
    { "VECTOR?", sch_is_vector },
    { "MAKE-VECTOR", sch_make_vector },
    { "VECTOR-GROW", sch_vector_grow },
    { "VECTOR-LENGTH", sch_vector_length },
    { "VECTOR-SET!", sch_vector_set },
    { "VECTOR-SWAP!", sch_vector_swap },
    { "VECTOR-REF", sch_vector_ref },
    { "VECTOR-FILL!", sch_vector_fill },
    { "VECTOR-ASSQ", sch_vector_assq },
    { "SUBVECTOR", sch_subvector },
    { "LIST->VECTOR", sch_list_to_vector },
    { "VECTOR->LIST", sch_vector_to_list },

    // Strings https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html#SEC61
    { "STRING?", sch_is_string },
    { "MAKE-STRING", sch_make_string },
    { "STRING=?", sch_equal_r },
    { "STRING<?", sch_string_less },
    { "SUBSTRING", sch_substring },
    { "STRING-NULL?", sch_string_is_null },
    { "STRING-LENGTH", sch_string_length },
    { "STRING-REF", sch_string_ref },
    { "STRING-SET!", sch_string_set },
    { "STRING-UPCASE", sch_string_upcase },
    { "STRING-DOWNCASE", sch_string_downcase },
    { "STRING-APPEND", sch_string_append },
    { "STRING->LIST", sch_string_to_list },
    { "LIST->STRING", sch_list_to_string },
    { "STRING->NUMBER", sch_string_to_number },
    { "NUMBER->STRING", sch_number_to_string },

    // Characters https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Characters.html#Characters
    { "CHAR?", sch_is_char },
    { "CHAR=?", sch_equals },
    { "CHAR<?", sch_char_less },
    { "CHAR-UPCASE", sch_char_upcase },
    { "CHAR-DOWNCASE", sch_char_downcase },
    { "CHAR-WHITESPACE?", sch_char_is_white },
    { "CHAR-ALPHANUMERIC?", sch_char_is_alphanum },
    { "CHAR-ALPHABETIC?", sch_char_is_alpha },
    { "CHAR-NUMERIC?", sch_char_is_number },
    { "CHAR->INTEGER", sch_char_to_int },

    // Association Lists https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Association-Lists.html
    // Numerical operations https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Numerical-operations.html
    { "=", sch_equals },
    { "+", sch_add },
    { "-", sch_sub },
    { "*", sch_mult },
    { "/", sch_divide },
    { "<", sch_less },
    { "INTEGER?", sch_is_int },
    { "EVEN?", sch_is_even },
    { "REAL?", sch_is_real },
    { "EXP", sch_exp },
    { "EXPT", sch_power },
    { "LOG", sch_log },
    { "SIN", sch_sin },
    { "COS", sch_cos },
    { "TAN", sch_tan },
    { "ATAN", sch_atan },
    { "SQRT", sch_sqrt },
    { "ROUND", sch_round },
    { "FLOOR", sch_floor },
    { "CEILING", sch_ceiling },
    { "QUOTIENT", sch_quotient },
    { "REMAINDER", sch_remainder },
    { "MODULO", sch_modulo },
    { "ABS", sch_abs },
    { "MAGNITUDE", sch_abs },

    
    { "EXACT?", sch_is_int },
    { "EXACT->INEXACT", sch_to_inexact },
    { "INEXACT->EXACT", sch_to_exact },
    
    // Symbols https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
    { "SYMBOL?", sch_is_symbol },
    { "SYMBOL<?", sch_symbol_less },
    { "STRING->SYMBOL", sch_string_to_symbol },
    { "SYMBOL->STRING", sch_symbol_to_string },
    { "GENERATE-UNINTERNED-SYMBOL", sch_gensym },
    { "GENSYM", sch_gensym },

    // Environments https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_14.html
    { "EVAL", sch_eval },
    { "SYSTEM-GLOBAL-ENVIRONMENT", sch_system_env },
    { "USER-INITIAL-ENVIRONMENT", sch_user_env },
    // { "THE-ENVIRONMENT", sch_current_env },
    
    // Hash Tables https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Basic-Hash-Table-Operations.html#Basic-Hash-Table-Operations
    { "HASH-TABLE?", sch_is_table },
    { "MAKE-HASH-TABLE", sch_table_make },
    { "HASH-TABLE-SET!", sch_table_set },
    { "HASH-TABLE-REF", sch_table_get },
    { "HASH-TABLE-SIZE", sch_table_size },
    { "HASH-TABLE->ALIST", sch_table_to_alist },

    { "PROMISE?", sch_is_promise },
    { "MAKE-PROMISE", sch_make_promise },
    { "_PROMISE-PROCEDURE", sch_promise_proc },
    { "_PROMISE-STORE!", sch_promise_store },
    { "PROMISE-VALUE", sch_promise_val },
    { "PROMISE-FORCED?", sch_promise_forced },
    
    // Procedures https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
    { "APPLY", sch_apply },
    { "COMPILED-PROCEDURE?", sch_is_func },
    { "COMPOUND-PROCEDURE?", sch_is_lambda },
    { "PROCEDURE-ENVIRONMENT", sch_lambda_env },
    // TOOD: Almost standard
    { "PROCEDURE-BODY", sch_lambda_body },
    { "CALL/CC", sch_call_cc },
    { "CONTINUATION?", sch_is_cont },

    // Random Numbers https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Random-Numbers.html
    { "RANDOM", sch_pseudo_rand },
    { "RANDOM-SEED!", sch_pseudo_seed },
   
    // Garbage Collection https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-user/Garbage-Collection.html
    { "GC-FLIP", sch_gc_flip },
    
    { NULL, NULL }
    
};

void lisp_lib_load(LispContext ctx)
{
    Lisp table = lisp_make_table(ctx);
    lisp_table_define_funcs(table, lib_cfunc_defs, ctx);
    Lisp system_env = lisp_env_extend(lisp_env_global(ctx), table, ctx);
    Lisp user_env = lisp_env_extend(system_env, lisp_make_table(ctx), ctx);
    lisp_env_set_global(user_env, ctx);

    const char* to_load[] = {
        lib_0_sequences_src_, lib_1_forms_src_,
        lib_2_forms_src_, lib_3_math_src_,
        lib_4_sequences_src_, lib_5_streams_src_,
        lib_6_other_src_, 
    };

    int n = sizeof(to_load) / sizeof(const char*);

    for (int i = 0; i < n; ++i)
    {
        LispError error;
        Lisp src = lisp_read(to_load[i], &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(ctx.p->err_port, "failed to parse system library %d: %s\n", i, lisp_error_string(error));
            return;
        }
 
        lisp_eval2(src, system_env, &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(ctx.p->err_port, "failed to eval system library %d: %s\n", i, lisp_error_string(error));
            return;
        }
    }
    lisp_collect(lisp_null(), ctx);

#ifdef LISP_DEBUG
        lisp_print_collect_stats(ctx);
#endif
}
