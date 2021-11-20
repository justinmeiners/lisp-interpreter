/*
 Copyright 2021 Justin Meiners (https://justinmeiners.github.io)
 License: MIT

 Single header style. Do this:
     #define LISP_IMPLEMENTATION
 before you include this file in *one* C or C++ file to create the pementation.

 ----------------------
 QUICKSTART
 ----------------------

    LispContext ctx = lisp_init();

    LispError error;
    Lisp program = lisp_read("(+ 1 2)", &error, ctx);
    Lisp result = lisp_eval(program, &error, ctx);

    if (error != LISP_ERROR_NONE)
    {
        lisp_print(result);
    }
    else
    {
        // ...
    }

    lisp_shutdown(ctx);

 ----------------------
 OPTIONS
 ----------------------

 These macros can be defined before inclusion to change options.

     #define LISP_DEBUG

 Build in debug mode with logging.
  
     #define LISP_NO_LIB

 Do not include the standard library.
 This reduces the amount of code if you just care about loading s-expressions 
 or want to heavily customize the language.

     #define LISP_NO_SYSTEM_LIB

 Build only with *safe* functions so the lisp environment cannot access
 system resources.

    #define LISP_FILE_CHUNK_SIZE 4096

 Change how much data is read from a file at a time.


 
 */


#ifndef LISP_H
#define LISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

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
    LISP_BOOL,
    LISP_VECTOR, // heterogenous array but contiguous allocation
    LISP_INTERNAL
} LispType;

typedef double LispReal;
typedef long long LispInt;

typedef union
{
    int char_val;
    LispReal real_val;
    LispInt int_val;  
    void* ptr_val;
} LispVal;

typedef struct
{
    LispVal val;
    LispType type;
} Lisp; 

typedef enum
{
    LISP_ERROR_NONE = 0,
    LISP_ERROR_FILE_OPEN,
    LISP_ERROR_PAREN_UNEXPECTED,
    LISP_ERROR_PAREN_EXPECTED,
    LISP_ERROR_HASH_UNEXPECTED,
    LISP_ERROR_DOT_UNEXPECTED,
    LISP_ERROR_BAD_TOKEN,
    
    LISP_ERROR_FORM_SYNTAX,
    LISP_ERROR_BAD_DEFINE,
    LISP_ERROR_BAD_MACRO,
    LISP_ERROR_BAD_LAMBDA,
    LISP_ERROR_MACRO_NO_EVAL,

    LISP_ERROR_UNKNOWN_VAR,
    LISP_ERROR_BAD_OP,
    LISP_ERROR_UNKNOWN_EVAL,
    LISP_ERROR_OUT_OF_BOUNDS,

    LISP_ERROR_SPLICE,
    LISP_ERROR_BAD_ARG,
    LISP_ERROR_RUNTIME,
} LispError;

typedef struct
{
    struct LispImpl* p;
} LispContext;

typedef Lisp(*LispCFunc)(Lisp, LispError*, LispContext);

// -----------------------------------------
// SETUP
// -----------------------------------------

// You can use these if you would like to set the _opt params.
#define LISP_DEFAULT_SYMBOL_TABLE_SIZE 512
#define LISP_DEFAULT_PAGE_SIZE 131072
#define LISP_DEFAULT_STACK_DEPTH 1024

#ifndef LISP_NO_LIB
LispContext lisp_init(void);
LispContext lisp_init_opt(int symbol_table_size, size_t stack_depth, size_t page_size, FILE* out_file);
#endif

LispContext lisp_init_empty(void);
LispContext lisp_init_empty_opt(int symbol_table_size, size_t stack_depth, size_t page_size, FILE* out_file);
void lisp_shutdown(LispContext ctx);

// garbage collection. 
// this will free all objects which are not reachable from root_to_save or the global env
Lisp lisp_collect(Lisp root_to_save, LispContext ctx);
void lisp_print_collect_stats(LispContext ctx);

// -----------------------------------------
// REPL
// -----------------------------------------

// Reads text into raw s-expressions. 
Lisp lisp_read(const char* text, LispError* out_error, LispContext ctx);
Lisp lisp_read_file(FILE* file, LispError* out_error, LispContext ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContext ctx);

// Expands special Lisp forms and checks syntax.
// The default eval will do this for you, but this can prepare statements that are run multiple times.
Lisp lisp_macroexpand(Lisp lisp, LispError* out_error, LispContext ctx);

// evaluate a lisp expression
Lisp lisp_eval_opt(Lisp expr, Lisp env, LispError* out_error, LispContext ctx);
// same as above but uses global environment
Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx);

// print out a lisp structure
void lisp_print(Lisp l);
void lisp_printf(FILE* file, Lisp l);
const char* lisp_error_string(LispError error);

// -----------------------------------------
// PRIMITIVES
// -----------------------------------------
#define lisp_type(x) ((x).type)
#define lisp_eq(a, b) ((a).val.ptr_val == (b).val.ptr_val)
int lisp_equal(Lisp a, Lisp b);
int lisp_equal_r(Lisp a, Lisp b);
Lisp lisp_make_null(void);

#define lisp_is_null(x) ((x).type == LISP_NULL)

// Pairs
Lisp lisp_car(Lisp p);
Lisp lisp_cdr(Lisp p);
void lisp_set_car(Lisp p, Lisp x);
void lisp_set_cdr(Lisp p, Lisp x);
Lisp lisp_cons(Lisp car, Lisp cdr, LispContext ctx);
#define lisp_is_pair(p) ((p).type == LISP_PAIR)


// Numbers
Lisp lisp_make_int(LispInt n);
LispInt lisp_int(Lisp x);

Lisp lisp_make_real(LispReal x);
LispReal lisp_real(Lisp x);

// Bools
Lisp lisp_make_bool(int t);
int lisp_bool(Lisp x);
#define lisp_true() (lisp_make_bool(1))
#define lisp_false() (lisp_make_bool(0))
int lisp_is_true(Lisp x);

// Strings
Lisp lisp_make_string(const char* c_string, LispContext ctx);
Lisp lisp_make_empty_string(unsigned int n, char c, LispContext ctx);
char lisp_string_ref(Lisp s, int n);
void lisp_string_set(Lisp s, int n, char c);
char* lisp_string(Lisp s);

Lisp lisp_make_char(int c);
int lisp_char(Lisp l);

// Symbols
// Pass NULL to generate a symbol
Lisp lisp_make_symbol(const char* symbol, LispContext ctx);
const char* lisp_symbol_string(Lisp x);

// -----------------------------------------
// DATA STRUCTURES
// -----------------------------------------

// Lists
Lisp lisp_list_copy(Lisp x, LispContext ctx);
Lisp lisp_make_list(Lisp x, int n, LispContext ctx);
// convenience function for cons'ing together items. arguments must be terminated with end of list entry.
Lisp lisp_make_listv(LispContext ctx, Lisp first, ...);
Lisp lisp_make_terminate();

// This operation modifiess the list
Lisp lisp_list_reverse(Lisp l, Lisp tail); // O(n)
Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx); // O(n)
// another helpful list building technique O(1)
void lisp_fast_append(Lisp* front, Lisp* back, Lisp x, LispContext ctx);
Lisp lisp_list_advance(Lisp l, int i); // O(n)
Lisp lisp_list_ref(Lisp l, int i); // O(n)
int lisp_list_index_of(Lisp l, Lisp x); // O(n)
int lisp_list_length(Lisp l); // O(n)
// given a list of pairs ((key1 val1) (key2 val2) ... (keyN valN))
// returns the pair with the given key or null of none
Lisp lisp_list_assoc(Lisp l, Lisp key); // O(n)

Lisp lisp_list_assq(Lisp l, Lisp key); // O(n)

// given a list of pairs returns the value of the pair with the given key. (car (cdr (assoc ..)))
Lisp lisp_list_for_key(Lisp l, Lisp key); // O(n)
 // concise CAR/CDR combos such as CADR, CAAADR, CAAADAAR....
Lisp lisp_list_accessor_mnemonic(Lisp p, const char* path);


// Vectors (heterogeneous)
Lisp lisp_make_vector(int n, Lisp x, LispContext ctx);
int lisp_vector_length(Lisp v);
Lisp lisp_vector_ref(Lisp v, int i);
void lisp_vector_set(Lisp v, int i, Lisp x);
void lisp_vector_fill(Lisp v, Lisp x);
Lisp lisp_vector_assq(Lisp v, Lisp key); // O(n)
Lisp lisp_vector_grow(Lisp v, int n, LispContext ctx);
Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx);

// Hash tables
Lisp lisp_make_table(unsigned int capacity, LispContext ctx);
void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx);
// returns the key value pair, or null if not found
Lisp lisp_table_get(Lisp t, Lisp key, LispContext ctx);
unsigned int lisp_table_size(Lisp t);
Lisp lisp_table_to_assoc_list(Lisp t, LispContext ctx);

// -----------------------------------------
// LANGUAGE
// -----------------------------------------

// compound procedures
Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx);
Lisp lisp_lambda_env(Lisp l);

// C functions
Lisp lisp_make_func(LispCFunc func_ptr);
LispCFunc lisp_func(Lisp l);
// This struct and function are a convenience for defining many C functions at a time. 
typedef struct
{
    const char* name;
    LispCFunc func_ptr;
} LispFuncDef;
void lisp_table_define_funcs(Lisp t, const LispFuncDef* defs, LispContext ctx);

// Evaluation environments
Lisp lisp_env_global(LispContext ctx);
void lisp_env_set_global(Lisp env, LispContext ctx);

Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx);
Lisp lisp_env_lookup(Lisp l, Lisp key, LispContext ctx);
void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx);
void lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx);

// Macros
Lisp lisp_macro_table(LispContext ctx);

#ifdef __cplusplus
}
#endif

#endif


#ifdef LISP_IMPLEMENTATION

/*

  References:
 Looks just like how mine ended up :) - http://piumarta.com/software/lysp/
  
  Eval - http://norvig.com/lispy.html
  Enviornments - https://mitpress.mit.edu/sicp/full-text/book/book-Z-H-21.html#%_sec_3.2
  Cons Representation - http://www.more-magic.net/posts/internals-data-representation.html
  GC - http://www.more-magic.net/posts/internals-gc.html
  http://home.pipeline.com/~hbaker1/CheneyMTA.html

  Enviornments as first-class objects - https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_14.html

  Reference counting symbol table? - http://sandbox.mc.edu/~bennet/cs404/ex/lisprcnt.html
*/

#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <time.h>

#ifndef LISP_FILE_CHUNK_SIZE
#define LISP_FILE_CHUNK_SIZE 4096
#endif


#define SCRATCH_MAX 1024

enum
{
    GC_CLEAR = 0,
    GC_MOVED = (1 << 0), // has this block been moved to the to-space?
    GC_VISITED = (1 << 1), // has this block's pointers been moved?
};

typedef struct Page
{
    struct Page* next;
    size_t size;
    size_t capacity;
    char buffer[];
} Page;

static Page* page_create(size_t capacity)
{
    Page* page = malloc(sizeof(Page) + capacity);
    page->capacity = capacity;
    page->size = 0;
    page->next = NULL;
    return page;
}

void page_destroy(Page* page) { free(page); }

typedef struct
{
    Page* bottom;
    Page* top;
    size_t size;
    size_t page_count;
    size_t page_size;
} Heap;

typedef struct Block
{   
    // Be careful with alignment and padding!
    // 32 or 64
    union
    {
        struct Block* forward;
        size_t size;
    } info;
    
    // 32
    union
    {
        struct
        {
            uint8_t car_type;
            uint8_t cdr_type;
        } pair;

        struct
        {
            uint32_t hash;
        } symbol;

        struct
        {
            int length;
        } vector;
    } d;

    // 32 or 64
    uint8_t gc_flags;
    uint8_t type;
} Block;

static void heap_init(Heap* heap, size_t page_size)
{
    heap->page_size = page_size;
    heap->bottom = page_create(page_size);
    heap->top = heap->bottom;
    
    heap->size = 0;
    heap->page_count = 1;
}

static void heap_shutdown(Heap* heap)
{
    Page* page = heap->bottom;
    while (page)
    {
        Page* next = page->next;
        page_destroy(page);
        page = next;
    }
    heap->bottom = NULL;
}

static size_t align_to_bytes(size_t n, size_t k)
{
    // https://stackoverflow.com/questions/29925524/how-do-i-round-to-the-next-32-bit-alignment
    return ((n + k - 1) / k) * k;
}

static void* heap_alloc(size_t alloc_size, LispType type, Heap* heap)
{
    assert(alloc_size > 0);

    // allocations should be aligned so that pointers to blocks are aligned. 
    // This will add a little bit of extra padding to strings and symbols.
    alloc_size = align_to_bytes(alloc_size, sizeof(LispVal));
    assert(alloc_size % sizeof(LispVal) == 0);

    Page* to_use;
    if (alloc_size >= heap->page_size)
    {
        /* add to bottom of stack.
         As soon as this page is made it it is full and can't be used.
         However, our current page may still have room.
         */
        to_use = page_create(alloc_size);
        to_use->next = heap->bottom;
        heap->bottom = to_use;
        ++heap->page_count;
    }
    else if (alloc_size + heap->top->size > heap->top->capacity)
    {
        /* add to top of the stack.
         need a new page because ours is full */
        to_use = page_create(heap->page_size);
        heap->top->next = to_use;
        heap->top = heap->top->next;
        ++heap->page_count;
    }
    else
    {
        /* use the current page on top */
        to_use = heap->top;
    }
    
    void* address = to_use->buffer + to_use->size;
    to_use->size += alloc_size;
    heap->size += alloc_size;

    Block* block = address;
    block->gc_flags = GC_CLEAR;
    block->info.size = alloc_size;
    block->type = type;
    return address;
}

enum {
    SYM_IF = 0,
    SYM_BEGIN,
    SYM_QUOTE,
    SYM_QUASI_QUOTE,
    SYM_UNQUOTE,
    SYM_UNQUOTE_SPLICE,
    SYM_DEFINE,
    SYM_DEFINE_MACRO,
    SYM_SET,
    SYM_LAMBDA,
    SYM_COUNT
};

struct LispImpl
{
    FILE* out_file;
    Heap heap;
    Heap to_heap;

    Lisp* stack;
    size_t stack_ptr;
    size_t stack_depth;

    Lisp symbol_table;
    Lisp global_env;
    Lisp macros;

    int lambda_counter;
    int symbol_counter;

    Lisp symbol_cache[SYM_COUNT];

    size_t gc_stat_freed;
    size_t gc_stat_time;
};

#define get_sym(_sym, _ctx) ((_ctx).p->symbol_cache[(_sym)])

static void* gc_alloc(size_t size, LispType type, LispContext ctx)
{
    return heap_alloc(size, type, &ctx.p->heap);
}

typedef struct
{
    Block block;
    LispVal car;
    LispVal cdr;
} Pair;

typedef struct
{
    Block block;
    char string[];
} String;

typedef struct
{
    Block block;
    char string[];
} Symbol;

typedef struct
{
    Block block;
    LispVal entries[];
} Vector;

// hash table
// linked list chaining
typedef struct
{
    Block block;
    unsigned int size;
    unsigned int capacity;
    LispVal entries[];
} Table;

