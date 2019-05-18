#ifndef LISP_H
#define LISP_H

#include <stdio.h>

#define LISP_DEBUG 0

// how much data the parser reads
// into memory at once from a file
#define LISP_FILE_CHUNK_SIZE 4096

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
    LISP_VECTOR, // homogenous array
} LispType;

typedef enum
{
    LISP_ERROR_NONE = 0,
    LISP_ERROR_FILE_OPEN,
    LISP_ERROR_PAREN_UNEXPECTED,
    LISP_ERROR_PAREN_EXPECTED,
    LISP_ERROR_DOT_UNEXPECTED,

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
    LISP_ERROR_OUT_OF_BOUNDS,

    LISP_ERROR_BAD_ARG,
} LispError;

typedef struct
{
    union LispVal
    {
        float float_val;
        int int_val;  
        void* ptr_val;
    } val;
    
    LispType type;
} Lisp; // holds all lisp values

typedef struct
{
    struct LispImpl* impl;
} LispContext;

typedef Lisp(*LispFunc)(Lisp, LispError*, LispContext);

// SETUP
// -----------------------------------------
LispContext lisp_init_lang(void);
LispContext lisp_init_lang_opt(int symbol_table_size, size_t page_size);

LispContext lisp_init_empty(void);
LispContext lisp_init_empty_opt(int symbol_table_size, size_t page_size);
void lisp_shutdown(LispContext ctx);

// garbage collection. 
// this will free all objects which are not reachable from root_to_save or the global env
Lisp lisp_collect(Lisp root_to_save, LispContext ctx);
const char* lisp_error_string(LispError error);

// LOADING
// -----------------------------------------
// reads text raw s-expressions. But does not apply any syntax expansions (equivalent to quoting the whole structure). 
// This is primarily for using Lisp as JSON/XML
// For code call expand after reading
Lisp lisp_read(const char* text, LispError* out_error, LispContext ctx);
Lisp lisp_read_file(FILE* file, LispError* out_error, LispContext ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContext ctx);

// expands Lisp syntax (For code)
Lisp lisp_expand(Lisp lisp, LispError* out_error, LispContext ctx);
// read and then expand for convenience
Lisp lisp_read_expand(const char* text, LispError* out_error, LispContext ctx);
Lisp lisp_read_expand_file(FILE* file, LispError* out_error, LispContext ctx);
Lisp lisp_read_expand_path(const char* path, LispError* out_error, LispContext ctx);

// EVALUATION
// -----------------------------------------
// evaluate a lisp expression
Lisp lisp_eval(Lisp expr, Lisp env, LispError* out_error, LispContext ctx);
// same as above but uses global environment
Lisp lisp_eval_global(Lisp expr, LispError* out_error, LispContext ctx);

// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// DATA STRUCTURES
// -----------------------------------------
#define lisp_type(x) ((x).type)
#define lisp_eq(a, b) ((a).val.ptr_val == (b).val.ptr_val)
Lisp lisp_make_null(void);
#define lisp_is_null(x) ((x).type == LISP_NULL)

Lisp lisp_make_int(int n);
int lisp_int(Lisp x);

Lisp lisp_make_float(float x);
float lisp_float(Lisp x);

Lisp lisp_make_string(const char* c_string, LispContext ctx);
char lisp_string_ref(Lisp s, int n);
void lisp_string_set(Lisp s, int n, char c);
const char* lisp_string(Lisp s);

Lisp lisp_make_symbol(const char* symbol, LispContext ctx);
const char* lisp_symbol(Lisp x);

Lisp lisp_car(Lisp p);
Lisp lisp_cdr(Lisp p);
void lisp_set_car(Lisp p, Lisp x);
void lisp_set_cdr(Lisp p, Lisp x);
Lisp lisp_cons(Lisp car, Lisp cdr, LispContext ctx);
#define lisp_is_pair(p) ((p).type == LISP_PAIR)

Lisp lisp_make_list(Lisp x, int n, LispContext ctx);
// conveniece function for cons'ing together items. arguments must be null terminated
Lisp lisp_make_listv(LispContext ctx, Lisp first, ...);
Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx); // O(n)
Lisp lisp_list_advance(Lisp l, int i); // O(n)
Lisp lisp_list_ref(Lisp l, int i); // O(n)
int lisp_list_index_of(Lisp l, Lisp x); // O(n)
int lisp_list_length(Lisp l); // O(n)
// given a list of pairs ((key1 val1) (key2 val2) ... (keyN valN))
// returns the pair with the given key or null of none
Lisp lisp_list_assoc(Lisp l, Lisp key); // O(n)
// given a list of pairs returns the value of the pair with the given key. (car (cdr (assoc ..)))
Lisp lisp_list_for_key(Lisp l, Lisp key); // O(n)
 // concise CAR/CDR combos such as CADR, CAAADR, CAAADAAR....
Lisp lisp_list_nav(Lisp p, const char* path);
// This operation modifys the list
Lisp lisp_list_reverse(Lisp l); // O(n)

Lisp lisp_make_vector(unsigned int n, Lisp x, LispContext ctx);
int lisp_vector_length(Lisp v);
Lisp lisp_vector_ref(Lisp v, unsigned int i);
void lisp_vector_set(Lisp v, unsigned int i, Lisp x);
Lisp lisp_vector_assoc(Lisp v, Lisp key); // O(n)
Lisp lisp_vector_grow(Lisp v, unsigned int n, LispContext ctx);

Lisp lisp_make_table(unsigned int capacity, LispContext ctx);
void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx);
// returns the key value pair, or null if not found
Lisp lisp_table_get(Lisp t, Lisp key, LispContext ctx);
void lisp_table_add_funcs(Lisp t, const char** names, LispFunc* funcs, LispContext ctx);

// programatically generate compound procedures
Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx);

// C functions
Lisp lisp_make_func(LispFunc func);
LispFunc lisp_func(Lisp l);

// evaluation environments
Lisp lisp_env_global(LispContext ctx);
Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx);
Lisp lisp_env_lookup(Lisp l, Lisp key, LispContext ctx);
void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx);
void lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx);

#endif
