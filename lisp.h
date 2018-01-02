#ifndef LISP_H
#define LISP_H

#include <stdio.h>

typedef enum
{
    LISP_CELL_FLOAT,
    LISP_CELL_INT,
    LISP_CELL_SYMBOL, // unquoted strings
    LISP_CELL_STRING, // quoted strings
    LISP_CELL_PAIR,   // cons pair (car, cdr)
    LISP_CELL_LAMBDA,   // user defined lambda
    LISP_CELL_PROC, // C function
    LISP_CELL_NULL,
} LispCellType;

typedef struct
{
    short length;
} BlockHeader;

typedef struct
{
    BlockHeader header;
    char data[];
} Block;

typedef struct
{
    LispCellType type;

    union
    {
        float float_val;
        int int_val;  
        void* val;
    };
} LispCell;

#define ENTRY_KEY_MAX 128

typedef struct
{
   char key[ENTRY_KEY_MAX];
   LispCell value; 
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

// Cell utilities
#define lisp_car(cell) ( ((LispCell*)(((Block*)(cell).val)->data))[0] )
#define lisp_cdr(cell) ( ((LispCell*)(((Block*)(cell).val)->data))[1] )
LispCell lisp_cons(LispCell car, LispCell cdr);
LispCell lisp_null();

LispCell lisp_create_int(int n);
LispCell lisp_create_float(float x);
LispCell lisp_create_string(const char* string);
LispCell lisp_create_symbol(const char* string);
LispCell lisp_create_proc(LispCell (*func)(LispCell));

// evaluation environments
void lisp_env_init(LispEnv* env, LispEnv* parent, int capacity);
void lisp_env_release(LispEnv* env);
void lisp_env_set(LispEnv* env, const char* key, LispCell value);
void lisp_env_print(LispEnv* env);

// default global enviornment
// you can start your own enviornment with this
// and then add functions and variable definitions
void lisp_env_init_default(LispEnv* env);

// Maxwell's equations of Software. REP
LispCell lisp_read(const char* program);
LispCell lisp_eval(LispCell cell, LispEnv* env);
void lisp_print(FILE* file, LispCell cell);


/* TODO:
 * - eval/apply combo
 * - naming conventions
 * - cell cleanup
 * - commenting
 * - env cleanup
 * - arg checking  
 *
 */

#endif