static Lisp _table_ref(const Table* table, int i)
{
    LispVal val = table->entries[i];
    Lisp existing = { val, val.ptr_val == NULL ? LISP_NULL : LISP_PAIR };
    return existing;
}

Lisp lisp_make_null()
{
    Lisp l;
    l.type = LISP_NULL;
    l.val.ptr_val = NULL;
    return l;
}

int lisp_equal(Lisp a, Lisp b)
{
    if (a.type != b.type)
    {
        return 0;
    }

    switch (a.type)
    {
        case LISP_NULL:
            return 1;
        case LISP_BOOL:
            return lisp_bool(a) == lisp_bool(b);
        case LISP_CHAR:
            return lisp_char(a) == lisp_char(b);
        case LISP_INT:
            return lisp_int(a) == lisp_int(b);
        case LISP_REAL:
            return lisp_real(a) == lisp_real(b);
        default:
            return a.val.ptr_val == b.val.ptr_val;
    }
}

int lisp_equal_r(Lisp a, Lisp b)
{
    if (a.type != b.type)
    {
        return 0;
    }

    switch (a.type)
    {
        case LISP_VECTOR:
        {
            int n = lisp_vector_length(a);
            int m = lisp_vector_length(b);
            if (n != m) return 0;
            
            for (int i = 0; i < n; ++i)
            {
                if (!lisp_equal_r(lisp_vector_ref(a, i), lisp_vector_ref(b, i)))
                    return 0;
            }
            
            return 1;
        }
        case LISP_PAIR:
        {
            while (lisp_is_pair(a) && lisp_is_pair(b))
            {
                if (!lisp_equal_r(lisp_car(a), lisp_car(b)))
                    return 0;
                
                a = lisp_cdr(a);
                b = lisp_cdr(b);
            }
            
            return lisp_equal_r(a, b);
        }
        default:
            return lisp_equal(a, b);
    }
}

static Table* lisp_table(Lisp t)
{
    assert(lisp_type(t) == LISP_TABLE);
    return t.val.ptr_val;
}

Lisp lisp_make_int(LispInt n)
{
    Lisp l;
    l.type = LISP_INT;
    l.val.int_val = n;
    return l;
}

LispInt lisp_int(Lisp x)
{
    if (x.type == LISP_REAL)
        return (LispInt)x.val.real_val;
    return x.val.int_val;
}

Lisp lisp_make_bool(int t)
{
    Lisp l;
    l.type = LISP_BOOL;
    l.val.char_val = t;
    return l;
}

int lisp_bool(Lisp x)
{
    return x.val.char_val;
}

int lisp_is_true(Lisp x)
{
     // In scheme everything which is not #f is true. 
     return (lisp_type(x) == LISP_BOOL && !lisp_bool(x)) ? 0 : 1;
}

Lisp lisp_make_real(LispReal x)
{
    Lisp l = lisp_make_null();
    l.type = LISP_REAL;
    l.val.real_val = x;
    return l;
}

LispReal lisp_real(Lisp x)
{
    if (x.type == LISP_INT)
        return(LispReal)x.val.int_val;
    return x.val.real_val;
}

Lisp lisp_car(Lisp p)
{
    assert(p.type == LISP_PAIR);
    const Pair* pair = p.val.ptr_val;
    Lisp x = { pair->car, (LispType)pair->block.d.pair.car_type };
    return x;
}

Lisp lisp_cdr(Lisp p)
{
    assert(p.type == LISP_PAIR);
    const Pair* pair = p.val.ptr_val;
    Lisp x = { pair->cdr, (LispType)pair->block.d.pair.cdr_type };
    return x;
}

void lisp_set_car(Lisp p, Lisp x)
{
    assert(p.type == LISP_PAIR);
    Pair* pair = p.val.ptr_val;
    pair->car = x.val;
    pair->block.d.pair.car_type = x.type;
}

void lisp_set_cdr(Lisp p, Lisp x)
{
    assert(p.type == LISP_PAIR);
    Pair* pair = p.val.ptr_val;
    pair->cdr = x.val;
    pair->block.d.pair.cdr_type = x.type;
}

Lisp lisp_cons(Lisp car, Lisp cdr, LispContext ctx)
{
    Pair* pair = gc_alloc(sizeof(Pair), LISP_PAIR, ctx);
    pair->car = car.val;
    pair->cdr = cdr.val;
    pair->block.d.pair.car_type = car.type;
    pair->block.d.pair.cdr_type = cdr.type;
    Lisp p;
    p.type = pair->block.type;
    p.val.ptr_val = pair;
    return p;
}

void lisp_fast_append(Lisp* front, Lisp* back, Lisp x, LispContext ctx)
{
    Lisp new_l = lisp_cons(x, lisp_make_null(), ctx);
    if (lisp_is_null(*back))
    {
        *back = new_l;
        *front = *back;
    }
    else
    {
        lisp_set_cdr(*back, new_l);
        *back = new_l;
    }
}

Lisp lisp_list_copy(Lisp x, LispContext ctx)
{
    Lisp front = lisp_make_null();
    Lisp back = lisp_make_null();
    
    while (lisp_is_pair(x))
    {
        lisp_fast_append(&front, &back, lisp_car(x), ctx);
        x = lisp_cdr(x);
    }
    return front;
}

Lisp lisp_make_list(Lisp x, int n, LispContext ctx)
{
    Lisp front = lisp_make_null();
    Lisp back = front;

    for (int i = 0; i < n; ++i)
        lisp_fast_append(&front, &back, x, ctx);
    return front;
}

Lisp lisp_make_terminate()
{
    Lisp l;
    l.type = LISP_INTERNAL;
    l.val.int_val = 0;
    return l;
}

static int is_end_of_list(Lisp l)
{
    return lisp_type(l) == LISP_INTERNAL && lisp_int(l) == 0;
}

Lisp lisp_make_listv(LispContext ctx, Lisp first, ...)
{
    Lisp front = lisp_cons(first, lisp_make_null(), ctx);
    Lisp back = front;

    va_list args;
    va_start(args, first);

    Lisp it = lisp_make_null();

    while (1)
    {
        it = va_arg(args, Lisp);
        if (is_end_of_list(it)) break;

        lisp_fast_append(&front, &back, it, ctx);
    }

    va_end(args);

    return front;
}

Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx)
{
    // (a b) (c) -> (a b c)
    l = lisp_list_reverse(lisp_list_copy(l, ctx), lisp_make_null());
    return lisp_list_reverse(l, tail);
}

Lisp lisp_list_advance(Lisp l, int i)
{
    while (i > 0)
    {
        if (!lisp_is_pair(l)) return l;
        l = lisp_cdr(l);
        --i;
    }
    return l;
}

Lisp lisp_list_ref(Lisp l, int n)
{
    l = lisp_list_advance(l, n);
    if (lisp_is_pair(l)) return lisp_car(l);
    return lisp_make_null();
}

int lisp_list_index_of(Lisp l, Lisp x)
{
    int i = 0;
    while (lisp_is_pair(l))
    {
        if (lisp_eq(lisp_car(l), x)) return i;
        ++i;
        l = lisp_cdr(l);
    }
    return -1;
}

int lisp_list_length(Lisp l)
{
    int count = 0;
    while (lisp_is_pair(l))
    {
        ++count;
        l = lisp_cdr(l);
    }
    return count;
}

Lisp lisp_list_assoc(Lisp l, Lisp key)
{
    while (lisp_is_pair(l))
    {
        Lisp pair = lisp_car(l);
        if (lisp_is_pair(pair) && lisp_equal_r(lisp_car(pair), key))
        {
            return pair;
        }

        l = lisp_cdr(l);
    }
    return lisp_make_null();
}

Lisp lisp_list_assq(Lisp l, Lisp key)
{
    while (lisp_is_pair(l))
    {
        Lisp pair = lisp_car(l);
        if (lisp_is_pair(pair) && lisp_eq(lisp_car(pair), key))
        {
            return pair;
        }

        l = lisp_cdr(l);
    }
    return lisp_make_null();
}

Lisp lisp_list_for_key(Lisp l, Lisp key)
{
    Lisp pair = lisp_list_assq(l, key);
    Lisp x = lisp_cdr(pair);

    if (lisp_is_pair(x))
    {
        return lisp_car(x);
    }
    else
    {
        return x;
    }
}

Lisp lisp_list_accessor_mnemonic(Lisp p, const char* c)
{
    if (toupper(*c) != 'C') return lisp_make_null();

    ++c;
    int i = 0;
    while (toupper(*c) != 'R' && *c)
    {
        ++i;
        ++c;
    }

    if (toupper(*c) != 'R') return lisp_make_null();
    --c;

    while (i > 0)
    {
        if (toupper(*c) == 'D')
        {
            p = lisp_cdr(p);
        }
        else if (toupper(*c) == 'A')
        {
            p = lisp_car(p);
        }
        else
        {
            return lisp_make_null();
        }
        --c;
        --i;
    }

    return p;
}

Lisp lisp_list_reverse(Lisp l, Lisp tail)
{
    while (lisp_is_pair(l))
    {
        Lisp next = lisp_cdr(l);
        lisp_set_cdr(l, tail);
        tail = l;
        l = next;        
    }
    return tail;
}

static int _vector_len(const Vector* v) { return v->block.d.vector.length; }
// https://gist.github.com/jibsen/da6be27cde4d526ee564
static char* _vector_types(Vector* v) {
    char* base = (char*)v;
    return base + sizeof(Vector) + sizeof(LispVal) * _vector_len(v);
};

Lisp lisp_make_vector(int n, Lisp x, LispContext ctx)
{
    Vector* vector = gc_alloc(sizeof(Vector) + sizeof(LispVal) * n + sizeof(char) * n, LISP_VECTOR, ctx);
    vector->block.d.vector.length = n;
    char* entry_types = _vector_types(vector);
    for (int i = 0; i < n; ++i)
    {
        vector->entries[i] = x.val;
        entry_types[i] = (char)x.type;
    }

    Lisp l;
    l.type = LISP_VECTOR;
    l.val.ptr_val = vector;
    return l;
}

static Vector* lisp_vector(Lisp v)
{
    assert(lisp_type(v) == LISP_VECTOR);
    return v.val.ptr_val;
}

int lisp_vector_length(Lisp v)
{
    return _vector_len(lisp_vector(v));
}

Lisp lisp_vector_ref(Lisp v, int i)
{
    Vector* vector = lisp_vector(v);
    assert(i < _vector_len(vector));
    Lisp x = { vector->entries[i], (LispType)(_vector_types(vector)[i]) };
    return x;
}

void lisp_vector_set(Lisp v, int i, Lisp x)
{
    Vector* vector = lisp_vector(v);
    assert(i < _vector_len(vector));
    vector->entries[i] = x.val;
    _vector_types(vector)[i] = (char)x.type;
}
void lisp_vector_fill(Lisp v, Lisp x)
{
    Vector* vector = lisp_vector(v);
    int n = _vector_len(vector);

    char* entry_types = _vector_types(vector);
    for (int i = 0; i < n; ++i)
    {
        vector->entries[i] = x.val;
        entry_types[i] = (char)x.type;
    }
}

Lisp lisp_vector_assq(Lisp v, Lisp key)
{
    int n = lisp_vector_length(v);
    for (int i = 0; i < n; ++i)
    {
        Lisp pair = lisp_vector_ref(v, i); 
        if (lisp_eq(lisp_car(pair), key))
        {
            return pair;
        }
    }
    return lisp_make_null();
}

Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx)
{
    assert(start <= end);
    
    Vector* src = old.val.ptr_val;
    int m = _vector_len(src);
    if (end > m) end = m;
    
    int n = end - start;
    Lisp new_v = lisp_make_vector(n, lisp_make_int(0), ctx);
    Vector* dst = lisp_vector(new_v);
    memcpy(dst->entries, src->entries, sizeof(LispVal) * n);
    memcpy(_vector_types(dst), _vector_types(src), sizeof(char) * n);
    return new_v;
}

Lisp lisp_vector_grow(Lisp v, int n, LispContext ctx)
{
    Vector* src = lisp_vector(v);
    int m = _vector_len(src);
    assert(n >= m);

    if (n == m)
    {
        return v;
    }
    else
    {
        Lisp new_v = lisp_make_vector(n, lisp_vector_ref(v, 0), ctx);
        Vector* dst = lisp_vector(new_v);
        memcpy(dst->entries, src->entries, sizeof(LispVal) * m);
        memcpy(_vector_types(dst), _vector_types(src), sizeof(char) * m);
        return new_v;
    }
}

Lisp lisp_make_empty_string(unsigned int n, char c, LispContext ctx)
{
    String* string = gc_alloc(sizeof(String) + n + 1, LISP_STRING, ctx);
    memset(string->string, c, n);
    string->string[n] = '\0';
    
    Lisp l;
    l.type = string->block.type;
    l.val.ptr_val = string;
    return l;
}

Lisp lisp_make_string(const char* c_string, LispContext ctx)
{
    size_t length = strlen(c_string) + 1;
    String* string = gc_alloc(sizeof(String) + length, LISP_STRING, ctx);
    memcpy(string->string, c_string, length);
    
    Lisp l;
    l.type = string->block.type;
    l.val.ptr_val = string;
    return l;
}

static String* get_string(Lisp s)
{
    assert(s.type == LISP_STRING);
    return s.val.ptr_val;
}

char* lisp_string(Lisp s)
{
    return get_string(s)->string;
}

char lisp_string_ref(Lisp s, int n)
{
    const char* str = lisp_string(s);
    return str[n];
}

void lisp_string_set(Lisp s, int n, char c)
{
    String* string = get_string(s);
    string->string[n] = c;
}

const char* lisp_symbol_string(Lisp l)
{
    assert(l.type == LISP_SYMBOL);
    Symbol* symbol = l.val.ptr_val;
    return symbol->string;
}

Lisp lisp_make_char(int c)
{
    Lisp l;
    l.type = LISP_CHAR;
    l.val.int_val = c;
    return l;
}

int lisp_char(Lisp l)
{
    return lisp_int(l);
}

static unsigned int symbol_hash(Lisp l)
{
    assert(l.type == LISP_SYMBOL);
    Symbol* symbol = l.val.ptr_val;
    return symbol->block.d.symbol.hash;
}

// TODO
#if defined(WIN32) || defined(WIN64)
#define strncasecmp _stricmp
#endif

static Lisp table_get_string(Lisp l, const char* string, unsigned int hash)
{
    Table* table = lisp_table(l);
    unsigned int i = hash % table->capacity;

    Lisp it = _table_ref(table, i);

    while (!lisp_is_null(it))
    {
        Lisp pair = lisp_car(it);
        Lisp symbol = lisp_car(pair);

        // TODO: max?
        if (strncasecmp(lisp_symbol_string(symbol), string, 2048) == 0)
        {
            return pair;
        }
        it = lisp_cdr(it);
    }
    return lisp_make_null();
}

static uint32_t hash_string(const char* c)
{     
    // adler 32
    // https://en.wikipedia.org/wiki/Adler-32
    uint32_t s1 = 1;
    uint32_t s2 = 0;

    while (*c)
    {
        s1 = (s1 + toupper(*c)) % 65521;
        s2 = (s2 + s1) % 65521;
        ++c; 
    } 
    return (s2 << 16) | s1;
}

