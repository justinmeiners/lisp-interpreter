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
    LISP_ERROR_FILE_OPEN,
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

    LISP_ERROR_UNKNOWN_VAR,
    LISP_ERROR_BAD_OP,
    LISP_ERROR_UNKNOWN_EVAL,

    LISP_ERROR_BAD_ARG,
} LispError;

typedef struct
{
    union
    {
        float float_val;
        int int_val;  
        void* val;
    };
    
    LispType type;
} Lisp; // holds all lisp values

typedef struct LispContext* LispContextRef;
typedef Lisp (*LispFunc)(Lisp, LispError*, LispContextRef);

// SETUP
LispContextRef lisp_init_interpreter(void);
LispContextRef lisp_init_reader(void);

void lisp_shutdown(LispContextRef ctx);
Lisp lisp_global_env(LispContextRef ctx);

// garbage collection. 
// this will free all objects which are not reachable from root_to_save or the global env
Lisp lisp_collect(Lisp root_to_save, LispContextRef ctx);
const char* lisp_error_string(LispError error);

// LOADING
// reads text raw s-expressions. But does not apply any syntax expansions (equivalent to quoting the whole structure). 
// This is primarily for using Lisp as JSON/XML
// For code call expand after reading
Lisp lisp_read(const char* text, LispError* out_error, LispContextRef ctx);
Lisp lisp_read_file(FILE* file, LispError* out_error, LispContextRef ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContextRef ctx);

// expands Lisp syntax (For code)
Lisp lisp_expand(Lisp lisp, LispError* out_error, LispContextRef ctx);

// EVALUATION
// evaluate a lisp expression
Lisp lisp_eval(Lisp expr, Lisp env, LispError* out_error, LispContextRef ctx);

// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// DATA STRUCTURES
#define lisp_type(l) ((l).type)
#define lisp_is_null(l) ((l).type == LISP_NULL)
#define lisp_eq(a, b) ((a).val == (b).val)

Lisp lisp_null(void);
Lisp lisp_make_int(int n);
int lisp_int(Lisp l);
Lisp lisp_make_float(float x);
float lisp_float(Lisp l);
Lisp lisp_make_string(const char* c_string, LispContextRef ctx);
const char* lisp_string(Lisp l);
Lisp lisp_make_symbol(const char* symbol, LispContextRef ctx);
const char* lisp_symbol(Lisp l);

Lisp lisp_car(Lisp l);
Lisp lisp_cdr(Lisp l);
void lisp_set_car(Lisp l, Lisp x);
void lisp_set_cdr(Lisp l, Lisp x);

Lisp lisp_cons(Lisp car, Lisp cdr, LispContextRef ctx);
Lisp lisp_append(Lisp l, Lisp tail, LispContextRef ctx); // O(n)
Lisp lisp_at_index(Lisp l, int n); // O(n)
int lisp_index_of(Lisp l, Lisp x);
// more concise CAR/CDR combos such as CADR, CAAADR, CAAADAAR....
Lisp lisp_nav(Lisp l, const char* path);
int lisp_length(Lisp l); // O(n)

Lisp lisp_make_list(Lisp x, int n, LispContextRef ctx);
// conveniece function for cons'ing together items. arguments must be null terminated
Lisp lisp_make_listv(LispContextRef ctx, Lisp first, ...);
Lisp lisp_reverse_inplace(Lisp l); // O(n)

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

#endif
