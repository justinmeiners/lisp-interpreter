#ifndef LISP_H
#define LISP_H

/*
 Created By: Justin Meiners (https://justinmeiners.github.io)
 License: MIT
 
Build without standard library. (subset of MIT Scheme)
The basic manipulation functions are still available in C.
#define LISP_NO_LIB 1
 
 Build without functions which give lisp access to the system,
 such as file access.
#define LISP_NO_SYSTEM_LIB 1
 
 Additional options: override how much data
 the parser reads into memory at once from a file.
 #define LISP_FILE_CHUNK_SIZE 4096
 
 */

#include <stdio.h>

#define LISP_DEBUG 0

/* how much data the parser reads
   into memory at once from a file */
#define LISP_FILE_CHUNK_SIZE 4096


typedef enum
{
    LISP_NULL = 0,
    LISP_REAL,  // decimal/floating point type
    LISP_INT,    // integer type
    LISP_CHAR,
    LISP_PAIR,   // cons pair (car, cdr)
    LISP_SYMBOL, // unquoted strings
    LISP_STRING, // quoted strings
    LISP_LAMBDA, // user defined lambda
    LISP_FUNC,   // C function
    LISP_TABLE,  // key/value storage
    LISP_VECTOR, // heterogenous array but contiguous allocation
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
        float real_val;
        int int_val;  
        void* ptr_val;
    } val;
    
    LispType type;
} Lisp; 

typedef struct
{
    struct LispImpl* impl;
} LispContext;

typedef Lisp(*LispCFunc)(Lisp, LispError*, LispContext);

/* no need to change these, just use the _opt variant */
#define LISP_DEFAULT_SYMBOL_TABLE_SIZE 512
#define LISP_DEFAULT_PAGE_SIZE 32768
#define LISP_DEFAULT_STACK_DEPTH 1024

// SETUP
// -----------------------------------------
#ifndef LISP_NO_LIB
LispContext lisp_init_lib(void);
LispContext lisp_init_lib_opt(int symbol_table_size, size_t stack_depth, size_t page_size);
#endif

LispContext lisp_init_empty(void);
LispContext lisp_init_empty_opt(int symbol_table_size, size_t stack_depth, size_t page_size);
void lisp_shutdown(LispContext ctx);

// garbage collection. 
// this will free all objects which are not reachable from root_to_save or the global env
Lisp lisp_collect(Lisp root_to_save, LispContext ctx);

// REPL
// -----------------------------------------

// reads text raw s-expressions. But does not apply any syntax expansions (equivalent to quoting the whole structure).
// This is primarily for using Lisp as JSON/XML
// For code call expand after reading
Lisp lisp_read(const char* text, LispError* out_error, LispContext ctx);
Lisp lisp_read_file(FILE* file, LispError* out_error, LispContext ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContext ctx);

// expands special Lisp forms (For code)
// The default eval will do this for you, but this can prepare statements
// that are run multiple times
Lisp lisp_expand(Lisp lisp, LispError* out_error, LispContext ctx);

// evaluate a lisp expression
Lisp lisp_eval_opt(Lisp expr, Lisp env, LispError* out_error, LispContext ctx);
// same as above but uses global environment
Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx);

// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);
const char* lisp_error_string(LispError error);

// DATA STRUCTURES
// -----------------------------------------
#define lisp_type(x) ((x).type)
#define lisp_eq(a, b) ((a).val.ptr_val == (b).val.ptr_val)
int lisp_eq_r(Lisp a, Lisp b);
Lisp lisp_make_null(void);

#define lisp_is_null(x) ((x).type == LISP_NULL)

Lisp lisp_make_int(int n);
int lisp_int(Lisp x);

Lisp lisp_make_real(float x);
float lisp_real(Lisp x);

Lisp lisp_make_string(const char* c_string, LispContext ctx);
Lisp lisp_make_empty_string(unsigned int n, char c, LispContext ctx);
char lisp_string_ref(Lisp s, int n);
void lisp_string_set(Lisp s, int n, char c);
char* lisp_string(Lisp s);

Lisp lisp_make_char(int c);
int lisp_char(Lisp l);

Lisp lisp_make_symbol(const char* symbol, LispContext ctx);
const char* lisp_symbol(Lisp x);

Lisp lisp_car(Lisp p);
Lisp lisp_cdr(Lisp p);
void lisp_set_car(Lisp p, Lisp x);
void lisp_set_cdr(Lisp p, Lisp x);
Lisp lisp_cons(Lisp car, Lisp cdr, LispContext ctx);
#define lisp_is_pair(p) ((p).type == LISP_PAIR)

Lisp lisp_list_copy(Lisp x, LispContext ctx);
Lisp lisp_make_list(Lisp x, int n, LispContext ctx);
// convenience function for cons'ing together items. arguments must be null terminated
Lisp lisp_make_listv(LispContext ctx, Lisp first, ...);
// another helpful list building technique O(1)
void lisp_fast_append(Lisp* front, Lisp* back, Lisp x, LispContext ctx);
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
Lisp lisp_vector_ref(Lisp v, int i);
void lisp_vector_set(Lisp v, int i, Lisp x);
Lisp lisp_vector_assoc(Lisp v, Lisp key); // O(n)
Lisp lisp_vector_grow(Lisp v, unsigned int n, LispContext ctx);
Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx);

Lisp lisp_make_table(unsigned int capacity, LispContext ctx);
void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx);
// returns the key value pair, or null if not found
Lisp lisp_table_get(Lisp t, Lisp key, LispContext ctx);
unsigned int lisp_table_size(Lisp t);
Lisp lisp_table_to_assoc_list(Lisp t, LispContext ctx);

/* This struct is just for making definitions a little less error prone,
   having separate arrays for names and functions leads to easy mistakes. */
typedef struct
{
    const char* name;
    LispCFunc func_ptr;
} LispFuncDef;
void lisp_table_define_funcs(Lisp t, const LispFuncDef* defs, LispContext ctx);

// programatically generate compound procedures
Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx);

// C functions
Lisp lisp_make_func(LispCFunc func_ptr);
LispCFunc lisp_func(Lisp l);

// evaluation environments
Lisp lisp_env_global(LispContext ctx);
void lisp_env_set_global(Lisp env, LispContext ctx);

Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx);
Lisp lisp_env_lookup(Lisp l, Lisp key, LispContext ctx);
void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx);
void lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx);

#endif