Lisp lisp_make_symbol(const char* string, LispContext ctx)
{
    char scratch[SCRATCH_MAX];
    if (!string)
    {
        snprintf(scratch, SCRATCH_MAX, ":G%d", ctx.p->symbol_counter++);
        string = scratch;
    }

    uint32_t hash = hash_string(string);
    Lisp pair = table_get_string(ctx.p->symbol_table, string, (unsigned int)hash);
    
    if (lisp_is_null(pair))
    {
        size_t string_length = strlen(string) + 1;
        // allocate a new block
        Symbol* symbol = gc_alloc(sizeof(Symbol) + string_length, LISP_SYMBOL, ctx);
        symbol->block.d.symbol.hash = hash;

        memcpy(symbol->string, string, string_length);

        // always convert symbols to uppercase
        char* c = symbol->string;
        while (*c)
        {
            *c = toupper(*c);
            ++c;
        }

        Lisp l;
        l.type = symbol->block.type;
        l.val.ptr_val = symbol;
        lisp_table_set(ctx.p->symbol_table, l, lisp_make_null(), ctx);
        return l;
    }
    else
    {
        return lisp_car(pair);
    }
}

Lisp lisp_make_func(LispCFunc func)
{
    Lisp l;
    l.type = LISP_FUNC;
    l.val.ptr_val = func;
    return l;
}

LispCFunc lisp_func(Lisp l)
{
    assert(lisp_type(l) == LISP_FUNC);
    return l.val.ptr_val;
}

typedef struct
{
    Block block;
    int identifier;
    Lisp args;
    Lisp body;
    Lisp env;
} Lambda;

Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx)
{
    Lambda* lambda = gc_alloc(sizeof(Lambda), LISP_LAMBDA, ctx);
    lambda->identifier = ctx.p->lambda_counter++;
    lambda->args = args;
    lambda->body = body;
    lambda->env = env;
    
    Lisp l;
    l.type = lambda->block.type;
    l.val.ptr_val = lambda;
    return l;
}

static Lambda* lisp_lambda(Lisp l)
{
    assert(l.type == LISP_LAMBDA);
    return l.val.ptr_val;
}

Lisp lisp_lambda_env(Lisp l)
{
    Lambda* lambda = lisp_lambda(l);
    return lambda->env;
}

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_DOT,
    TOKEN_QUOTE,
    TOKEN_BQUOTE,
    TOKEN_COMMA,
    TOKEN_AT,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_BOOL,
    TOKEN_HASH_L_PAREN,
} TokenType;

/* for debug
static const char* token_type_name[] = {
    "NONE", "L_PAREN", "R_PAREN", "#", ".", "QUOTE", "SYMBOL", "STRING", "INT", "FLOAT",
}; */

typedef struct
{
    FILE* file;

    const char* sc; // start of token
    const char* c;  // scanner
    size_t scan_length;
    TokenType token;

    int sc_buff_index;
    int c_buff_index; 

    char* buffs[2];
    int buff_number[2];
    size_t buff_size;
} Lexer;

static void lexer_shutdown(Lexer* lex)
{
    if (lex->file)
    {
        free(lex->buffs[0]);
        free(lex->buffs[1]);
    }
}

static void lexer_init(Lexer* lex, const char* program)
{
    lex->file = NULL;
    lex->sc_buff_index = 0;
    lex->c_buff_index = 0; 
    lex->buffs[0] = (char*)program;
    lex->buffs[1] = NULL;
    lex->buff_number[0] = 0;
    lex->buff_number[1] = -1;
    lex->sc = lex->c = lex->buffs[0];
    lex->scan_length = 0;
}

static void lexer_init_file(Lexer* lex, FILE* file)
{
    lex->file = file;

    lex->buff_size = LISP_FILE_CHUNK_SIZE;

     // LispReal input buffering
    lex->sc_buff_index = 0;
    lex->c_buff_index = 0;

    lex->buffs[0] = malloc(lex->buff_size + 1);
    lex->buffs[1] = malloc(lex->buff_size + 1);
    lex->buffs[0][lex->buff_size] = '\0';
    lex->buffs[1][lex->buff_size] = '\0';

    // sc the pointers out
    lex->sc = lex->c = lex->buffs[0];
    lex->scan_length = 0;

    // read a new block
    size_t read = fread(lex->buffs[0], 1, lex->buff_size, lex->file);
    lex->buffs[0][read] = '\0';

    lex->buff_number[0] = 0;
    lex->buff_number[1] = -1;
}

static void lexer_advance_start(Lexer* lex)
{
    lex->sc = lex->c;
    lex->sc_buff_index = lex->c_buff_index;
    lex->scan_length = 0;
}

static void lexer_restart_scan(Lexer* lex)
{
    lex->c = lex->sc;
    lex->c_buff_index = lex->sc_buff_index;
    lex->scan_length = 0;
}

static int lexer_step(Lexer* lex)
{
    ++lex->c;
    ++lex->scan_length;

    if (*lex->c == '\0')
    { 
        if (lex->file)
        {
            // flip the buffer
            int previous_index = lex->c_buff_index;
            int new_index = !previous_index;

            if (new_index == lex->sc_buff_index)
            {
                fprintf(stderr, "token too long\n");
                return 0;
            }
 
            // next block is older. so read a new one
            if (lex->buff_number[new_index] < lex->buff_number[previous_index])
            {
                if (!feof(lex->file))
                {
                    size_t read = fread(lex->buffs[new_index], 1, lex->buff_size, lex->file);
                    lex->buff_number[new_index] = lex->buff_number[previous_index] + 1;
                    lex->buffs[new_index][read] = '\0';
                }
                else
                {
                    return 0;
                }
            }

            lex->c_buff_index = new_index;
            lex->c = lex->buffs[new_index];
        }  
        else
        {
            return 0;
        }
    }

    return 1;
}

static void lexer_skip_empty(Lexer* lex)
{
    while (lex->c)
    {
        // skip whitespace
        while (isspace(*lex->c)) lexer_step(lex);

        // skip comments to end of line
        if (*lex->c == ';')
        {
            while (*lex->c && *lex->c != '\n') lexer_step(lex);
        }
        else
        {
            break;
        }
    }
}

static int lexer_match_char(Lexer* lex)
{
    lexer_restart_scan(lex);
    if (*lex->c != '#') return 0;
    lexer_step(lex);
    if (*lex->c != '\\') return 0;
    lexer_step(lex);

    if (isalnum(*lex->c))
    {
        lexer_step(lex);
        while (isalnum(*lex->c)) lexer_step(lex);
    }
    else if (isprint(*lex->c))
    {
        lexer_step(lex);
    }
    return 1;
}

static int lexer_match_bool(Lexer* lex)
{
    lexer_restart_scan(lex);
    if (*lex->c != '#') return 0;
    lexer_step(lex);
    if (*lex->c != 't' && *lex->c != 'f') return 0;
    lexer_step(lex);
    return 1;
}

static int lexer_match_hash_paren(Lexer* lex)
{
    lexer_restart_scan(lex);
    if (*lex->c != '#') return 0;
    lexer_step(lex);
    if (*lex->c != '(') return 0;
    lexer_step(lex);
    return 1;
}

static int lexer_match_int(Lexer* lex)
{
    lexer_restart_scan(lex);
    
    // need at least one digit or -
    if (!isdigit(*lex->c))
    {
        if (*lex->c == '-' || *lex->c == '+')
        {
            lexer_step(lex);
            if (!isdigit(*lex->c)) return 0;
        }
        else
        {
            return 0;
        }
    }

    lexer_step(lex);
    // + any other digits
    while (isdigit(*lex->c)) lexer_step(lex);
    return 1;
}

static int lexer_match_real(Lexer* lex)
{
    lexer_restart_scan(lex);
    
    // need at least one digit or -
    if (!isdigit(*lex->c))
    {
        if (*lex->c == '-' || *lex->c == '+')
        {
            lexer_step(lex);
            if (!isdigit(*lex->c)) return 0;
        }
        else
        {
            return 0;
        }
    }
    lexer_step(lex);

    int found_decimal = 0;
    while (isdigit(*lex->c) || *lex->c == '.')
    {
        if (*lex->c == '.') found_decimal = 1;
        lexer_step(lex);
    }

    // must have a decimal to be a float
    return found_decimal;
}

static int is_symbol(char c)
{
    if (c < '!' || c > 'z') return 0;
    const char* illegal= "()#;";
    do
    {
        if (c == *illegal) return 0;
        ++illegal;
    } while (*illegal);
    return 1;
}

static int lexer_match_symbol(Lexer* lex)
{   
    lexer_restart_scan(lex);
    // need at least one valid symbol character
    if (!is_symbol(*lex->c)) return 0;
    lexer_step(lex);
    while (is_symbol(*lex->c)) lexer_step(lex);
    return 1;
}

static int lexer_match_string(Lexer* lex)
{
    lexer_restart_scan(lex);
    // start with quote
    if (*lex->c != '"') return 0;
    lexer_step(lex);

    while (*lex->c != '"')
    {
        if (*lex->c == '\0' || *lex->c == '\n')
            return 0;
        lexer_step(lex);
    }

    lexer_step(lex);
    return 1;    
}

static void lexer_copy_token(Lexer* lex, size_t start_index, size_t length, char* dest)
{
    size_t token_length = lex->scan_length;
    assert((start_index + length) <= token_length);

    if (lex->c_buff_index == lex->sc_buff_index)
    {
        // both pointers are in the same buffer
        memcpy(dest, lex->sc + start_index, length);
    }
    else
    {
        // the pointers are split across buffers. So do two copies.
        const char* sc_end = lex->buffs[lex->sc_buff_index] + lex->buff_size;
        const char* sc = (lex->sc + start_index);
        
        ptrdiff_t first_part_length = sc_end - sc;  
        if (first_part_length < 0) first_part_length = 0;
        if (first_part_length > length) first_part_length = length;
        
        if (first_part_length > 0)
        {
            // copy from sc to the end of its buffer (or to length)
            memcpy(dest, sc, first_part_length);
        }
        
        ptrdiff_t last_part = length - first_part_length;        
        if (last_part > 0)
        {
            // copy from the start of c's buffer
            memcpy(dest + first_part_length, lex->buffs[lex->c_buff_index], length - first_part_length);
        }
    }
}

static TokenType token_from_char(char c)
{
    switch (c)
    {
        case '\0':
            return TOKEN_NONE;
        case '(':
            return TOKEN_L_PAREN;
        case ')':
            return TOKEN_R_PAREN;
        case '.':
            return TOKEN_DOT;
        case '\'':
            return TOKEN_QUOTE;
        case '`':
            return TOKEN_BQUOTE;
        case ',':
            return TOKEN_COMMA;
        case '@':
            return TOKEN_AT;
        default:
            return TOKEN_NONE;
    }
}

static void lexer_next_token(Lexer* lex)
{
    lexer_skip_empty(lex);
    lexer_advance_start(lex);

    lex->token = token_from_char(*lex->c);
    if (lex->token != TOKEN_NONE)
    {
        lexer_step(lex);
    }
    else
    {
        if (lexer_match_string(lex))
        {
            lex->token = TOKEN_STRING;
        }
        else if (lexer_match_real(lex))
        {
            lex->token = TOKEN_FLOAT;
        }
        else if (lexer_match_int(lex))
        {
            lex->token = TOKEN_INT;
        }
        else if (lexer_match_symbol(lex))
        {
            lex->token = TOKEN_SYMBOL;
        }
        else if (lexer_match_char(lex))
        {
            lex->token = TOKEN_CHAR;
        }
        else if (lexer_match_bool(lex))
        {
            lex->token = TOKEN_BOOL;
        }
        else if (lexer_match_hash_paren(lex))
        {
            lex->token = TOKEN_HASH_L_PAREN;
        }
    }
}

static Lisp parse_atom(Lexer* lex, jmp_buf error_jmp,  LispContext ctx)
{ 
    char scratch[SCRATCH_MAX];
    size_t length = lex->scan_length;
    Lisp l = lisp_make_null();

    switch (lex->token)
    {
        case TOKEN_INT:
        {
            lexer_copy_token(lex, 0, length, scratch);
            scratch[length] = '\0';
            l = lisp_make_int(atoi(scratch));
            break;
        }
        case TOKEN_FLOAT:
        {
            lexer_copy_token(lex, 0, length, scratch);
            scratch[length] = '\0'; 
            l = lisp_make_real(atof(scratch));
            break;
        }
        case TOKEN_STRING:
        {
            // -2 length to skip quotes
            String* string = gc_alloc(sizeof(String) + length - 1, LISP_STRING, ctx);
            lexer_copy_token(lex, 1, length - 2, string->string);
            string->string[length - 2] = '\0';
            
            l.type = string->block.type;
            l.val.ptr_val = string;
            break;
        }
        case TOKEN_SYMBOL:
        {
            // always convert symbols to uppercase
            lexer_copy_token(lex, 0, length, scratch);
            scratch[length] = '\0';
            l = lisp_make_symbol(scratch, ctx);
            break;
        }
        default: 
            longjmp(error_jmp, LISP_ERROR_BAD_TOKEN);
    }
    
    lexer_next_token(lex);
    return l;
}

static const char* ascii_char_name_table[] =
{
    "NUL", "SOH", "STX", "ETX", "EOT",
    "ENQ", "ACK", "BEL", "backspace", "tab",
    "newline", "VT", "page", "return", "SO",
    "SI", "DLE", "DC1", "DC2", "DC3",
    "DC4", "NAK", "SYN", "ETB", "CAN",
    "EM", "SUB", "altmode", "FS", "GS", "RS",
    "backnext", "space", NULL
};

static int parse_char_token(Lexer* lex)
{
    char scratch[SCRATCH_MAX];
    size_t length = lex->scan_length - 2;
    lexer_copy_token(lex, 2, length, scratch);
    
    if (length == 1)
    {
        return (int)scratch[0];
    }
    else
    {
        scratch[length] = '\0';
        const char** name_it = ascii_char_name_table;
        
        int c = 0;
        while (*name_it)
        {
            if (strcmp(*name_it, scratch) == 0)
            {
                return c;
            }
            ++name_it;
            ++c;
        }
        return -1;
    }
}

