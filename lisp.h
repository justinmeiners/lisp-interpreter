#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdint.h>

#define LISP_DEBUG 1

typedef enum
{
    LISP_NULL = 0,
    LISP_FLOAT,  // decimal/floating point type
    LISP_INT,    // integer type
    LISP_PAIR,   // cons pair (car, cdr)
    LISP_SYMBOL, // unquoted strings
    LISP_STRING, // quoted strings
    LISP_LAMBDA, // user defined lambda
    LISP_FUNC,   // C function
    LISP_TABLE,  // key/value storage
} LispType;

typedef enum
{
    LISP_ERROR_NONE = 0,
    LISP_ERROR_PAREN_UNEXPECTED,
    LISP_ERROR_PAREN_EXPECTED,
    LISP_ERROR_BAD_TOKEN,
    LISP_ERROR_BAD_QUOTE,
    LISP_ERROR_BAD_DEFINE,
    LISP_ERROR_BAD_SET,
    LISP_ERROR_BAD_COND,
    LISP_ERROR_BAD_AND,
    LISP_ERROR_BAD_OR,
    LISP_ERROR_BAD_LET,
    LISP_ERROR_BAD_LAMBDA,
} LispError;

typedef struct
{
    LispType type;

    union
    {
        float float_val;
        int int_val;  
        void* val;
    };
} Lisp; // holds all lisp values

typedef struct
{
    unsigned char gc_flags;
    unsigned char type;
    /* 32Kb limit for a single allocation
       the token length limit is 4k, so
       unless someone is allocating large arbitray
       buffers this shouldn't be a problem.
      If you need larger change to int. */
    unsigned short data_size;     
    char data[];
} LispBlock;

typedef struct LispContext* LispContextRef;
typedef Lisp (*LispFunc)(Lisp, LispContextRef);

// Primitive types
#define lisp_type(l) ((l).type)
#define lisp_is_null(l) ((l).type == LISP_NULL)
#define lisp_eq(a, b) ((a).val == (b).val)
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
#define lisp_set_car(l, x) (lisp_car((l)) = (x))
#define lisp_set_cdr(l, x) (lisp_cdr((l)) = (x))
Lisp lisp_cons(Lisp car, Lisp cdr, LispContextRef ctx);
Lisp lisp_append(Lisp l, Lisp tail, LispContextRef ctx);
Lisp lisp_at_index(Lisp l, int n); // O(n)
// more concise CAR/CDR combos such as CADR, CAAADR, CAAADAAR....
Lisp lisp_nav(Lisp l, const char* path);
int lisp_length(Lisp l); // O(n)
// conveniece function for cons'ing together items. arguments must be null terminated
Lisp lisp_make_list(LispContextRef ctx, Lisp first, ...);
Lisp lisp_reverse_inplace(Lisp l);

// given a list of pairs ((key1 val1) (key2 val2) ... (keyN valN)) 
// returns the pair with the given key or null of none
Lisp lisp_assoc(Lisp list, Lisp key_symbol); // O(n)
// given a list of pairs returns the value of the pair with the given key. (car (cdr (assoc ..)))
Lisp lisp_for_key(Lisp list, Lisp key_symbol); // O(n)

// if you want to progromatically generate compound procedures
Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContextRef ctx);
// C functions
Lisp lisp_make_func(LispFunc func);

// tables
Lisp lisp_make_table(unsigned int capacity, LispContextRef ctx);
void lisp_table_set(Lisp table, Lisp symbol, Lisp value, LispContextRef ctx);
// returns the key value pair, or null if not found
Lisp lisp_table_get(Lisp table, Lisp symbol, LispContextRef ctx);
void lisp_table_add_funcs(Lisp table, const char** names, LispFunc* funcs, LispContextRef ctx);

// evaluation environments
Lisp lisp_make_env(Lisp table, LispContextRef ctx);
Lisp lisp_env_extend(Lisp env, Lisp table, LispContextRef ctx);
Lisp lisp_env_lookup(Lisp env, Lisp symbol, LispContextRef ctx);
void lisp_env_define(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx);
void lisp_env_set(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx);

// Maxwell's equations of Software. REP
// reads text raw s-expressions. But does not apply any syntax expansions (equivalent to quoting the whole structure). 
// This is primarily for using Lisp as JSON/XML
// For code call expand after reading
Lisp lisp_read(const char* program, LispError* out_error, LispContextRef ctx);
Lisp lisp_read_file(FILE* file, LispError* out_error, LispContextRef ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContextRef ctx);

// expands Lisp syntax (For code)
Lisp lisp_expand(Lisp lisp, LispError* out_error, LispContextRef ctx);

// evaluate a lisp expression
Lisp lisp_eval(Lisp expr, Lisp env, LispContextRef ctx);
// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// memory managment and garbage collection
LispContextRef lisp_init_default(unsigned int heap_size);
void lisp_shutdown(LispContextRef ctx);
Lisp lisp_get_global_env(LispContextRef ctx);

// garbage collection. free up memory from unused objects. 
// this will free all objects which are not reachable from root_to_save
// usually the root environment should be this parameter
Lisp lisp_collect(Lisp root_to_save, LispContextRef ctx);

const char* lisp_error_string(LispError error);

#endif
