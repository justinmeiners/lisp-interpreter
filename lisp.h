#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdint.h>

#define LISP_DEBUG 0

typedef enum
{
    LISP_FLOAT,
    LISP_INT,
    LISP_PAIR,   // cons pair (car, cdr)
    LISP_SYMBOL, // unquoted strings
    LISP_STRING, // quoted strings
    LISP_LAMBDA, // user defined lambda
    LISP_PROC,   // C function
    LISP_ENV,    // evaluation environment
    LISP_NULL,
} LispType;

typedef struct
{
    LispType type;

    union
    {
        float float_val;
        int int_val;  
        void* val;
    };
} Lisp;

typedef struct
{
    unsigned short gc_flags;
    unsigned short type;
    unsigned int data_size;
    char data[];
} LispBlock;

typedef struct LispContext* LispContextRef;
typedef Lisp (*LispProc)(Lisp, LispContextRef);

// Primitive types
#define lisp_type(l) ((l).type)
#define lisp_is_null(l) ((l).type == LISP_NULL)
int lisp_eq(Lisp a, Lisp b);
Lisp lisp_null();
Lisp lisp_make_int(int n);
int lisp_int(Lisp l);
Lisp lisp_make_float(float x);
float lisp_float(Lisp l);
Lisp lisp_make_string(const char* string, LispContextRef ctx);
const char* lisp_string(Lisp l);
Lisp lisp_make_symbol(const char* symbol, LispContextRef ctx);
const char* lisp_symbol(Lisp l);

// Lists
#define lisp_car(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[0] )
#define lisp_cdr(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[1] )
Lisp lisp_cons(Lisp car, Lisp cdr, LispContextRef ctx);
Lisp lisp_at_index(Lisp l, int n);

// convience function for cons'ing together items. arguments must be null terminated
Lisp lisp_list(LispContextRef ctx, Lisp first, ...);

// Dictionaries
Lisp lisp_for_key(Lisp l, Lisp symbol);

// procedures
Lisp lisp_make_proc(LispProc proc);

// evaluation environments
Lisp lisp_make_env(Lisp parent, int capacity, LispContextRef ctx);
void lisp_env_set(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx);
void lisp_env_add_procs(Lisp env, const char** names, LispProc* funcs, LispContextRef ctx);
Lisp lisp_make_default_env(struct LispContext* ctx);

// Maxwell's equations of Software. REP
Lisp lisp_parse(const char* program, LispContextRef ctx);
Lisp lisp_read(const char* program, LispContextRef ctx);
Lisp lisp_eval(Lisp symbol, Lisp env, LispContextRef ctx);
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// memory managment/garbage collection
LispContextRef lisp_init(unsigned int heap_size);
void lisp_shutdown(LispContextRef ctx);
Lisp lisp_collect(LispContextRef ctx, Lisp root);

#endif