// read tokens and construct S-expresions
static Lisp parse_list_r(Lexer* lex, jmp_buf error_jmp, LispContext ctx)
{  
    int quote_type = SYM_QUOTE;
    switch (lex->token)
    {
        case TOKEN_NONE:
            longjmp(error_jmp, LISP_ERROR_PAREN_EXPECTED);
        case TOKEN_DOT:
            longjmp(error_jmp, LISP_ERROR_DOT_UNEXPECTED);
        case TOKEN_L_PAREN:
        {
            Lisp front = lisp_make_null();
            Lisp back = lisp_make_null();

            // (
            lexer_next_token(lex);
            while (lex->token != TOKEN_R_PAREN && lex->token != TOKEN_DOT)
            {
                Lisp x = parse_list_r(lex, error_jmp, ctx);
                lisp_fast_append(&front, &back, x, ctx);
            }

            // A dot at the end of a list assigns the cdr
            if (lex->token == TOKEN_DOT)
            {
                if (lisp_is_null(back)) longjmp(error_jmp, LISP_ERROR_DOT_UNEXPECTED);

                lexer_next_token(lex);
                if (lex->token != TOKEN_R_PAREN)
                {
                    Lisp x = parse_list_r(lex, error_jmp, ctx);
                    lisp_set_cdr(back, x);
                }
            }

            if (lex->token != TOKEN_R_PAREN) longjmp(error_jmp, LISP_ERROR_PAREN_EXPECTED);

            // )
            lexer_next_token(lex);
            return front;
        }
        case TOKEN_R_PAREN:
            longjmp(error_jmp, LISP_ERROR_PAREN_UNEXPECTED);
        case TOKEN_CHAR:
        {
            int c = parse_char_token(lex);
            lexer_next_token(lex);
            return lisp_make_char(c);
        }
        case TOKEN_BOOL:
        {
            char c;
            lexer_copy_token(lex, 1, 1, &c);
            lexer_next_token(lex);
            return lisp_make_bool(c == 't' ? 1 : 0);
        }
        case TOKEN_HASH_L_PAREN:
        {
            // #(
            lexer_next_token(lex);

            Lisp v = lisp_make_null();
            int count = 0;
            while (lex->token != TOKEN_R_PAREN)
            {
                Lisp x = parse_list_r(lex, error_jmp, ctx);

                if (lisp_is_null(v))
                {
                    v = lisp_make_vector(16, x, ctx);
                }
                else
                {
                    int length = lisp_vector_length(v);
                    if (count + 1 >= length)
                    {
                        v = lisp_vector_grow(v, length * 2, ctx);
                    }

                    lisp_vector_set(v, count, x);
                }
                ++count;
            }
            // )
            lexer_next_token(lex);
            return lisp_subvector(v, 0, count, ctx);
        }
        case TOKEN_COMMA:
            lexer_next_token(lex);

            if (lex->token == TOKEN_AT)
            {
                quote_type = SYM_UNQUOTE_SPLICE;
                lexer_next_token(lex);
            }
            else
            {
                quote_type = SYM_UNQUOTE;
            }
            goto quote;
        case TOKEN_BQUOTE:
            quote_type = SYM_QUASI_QUOTE;
            lexer_next_token(lex);
            goto quote;
        case TOKEN_QUOTE:
            lexer_next_token(lex);
            goto quote;
        quote:
        {
             // '
             Lisp l = lisp_cons(parse_list_r(lex, error_jmp, ctx), lisp_make_null(), ctx);
             return lisp_cons(get_sym(quote_type, ctx), l, ctx);
        }
        default:
        {
            return parse_atom(lex, error_jmp, ctx);
        } 
    }
}

static Lisp parse(Lexer* lex, LispError* out_error, LispContext ctx)
{
    jmp_buf error_jmp;
    LispError error = setjmp(error_jmp);

    if (error != LISP_ERROR_NONE)
    {
        if (out_error) *out_error = error;
        return lisp_make_null();
    }

    lexer_next_token(lex);
    Lisp result = parse_list_r(lex, error_jmp, ctx);
    
    if (lex->token != TOKEN_NONE)
    {
        Lisp back = lisp_cons(result, lisp_make_null(), ctx);
        Lisp front = lisp_cons(get_sym(SYM_BEGIN, ctx), back, ctx);
        
        while (lex->token != TOKEN_NONE)
        {
            Lisp next_result = parse_list_r(lex, error_jmp, ctx);
            lisp_fast_append(&front, &back, next_result, ctx);
        } 

       result = front;
    }

    if (out_error) *out_error = error;
    return result;
}

Lisp lisp_read(const char* program, LispError* out_error, LispContext ctx)
{
    Lexer lex;
    lexer_init(&lex, program);
    Lisp l = parse(&lex, out_error, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_file(FILE* file, LispError* out_error, LispContext ctx)
{
    Lexer lex;
    lexer_init_file(&lex, file);
    Lisp l = parse(&lex, out_error, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_path(const char* path, LispError* out_error, LispContext ctx)
{
    FILE* file = fopen(path, "r");

    if (!file)
    {
        *out_error = LISP_ERROR_FILE_OPEN;
        return lisp_make_null();
    }

    Lisp l = lisp_read_file(file, out_error, ctx);
    fclose(file);
    return l;
}

Lisp lisp_make_table(unsigned int capacity, LispContext ctx)
{
    size_t size = sizeof(Table) + sizeof(LispVal) * capacity;
    Table* table = gc_alloc(size, LISP_TABLE, ctx);
    table->size = 0;
    table->capacity = capacity;

    // clear the table
    memset(table->entries, 0, sizeof(LispVal) * capacity);
    Lisp l;
    l.type = table->block.type;
    l.val.ptr_val = table;
    return l;
}

void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx)
{
    Table* table = lisp_table(t);
    unsigned int i = symbol_hash(key) % table->capacity;

    Lisp existing = _table_ref(table, i);
    Lisp pair = lisp_list_assq(existing, key);

    if (lisp_is_null(pair))
    {
        // new value. prepend to front of chain
        pair = lisp_cons(key, x, ctx);
        table->entries[i] = lisp_cons(pair, existing, ctx).val;
        ++table->size;
    }
    else
    {
        // reassign cdr value (key, val)
        lisp_set_cdr(pair, x);
    }
}

Lisp lisp_table_get(Lisp t, Lisp symbol, LispContext ctx)
{
    const Table* table = lisp_table(t);
    unsigned int i = symbol_hash(symbol) % table->capacity;
    return lisp_list_assq(_table_ref(table, i), symbol);
}

Lisp lisp_table_to_assoc_list(Lisp t, LispContext ctx)
{
    const Table* table = lisp_table(t);
    Lisp result = lisp_make_null();
    
    for (int i = 0; i < table->capacity; ++i)
    {
        Lisp it = _table_ref(table, i);
        while (!lisp_is_null(it))
        {
            result = lisp_cons(lisp_car(it), result, ctx);
            it = lisp_cdr(it);
        }
    }
    return result;
}

unsigned int lisp_table_size(Lisp t)
{
    const Table* table = lisp_table(t);
    return table->size;
}

void lisp_table_define_funcs(Lisp t, const LispFuncDef* defs, LispContext ctx)
{
    while (defs->name)
    {
        lisp_table_set(t, lisp_make_symbol(defs->name, ctx), lisp_make_func(defs->func_ptr), ctx);
        ++defs;
    }
}

Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx)
{
    return lisp_cons(table, l, ctx);
}

Lisp lisp_env_lookup(Lisp l, Lisp key, LispContext ctx)
{
    while (lisp_is_pair(l))
    {
        Lisp x = lisp_table_get(lisp_car(l), key, ctx);
        if (!lisp_is_null(x)) return x;
        l = lisp_cdr(l);
    }
    
    return lisp_make_null();
}

void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx)
{
    lisp_table_set(lisp_car(l), key, x, ctx);
}

void lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx)
{
    Lisp pair = lisp_env_lookup(l, key, ctx);
    if (lisp_is_null(pair))
    {
        fprintf(stderr, "error: unknown variable: %s\n", lisp_symbol_string(key));
    }
    lisp_set_cdr(pair, x);
}

static void lisp_print_r(FILE* file, Lisp l, int is_cdr)
{
    switch (lisp_type(l))
    {
        case LISP_INT:
            fprintf(file, "%lli", lisp_int(l));
            break;
        case LISP_BOOL:
            fprintf(file, "#%c", lisp_bool(l) == 0 ? 'f' : 't');
            break;
        case LISP_REAL:
            fprintf(file, "%f", lisp_real(l));
            break;
        case LISP_NULL:
            fprintf(file, "NIL");
            break;
        case LISP_SYMBOL:
            fprintf(file, "%s", lisp_symbol_string(l));
            break;
        case LISP_STRING:
            fprintf(file, "\"%s\"", lisp_string(l));
            break;
        case LISP_CHAR:
        {
            int c = lisp_int(l);
            
            if (c >= 0 && c < 33)
            {
                fprintf(file, "#\\%s", ascii_char_name_table[c]);
            }
            else if (isprint(c))
            {
                fprintf(file, "#\\%c", (char)c);
            }
            else
            {
                fprintf(file, "#\\+%d", c);
            }
            break;
        }
        case LISP_LAMBDA:
            fprintf(file, "lambda-%i", lisp_lambda(l)->identifier);
            break;
        case LISP_FUNC:
            fprintf(file, "c-func-%p", lisp_func(l));
            break;
        case LISP_TABLE:
        {
            const Table* table = lisp_table(l);
            fprintf(file, "{");
            for (int i = 0; i < table->capacity; ++i)
            {
                Lisp entry = _table_ref(table, i);
                if (lisp_is_null(entry)) continue;
                lisp_print_r(file, entry, 0);
                fprintf(file, " ");
            }
            fprintf(file, "}");
            break;
        }
        case LISP_VECTOR:
        {
            fprintf(file, "#(");
            int N = lisp_vector_length(l);
            for (int i = 0; i < N; ++i)
            {
                lisp_print_r(file, lisp_vector_ref(l, i), 0);
                if (i + 1 < N)
                {
                    fprintf(file, " ");
                }
            }
            fprintf(file, ")");
            break;
        }
        case LISP_PAIR:
        {
            if (!is_cdr) fprintf(file, "(");
            lisp_print_r(file, lisp_car(l), 0);

            if (lisp_type(lisp_cdr(l)) != LISP_PAIR)
            {
                if (!lisp_is_null(lisp_cdr(l)))
                { 
                    fprintf(file, " . ");
                    lisp_print_r(file, lisp_cdr(l), 0);
                }

                fprintf(file, ")");
            }
            else
            {
                fprintf(file, " ");
                lisp_print_r(file, lisp_cdr(l), 1);
            } 
            break;
        }
        default:
            fprintf(stderr, "printing unknown lisp type\n");
            
            assert(0);
            break;
    }
}

void lisp_printf(FILE* file, Lisp l) { lisp_print_r(file, l, 0);  }
void lisp_print(Lisp l) {  lisp_printf(stdout, l); }


static void lisp_stack_push(Lisp x, LispContext ctx)
{
#ifdef LISP_DEBUG
    if (ctx.p->stack_ptr + 1 >= ctx.p->stack_depth)
    {
        fprintf(stderr, "stack overflow\n");
    }
#endif 

    ctx.p->stack[ctx.p->stack_ptr] = x;
    ++ctx.p->stack_ptr;
}

static Lisp lisp_stack_pop(LispContext ctx)
{
    ctx.p->stack_ptr--;
    
#ifdef LISP_DEBUG
    if (ctx.p->stack_ptr < 0)
    {
        fprintf(stderr, "stack underflow\n");
    }
#endif
    return ctx.p->stack[ctx.p->stack_ptr];
}

static Lisp* lisp_stack_peek(size_t i, LispContext ctx)
{
    return ctx.p->stack + (ctx.p->stack_ptr - i);
}

// returns whether the result is final, or needs to be eval'd.
static int apply(Lisp operator, Lisp args, Lisp* out_result, Lisp* out_env, LispError* error, LispContext ctx)
{
    switch (lisp_type(operator))
    {
        case LISP_LAMBDA: // lambda call (compound procedure)
        {
            const Lambda* lambda = lisp_lambda(operator);
            
            if (lisp_is_null(lambda->args))
            {
                *out_env = lambda->env;
            }
            else
            {
                // make a new environment
                Lisp new_table = lisp_make_table(13, ctx);
                
                // bind parameters to arguments
                // to pass into function call
                Lisp keyIt = lambda->args;
                Lisp valIt = args;

                while (lisp_is_pair(keyIt))
                {
                    lisp_table_set(new_table, lisp_car(keyIt), lisp_car(valIt), ctx);
                    keyIt = lisp_cdr(keyIt);
                    valIt = lisp_cdr(valIt);
                }
                
                if (lisp_type(keyIt) == LISP_SYMBOL)
                {
                    // variable length arguments
                    lisp_table_set(new_table, keyIt, valIt, ctx);
                }
                
                // extend the environment
                *out_env = lisp_env_extend(lambda->env, new_table, ctx);
            }
            
            // normally we would eval the body here
            // but while will eval
            *out_result = lambda->body;
            return 1;
        }
        case LISP_FUNC: // call into C functions
        {
            // no environment required
            LispCFunc f = lisp_func(operator);
            *out_result = f(args, error, ctx);
            return 0;
        }
        default:
        {
            lisp_printf(stderr, operator);
            fprintf(stderr, " is not an operator.\n");

            *error = LISP_ERROR_BAD_OP;
            return 0;
        }
    }
}

static Lisp eval_r(jmp_buf error_jmp, LispContext ctx)
{
    Lisp* env = lisp_stack_peek(2, ctx);
    Lisp* x = lisp_stack_peek(1, ctx);
    
    while (1)
    {
        assert(!lisp_is_null(*env));
        
        switch (lisp_type(*x))
        {
            case LISP_INT:
            case LISP_BOOL:
            case LISP_REAL:
            case LISP_CHAR:
            case LISP_STRING:
            case LISP_LAMBDA:
            case LISP_VECTOR:
            case LISP_NULL:
                return *x; // atom
            case LISP_SYMBOL: // variable reference
            {
                Lisp pair = lisp_env_lookup(*env, *x, ctx);
                
                if (lisp_is_null(pair))
                {
                    fprintf(stderr, "%s is not defined.\n", lisp_symbol_string(*x));
                    longjmp(error_jmp, LISP_ERROR_UNKNOWN_VAR); 
                    return lisp_make_null();
                }
                return lisp_cdr(pair);
            }
            case LISP_PAIR:
            {
                Lisp op_sym = lisp_car(*x);
                int op_valid = lisp_type(op_sym) == LISP_SYMBOL;

                if (lisp_eq(op_sym, get_sym(SYM_IF, ctx)) && op_valid) 
                {
                    // if conditional statemetns
                    Lisp predicate = lisp_list_ref(*x, 1);
                    
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(predicate, ctx);
 
                    if (lisp_is_true(eval_r(error_jmp, ctx)))
                    {
                        // consequence
                        *x = lisp_list_ref(*x, 2);
                        // while will eval
                    }
                    else
                    {
                        // alternative
                        *x = lisp_list_ref(*x, 3);
                        // while will eval
                    }
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                }
                else if (lisp_eq(op_sym, get_sym(SYM_BEGIN, ctx)) && op_valid)
                {
                    Lisp it = lisp_cdr(*x);
                    if (lisp_is_null(it)) return it;
                    
                    // eval all but last
                    while (lisp_is_pair(lisp_cdr(it)))
                    {
                        // save next thing
                        lisp_stack_push(lisp_cdr(it), ctx);
                        
                        lisp_stack_push(*env, ctx);
                        lisp_stack_push(lisp_car(it), ctx);
                        
                        eval_r(error_jmp, ctx);
        
                        lisp_stack_pop(ctx);
                        lisp_stack_pop(ctx);
                        
                        it = lisp_stack_pop(ctx);
                        //it = lisp_cdr(it);
                    }
                    
                    *x = lisp_car(it);
                    // while will eval last
                }
                else if (lisp_eq(op_sym, get_sym(SYM_QUOTE, ctx)) && op_valid)
                {
                    return lisp_list_ref(*x, 1);
                }
                else if (lisp_eq(op_sym, get_sym(SYM_DEFINE, ctx))) 
                {
                    // variable definitions
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(lisp_list_ref(*x, 2), ctx);
                    
                    Lisp value = eval_r(error_jmp, ctx);
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    Lisp symbol = lisp_list_ref(*x, 1);
                    lisp_env_define(*env, symbol, value, ctx);
                    return lisp_make_null();
                }
                else if (lisp_eq(op_sym, get_sym(SYM_SET, ctx)) && op_valid)
                {
                    // mutablity
                    // like def, but requires existence
                    // and will search up the environment chain
                    
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(lisp_list_ref(*x, 2), ctx);
                    
                    Lisp value = eval_r(error_jmp, ctx);
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    Lisp symbol = lisp_list_ref(*x, 1);
                    lisp_env_set(*env, symbol, value, ctx);
                    return lisp_make_null();
                }
                else if (lisp_eq(op_sym, get_sym(SYM_LAMBDA, ctx)) && op_valid) 
                {
                    // lambda defintions (compound procedures)
                    Lisp args = lisp_list_ref(*x, 1);
                    Lisp body = lisp_list_ref(*x, 2);
                    return lisp_make_lambda(args, body, *env, ctx);
                }
                else 
                {
                    // operator application
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(lisp_car(*x), ctx);
                    
                    Lisp operator = eval_r(error_jmp, ctx);
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    lisp_stack_push(operator, ctx);
                    
                    Lisp arg_expr = lisp_cdr(*x);
                    
                    Lisp args_front = lisp_make_null();
                    Lisp args_back = lisp_make_null();
                    
                    while (lisp_is_pair(arg_expr))
                    {
                        // save next
                        lisp_stack_push(lisp_cdr(arg_expr), ctx);
                        lisp_stack_push(args_back, ctx);
                        lisp_stack_push(args_front, ctx);

                        lisp_stack_push(*env, ctx);
                        lisp_stack_push(lisp_car(arg_expr), ctx);
                        Lisp new_arg = eval_r(error_jmp, ctx);
                        lisp_stack_pop(ctx);
                        lisp_stack_pop(ctx);

                        args_front = lisp_stack_pop(ctx);
                        args_back = lisp_stack_pop(ctx);
                        
                        lisp_fast_append(&args_front, &args_back, new_arg, ctx);
                        
                        arg_expr = lisp_stack_pop(ctx);
                    }
                    
                    operator = lisp_stack_pop(ctx);
                    
                    LispError error = LISP_ERROR_NONE;
                    int needs_to_eval = apply(operator, args_front, x, env, &error, ctx);
                    if (error != LISP_ERROR_NONE) longjmp(error_jmp, error);

                    if (!needs_to_eval)
                    {
                        return *x;
                    }
                    // Otherwise while will eval
                }
                break;
            }
            default:
                longjmp(error_jmp, LISP_ERROR_UNKNOWN_EVAL);
        }
    }
}

