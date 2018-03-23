/* EXAMPLE

   Lisp for scripting:
   -------------------------
   LispContextRef ctx = lisp_init(1048576); // setup lisp with 1 MB of heap
   Lisp env = lisp_make_default_env(ctx);
   Lisp program = lisp_expand(lisp_read_file(file, ctx), ctx); // load lisp structure
   lisp_eval(program, env, ctx); 
   lisp_collect(ctx, env);


   Lisp for data:
   -------------------------
   lisp_init(1048576); // setup lisp with 1 MB of heap
   Lisp data = lisp_read_file(file, ctx); // load lisp structure
   
   lisp_collect(ctx, env);
*/


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
    LISP_FUNC,   // C function
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
typedef Lisp (*LispFunc)(Lisp, LispContextRef);

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
// O(n)
Lisp lisp_at_index(Lisp list, int n);
// O(n)
int lisp_length(Lisp list);

// conveniece function for cons'ing together items. arguments must be null terminated
Lisp lisp_list(LispContextRef ctx, Lisp first, ...);

// Dictionaries (like assoc) O(n)
Lisp lisp_for_key(Lisp list, Lisp key_symbol);

// C functions
Lisp lisp_make_func(LispFunc func);

// evaluation environments
// these store variable state and function names
Lisp lisp_make_env(Lisp parent, int capacity, LispContextRef ctx);
void lisp_env_set(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx);
void lisp_env_add_funcs(Lisp env, const char** names, LispFunc* funcs, LispContextRef ctx);
Lisp lisp_make_default_env(struct LispContext* ctx);

// Maxwell's equations of Software. REP
// reads text raw s-expressions. But does not apply any syntax expansions (equivalent to quoting the whole structure). 
// This is primarily for using Lisp as JSON/XML
// For code call expand after reading
Lisp lisp_read(const char* program, LispContextRef ctx);
Lisp lisp_read_file(FILE* file, LispContextRef ctx);
Lisp lisp_read_path(const char* path, LispContextRef ctx);

// expands Lisp syntax (For code)
Lisp lisp_expand(Lisp lisp, LispContextRef ctx);

// evaluate a lisp expression
Lisp lisp_eval(Lisp expr, Lisp env, LispContextRef ctx);
// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);

// memory managment and garbage collection
LispContextRef lisp_init(unsigned int heap_size);
void lisp_shutdown(LispContextRef ctx);

// garbage collection. free up memory from unused objects. 
// this will free all objects which are not reachable from root_to_save
// usually the root environment should be this parameter
Lisp lisp_collect(Lisp root_to_save, LispContextRef ctx);

#endif
