#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdint.h>

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

typedef struct
{
    char* buffer;
    unsigned int size;
    unsigned int capacity;
} LispHeap;

typedef struct
{
    const LispBlock** symbols;
    unsigned int size;
    unsigned int capacity;
} SymbolTable;

typedef struct
{
    LispHeap heap;
    Lisp env;
    SymbolTable symbols;
    int debug;
    
    int lambda_counter;
} LispContext;

typedef Lisp (*LispProc)(Lisp, LispContext*);

// Primitive types
LispType lisp_type(Lisp l);
int lisp_eq(Lisp a, Lisp b);
Lisp lisp_null();
int lisp_is_null(Lisp l);
Lisp lisp_make_int(int n);
int lisp_int(Lisp l);
Lisp lisp_make_float(float x);
float lisp_float(Lisp l);
Lisp lisp_make_string(const char* string, LispContext* ctx);
const char* lisp_string(Lisp l);
Lisp lisp_make_symbol(const char* symbol, LispContext* ctx);
const char* lisp_symbol(Lisp l);

// Lists
#define lisp_car(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[0] )
#define lisp_cdr(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[1] )
Lisp lisp_cons(Lisp car, Lisp cdr, LispContext* ctx);
Lisp lisp_at_index(Lisp l, int n);

// Dictionaries
Lisp lisp_for_key(Lisp l, Lisp symbol);

// procedures
Lisp lisp_make_proc(LispProc proc);

// evaluation environments
Lisp lisp_make_env(Lisp parent, int capacity, LispContext* ctx);
void lisp_env_set(Lisp env, Lisp symbol, Lisp value, LispContext* ctx);
void lisp_env_add_procs(Lisp env, const char** names, LispProc* funcs, LispContext* ctx);

// Maxwell's equations of Software. REP
Lisp lisp_read(const char* program, LispContext* ctx);
Lisp lisp_eval(Lisp symbol, Lisp env, LispContext* ctx);
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// memory managment/garbage collection
int lisp_init(LispContext* ctx);
size_t lisp_collect(LispContext* ctx);


#endif