static Lisp expand_quasi_r(Lisp l, jmp_buf error_jmp, LispContext ctx)
{
    if (lisp_type(l) != LISP_PAIR)
    {
        return lisp_make_listv(
                ctx,
                get_sym(SYM_QUOTE, ctx),
                l,
                lisp_make_terminate());
    }

    Lisp op = lisp_car(l);
    int op_valid = lisp_type(op) == LISP_SYMBOL;

    if (lisp_eq(op, get_sym(SYM_UNQUOTE, ctx)) && op_valid)
    {
        return lisp_car(lisp_cdr(l));
    }
    else if (lisp_eq(op, get_sym(SYM_UNQUOTE_SPLICE, ctx)) && op_valid)
    {
        longjmp(error_jmp, LISP_ERROR_SPLICE);
    }
    else
    {
        return lisp_make_listv(
                ctx,
                lisp_make_symbol("CONS", ctx),
                expand_quasi_r(lisp_car(l), error_jmp, ctx),
                expand_quasi_r(lisp_cdr(l), error_jmp, ctx),
                lisp_make_terminate());
    }
}

static Lisp expand_r(Lisp l, jmp_buf error_jmp, LispContext ctx)
{
    if (lisp_type(l) != LISP_PAIR) return l;

    // 1. expand extended syntax into primitive syntax
    // 2. perform optimizations
    // 3. check syntax    

    Lisp op = lisp_car(l);
    int op_valid = lisp_type(op) == LISP_SYMBOL;

    if (lisp_eq(op, get_sym(SYM_QUOTE, ctx)) && op_valid)
    {
        // don't expand quotes
        if (lisp_list_length(l) != 2)
        {
            fprintf(stderr, "(quote x)\n");
            longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
        }
        return l;
    }
    else if (lisp_eq(op, get_sym(SYM_QUASI_QUOTE, ctx)) && op_valid)
    {
        return expand_quasi_r(lisp_car(lisp_cdr(l)), error_jmp, ctx);
    }
    else if (lisp_eq(op, get_sym(SYM_DEFINE_MACRO, ctx)) && op_valid)
    {
        if (lisp_list_length(l) != 3)
        {
            fprintf(stderr, "(define-macro name proc)\n");
            longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
        } 

        Lisp symbol = lisp_list_ref(l, 1);
        Lisp body = lisp_list_ref(l, 2);

        LispError e;
        Lisp lambda = lisp_eval(body, &e, ctx);

        if (e != LISP_ERROR_NONE)
        {
            longjmp(error_jmp, e);
        }
        if (lisp_type(lambda) != LISP_LAMBDA)
        {
            fprintf(stderr, "(define-macro name proc) not a procedure\n");
            longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
        } 

        lisp_table_set(ctx.p->macros, symbol, lambda, ctx);
        return lisp_make_null();
    }
    else if (lisp_eq(op, get_sym(SYM_DEFINE, ctx)) && op_valid)
    {
        int length = lisp_list_length(l);

        Lisp rest = lisp_cdr(l);
        Lisp signature = lisp_car(rest);

        switch (lisp_type(signature))
        {
            case LISP_PAIR:
                {
                    // (define (<name> <arg0> ... <argn>) <body0> ... <bodyN>)
                    // -> (define <name> (lambda (<arg0> ... <argn>) <body> ... <bodyN>))

                    if (length < 3) longjmp(error_jmp, LISP_ERROR_BAD_DEFINE);
                    Lisp name = lisp_car(signature);

                    if (lisp_type(name) != LISP_SYMBOL) longjmp(error_jmp, LISP_ERROR_BAD_DEFINE); 

                    Lisp args = lisp_cdr(signature);
                    Lisp lambda = lisp_cdr(rest); // start with body
                    lambda = lisp_cons(args, lambda, ctx);
                    lambda = lisp_cons(get_sym(SYM_LAMBDA, ctx), lambda, ctx);

                    lisp_set_cdr(l, lisp_make_listv(ctx,
                                name,
                                expand_r(lambda, error_jmp, ctx),
                                lisp_make_terminate()));
                    return l;
                }
            case LISP_SYMBOL:
                {
                    if (length != 3) longjmp(error_jmp, LISP_ERROR_BAD_DEFINE); 
                    lisp_set_cdr(rest, expand_r(lisp_cdr(rest), error_jmp, ctx));
                    return l;
                }
            default:
                longjmp(error_jmp, LISP_ERROR_BAD_DEFINE);
                break;
        }
    }
    else if (lisp_eq(op, get_sym(SYM_SET, ctx)) && op_valid)
    {
        if (lisp_list_length(l) != 3) {
            fprintf(stderr, "(set! symbol x)\n");
            lisp_printf(stderr, l);
            longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
        }

        Lisp var = lisp_list_ref(l, 1);
        if (lisp_type(var) != LISP_SYMBOL) {
            fprintf(stderr, "(set! symbol x) not a symbol\n");
            longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
        }
        // continue with expansion
    }
    else if (lisp_eq(op, get_sym(SYM_LAMBDA, ctx)) && op_valid)
    {
        // (LAMBDA (<var0> ... <varN>) <expr0> ... <exprN>)
        // (LAMBDA (<var0> ... <varN>) (BEGIN <expr0> ... <expr1>)) 
        if (lisp_list_length(l) > 3)
        {
            Lisp body_exprs = expand_r(lisp_list_advance(l, 2), error_jmp, ctx); 
            Lisp begin = lisp_cons(get_sym(SYM_BEGIN, ctx), body_exprs, ctx);

            Lisp vars = lisp_list_ref(l, 1);
            if (!lisp_is_pair(vars) && !lisp_is_null(vars)) longjmp(error_jmp, LISP_ERROR_BAD_LAMBDA);

            Lisp lambda = lisp_cons(begin, lisp_make_null(), ctx);
            lambda = lisp_cons(vars, lambda, ctx);
            lambda = lisp_cons(lisp_car(l), lambda, ctx);
            return lambda;
        }
        else
        {
            Lisp body = lisp_list_advance(l, 2);
            lisp_set_cdr(lisp_cdr(l), expand_r(body, error_jmp, ctx));
            return l;
        }
    }
    else if (op_valid)
    {
        Lisp entry = lisp_table_get(ctx.p->macros, op, ctx);

        if (!lisp_is_null(entry))
        {
            // EXPAND MACRO
            Lisp proc = lisp_cdr(entry);

            // TODO: need to make sure collection is not triggered
            // while evaling a macro.

            Lisp result;
            Lisp calling_env;
            LispError error = LISP_ERROR_NONE;
            if (apply(proc, lisp_cdr(l), &result, &calling_env, &error, ctx) == 1)
            {
                result = lisp_eval_opt(result, calling_env, &error, ctx);
            }

            if (error != LISP_ERROR_NONE) longjmp(error_jmp, error);
            return expand_r(result, error_jmp, ctx);
        }
    }

    // list
    Lisp it = l;
    while (lisp_is_pair(it))
    {
        lisp_set_car(it, expand_r(lisp_car(it), error_jmp, ctx));
        it = lisp_cdr(it);
    }
    return l;

}

Lisp lisp_macroexpand(Lisp lisp, LispError* out_error, LispContext ctx)
{
    jmp_buf error_jmp;
    LispError error = setjmp(error_jmp);

    if (error == LISP_ERROR_NONE)
    {
        Lisp result = expand_r(lisp, error_jmp, ctx);
        *out_error = error;
        return result;
    }
    else
    {
        *out_error = error;
        return lisp_make_null();
    }
}

Lisp lisp_eval_opt(Lisp l, Lisp env, LispError* out_error, LispContext ctx)
{
    LispError error;
    Lisp expanded = lisp_macroexpand(l, &error, ctx);
    
    if (error != LISP_ERROR_NONE)
    {
        if (out_error) *out_error = error;
        return lisp_make_null();
    }
    
    size_t save_stack = ctx.p->stack_ptr;
    
    jmp_buf error_jmp;
    error = setjmp(error_jmp);

    if (error == LISP_ERROR_NONE)
    {
        lisp_stack_push(env, ctx);
        lisp_stack_push(expanded, ctx);
        
        Lisp result = eval_r(error_jmp, ctx);
        
        lisp_stack_pop(ctx);
        lisp_stack_pop(ctx);

        if (out_error)
        {
            *out_error = error;
        }

        return result; 
    }
    else
    {
        if (out_error)
        {
            ctx.p->stack_ptr = save_stack;
            *out_error = error;
        }

        return lisp_make_null();
    }
}

Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx)
{
    return lisp_eval_opt(expr, lisp_env_global(ctx), out_error, ctx);
}

static Lisp gc_move(Lisp l, Heap* to);

static LispVal gc_move_val(LispVal val, LispType type, Heap* to)
{
    switch (type)
    {
        case LISP_PAIR:
        case LISP_SYMBOL:
        case LISP_STRING:
        case LISP_LAMBDA:
        case LISP_VECTOR:
        {
            Block* block = val.ptr_val;
            if (!(block->gc_flags & GC_MOVED))
            {
                // copy the data to new block
                Block* dest = heap_alloc(block->info.size, block->type, to);
                memcpy(dest, block, block->info.size);
                dest->gc_flags = GC_CLEAR;
                
                // save forwarding address (offset in to)
                block->info.forward = dest;
                block->gc_flags = GC_MOVED;
            }
            
            // return the moved block address
            val.ptr_val = block->info.forward;
            return val;
        }
        case LISP_TABLE:
        {
            Table* table = val.ptr_val;
            if (!(table->block.gc_flags & GC_MOVED))
            {
                float load_factor = table->size / (float)table->capacity;
                unsigned int new_capacity = table->capacity;
                
                /// TODO: research these numbers
                if (load_factor > 0.75f || load_factor <= 0.05f)
                {
                    if (table->size > 8)
                    {
                        new_capacity = (table->size * 3) - 1;
                    }
                    else
                    {
                        new_capacity = 13;
                    }
                }

                size_t new_size = sizeof(Table) + new_capacity * sizeof(LispVal);
                Table* dest_table = heap_alloc(new_size, LISP_TABLE, to);
                dest_table->size = table->size;
                dest_table->capacity = new_capacity;
                
                // save forwarding address (offset in to)
                table->block.info.forward = &dest_table->block;
                table->block.gc_flags = GC_MOVED;
                
                // clear the table
                memset(dest_table->entries, 0, new_capacity * sizeof(LispVal));

#ifdef LISP_DEBUG
                if (new_capacity != table->capacity)
                {
                    printf("resizing table %i -> %i\n", table->capacity, new_capacity);
                }
#endif
                
                for (unsigned int i = 0; i < table->capacity; ++i)
                {
                    Lisp it = _table_ref(table, i);
                    
                    while (!lisp_is_null(it))
                    {
                        unsigned int new_index = i;
                        
                        if (new_capacity != table->capacity)
                            new_index = symbol_hash(lisp_car(lisp_car(it))) % new_capacity;
                        
                        // allocate a new pair in the to space
                        Pair* cons_block = heap_alloc(sizeof(Pair), LISP_PAIR, to);
                        cons_block->block.gc_flags = GC_VISITED;
                        Lisp cons;
                        cons.val.ptr_val = cons_block;
                        cons.type = LISP_PAIR;
                        lisp_set_car(cons, gc_move(lisp_car(it), to));
                        lisp_set_cdr(cons, _table_ref(dest_table, new_index));

                        dest_table->entries[new_index].ptr_val = cons_block;
                        it = lisp_cdr(it);
                    }
                }
            }
            
            // return the moved table address
            val.ptr_val = table->block.info.forward;
            return val;
        }
        default:
            return val;
    }
}

static Lisp gc_move(Lisp l, Heap* to)
{
    l.val = gc_move_val(l.val, l.type, to);
    return l;
}

static void gc_move_v(Lisp* start, int n, Heap* to)
{
    int i;
    for (i = 0; i < n; ++i)
        start[i] = gc_move(start[i], to);
}

