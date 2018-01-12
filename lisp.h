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

// Cell utilities
#define lisp_car(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[0] )
#define lisp_cdr(l) ( ((Lisp*)(((LispBlock*)(l).val)->data))[1] )

LispType lisp_type(Lisp l);
int lisp_is_null(Lisp l);
Lisp lisp_null();
Lisp lisp_create_int(int n);
int lisp_int(Lisp l);
Lisp lisp_create_float(float x);
float lisp_float(Lisp l);
Lisp lisp_cons(Lisp car, Lisp cdr, LispContext* ctx);
Lisp lisp_create_string(const char* string, LispContext* ctx);
const char* lisp_string(Lisp l);
Lisp lisp_create_symbol(const char* symbol, LispContext* ctx);
const char* lisp_symbol(Lisp l);

// procedures
Lisp lisp_create_proc(LispProc proc);

// evaluation environments
Lisp lisp_create_env(Lisp parent, int capacity, LispContext* ctx);
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
