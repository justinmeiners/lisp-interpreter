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
} LispWord;

typedef struct
{
    int gc_flags;
    int type;
    intptr_t data_size;
    char data[];
} LispBlock;

typedef struct
{
   LispWord symbol;
   LispWord val;
} LispEnvEntry;

// hash table
// open addressing with dynamic resizing
typedef struct LispEnv
{
    struct LispEnv* parent;
    int size;
    int capacity;
    LispEnvEntry* table;
} LispEnv;

typedef struct
{
    char* buffer;
    size_t size;
    size_t capacity;
} LispHeap;


typedef struct
{
    const LispBlock** symbols;
    int size;
    int capacity;
} SymbolTable;

typedef struct
{
    LispHeap heap;
    LispEnv global;
    SymbolTable symbols;
    int debug;
    
    int lambda_counter;
} LispContext;

typedef LispWord (*LispProc)(LispWord, LispContext*);

// Cell utilities
#define lisp_car(word) ( ((LispWord*)(((LispBlock*)(word).val)->data))[0] )
#define lisp_cdr(word) ( ((LispWord*)(((LispBlock*)(word).val)->data))[1] )

LispType lisp_type(LispWord word);
int lisp_is_null(LispWord word);
LispWord lisp_null();
LispWord lisp_create_int(int n);
int lisp_int(LispWord word);
LispWord lisp_create_float(float x);
float lisp_float(LispWord word);
LispWord lisp_cons(LispWord car, LispWord cdr, LispContext* ctx);
LispWord lisp_create_string(const char* string, LispContext* ctx);
const char* lisp_string(LispWord word);
LispWord lisp_create_symbol(const char* symbol, LispContext* ctx);
const char* lisp_symbol(LispWord word);
// procedures
LispWord lisp_create_proc(LispProc proc);

// evaluation environments
void lisp_env_init(LispEnv* env, LispEnv* parent, int capacity);
void lisp_env_set(LispEnv* env, LispWord symbol, LispWord value);
void lisp_env_print(LispEnv* env);

// Maxwell's equations of Software. REP
LispWord lisp_read(const char* program, LispContext* ctx);
LispWord lisp_eval(LispWord symbol, LispEnv* env, LispContext* ctx);
void lisp_print(LispWord word);
void lisp_printf(FILE* file, LispWord word);

// memory managment/garbage collection
int lisp_init(LispContext* ctx);
// this is just a shortcut for
// lisp_env_set(.. lisp_create_symbol(..), lisp_create_proc(proc))
void lisp_add_proc(const char* name, LispProc proc, LispContext* ctx);
size_t lisp_collect(LispContext* ctx);


#endif