Lisp lisp_collect(Lisp root_to_save, LispContext ctx)
{
    time_t start_time = clock();

    Heap* to = &ctx.p->to_heap;
    heap_init(to, ctx.p->heap.page_size);

    // move root object
    ctx.p->symbol_table = gc_move(ctx.p->symbol_table, to);
    ctx.p->global_env = gc_move(ctx.p->global_env, to);
    ctx.p->macros = gc_move(ctx.p->macros, to);

    gc_move_v(ctx.p->symbol_cache, SYM_COUNT, to);
    gc_move_v(ctx.p->stack, ctx.p->stack_ptr, to);

    Lisp result = gc_move(root_to_save, to);

    // move references
    const Page* page = to->bottom;
    int page_counter = 0;
    while (page)
    {
        size_t offset = 0;
        while (offset < page->size)
        {
            Block* block = (Block*)(page->buffer + offset);
            if (!(block->gc_flags & GC_VISITED))
            {
                switch (block->type)
                {
                    // these add to the buffer!
                    // so lists are handled in a single pass
                    case LISP_PAIR:
                    {
                        // move the CAR and CDR
                        Pair* pair = (Pair*)block;
                        pair->car = gc_move_val(pair->car, pair->block.d.pair.car_type, to);
                        pair->cdr = gc_move_val(pair->cdr, pair->block.d.pair.cdr_type, to);
                        break;
                    }
                    case LISP_VECTOR:
                    {
                        Vector* vector = (Vector*)block;
                        int n = _vector_len(vector);
                        char* entry_types = _vector_types(vector);
                        for (int i = 0; i < n; ++i)
                            gc_move_val(vector->entries[i], entry_types[i], to);
                        break;
                    }
                    case LISP_LAMBDA:
                    {
                        // move the body and args
                        Lambda* lambda = (Lambda*)block;
                        lambda->args = gc_move(lambda->args, to);
                        lambda->body = gc_move(lambda->body, to);
                        lambda->env = gc_move(lambda->env, to);
                        break;
                    }
                    default: break;
                }
                block->gc_flags |= GC_VISITED;
            }
            offset += block->info.size;
        }
        page = page->next;
        ++page_counter;
    }
    // check that we visited all the pages
    assert(page_counter == to->page_count);
    
#ifdef LISP_DEBUG
     {
          // DEBUG, check offsets
          const Page* page = to->bottom;
          while (page)
          {
              size_t offset = 0;
              while (offset < page->size)
              {
                  Block* block = (Block*)(page->buffer + offset);
                  assert(block->info.size % sizeof(LispVal) == 0);
                  offset += block->info.size;
              }
              assert(offset == page->size);
              page = page->next;
          }
    }
#endif
    
    size_t diff = ctx.p->heap.size - ctx.p->to_heap.size;

    // swap the heaps
    Heap temp = ctx.p->heap;
    ctx.p->heap = ctx.p->to_heap;
    ctx.p->to_heap = temp;
    
    // reset the heap
    heap_shutdown(&ctx.p->to_heap);
    
    time_t end_time = clock();
    ctx.p->gc_stat_freed = diff;
    ctx.p->gc_stat_time = 1000000 * (end_time - start_time) / CLOCKS_PER_SEC;
    return result;
}

void lisp_print_collect_stats(LispContext ctx)
{
    Page* page = ctx.p->heap.bottom;
    while (page)
    {
        printf("%lu/%lu ", page->size, page->capacity);
        page = page->next;
    }
    fprintf(ctx.p->out_file, "\ngc collected: %lu\t time: %lu us\n", ctx.p->gc_stat_freed, ctx.p->gc_stat_time);
    fprintf(ctx.p->out_file, "heap size: %lu\t pages: %lu\n", ctx.p->heap.size, ctx.p->heap.page_count);
    fprintf(ctx.p->out_file, "symbols: %lu \n", (size_t)lisp_table_size(ctx.p->symbol_table));
}


Lisp lisp_env_global(LispContext ctx)
{
    return ctx.p->global_env;
}

void lisp_env_set_global(Lisp env, LispContext ctx)
{
    ctx.p->global_env = env;
}

void lisp_shutdown(LispContext ctx)
{
    heap_shutdown(&ctx.p->heap);
    heap_shutdown(&ctx.p->to_heap);
    free(ctx.p->stack);
    free(ctx.p);
}

const char* lisp_error_string(LispError error)
{
    switch (error)
    {
        case LISP_ERROR_NONE:
            return "none";
        case LISP_ERROR_FILE_OPEN:
            return "file error: could not open file";
        case LISP_ERROR_PAREN_UNEXPECTED:
            return "syntax error: unexpected ) paren";
        case LISP_ERROR_HASH_UNEXPECTED:
            return "syntax error: sharpsign # error. valid forms are #t, #f, #\\, or #(";
        case LISP_ERROR_PAREN_EXPECTED:
            return "syntax error: expected ) paren";
        case LISP_ERROR_DOT_UNEXPECTED:
            return "syntax error: dot . was unexpected";
        case LISP_ERROR_BAD_TOKEN:
            return "syntax error: bad token";
        case LISP_ERROR_BAD_DEFINE:
            return "expand error: bad define (define var x)";
        case LISP_ERROR_FORM_SYNTAX:
            return "expand error: bad special form";
        case LISP_ERROR_BAD_LAMBDA:
            return "expand error: bad lambda";
        case LISP_ERROR_UNKNOWN_VAR:
            return "eval error: unknown variable";
        case LISP_ERROR_BAD_OP:
            return "eval error: attempt to apply something which was not an operator";
        case LISP_ERROR_UNKNOWN_EVAL:
            return "eval error: got into a bad state";
        case LISP_ERROR_BAD_ARG:
            return "eval error: bad argument type";
        case LISP_ERROR_OUT_OF_BOUNDS:
            return "eval error: index out of bounds";
        case LISP_ERROR_SPLICE:
            return "expand error: slicing ,@ must be in a backquoted list.";
        case LISP_ERROR_RUNTIME:
            return "evaluation called (error) and it was not handled";
        default:
            return "unknown error code";
    }
}


LispContext lisp_init_empty_opt(int symbol_table_size, size_t stack_depth, size_t page_size, FILE* out_file)
{
    LispContext ctx;
    ctx.p = malloc(sizeof(struct LispImpl));
    if (!ctx.p) return ctx;

    ctx.p->out_file = out_file;
    ctx.p->lambda_counter = 0;
    ctx.p->symbol_counter = 0;
    ctx.p->stack_ptr = 0;
    ctx.p->stack_depth = stack_depth;
    ctx.p->stack = malloc(sizeof(Lisp) * stack_depth);
    ctx.p->gc_stat_freed = 0;
    ctx.p->gc_stat_time = 0;
    
    heap_init(&ctx.p->heap, page_size);
    heap_init(&ctx.p->to_heap, page_size);

    ctx.p->symbol_table = lisp_make_table(symbol_table_size, ctx);
    ctx.p->global_env = lisp_make_null();
    ctx.p->macros = lisp_make_table(20, ctx);

    Lisp* c = ctx.p->symbol_cache;
    c[SYM_IF] = lisp_make_symbol("IF", ctx);
    c[SYM_BEGIN] = lisp_make_symbol("BEGIN", ctx);
    c[SYM_QUOTE] = lisp_make_symbol("QUOTE", ctx);
    c[SYM_QUASI_QUOTE] = lisp_make_symbol("QUASIQUOTE", ctx);
    c[SYM_UNQUOTE] = lisp_make_symbol("UNQUOTE", ctx);
    c[SYM_UNQUOTE_SPLICE] = lisp_make_symbol("UNQUOTESPLICE", ctx);
    c[SYM_DEFINE] = lisp_make_symbol("DEFINE", ctx);
    c[SYM_DEFINE_MACRO] = lisp_make_symbol("DEFINE-MACRO", ctx);
    c[SYM_SET] = lisp_make_symbol("SET!", ctx);
    c[SYM_LAMBDA] = lisp_make_symbol("LAMBDA", ctx);
    return ctx;
}

LispContext lisp_init_empty(void)
{
    return lisp_init_empty_opt(LISP_DEFAULT_SYMBOL_TABLE_SIZE, LISP_DEFAULT_STACK_DEPTH, LISP_DEFAULT_PAGE_SIZE, stdout);
}

#ifndef LISP_NO_LIB

static Lisp sch_cons(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_cons(lisp_car(args), lisp_car(lisp_cdr(args)), ctx);
}

static Lisp sch_car(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_car(lisp_car(args));
}

static Lisp sch_cdr(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_cdr(lisp_car(args));
}

static Lisp sch_set_car(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    lisp_set_car(a, b);
    return lisp_make_null();
}

static Lisp sch_set_cdr(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    lisp_set_cdr(a, b);
    return lisp_make_null();
}

static Lisp sch_accessor_mnemonic(Lisp args, LispError* e, LispContext ctx)
{
    Lisp path = lisp_car(args);
    Lisp l = lisp_car(lisp_cdr(args));

    return lisp_list_accessor_mnemonic(l, lisp_string(path));
}

static Lisp sch_exact_eq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_eq(a, b));
}

static Lisp sch_equal(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_bool(lisp_equal(a, b));
}

static Lisp sch_equal_r(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    
    return lisp_make_bool(lisp_equal_r(a, b));
}

static Lisp sch_is_null(Lisp args, LispError* e, LispContext ctx)
{
    while (!lisp_is_null(args))
    {
        if (!lisp_is_null(lisp_car(args))) return lisp_false();
        args = lisp_cdr(args);
    }
    return lisp_true();
}

static Lisp sch_is_pair(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if (!lisp_is_pair(lisp_car(args))) return lisp_false();
        args = lisp_cdr(args);
    }
    return lisp_true();
}

static Lisp sch_display(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    if (lisp_type(l) == LISP_STRING)
    {
        fputs(lisp_string(l), ctx.p->out_file);
    }
    else
    {
        lisp_printf(ctx.p->out_file, l);
    }
    return lisp_make_null();
}

static Lisp sch_write_char(Lisp args, LispError* e, LispContext ctx)
{
    fputc(lisp_char(lisp_car(args)), ctx.p->out_file);
    return lisp_make_null();
}

static Lisp sch_flush(Lisp args, LispError* e, LispContext ctx)
{
    fflush(ctx.p->out_file); return lisp_make_null();
}

static Lisp sch_error(Lisp args, LispError* e, LispContext ctx)
{
   Lisp l = lisp_car(args);
   fputs(lisp_string(l), ctx.p->out_file);

   *e = LISP_ERROR_RUNTIME;
   return lisp_make_null();
}

static Lisp sch_syntax_error(Lisp args, LispError* e, LispContext ctx)
{
    fprintf(stderr, "expand error: %s ", lisp_string(lisp_car(args)));
    args = lisp_cdr(args);
    if (!lisp_is_null(args))
    {
        lisp_printf(stderr, lisp_car(args));
    }
    fprintf(stderr, "\n");

    *e = LISP_ERROR_FORM_SYNTAX;
    return lisp_make_null();
}

static Lisp sch_equals(Lisp args, LispError* e, LispContext ctx)
{
    Lisp to_check  = lisp_car(args);
    if (lisp_is_null(to_check)) return lisp_true();
    
    args = lisp_cdr(args);
    while (lisp_is_pair(args))
    {
        if (lisp_int(lisp_car(args)) != lisp_int(to_check)) return lisp_false();
        args = lisp_cdr(args);
    }
    
    return lisp_true();
}

static Lisp sch_list(Lisp args, LispError* e, LispContext ctx) { return args; }

static Lisp sch_list_copy(Lisp args, LispError* e, LispContext ctx) {
    return lisp_list_copy(lisp_car(args), ctx);
}

static Lisp sch_append(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_make_null();  

    while (lisp_is_pair(args))
    {
        l = lisp_list_append(l, lisp_car(args), ctx);
        args = lisp_cdr(args);
    }
    return l;
}

static Lisp sch_list_ref(Lisp args, LispError* e, LispContext ctx)
{
    Lisp list = lisp_car(args);
    Lisp index = lisp_car(lisp_cdr(args));
    return lisp_list_ref(list, lisp_int(index));
}

static Lisp sch_length(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_int(lisp_list_length(lisp_car(args)));
}

static Lisp sch_reverse_inplace(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_list_reverse(lisp_car(args), lisp_make_null());
}

static Lisp sch_assoc(Lisp args, LispError* e, LispContext ctx)
{
    Lisp key = lisp_car(args);
    Lisp l = lisp_car(lisp_cdr(args));
    return lisp_list_assoc(l, key);
}

static Lisp sch_assq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp key = lisp_car(args);
    Lisp l = lisp_car(lisp_cdr(args));
    return lisp_list_assq(l, key);
}

