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
    LISP_LAMBDA,   // user defined lambda
    LISP_PROC, // C function
    LISP_NULL,
} LispType;

typedef struct
{
    int gc_flags;
    int type;
    intptr_t data_size;
    char data[];
} LispBlock;

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
    char* buffer;
    size_t size;
    size_t capacity;
} LispHeap;

#define ENTRY_KEY_MAX 128

typedef struct
{
   char key[ENTRY_KEY_MAX];
   LispWord value; 
} LispEnvEntry;

// hash table
// open addressing with dynamic resizing
typedef struct LispEnv
{
    struct LispEnv* parent;

    int retain_count;
    int count;
    int capacity;
    LispEnvEntry* table;
} LispEnv;

typedef struct
{
    LispHeap heap;
    LispEnv env;
} LispContext;

// Cell utilities
#define lisp_car(word) ( ((LispWord*)(((LispBlock*)(word).val)->data))[0] )
#define lisp_cdr(word) ( ((LispWord*)(((LispBlock*)(word).val)->data))[1] )
LispWord lisp_null();
LispWord lisp_create_int(int n);
LispWord lisp_create_float(float x);
LispWord lisp_cons(LispWord car, LispWord cdr, LispHeap* heap);
LispWord lisp_create_string(const char* string, LispHeap* heap);
const char* lisp_string(LispWord word);
LispWord lisp_create_symbol(const char* symbol, LispHeap* heap);
const char* lisp_symbol(LispWord word);
// procedures
LispWord lisp_create_proc(LispWord (*func)(LispWord));

// memory managment/garbage collection
void lisp_heap_init(LispHeap* heap);

// evaluation environments
void lisp_env_init(LispEnv* env, LispEnv* parent, int capacity);
// reference counting memory managmeent
void lisp_env_retain(LispEnv* env);
void lisp_env_release(LispEnv* env);
void lisp_env_set(LispEnv* env, const char* key, LispWord value);
void lisp_env_print(LispEnv* env);

// default global enviornment
// you can start your own enviornment with this
// and then add functions and variable definitions
void lisp_env_init_default(LispEnv* env);

// Maxwell's equations of Software. REP
LispWord lisp_read(const char* program, LispHeap* heap);
LispWord lisp_eval(LispWord word, LispEnv* env, LispHeap* heap);
void lisp_print(FILE* file, LispWord word);

#endif