static Lisp sch_add(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    // TODO: types
    while (lisp_is_pair(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.val.int_val += lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_REAL)
        {
            accum.val.real_val += lisp_real(lisp_car(args));
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp sch_sub(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (lisp_is_pair(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.val.int_val -= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_REAL)
        {
            accum.val.real_val -= lisp_real(lisp_car(args));
        }
        else
        {
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp sch_mult(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (lisp_is_pair(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.val.int_val *= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_REAL)
        {
            accum.val.real_val *= lisp_real(lisp_car(args));
        }
        else
        {
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp sch_divide(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (lisp_is_pair(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.val.int_val /= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_REAL)
        {
            accum.val.real_val /= lisp_real(lisp_car(args));
        }
        else
        {
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp sch_less(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);
    int result = 0;

    if (lisp_type(accum) == LISP_INT)
    {
        result = lisp_int(accum) < lisp_int(lisp_car(args));
    }
    else if (lisp_type(accum) == LISP_REAL)
    {
        result = lisp_real(accum) < lisp_real(lisp_car(args));
    }
    else
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }
    return lisp_make_bool(result);
}

static Lisp sch_to_exact(Lisp args, LispError* e, LispContext ctx)
{
    Lisp val = lisp_car(args);
    switch (lisp_type(val))
    {
        case LISP_INT:
            return val;
        case LISP_CHAR:
            return lisp_make_int(lisp_char(val));
        case LISP_REAL:
            return lisp_make_int((int)lisp_real(val));
            
        // TODO: string pementations probably nonstandard
        case LISP_STRING:
            return lisp_make_int(atoi(lisp_string(val)));
        default:
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
    }
}

static Lisp sch_to_inexact(Lisp args, LispError* e, LispContext ctx)
{
    Lisp val = lisp_car(args);
    switch (lisp_type(val))
    {
        case LISP_REAL:
            return val;
        case LISP_INT:
            return lisp_make_real(lisp_real(val));
        case LISP_STRING:
            return lisp_make_real(atof(lisp_string(val)));
        default:
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
    }
}

static Lisp sch_to_string(Lisp args, LispError* e, LispContext ctx)
{
    char scratch[SCRATCH_MAX];
    Lisp val = lisp_car(args);
    switch (lisp_type(val))
    {
        case LISP_REAL:
            snprintf(scratch, SCRATCH_MAX, "%f", lisp_real(val));
            return lisp_make_string(scratch, ctx);
        case LISP_INT:
            snprintf(scratch, SCRATCH_MAX, "%lli", lisp_int(val));
            return lisp_make_string(scratch, ctx);
        case LISP_SYMBOL:
            return lisp_make_string(lisp_symbol_string(val), ctx);
        case LISP_STRING:
            return val;
        default:
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
    }
}

static Lisp sch_symbol_to_string(Lisp args, LispError* e, LispContext ctx)
{
    Lisp val = lisp_car(args);
    if (lisp_type(val) != LISP_SYMBOL)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }
    else
    {
        return lisp_make_string(lisp_symbol_string(val), ctx);
    }
}

static Lisp sch_is_symbol(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_SYMBOL);
}

static Lisp sch_string_to_symbol(Lisp args, LispError* e, LispContext ctx)
{
    Lisp val = lisp_car(args);
    if (lisp_type(val) != LISP_STRING)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }
    else
    {
        return lisp_make_symbol(lisp_string(val), ctx);
    }
}

static Lisp sch_gensym(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_symbol(NULL, ctx);
}

static Lisp sch_is_string(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if (lisp_type(lisp_car(args)) != LISP_STRING) return lisp_false();
        args = lisp_cdr(args);
    }
    return lisp_true();
}

static Lisp sch_string_is_null(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    return lisp_make_bool(lisp_string(a)[0] == '\0');
}

static Lisp sch_make_string(Lisp args, LispError* e, LispContext ctx)
{
    Lisp n = lisp_car(args);
    int c = '_';
    
    args = lisp_cdr(args);
    if (lisp_is_pair(args)) {
        c = lisp_char(lisp_car(args));
    }
    return lisp_make_empty_string(lisp_int(n), c, ctx);
}

static Lisp sch_string_equal(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    int result = strcmp(lisp_string(a), lisp_string(b)) == 0;
    return lisp_make_bool(result);
}

static Lisp sch_string_less(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    int result = strcmp(lisp_string(a), lisp_string(b)) < 0;
    return lisp_make_bool(result);
}

static Lisp sch_string_copy(Lisp args, LispError* e, LispContext ctx)
{
    Lisp val = lisp_car(args);
    if (lisp_type(val) != LISP_STRING)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }
     return lisp_make_string(lisp_string(val), ctx);
}

static Lisp sch_string_length(Lisp args, LispError* e, LispContext ctx)
{
    Lisp x = lisp_car(args);
    if (lisp_type(x) != LISP_STRING)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    return lisp_make_int((LispInt)strlen(lisp_string(x)));
}

static Lisp sch_string_ref(Lisp args, LispError* e, LispContext ctx)
{
    Lisp str = lisp_car(args);
    Lisp index = lisp_car(lisp_cdr(args));
    if (lisp_type(str) != LISP_STRING || lisp_type(index) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    return lisp_make_char((int)lisp_string_ref(str, lisp_int(index)));
}

static Lisp sch_string_set(Lisp args, LispError* e, LispContext ctx)
{
    Lisp str = lisp_list_ref(args, 0);
    Lisp index = lisp_list_ref(args, 1);
    Lisp val = lisp_list_ref(args, 2);
    if (lisp_type(str) != LISP_STRING || lisp_type(index) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    lisp_string_set(str, lisp_int(index), (char)lisp_int(val));
    return lisp_make_null();
}

static Lisp sch_string_upcase(Lisp args, LispError* e, LispContext ctx)
{
    Lisp s = lisp_car(args);
    Lisp r = lisp_make_string(lisp_string(s), ctx);
    
    char* c = lisp_string(r);
    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }
    return r;
}

static Lisp sch_string_downcase(Lisp args, LispError* e, LispContext ctx)
{
    Lisp s = lisp_car(args);
    Lisp r = lisp_make_string(lisp_string(s), ctx);
    
    char* c = lisp_string(r);
    while (*c)
    {
        *c = tolower(*c);
        ++c;
    }
    return r;
}

static Lisp sch_string_to_list(Lisp args, LispError* e, LispContext ctx)
{
    Lisp s = lisp_car(args);
    char* c = lisp_string(s);
    
    Lisp front = lisp_make_null();
    Lisp back = lisp_make_null();
    while (*c)
    {
        lisp_fast_append(&front, &back, lisp_make_char(*c), ctx);
        ++c;
    }
    return front;
}


static Lisp sch_list_to_string(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    Lisp result = lisp_make_empty_string(lisp_list_length(l), '\0', ctx);
    char* s = lisp_string(result);
    
    while (lisp_is_pair(l))
    {
        *s = (char)lisp_char(lisp_car(l));
        ++s;
        l = lisp_cdr(l);
    }
    return result;
}

static Lisp sch_is_char(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_CHAR);
}

static Lisp sch_char_upcase(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_char(toupper(c));
}

static Lisp sch_char_downcase(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_char(tolower(c));
}

static Lisp sch_char_is_alphanum(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isalnum(c));
}

static Lisp sch_char_is_alpha(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isalpha(c));
}

static Lisp sch_char_is_number(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isdigit(c));
}

static Lisp sch_char_is_white(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_bool(isblank(c));
}

static Lisp sch_is_exact(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_INT);
}

static Lisp sch_is_int(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool(lisp_type(lisp_car(args)) == LISP_INT);
}

static Lisp sch_is_real(Lisp args, LispError* e, LispContext ctx)
{
    LispType t = lisp_type(lisp_car(args));
    return lisp_make_bool(t == LISP_INT || t == LISP_REAL);
}

static Lisp sch_is_boolean(Lisp args, LispError* e, LispContext ctx)
{
    LispType t = lisp_type(lisp_car(args));
    return lisp_make_bool(t == LISP_BOOL);
}

static Lisp sch_is_even(Lisp args, LispError* e, LispContext ctx)
{
    while (!lisp_is_null(args))
    {
        if ((lisp_int(lisp_car(args)) & 1) == 1) return lisp_false();
        args = lisp_cdr(args);
    }
    return lisp_true();
}

static Lisp sch_exp(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = exp(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_log(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = log(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_sin(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = sin(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_cos(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = cos(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_tan(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = tan(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_sqrt(Lisp args, LispError* e, LispContext ctx)
{
    LispReal x = sqrt(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_quotient(Lisp args, LispError* e, LispContext ctx)
{
    int a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    int b = lisp_int(lisp_car(args));
    return lisp_make_int(a / b);
}

static Lisp sch_remainder(Lisp args, LispError* e, LispContext ctx)
{
    int a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    int b = lisp_int(lisp_car(args));
    return lisp_make_int(a % b);

}

static Lisp sch_modulo(Lisp args, LispError* e, LispContext ctx)
{
    int m = lisp_int(sch_remainder(args, e, ctx));
    if (m < 0) {
        int b = lisp_int(lisp_car(lisp_cdr(args)));
        m = (b < 0) ? m - b : m + b;
    }
    return lisp_make_int(m);
}

static Lisp sch_abs(Lisp args, LispError* e, LispContext ctx)
{
    switch (lisp_type(lisp_car(args)))
    {
        case LISP_INT:
            return lisp_make_int(llabs(lisp_int(lisp_car(args))));
        case LISP_REAL:
            return lisp_make_real(fabs(lisp_real(lisp_car(args))));
        default:
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
    }
}

static Lisp sch_vector(Lisp args, LispError* e, LispContext ctx)
{
    int N = lisp_list_length(args);
    Lisp v = lisp_make_vector(N, lisp_make_null(), ctx);
    for (int i = 0; i < N; ++i)
    {
        lisp_vector_set(v, i, lisp_car(args));
        args = lisp_cdr(args);
    }
    return v;
}

static Lisp sch_is_vector(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_bool( lisp_type(lisp_car(args)) == LISP_VECTOR );
}

static Lisp sch_make_vector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp length = lisp_car(args);

    if (lisp_type(length) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    Lisp next = lisp_cdr(args);

    Lisp val;
    if (lisp_is_null(next))
    {
        val = lisp_make_null();
    }
    else
    {
        val = lisp_car(lisp_cdr(args));
    }

    return lisp_make_vector(lisp_int(length), val, ctx);
}

static Lisp sch_vector_grow(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    Lisp length = lisp_car(lisp_cdr(args));

    if (lisp_type(length) != LISP_INT || lisp_type(v) != LISP_VECTOR)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    if (lisp_int(length) < lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_make_null();
    }

    return lisp_vector_grow(v, lisp_int(length), ctx);
}

static Lisp sch_vector_length(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    if (lisp_type(v) != LISP_VECTOR)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    return lisp_make_int(lisp_vector_length(v));
}

static Lisp sch_vector_ref(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    Lisp i = lisp_car(lisp_cdr(args));

    if (lisp_type(v) != LISP_VECTOR || lisp_type(i) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    if (lisp_int(i) >= lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_make_null();
    }

    return lisp_vector_ref(v, lisp_int(i));
}

static Lisp sch_vector_set(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_list_ref(args, 0);
    Lisp i = lisp_list_ref(args, 1);
    Lisp x = lisp_list_ref(args, 2);

    if (lisp_type(v) != LISP_VECTOR || lisp_type(i) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    if (lisp_int(i) >= lisp_vector_length(v))
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_make_null();
    }

    lisp_vector_set(v, lisp_int(i), x);
    return lisp_make_null();
}

static Lisp sch_vector_fill(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_list_ref(args, 0);
    Lisp x = lisp_list_ref(args, 1);
    lisp_vector_fill(v, x);
    return lisp_make_null();
}

static Lisp sch_vector_assq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp key = lisp_car(args);
    Lisp v = lisp_car(lisp_cdr(args));
    return lisp_vector_assq(v, key);
}

static Lisp sch_subvector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    args = lisp_cdr(args);
    Lisp start = lisp_car(args);
    args = lisp_cdr(args);
    Lisp end = lisp_car(args);
    return lisp_subvector(v, lisp_int(start), lisp_int(end), ctx);
}

static Lisp sch_list_to_vector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    unsigned int n = lisp_list_length(l);
    Lisp v = lisp_make_vector(n, lisp_make_null(), ctx);
    
    int i = 0;
    while (!lisp_is_null(l))
    {
        lisp_vector_set(v, i, lisp_car(l));
        l = lisp_cdr(l);
        ++i;
    }
    return v;
}

static Lisp sch_vector_to_list(Lisp args, LispError* e, LispContext ctx)
{
    Lisp v = lisp_car(args);
    unsigned int n = lisp_vector_length(v);
    
    Lisp front = lisp_make_null();
    Lisp back = lisp_make_null();
    int i;
    for (i = 0; i < n; ++i)
    {
        lisp_fast_append(&front, &back, lisp_vector_ref(v, i), ctx);
    }
    return front;
}

static Lisp sch_pseudo_seed(Lisp args, LispError* e, LispContext ctx)
{
    Lisp seed = lisp_car(args);
    srand((unsigned int)lisp_int(seed));
    return lisp_make_null();
}

static Lisp sch_pseudo_rand(Lisp args, LispError* e, LispContext ctx)
{
    Lisp n = lisp_car(args);
    return lisp_make_int(rand() % lisp_int(n));
}

static Lisp sch_univeral_time(Lisp args, LispError* e, LispContext ctx)
{
    // TODO: loss of precision
    return lisp_make_int((int)time(NULL));
}

static Lisp sch_table_make(Lisp args, LispError* e, LispContext ctx)
{
    if (!lisp_is_null(args))
    {
        Lisp x = lisp_car(args);
        return lisp_make_table(lisp_int(x), ctx);
    }
    else
    {
        return lisp_make_table(23, ctx);
    }
}

static Lisp sch_table_get(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    args = lisp_cdr(args);
    Lisp key = lisp_car(args);
    return lisp_table_get(table, key, ctx);
}

static Lisp sch_table_set(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    args = lisp_cdr(args);
    Lisp key = lisp_car(args);
    args = lisp_cdr(args);
    Lisp x = lisp_car(args);
    lisp_table_set(table, key, x, ctx);
    return lisp_make_null();
}

static Lisp sch_table_size(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    return lisp_make_int(lisp_table_size(table));
}

static Lisp sch_table_to_alist(Lisp args, LispError* e, LispContext ctx)
{
    Lisp table = lisp_car(args);
    return lisp_table_to_assoc_list(table, ctx);
}

static Lisp sch_apply(Lisp args, LispError* e, LispContext ctx)
{
    Lisp operator = lisp_car(args);
    args = lisp_cdr(args);
    Lisp op_args = lisp_car(args);
    
    // TODO: argument passing is a little more sophisitaed
    Lisp x;
    Lisp env;
    int needs_to_eval = apply(operator, op_args, &x, &env, e, ctx);
    
    if (needs_to_eval)
    {
        // TODO: I don't think its safe to garbage collect
        return lisp_eval_opt(x, env, e, ctx);
    }
    else
    {
        return x;
    }
}

static Lisp sch_is_lambda(Lisp args, LispError* e, LispContext ctx)
{
    int type = lisp_type(lisp_car(args));
    return lisp_make_bool(type == LISP_LAMBDA);
}

static Lisp sch_lambda_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_lambda_env(lisp_car(args));
}

static Lisp sch_is_func(Lisp args, LispError* e, LispContext ctx)
{
    int type = lisp_type(lisp_car(args));
    return lisp_make_bool(type == LISP_FUNC);
}

static Lisp sch_lambda_body(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    Lambda* lambda = lisp_lambda(l);
    return lambda->body;
}

static Lisp sch_macroexpand(Lisp args, LispError* e, LispContext ctx)
{
    Lisp expr = lisp_car(args);
    Lisp result = lisp_macroexpand(expr, e, ctx);
    return result;
}

static Lisp sch_eval(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_eval_opt(lisp_car(args), lisp_car(lisp_cdr(args)), e, ctx);
}

static Lisp sch_system_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_cdr(lisp_env_global(ctx));
}

static Lisp sch_user_env(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_env_global(ctx);
}

static Lisp sch_gc_flip(Lisp args, LispError* e, LispContext ctx)
{
    lisp_collect(lisp_make_null(), ctx);
    return lisp_make_null();
}
static Lisp sch_print_gc_stats(Lisp args, LispError* e, LispContext ctx)
{
    lisp_print_collect_stats(ctx);
    return lisp_make_null();
}

#ifndef LISP_NO_SYSTEM_LIB
static Lisp sch_read_path(Lisp args, LispError *e, LispContext ctx)
{
    const char* path = lisp_string(lisp_car(args));
    Lisp result = lisp_read_path(path, e, ctx);
    return result;
}
#endif

static const LispFuncDef lib_cfunc_defs[] = {
    
    // NON STANDARD ADDITINONS
    { "ERROR", sch_error },
    { "SYNTAX-ERROR", sch_syntax_error },
    
#ifndef LISP_NO_SYSTEM_LIB
    { "READ-PATH", sch_read_path },

    // Output Procedures https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Output-Procedures.html
    { "DISPLAY", sch_display },
    { "WRITE-CHAR", sch_write_char },
    { "FLUSH-OUTPUT-PORT", sch_flush },
    
   
    // Universal Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
    { "GET-UNIVERSAL-TIME", sch_univeral_time },

    { "PRINT-GC-STATISTICS", sch_print_gc_stats },
#endif
    
    { "MACROEXPAND", sch_macroexpand },
    
    { "ACCESSOR-MNEMONIC", sch_accessor_mnemonic },
    
    // Equivalence Predicates https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Equivalence-Predicates.html
    { "EQ?", sch_exact_eq },
    { "EQV?", sch_equal },
    { "EQUAL?", sch_equal_r },
    
    // Booleans https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Booleans.html
    
    // Lists https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_8.html
    { "CONS", sch_cons },
    { "CAR", sch_car },
    { "CDR", sch_cdr },
    { "SET-CAR!", sch_set_car },
    { "SET-CDR!", sch_set_cdr },
    { "NULL?", sch_is_null },
    { "PAIR?", sch_is_pair },
    { "LIST", sch_list },
    { "LIST-COPY", sch_list_copy },
    { "LENGTH", sch_length },
    { "APPEND", sch_append },
    { "LIST-REF", sch_list_ref },
    { "REVERSE!", sch_reverse_inplace },

    // Vectors https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_9.html#SEC82
    { "VECTOR", sch_vector },
    { "VECTOR?", sch_is_vector },
    { "MAKE-VECTOR", sch_make_vector },
    { "VECTOR-GROW", sch_vector_grow },
    { "VECTOR-LENGTH", sch_vector_length },
    { "VECTOR-SET!", sch_vector_set },
    { "VECTOR-REF", sch_vector_ref },
    { "VECTOR-FILL!", sch_vector_fill },
    { "SUBVECTOR", sch_subvector },
    { "LIST->VECTOR", sch_list_to_vector },
    { "VECTOR->LIST", sch_vector_to_list },
    // TODO: sort

    // Strings https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html#SEC61
    { "STRING?", sch_is_string },
    { "MAKE-STRING", sch_make_string },
    { "STRING=?", sch_string_equal },
    { "STRING<?", sch_string_less },
    { "STRING-COPY", sch_string_copy },
    { "STRING-NULL?", sch_string_is_null },
    { "STRING-LENGTH", sch_string_length },
    { "STRING-REF", sch_string_ref },
    { "STRING-SET!", sch_string_set },
    { "STRING-UPCASE", sch_string_upcase },
    { "STRING-DOWNCASE", sch_string_downcase },
    { "STRING->LIST", sch_string_to_list },
    { "LIST->STRING", sch_list_to_string },
    
    // Characters https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Characters.html#Characters
    { "CHAR?", sch_is_char },
    { "CHAR=?", sch_equals },
    { "CHAR<?", sch_less },
    { "CHAR-UPCASE", sch_char_upcase },
    { "CHAR-DOWNCASE", sch_char_downcase },
    { "CHAR-WHITESPACE?", sch_char_is_white },
    { "CHAR-ALPHANUMERIC?", sch_char_is_alphanum },
    { "CHAR-ALPHABETIC?", sch_char_is_alpha },
    { "CHAR-NUMERIC?", sch_char_is_number },
    { "CHAR->INTEGER", sch_to_exact },

    // Association Lists https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Association-Lists.html
    { "ASSOC", sch_assoc },
    { "ASSQ", sch_assq },

    // TODO: Non Standard
    { "VECTOR-ASSQ", sch_vector_assq },
    { "BOOLEAN?", sch_is_boolean },

    // Numerical operations https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Numerical-operations.html
    { "=", sch_equals },
    { "+", sch_add },
    { "-", sch_sub },
    { "*", sch_mult },
    { "/", sch_divide },
    { "<", sch_less },
    { "INTEGER?", sch_is_int },
    { "EVEN?", sch_is_even },
    { "REAL?", sch_is_real },
    { "EXP", sch_exp },
    { "LOG", sch_log },
    { "SIN", sch_sin },
    { "COS", sch_cos },
    { "TAN", sch_tan },
    { "SQRT", sch_sqrt },
    { "QUOTIENT", sch_quotient },
    { "REMAINDER", sch_remainder },
    { "MODULO", sch_modulo },
    { "ABS", sch_abs },
    
    { "EXACT?", sch_is_exact },
    { "EXACT->INEXACT", sch_to_inexact },
    { "INEXACT->EXACT", sch_to_exact },
    
    // Symbols https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
    { "SYMBOL?", sch_is_symbol },
    { "STRING->SYMBOL", sch_string_to_symbol },
    { "SYMBOL->STRING", sch_symbol_to_string },
    { "GENERATE-UNINTERNED-SYMBOL", sch_gensym },
    { "GENSYM", sch_gensym },

    // Environments https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_14.html
    { "EVAL", sch_eval },
    { "SYSTEM-GLOBAL-ENVIRONMENT", sch_system_env },
    { "USER-INITIAL-ENVIRONMENT", sch_user_env },
    // { "THE-ENVIRONMENT", sch_current_env },
    // TODO: purify
    
    // Hash Tables https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Basic-Hash-Table-Operations.html#Basic-Hash-Table-Operations
    { "MAKE-HASH-TABLE", sch_table_make },
    { "HASH-TABLE-SET!", sch_table_set },
    { "HASH-TABLE-REF", sch_table_get },
    { "HASH-TABLE-SIZE", sch_table_size },
    { "HASH-TABLE->ALIST", sch_table_to_alist },
    
    // Procedures https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
    { "APPLY", sch_apply },
    { "COMPILED-PROCEDURE?", sch_is_func },
    { "COMPOUND-PROCEDURE?", sch_is_lambda },
    { "PROCEDURE-ENVIRONMENT", sch_lambda_env },
    // TOOD: Almost standard
    { "PROCEDURE-BODY", sch_lambda_body },

    // Random Numbers https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Random-Numbers.html
    { "RANDOM", sch_pseudo_rand },
    
    // TODO: this is nonstandard
    { "RANDOM-SEED!", sch_pseudo_seed },
   
    // Garbage Collection https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-user/Garbage-Collection.html
    { "GC-FLIP", sch_gc_flip },
    
    { NULL, NULL }
    
};

// We have several batches of code as some macros
// Need to be prepared before other code to eval.

// MACRO GUIDE

// (LET ((<var0> <expr0>) ... (<varN> <expr1>)) <body0> ... <bodyN>)
//  -> ((LAMBDA (<var0> ... <varN>) (BEGIN <body0> ... <bodyN>)) <expr0> ... <expr1>)            

// (COND (<pred0> <expr0>)
//       (<pred1> <expr1>)
//        ...
//        (else <expr-1>)) ->
//
//  (IF <pred0> <expr0>
//      (if <pred1> <expr1>
//          ....
//      (if <predN> <exprN> <expr-1>)) ... )

// (DO ((<var0> <init0> <step0>) ...) (<test> <result>) <body>)
// -> ((lambda (f)
//        (begin
//          (set! f (lambda (<var0> ... <varN>)
//                   (if <test>
//                       <result>
//                       (begin
//                          <body>
//                          (f <step0> ... <stepN>)))))
//          (f <init0> ... <initN>))) NULL)

// (AND <pred0> <pred1> ... <predN>) 
// -> (IF <pred0> 
//      (IF <pred1> ...
//          (IF <predN> t f)


// (OR <pred0> <pred1> ... <predN>)
// -> (IF (<pred0>) t
//      (IF <pred1> t ...
//          (if <predN> t f))

static const char* lib_code0 = "\
(define (not x) (if x #f #t)) \
\
(define (first x) (car x)) \
(define (second x) (car (cdr x))) \
(define (third x) (car (cdr (cdr x)))) \
\
(define (some? pred l) \
  (if (null? l) #f \
    (if (pred (car l)) #t \
       (some? pred (cdr l))))) \
\
(define (map1 proc l result) \
  (if (null? l) \
    (reverse! result) \
    (map1 proc \
          (cdr l) \
          (cons (proc (car l)) result)))) \
\
(define (for-each1 proc l) \
    (if (null? l) '() \
         (begin (proc (car l)) (for-each1 proc (cdr l ))))) \
\
(define (reverse-append! l tail) \
  (if (null? l) tail \
    (let ((next (cdr l))) \
      (set-cdr! l tail) \
      (reverse-append! next l)))) \
\
(define-macro let (lambda (def-list . body) \
  (for-each1 (lambda (entry) \
    (if (not (pair? entry)) (syntax-error \"bad let entry\" entry)) \
    (if (not (symbol? (first entry))) (syntax-error \"let entry missing symbol\" entry))) def-list) \
  (cons `(lambda \
    ,(map1 (lambda (entry) (car entry)) def-list '())  \
    ,(cons 'BEGIN body)) \
    (map1 (lambda (entry) (car (cdr entry))) def-list '())) )) \
\
(define (_cond-helper clauses) \
 (if (null? clauses) \
  '() \
  (if (eq? (car (car clauses)) 'ELSE) \
   (cons 'BEGIN (cdr (car clauses))) \
   (list 'IF \
    (car (car clauses)) \
    (cons 'BEGIN (cdr (car clauses))) \
    (_cond-helper (cdr clauses)))))) \
\
(define-macro cond \
 (lambda clauses \
  (begin \
   (for-each1 (lambda (clause) \
              (if (null? (cdr clause)) \
               (syntax-error \"(cond (pred expression...)...)\")) \
             ) clauses) \
   (_cond-helper clauses)))) \
\
(define (_and-helper preds) \
 (if (null? preds) #t \
  (cons 'IF \
   (cons (car preds) \
    (cons (_and-helper (cdr preds)) (cons #f '())) )))) \
\
(define-macro and \
 (lambda preds (_and-helper preds))) \
\
(define (_or-helper preds) \
 (if (null? preds) #f \
  (cons 'IF \
   (cons (car preds) \
    (cons #t (cons (_or-helper (cdr preds)) '()) ))))) \
\
(define-macro or \
 (lambda preds (_or-helper preds))) \
";

static const char* lib_code1 = " \
(define (newline) (write-char #\\newline)) \
\
(define-macro assert \
 (lambda (body) \
  `(if ,body '() \
      (begin \
       (display (quote ,body)) \
       (error \" assert failed\"))))) \
\
(define-macro push \
 (lambda (v l) \
   `(begin (set! ,l (cons ,v ,l)) ,l))) \
\
(define-macro do \
 (lambda (vars loop-check loop) \
  (let ((names '()) \
        (inits '()) \
        (steps '()) \
        (f (gensym))) \
   (for-each1 (lambda (var)  \
               (push (car var) names) \
               (set! var (cdr var)) \
               (push (car var) inits) \
               (set! var (cdr var)) \
               (push (car var) steps)) vars) \
   `((lambda (,f) \
           (begin \
            (set! ,f (lambda ,names \
                      (if ,(car loop-check) \
                       ,(car (cdr loop-check)) \
                       ,(cons 'BEGIN (list loop (cons f steps))) )))    \
            ,(cons f inits) \
           )) '()) ))) \
\
(define (number? x) (real? x)) \
(define (odd? x) (not (even? x))) \
(define (inexact? x) (not (exact? x))) \
(define (zero? x) (= x 0)) \
 \
(define (>= a b) (not (< a b))) \
(define (> a b) (< b a)) \
(define (<= a b) (not (> a b))) \
\
(define (last-pair x) \
 (if (pair? (cdr x)) \
  (last-pair (cdr x)) x)) \
";

static const char* lib_code2 = " \
\
(define (map proc . rest) \
  (define (helper lists result) \
    (if (some? null? lists) \
      (reverse! result) \
      (helper (map1 cdr lists '()) \
              (cons (apply proc (map1 car lists '())) result)))) \
  (helper rest '())) \
\
(define (for-each proc . rest) \
  (define (helper lists) \
    (if (some? null? lists) \
      '() \
      (begin \
        (apply proc (map1 car lists '())) \
        (helper (map1 cdr lists '()))))) \
  (helper rest)) \
\
(define (memq x list) \
    (cond ((null? list) #f) \
          ((equal? (car list) x) list) \
          (else (memq x (cdr list))))) \
\
(define (member x list) \
    (cond ((null? list) #f) \
          ((equal? (car list) x) list) \
          (else (memq x (cdr list))))) \
\
(define (make-list k elem) \
   (define (helper k l) \
       (if (= k 0) l \
	   (helper (- k 1) (cons elem l)))) \
   (reverse! (helper k '()))) \
\
(define (list-tail x k) \
    (if (zero? k) x \
        (list-tail (cdr x) (- k 1)))) \
\
(define (filter pred l) \
  (define (helper l result) \
    (cond ((null? l) result) \
          ((pred (car l)) \
           (helper (cdr l) (cons (car l) result))) \
          (else \
            (helper (cdr l) result)))) \
  (reverse! (helper l '()))) \
\
(define (alist->hash-table alist) \
  (define h (make-hash-table)) \
  (for-each1 (lambda (pair) \
              (hash-table-set! h (car pair) (cdr pair))) \
            alist) \
  h) \
\
(define (reduce op acc lst) \
    (if (null? lst) acc \
        (reduce op (op acc (car lst)) (cdr lst)))) \
\
(define (max . ls) \
  (reduce (lambda (m x) \
            (if (> x m) \
              x \
              m)) (car ls) (cdr ls))) \
\
(define (min . ls) \
  (reduce (lambda (m x) \
            (if (< x m) \
              x \
              m)) (car ls) (cdr ls))) \
\
(define (_gcd-helper a b) \
  (if (= b 0) a (_gcd-helper b (modulo a b)))) \
\
(define (gcd . args) \
  (if (null? args) 0 \
      (_gcd-helper (car args) (car (cdr args))))) \
\
(define (reverse l) (reverse! (list-copy l))) \
(define (vector-head v end) (subvector v 0 end)) \
(define (vector-tail v start) (subvector v start (vector-length v))) \
\
(define (make-initialized-vector l fn) \
  (let ((v (make-vector l '()))) \
	(do ((i 0 (+ i 1))) \
     ((>= i l) v) \
	  (vector-set! v i (fn i))))) \
\
(define (vector-map fn v) \
 (make-initialized-vector \
  (vector-length v) \
  (lambda (i) (fn (vector-ref v i))))) \
\
(define (vector-binary-search v key< unwrap-key key) \
  (define (helper low high mid) \
    (if (<= (- high low) 1) \
        (if (key< (unwrap-key (vector-ref v low)) key) '() (vector-ref v low)) \
        (begin \
          (set! mid (+ low (quotient (- high low) 2))) \
          (if (key< key (unwrap-key (vector-ref v mid))) \
              (helper low mid 0) \
              (helper mid high 0))))) \
  (helper 0 (vector-length v) 0)) \
\
(define (quicksort-list l op)  \
  (if (null? l) '()  \
      (append (quicksort-list (filter (lambda (x) (op x (car l))) \
                                 (cdr l)) op) \
              (list (car l)) \
              (quicksort-list (filter (lambda (x) (not (op x (car l)))) \
                                 (cdr l)) op)))) \
(define sort quicksort-list) \
\
(define (procedure? p) (or (compiled-procedure? p) (compound-procedure? p))) \
";

LispContext lisp_init(void)
{
    return lisp_init_opt(LISP_DEFAULT_SYMBOL_TABLE_SIZE, LISP_DEFAULT_STACK_DEPTH, LISP_DEFAULT_PAGE_SIZE, stdout);
}

LispContext lisp_init_opt(int symbol_table_size, size_t stack_depth, size_t page_size, FILE* out_file)
{
    LispContext ctx = lisp_init_empty_opt(symbol_table_size, stack_depth, page_size, out_file);

    Lisp table = lisp_make_table(300, ctx);
    lisp_table_define_funcs(table, lib_cfunc_defs, ctx);
    Lisp system_env = lisp_env_extend(lisp_make_null(), table, ctx);
    ctx.p->global_env = lisp_env_extend(system_env, lisp_make_table(20, ctx), ctx);

    LispError error;

    const char* to_load[] = { lib_code0, lib_code1, lib_code2 };
    for (int i = 0; i < 3; ++i)
    {
        lisp_eval_opt(lisp_read(to_load[i], NULL, ctx), system_env, &error, ctx);

        if (error != LISP_ERROR_NONE)
        {
            fprintf(stderr, "failed to init system library %d: %s\n", i, lisp_error_string(error));
            lisp_shutdown(ctx);
            break;
        }
    }

    if (error == LISP_ERROR_NONE)
    {
        lisp_collect(lisp_make_null(), ctx);

#ifdef LISP_DEBUG
        lisp_print_collect_stats(ctx);
#endif
    }

    return ctx;
}

Lisp lisp_macro_table(LispContext ctx)
{
    return ctx.p->macros;
}

#endif


#undef SCRATCH_MAX

#endif
