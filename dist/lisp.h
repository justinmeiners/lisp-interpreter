/*
 Created by: Justin Meiners 
 Repo; https://github.com/justinmeiners/lisp-interpreter
 License: MIT (See end if file)

 Do this:
     #define LISP_IMPLEMENTATION
     #include "lisp.h"
     #include "lisp_lib.h"
     
 in at least one C or C++ file in order to generate the implementation.

 ----------------------
 QUICKSTART
 ----------------------

    LispContext ctx = lisp_init();
    lisp_load_lib(ctx);

    LispError error;
    Lisp program = lisp_read("(+ 1 2)", &error, ctx);
    Lisp result = lisp_eval(program, &error, ctx);

    if (error != LISP_ERROR_NONE)
        lisp_print(result);

    lisp_shutdown(ctx);

 ----------------------
 OPTIONS
 ----------------------

 These macros can be defined before include to configure options.

 // Build in debug mode with extra checks and logs:
 #define LISP_DEBUG

 // Change how much data is read from a file at a time.
 #define LISP_FILE_CHUNK_SIZE 8192
 */


#ifndef LISP_H
#define LISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

typedef enum
{
    LISP_NULL = 0,
    LISP_REAL,    // decimal/floating point type
    LISP_INT,     // integer type
    LISP_CHAR,    // ASCII character
    LISP_PAIR,    // cons pair (car, cdr)
    LISP_SYMBOL,  // unquoted strings
    LISP_STRING,  // quoted strings
    LISP_LAMBDA,  // user defined lambda
    LISP_FUNC,    // C function
    LISP_TABLE,   // key/value storage
    LISP_BOOL,    // t/f
    LISP_VECTOR,  // heterogenous array but contiguous allocation
    LISP_PROMISE, // lazy value
    LISP_JUMP,    // jump point/non-escaping continuation
    LISP_PTR,     // pointer to arbitary C object.
} LispType;

typedef double LispReal;
typedef long long LispInt;

typedef union
{
    int char_val;
    LispReal real_val;
    LispInt int_val;  
    void* ptr_val;
    void(*func_val)(void);
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
    LISP_ERROR_READ_SYNTAX,
    LISP_ERROR_FORM_SYNTAX,
    LISP_ERROR_UNDEFINED_VAR,
    LISP_ERROR_BAD_OP,
    LISP_ERROR_OUT_OF_BOUNDS,
    LISP_ERROR_ARG_TYPE,
    LISP_ERROR_TOO_MANY_ARGS,
    LISP_ERROR_TOO_FEW_ARGS,
    LISP_ERROR_RUNTIME,
} LispError;

typedef struct
{
    struct LispImpl* p;
} LispContext;

typedef Lisp(*LispCFunc)(Lisp, LispError*, LispContext);

// -----------------------------------------
// CONTEXT
// -----------------------------------------

LispContext lisp_init(void);
void lisp_shutdown(LispContext ctx);

// garbage collection. 
// this will free all objects which are not reachable from root_to_save or the global env
Lisp lisp_collect(Lisp root_to_save, LispContext ctx);
void lisp_print_collect_stats(LispContext ctx);
const char *lisp_error_string(LispError error);

void lisp_set_env(Lisp env, LispContext ctx);
Lisp lisp_env(LispContext ctx);

void lisp_set_stdin(FILE *file, LispContext ctx);
void lisp_set_stderr(FILE *file, LispContext ctx);
void lisp_set_stdout(FILE *file, LispContext ctx);

FILE *lisp_stdin(LispContext ctx);
FILE *lisp_stderr(LispContext ctx);
FILE *lisp_stdout(LispContext ctx);

// Macros
Lisp lisp_macro_table(LispContext ctx);
// the macro table keeps strong references to its members. 
// So if you want to delete them you need to make a new table.
void lisp_set_macro_table(Lisp table, LispContext ctx);

// -----------------------------------------
// REPL
// -----------------------------------------

// Reads text into raw s-expressions. 
Lisp lisp_read(const char *text, LispError* out_error, LispContext ctx);
Lisp lisp_read_file(FILE *file, LispError* out_error, LispContext ctx);
Lisp lisp_read_path(const char* path, LispError* out_error, LispContext ctx);

// evaluate a lisp expression
Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx);
Lisp lisp_eval2(Lisp expr, Lisp env, LispError* out_error, LispContext ctx);
Lisp lisp_apply(Lisp operator, Lisp args, LispError* out_error, LispContext ctx);
// Expands special Lisp forms and checks syntax (called by eval).
Lisp lisp_macroexpand(Lisp lisp, LispError* out_error, LispContext ctx);

// print out a lisp structure in 
void lisp_print(Lisp l);
void lisp_printf(FILE *file, Lisp l);

void lisp_displayf(FILE *file, Lisp l);

// Calls proc with an argument containing the current continuation.
Lisp lisp_call_cc(Lisp proc, LispError* out_error, LispContext ctx);

// -----------------------------------------
// PRIMITIVES
// -----------------------------------------
#define lisp_type(x) ((x).type)
#define lisp_eq(a, b) ((a).val.ptr_val == (b).val.ptr_val)
int lisp_equal(Lisp a, Lisp b);
int lisp_equal_r(Lisp a, Lisp b);
#define lisp_null() ((Lisp) { .val = { .ptr_val = NULL }, .type = LISP_NULL })

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
Lisp lisp_parse_int(const char *string);

Lisp lisp_make_real(LispReal x);
LispReal lisp_real(Lisp x);
Lisp lisp_parse_real(const char *string);

LispReal lisp_number_to_real(Lisp x);
LispInt lisp_number_to_int(Lisp x);

// Bools
Lisp lisp_make_bool(int t);
int lisp_bool(Lisp x);
#define lisp_true() (lisp_make_bool(1))
#define lisp_false() (lisp_make_bool(0))
int lisp_is_true(Lisp x);

// Characters
Lisp lisp_make_char(int c);
int lisp_char(Lisp l);
Lisp lisp_eof(void);

// Null terminated byte (ASCII) strings
Lisp lisp_make_string(int n, LispContext ctx);
Lisp lisp_make_string2(const char *c_string, LispContext ctx);
Lisp lisp_substring(Lisp s, int start, int end, LispContext ctx);

void lisp_string_fill(Lisp s, Lisp character); // inplace.
int lisp_string_ref(Lisp s, int i);
void lisp_string_set(Lisp s, int i, int c);
int lisp_string_length(Lisp s);
const char *lisp_string(Lisp s);

// Low level string storage
Lisp lisp_make_buffer(int cap, LispContext ctx);
Lisp lisp_buffer_copy(Lisp s, LispContext ctx);
void lisp_buffer_fill(Lisp s, int start, int end, int x);
char *lisp_buffer(Lisp s);
int lisp_buffer_capacity(Lisp s);

// Symbols (interned strings)
Lisp lisp_make_symbol(const char *string, LispContext ctx);
Lisp lisp_gen_symbol(LispContext ctx); // uninterned
const char *lisp_symbol_string(Lisp x);

// Arbitrary C objects.
Lisp lisp_make_ptr(void *ptr);
void *lisp_ptr(Lisp l);

// -----------------------------------------
// DATA STRUCTURES
// -----------------------------------------

// Lists
Lisp lisp_list_copy(Lisp x, LispContext ctx);
Lisp lisp_make_list(Lisp x, int n, LispContext ctx);
Lisp lisp_make_list2(Lisp *x, int n, LispContext ctx);

Lisp lisp_list_reverse(Lisp l); // O(n). inplace.
Lisp lisp_list_reverse2(Lisp l, Lisp tail); // O(n)
Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx); // O(n)
Lisp lisp_list_advance(Lisp l, int i); // O(n)
Lisp lisp_list_ref(Lisp l, int i); // O(n)
int lisp_list_length(Lisp l); // O(n)
int lisp_is_list(Lisp l); // O(n)

// Association lists "alists"
// Given a list of pairs ((key1 val1) (key2 val2) ... (keyN valN))
// returns the value with tgiven key.
Lisp lisp_alist_ref(Lisp l, Lisp key); // O(n)

// Vectors (like C arrays, but heterogeneous).
Lisp lisp_make_vector(int n, LispContext ctx); // Contents are uninitialized. Be careful.
Lisp lisp_make_vector2(Lisp *x, int n, LispContext ctx);

int lisp_vector_length(Lisp v);
Lisp lisp_vector_ref(Lisp v, int i);
void lisp_vector_set(Lisp v, int i, Lisp x); // inplace.
Lisp lisp_vector_swap(Lisp v, int i, int j); // inplace.
void lisp_vector_fill(Lisp v, Lisp x); // inplace.
Lisp lisp_vector_grow(Lisp v, int n, LispContext ctx);
Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx);

// association vector like "alist"
Lisp lisp_avector_ref(Lisp l, Lisp key); // O(n)

// Hash tables
Lisp lisp_make_table(LispContext ctx);
void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx);
Lisp lisp_table_get(Lisp t, Lisp key, int* present);
int lisp_table_size(Lisp t);
Lisp lisp_table_to_alist(Lisp t, LispContext ctx);

// -----------------------------------------
// LANGUAGE
// -----------------------------------------

// Lambdas (compound procedures)
Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx);
Lisp lisp_lambda_body(Lisp l);
Lisp lisp_lambda_env(Lisp l);

// C functions (compiled procedures)
Lisp lisp_make_func(LispCFunc func_ptr);
LispCFunc lisp_func(Lisp l);

// Convenience for defining many C functions at a time. 
typedef struct
{
    const char* name;
    LispCFunc func_ptr;
} LispFuncDef;
void lisp_table_define_funcs(Lisp t, const LispFuncDef* defs, LispContext ctx);

// Evaluation environments
Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx);
Lisp lisp_env_lookup(Lisp l, Lisp key, int *present);
void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx);
int lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx);
int lisp_is_env(Lisp l);

// Promises
Lisp lisp_make_promise(Lisp proc, LispContext ctx);
int lisp_promise_forced(Lisp p);
Lisp lisp_promise_val(Lisp p);
Lisp lisp_promise_proc(Lisp p);
void lisp_promise_store(Lisp p, Lisp x);

#ifndef LISP_FILE_CHUNK_SIZE
#define LISP_FILE_CHUNK_SIZE 8192
#endif

#ifndef LISP_PAGE_SIZE
#define LISP_PAGE_SIZE 512 * 1024
#endif

#ifndef LISP_STACK_DEPTH
#define LISP_STACK_DEPTH 1024
#endif

#ifndef LISP_IDENTIFIER_MAX
#define LISP_IDENTIFIER_MAX 1024
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifdef LISP_IMPLEMENTATION

#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)

enum
{
    GC_CLEAR = 0,
    GC_GONE = 1, 
    GC_NEED_VISIT = 2, 
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
            int length;
        } vector;

        struct
        {
            int length;
        } symbol;

        struct
        {
            uint8_t cached;
            uint8_t type;
        } promise;

        struct
        {
            uint8_t body_type;
            uint8_t args_type;
        } lambda;

        struct
        {
            int capacity;
        } string;
    } d;

    // 32
    uint8_t gc_state;
    uint8_t type;
} Block;

static void heap_init(Heap* heap)
{
    heap->bottom = page_create(LISP_PAGE_SIZE);
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
    heap->top = NULL;
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
    if (alloc_size >= LISP_PAGE_SIZE)
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
        to_use = page_create(LISP_PAGE_SIZE);
        heap->top->next = to_use;
        heap->top = to_use; 
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
    block->gc_state = GC_CLEAR;
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
    SYM_CONS,
    SYM_COUNT
};

struct LispImpl
{
    Heap heap;

    Lisp* stack;
    size_t stack_ptr;
    size_t stack_depth;

    FILE* out_port;
    FILE* err_port;
    FILE* in_port;

    Lisp symbols;
    Lisp env;
    Lisp macros;

    int symbol_counter;

    Lisp symbol_cache[SYM_COUNT];

    size_t gc_stat_freed;
    size_t gc_stat_time;
};

static Lisp get_sym(int sym, LispContext ctx) { return ctx.p->symbol_cache[sym]; }

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
    LispVal entries[];
} Vector;

static Lisp val_to_list_(LispVal x)
{
    return (Lisp) { x, x.ptr_val == NULL ? LISP_NULL : LISP_PAIR };
}

int lisp_equal(Lisp a, Lisp b)
{
    switch (a.type)
    {
        case LISP_NULL:
            return a.type == b.type;
        case LISP_BOOL:
            return lisp_bool(a) == lisp_bool(b) && a.type == b.type;
        case LISP_CHAR:
            return lisp_char(a) == lisp_char(b) && a.type == b.type;
        case LISP_FUNC:
            return lisp_func(a) == lisp_func(b) && a.type == b.type;
        case LISP_INT:
            if (b.type == LISP_INT) return lisp_int(a) == lisp_int(b);
            else return lisp_number_to_real(a) == lisp_number_to_real(b);
        case LISP_REAL:
            return lisp_real(a) == lisp_number_to_real(b);
        default:
            return a.val.ptr_val == b.val.ptr_val && a.type == b.type;
    }
}

int lisp_equal_r(Lisp a, Lisp b)
{
    switch (a.type)
    {
        case LISP_VECTOR:
        {
            if (a.type != b.type) return 0;
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
            if (a.type != b.type) return 0;
            while (lisp_is_pair(a) && lisp_is_pair(b))
            {
                if (!lisp_equal_r(lisp_car(a), lisp_car(b))) return 0;
                a = lisp_cdr(a);
                b = lisp_cdr(b);
            }
            
            return lisp_equal_r(a, b);
        }
        case LISP_STRING:
        {
            return a.type == b.type && strcmp(lisp_string(a), lisp_string(b)) == 0;
        }
        default:
            return lisp_equal(a, b);
    }
}

Lisp lisp_make_int(LispInt n)
{
    Lisp l;
    l.type = LISP_INT;
    l.val.int_val = n;
    return l;
}

LispInt lisp_int(Lisp x) { return x.val.int_val; }

Lisp lisp_parse_int(const char* string)
{
    return lisp_make_int((LispInt)strtol(string, NULL, 10));
}

Lisp lisp_make_bool(int t)
{
    LispVal val;
    val.char_val = t;
    return (Lisp) { val, LISP_BOOL };
}

int lisp_bool(Lisp x) { return x.val.char_val; }

int lisp_is_true(Lisp x)
{
     // In scheme everything which is not #f is true. 
     return (lisp_type(x) == LISP_BOOL && !lisp_bool(x)) ? 0 : 1;
}

Lisp lisp_make_real(LispReal x)
{
    return (Lisp) { .val.real_val = x, .type = LISP_REAL };
}

Lisp lisp_parse_real(const char* string)
{
    return lisp_make_real(strtod(string, NULL));
}

LispReal lisp_real(Lisp x) { return x.val.real_val; }

LispReal lisp_number_to_real(Lisp x)
{
    return lisp_type(x) == LISP_REAL ? x.val.real_val : (LispReal)lisp_int(x);
}

LispInt lisp_number_to_int(Lisp x)
{
    return lisp_type(x) == LISP_INT ? x.val.int_val : (LispInt)lisp_real(x);
}

static Pair* pair_get_(Lisp p)
{
    assert(p.type == LISP_PAIR);
    return p.val.ptr_val;
}

Lisp lisp_car(Lisp p)
{
    const Pair* pair = pair_get_(p); 
    return (Lisp) { pair->car, (LispType)pair->block.d.pair.car_type };
}

Lisp lisp_cdr(Lisp p)
{
    const Pair* pair = pair_get_(p); 
    return (Lisp) { pair->cdr, (LispType)pair->block.d.pair.cdr_type };
}

void lisp_set_car(Lisp p, Lisp x)
{
    Pair* pair = pair_get_(p); 
    pair->car = x.val;
    pair->block.d.pair.car_type = x.type;
}

void lisp_set_cdr(Lisp p, Lisp x)
{
    Pair* pair = pair_get_(p); 
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

Lisp lisp_list_copy(Lisp l, LispContext ctx)
{
    Lisp tail = lisp_null();
    while (lisp_is_pair(l))
    {
        tail = lisp_cons(lisp_car(l), tail, ctx);
        l = lisp_cdr(l);
    }
    return lisp_list_reverse(tail);
}

Lisp lisp_make_list(Lisp x, int n, LispContext ctx)
{
    Lisp tail = lisp_null();
    for (int i = 0; i < n; ++i)
        tail = lisp_cons(x, tail, ctx);
    return tail;
}

Lisp lisp_make_list2(Lisp *x, int n, LispContext ctx)
{
    Lisp tail = lisp_null();
    for (int i = n - 1; i >= 0; --i)
        tail = lisp_cons(x[i], tail, ctx);
    return tail;
}

Lisp lisp_list_reverse2(Lisp l, Lisp tail)
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

Lisp lisp_list_reverse(Lisp l) { return lisp_list_reverse2(l, lisp_null()); }

Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx)
{
    // (a b) (c) -> (a b c)
    l = lisp_list_reverse(lisp_list_copy(l, ctx));
    return lisp_list_reverse2(l, tail);
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
    return lisp_null();
}

int lisp_list_length(Lisp l)
{
    int n = 0;
    while (lisp_is_pair(l)) { ++n; l = lisp_cdr(l); }
    return n;
}

int lisp_is_list(Lisp l)
{
    if (lisp_is_null(l))
    {
        return 1;
    }
    else if (lisp_is_pair(l))
    {
        return lisp_is_list(lisp_cdr(l));
    }
    else
    {
        return 0;
    }
}

Lisp lisp_alist_ref(Lisp l, Lisp key)
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
    return lisp_false();
}

static int vector_len_(const Vector* v) { return v->block.d.vector.length; }

// types are stored in an array of bytes at the end of the data.
static char* vector_types_(Vector* v)
{
    // should be safe with aliasing.
    // https://gist.github.com/jibsen/da6be27cde4d526ee564
    char* base = (char*)v;
    return base + sizeof(Vector) + sizeof(LispVal) * vector_len_(v);
}

static Vector* vector_get_(Lisp v)
{
    assert(lisp_type(v) == LISP_VECTOR);
    return v.val.ptr_val;
}

Lisp lisp_make_vector(int n, LispContext ctx)
{
    size_t size = sizeof(Vector) + sizeof(LispVal) * n + sizeof(char) * n;
    Vector* vector = gc_alloc(size, LISP_VECTOR, ctx);
    vector->block.d.vector.length = n;

    LispVal val;
    val.ptr_val = vector;
    return (Lisp) { val, LISP_VECTOR };
}

Lisp lisp_make_vector2(Lisp *x, int n, LispContext ctx)
{
    Lisp v = lisp_make_vector(n, ctx);
    for (int i = 0; i < n; ++i)
        lisp_vector_set(v, i, x[i]);
    return v;
}

int lisp_vector_length(Lisp v) { return vector_len_(vector_get_(v)); }

Lisp lisp_vector_ref(Lisp v, int i)
{
    Vector* vector = vector_get_(v);
    assert(i < vector_len_(vector));
    Lisp x = { vector->entries[i], (LispType)(vector_types_(vector)[i]) };
    return x;
}

void lisp_vector_set(Lisp v, int i, Lisp x)
{
    Vector* vector = vector_get_(v);
    assert(i < vector_len_(vector));
    vector->entries[i] = x.val;
    vector_types_(vector)[i] = (char)x.type;
}

Lisp lisp_vector_swap(Lisp v, int i, int j)
{
    Lisp tmp = lisp_vector_ref(v, i);
    lisp_vector_set(v, i, lisp_vector_ref(v, j));
    lisp_vector_set(v, j, tmp);
    return v;
}

void lisp_vector_fill(Lisp v, Lisp x)
{
    int n = lisp_vector_length(v);
    Vector* vector = vector_get_(v);
    char* entry_types = vector_types_(vector);

    for (int i = 0; i < n; ++i)
    {
        vector->entries[i] = x.val;
        entry_types[i] = (char)x.type;
    }
}

Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx)
{
    assert(start <= end);
    Vector* src = vector_get_(old);

    int m = vector_len_(src);
    if (end > m) end = m;
    
    int n = end - start;
    Lisp new_v = lisp_make_vector(n, ctx);
    Vector* dst = vector_get_(new_v);
    memcpy(dst->entries, src->entries + start, sizeof(LispVal) * n);
    memcpy(vector_types_(dst), vector_types_(src) + start, sizeof(char) * n);
    return new_v;
}

Lisp lisp_vector_grow(Lisp v, int n, LispContext ctx)
{
    Vector* src = vector_get_(v);
    int m = vector_len_(src);
    assert(n >= m);

    if (n == m)
    {
        return v;
    }
    else
    {
        Lisp new_v = lisp_make_vector(n, ctx);
        Vector* dst = vector_get_(new_v);
        memcpy(dst->entries, src->entries, sizeof(LispVal) * m);
        memcpy(vector_types_(dst), vector_types_(src), sizeof(char) * m);
        return new_v;
    }
}

Lisp lisp_avector_ref(Lisp v, Lisp key)
{
    int n = lisp_vector_length(v);
    for (int i = 0; i < n; ++i)
    {
        Lisp pair = lisp_vector_ref(v, i);
        if (lisp_is_pair(pair) && lisp_eq(lisp_car(pair), key)) return pair;
    }
    return lisp_false();
}

static uint64_t hash_uint64(uint64_t x)
{
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

static uint64_t hash_val(LispVal x) { return hash_uint64((uint64_t)x.int_val); }

// hash table
// linked list chaining
typedef struct
{
    Block block;
    int size;
    int capacity;

    // vectors. uninitialized if capacity == 0.
    // entries of vals may be unitialized if the correspding key entry is.
    LispVal keys;
    LispVal vals;
} Table;

static Table *table_get_(Lisp t)
{
    assert(lisp_type(t) == LISP_TABLE);
    return t.val.ptr_val;
}

Lisp lisp_make_table(LispContext ctx)
{
    Table *table = gc_alloc(sizeof(Table), LISP_TABLE, ctx);
    table->size = 0;
    table->capacity = 0;

    LispVal x;
    x.ptr_val = table;
    return (Lisp) { x, LISP_TABLE };
}

static void table_grow_(Lisp t, size_t new_capacity, LispContext ctx)
{
    Table *table = table_get_(t);
    if (new_capacity < 16) new_capacity = 16;
    assert(IS_POW2(new_capacity));

    int old_capacity = table->capacity;
    Lisp old_keys = { table->keys, LISP_VECTOR };
    Lisp old_vals = { table->vals, LISP_VECTOR };

    table->capacity = new_capacity;
    int n = table->size;
    table->size = 0;

    // table vals are uninitialized.
    Lisp new_vals = lisp_make_vector(new_capacity, ctx);
    Lisp new_keys = lisp_make_vector(new_capacity, ctx);
    lisp_vector_fill(new_keys, lisp_null());
    table->vals = new_vals.val;
    table->keys = new_keys.val;

    for (int i = 0; i < old_capacity; ++i)
    {
        Lisp key = lisp_vector_ref(old_keys, i);
        if (!lisp_is_null(key))
            lisp_table_set(t, key, lisp_vector_ref(old_vals, i), ctx);
    }
    assert(n == table->size);
}

void lisp_table_set(Lisp t, Lisp key, Lisp x, LispContext ctx)
{ 
    Table *table = table_get_(t);
    if (2 * table->size >= table->capacity)
    {
        table_grow_(t, table->capacity * 2, ctx);
    }
    assert(2 * table->size < table->capacity);

    Lisp keys = { table->keys, LISP_VECTOR };
    Lisp vals = { table->vals, LISP_VECTOR };

    uint32_t i = hash_val(key.val);
    while (1)
    {
        i &= (table->capacity - 1);

        Lisp saved_key = lisp_vector_ref(keys, i);
        if (lisp_is_null(saved_key))
        {
            ++table->size;
            lisp_vector_set(keys, i, key);
            lisp_vector_set(vals, i, x);
            return;
        }
        else if (lisp_eq(saved_key, key))
        {
            lisp_vector_set(vals, i, x);
            return;
        }
        ++i;
    }
}

Lisp lisp_table_get(Lisp t, Lisp key, int* present)
{
    Table *table = table_get_(t);
    int capacity = table->capacity;
    if (capacity == 0)
    {
       *present = 0;
       return lisp_null();
    }

    Lisp keys = { table->keys, LISP_VECTOR };
    Lisp vals = { table->vals, LISP_VECTOR };

    uint32_t i = hash_val(key.val);
    while (1)
    {
        i &= (capacity - 1);

        Lisp saved_key = lisp_vector_ref(keys, i);

        if (lisp_is_null(saved_key))
        {
            *present = 0;
            return lisp_null();
        }
        else if (lisp_eq(saved_key, key))
        {
            *present = 1;
            return lisp_vector_ref(vals, i);
        }
        ++i;
    }
}

Lisp lisp_table_to_alist(Lisp t, LispContext ctx)
{
    const Table *table = table_get_(t);
    Lisp result = lisp_null();

    Lisp keys = { table->keys, LISP_VECTOR };
    Lisp vals = { table->vals, LISP_VECTOR };
    
    for (int i = 0; i < table->capacity; ++i)
    {
        Lisp key = lisp_vector_ref(keys, i);
        if (!lisp_is_null(key))
        {
            result = lisp_cons(lisp_cons(key, lisp_vector_ref(vals, i), ctx), result, ctx);
        }
    }
    return result;
}

int lisp_table_size(Lisp t) { return table_get_(t)->size; }

void lisp_table_define_funcs(Lisp t, const LispFuncDef* defs, LispContext ctx)
{
    while (defs->name)
    {
        lisp_table_set(t, lisp_make_symbol(defs->name, ctx), lisp_make_func(defs->func_ptr), ctx);
        ++defs;
    }
}

static String* string_get_(Lisp s)
{
    assert(lisp_type(s) == LISP_STRING);
    return s.val.ptr_val;
}

Lisp lisp_make_buffer(int cap, LispContext ctx)
{
    assert(cap >= 0);
    String* string = gc_alloc(sizeof(String) + cap, LISP_STRING, ctx);
    string->block.d.string.capacity = cap;
    
    LispVal val;
    val.ptr_val = string;
    return (Lisp){ val, string->block.type };
}

Lisp lisp_buffer_copy(Lisp s, LispContext ctx)
{
    int cap = lisp_buffer_capacity(s);
    Lisp b = lisp_make_buffer(cap, ctx);
    memcpy(lisp_buffer(b), lisp_buffer(s), cap);
    return b;
}

int lisp_buffer_capacity(Lisp s)
{
    return string_get_(s)->block.d.string.capacity;
}

void lisp_buffer_fill(Lisp s, int start, int end, int x)
{
    int n = lisp_buffer_capacity(s);
    if (start > n) start = n;
    if (end > n) end = n;
    memset(lisp_buffer(s) + start, x, end - start);
}

char *lisp_buffer(Lisp s) { return string_get_(s)->string; }
const char *lisp_string(Lisp s) { return lisp_buffer(s); }

Lisp lisp_make_string(int n, LispContext ctx)
{
    Lisp s = lisp_make_buffer(n + 1, ctx);
    lisp_buffer(s)[n] = '\0';
    return s;
}

Lisp lisp_make_string2(const char* c_string, LispContext ctx)
{
    size_t length = strlen(c_string);
    Lisp s = lisp_make_string(length, ctx);
    memcpy(lisp_buffer(s), c_string, length);
    return s; 
}

int lisp_string_length(Lisp s) { return strlen(lisp_string(s)); }

int lisp_string_ref(Lisp s, int i) {
    const String* str = string_get_(s);
    assert(i >= 0 && i < lisp_buffer_capacity(s));
    return (int)str->string[i]; 
}

void lisp_string_set(Lisp s, int i, int c)
{
    assert(c >= 0 && c <= 127);
    assert(i >= 0 && i < lisp_buffer_capacity(s));
    string_get_(s)->string[i] = (char)c;
}

Lisp lisp_substring(Lisp s, int start, int end, LispContext ctx)
{
    assert(start <= end);

    int count = start;
    const char *first = lisp_string(s);
    while (*first && count) {
        --count;
        ++first;
    }

    count = (end - start);
    const char *last = first;
    while (*last && count)
    {
        --count;
        ++last;
    }
    Lisp result = lisp_make_string(last - first, ctx);
    memcpy(lisp_buffer(result), first, last - first);
    return result;
}

Lisp lisp_make_char(int c)
{
    Lisp l;
    l.type = LISP_CHAR;
    l.val.char_val = c;
    return l;
}

int lisp_char(Lisp l) { return l.val.char_val; }
Lisp lisp_eof(void) { return lisp_make_char(-1); }

static uint64_t hash_bytes(const char *buffer, size_t n)
{
    uint64_t x = 0xcbf29ce484222325;
    for (size_t i = 0; i < n; i++)
    {
        x ^= buffer[i];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

typedef struct
{
    Block block;
    // built in linked list
    LispVal next;
    char text[];
} Symbol;

static Symbol* symbol_get_(Lisp x)
{
    assert(lisp_type(x) == LISP_SYMBOL);
    return x.val.ptr_val;
}
int  lisp_symbol_length(Lisp l) { return symbol_get_(l)->block.d.symbol.length; }
const char* lisp_symbol_string(Lisp l) { return symbol_get_(l)->text; }

static Lisp symbol_make_(const char* string, int length, LispContext ctx)
{
    Symbol* symbol = gc_alloc(sizeof(Symbol) + (length + 1), LISP_SYMBOL, ctx);
    memcpy(symbol->text, string, length);
    symbol->text[length] = '\0';
    symbol->next.ptr_val = NULL; 
    symbol->block.d.symbol.length = length;

    LispVal x;
    x.ptr_val = symbol;
    return (Lisp) { x, LISP_SYMBOL };
}

static Lisp symbol_intern_(Lisp table, const char* string, size_t length, LispContext ctx)
{
    uint64_t hash = hash_bytes(string, length);

    // the key in the hash table is the string hash
    Lisp key;
    key.type = LISP_INT;
    key.val.int_val = (LispInt)hash;

    // linked list chaining in the resulting value.
    int present;
    Lisp first_symbol = lisp_table_get(table, key, &present);

    // symbol found in linked list chain
    if (present)
    {
        Lisp it = first_symbol;
        while (it.val.ptr_val != NULL)
        {
            if (lisp_symbol_length(it) == length &&
                strncmp(lisp_symbol_string(it), string, length) == 0) return it;
            it.val = symbol_get_(it)->next;
        }
    }

    // new symbol
    Lisp symbol = symbol_make_(string, length, ctx);

    symbol_get_(symbol)->next = first_symbol.val;
    lisp_table_set(table, key, symbol, ctx);
    return symbol;
}

Lisp lisp_make_symbol(const char* string, LispContext ctx)
{
    assert(string);
    int length = strnlen(string, LISP_IDENTIFIER_MAX);
    return symbol_intern_(ctx.p->symbols, string, length, ctx);
}

Lisp lisp_gen_symbol(LispContext ctx)
{
    char text[64];
    int bytes = snprintf(text, 64, ":G%d", ctx.p->symbol_counter++);
    return symbol_make_(text, bytes, ctx);
}

Lisp lisp_make_ptr(void *ptr)
{
    return (Lisp) { .val = { .ptr_val = ptr }, .type = LISP_PTR };
}

void *lisp_ptr(Lisp l)
{
    assert(lisp_type(l) == LISP_PTR);
    return l.val.ptr_val;
}

Lisp lisp_make_func(LispCFunc func)
{
    Lisp l;
    l.type = LISP_FUNC;
    l.val.func_val = (void(*)(void))func;
    return l;
}

LispCFunc lisp_func(Lisp l)
{
    assert(lisp_type(l) == LISP_FUNC);
    return (LispCFunc)l.val.func_val;
}

typedef struct
{
    Block block;
    LispVal body;
    LispVal args;
    LispVal env;
} Lambda;

Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContext ctx)
{
    Lambda* lambda = gc_alloc(sizeof(Lambda), LISP_LAMBDA, ctx);
    lambda->block.d.lambda.body_type = (uint8_t)lisp_type(body);
    lambda->block.d.lambda.args_type = (uint8_t)lisp_type(args);

    assert(lisp_is_env(env));

    lambda->args = args.val;
    lambda->body = body.val;
    lambda->env = env.val;
    
    LispVal val;
    val.ptr_val = lambda;
    return (Lisp) { val, LISP_LAMBDA };
}

static Lambda* lambda_get_(Lisp l)
{
    assert(l.type == LISP_LAMBDA);
    return l.val.ptr_val;
}

Lisp lisp_lambda_body(Lisp l)
{
     const Lambda* lambda = lambda_get_(l);
     return (Lisp) { lambda->body, (LispType)lambda->block.d.lambda.body_type };
}

Lisp lambda_args_(Lisp l)
{
     const Lambda* lambda = lambda_get_(l);
     return (Lisp) { lambda->args, (LispType)lambda->block.d.lambda.args_type };
}

Lisp lisp_lambda_env(Lisp l)
{
    const Lambda* lambda = lambda_get_(l);
    return val_to_list_(lambda->env);
}

typedef struct
{
    Block block;
    LispVal val_or_proc;
} Promise;

Lisp lisp_make_promise(Lisp proc, LispContext ctx)
{
    assert(lisp_type(proc) == LISP_LAMBDA || lisp_type(proc) == LISP_FUNC);
    Promise* promise = gc_alloc(sizeof(Promise), LISP_PROMISE, ctx);
    promise->block.d.promise.cached = 0;
    promise->block.d.promise.type = lisp_type(proc);
    promise->val_or_proc = proc.val;

    LispVal val;
    val.ptr_val = promise;
    return (Lisp) { val, LISP_PROMISE };
}

static Promise* promise_get_(Lisp p)
{
    assert(p.type == LISP_PROMISE);
    return p.val.ptr_val;
}

void lisp_promise_store(Lisp p, Lisp x)
{
    Promise* promise = promise_get_(p); 
    assert(!promise->block.d.promise.cached);
    promise->block.d.promise.cached = 1;
    promise->block.d.promise.type = lisp_type(x);
    promise->val_or_proc = x.val;
}

int lisp_promise_forced(Lisp p)
{
    const Promise* promise = promise_get_(p); 
    return (int)promise->block.d.promise.cached;
}

static Lisp promise_body_or_val_(Lisp p)
{
    const Promise* promise = promise_get_(p); 
    LispType type = (LispType)promise->block.d.promise.type;
    return (Lisp) { promise->val_or_proc, type }; 
}

Lisp lisp_promise_proc(Lisp p)
{
    const Promise* promise = promise_get_(p); 
    assert(!promise->block.d.promise.cached);
    return promise_body_or_val_(p);
}

Lisp lisp_promise_val(Lisp p)
{
    const Promise* promise = promise_get_(p); 
    assert(promise->block.d.promise.cached);
    return promise_body_or_val_(p);
}

typedef struct
{
    Block block;
    Lisp result;
    jmp_buf jmp;
    int stack_ptr;
} Jump;

static Jump* jump_get_(Lisp x) {
    assert(x.type == LISP_JUMP);
    return x.val.ptr_val;
}

static Lisp make_jump_(LispContext ctx)
{
    Jump* j = gc_alloc(sizeof(Jump), LISP_JUMP, ctx);
    j->result = lisp_false();
    return (Lisp) { .val = { .ptr_val = j }, .type = LISP_JUMP };
}

// READER
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

    size_t position;
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
    lex->position = 0;
}

static void lexer_init_file(Lexer* lex, FILE* file)
{
    lex->file = file;

    lex->buff_size = LISP_FILE_CHUNK_SIZE;

     // double input buffering
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
    lex->position = 0;
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
    ++lex->position;

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

static int lexer_match_float(Lexer* lex)
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

    while (1)
    {
        switch (*lex->c)
        {
            case '"':
                lexer_step(lex);
                return 1;
            case '\\':
                lexer_step(lex);
                lexer_step(lex);
                break;
            case '\0':
            case '\n':
                return 0;
            default:
                lexer_step(lex);
        }
    }
}

static size_t lexer_copy_token(Lexer* lex, size_t start_index, size_t max_length, char* dest)
{
    size_t length;
    if (start_index + max_length > lex->scan_length)
        length = lex->scan_length - start_index;
    else
        length = max_length;

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
    return length;
}

static TokenType token_from_char(char c)
{
    switch (c)
    {
        case '\0': return TOKEN_NONE;
        case '(':  return TOKEN_L_PAREN;
        case ')':  return TOKEN_R_PAREN;
        case '.':  return TOKEN_DOT;
        case '\'': return TOKEN_QUOTE;
        case '`':  return TOKEN_BQUOTE;
        case ',':  return TOKEN_COMMA;
        case '@':  return TOKEN_AT;
        default:   return TOKEN_NONE;
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
        if (lexer_match_string(lex)) lex->token = TOKEN_STRING;
        else if (lexer_match_float(lex)) lex->token = TOKEN_FLOAT;
        else if (lexer_match_int(lex)) lex->token = TOKEN_INT;
        else if (lexer_match_symbol(lex)) lex->token = TOKEN_SYMBOL;
        else if (lexer_match_char(lex)) lex->token = TOKEN_CHAR;
        else if (lexer_match_bool(lex)) lex->token = TOKEN_BOOL;
        else if (lexer_match_hash_paren(lex)) lex->token = TOKEN_HASH_L_PAREN;
    }
}

// requires: length(out) >= (last - first)
static char* string_unescape_(const char* first, const char* last, char* out)
{
    // becase first >= out we can use this in place
    while (first != last)
    {
        if (*first == '\\')
        {
            ++first;
            switch (*first)
            {
                case '\\': *out = '\\'; break;
                case 'n': *out = '\n'; break;
                case 't': *out = '\t'; break;
                case 'f': *out = '\f'; break;
                case '"': *out = '"'; break;
                default: break;
            }
        }
        else
        {
            *out = *first;
        }
        ++out;
        ++first;
    }
    return out;
}

static void print_escaped_(const char* c, FILE* file)
{
    while (*c)
    {
        switch (*c)
        {
            case '\n':
                fputc('\\', file);
                fputc('n', file);
                break;
            case '\t':
                fputc('\\', file);
                fputc('t', file);
                break;
            case '\f':
                fputc('\\', file);
                fputc('f', file);
                break;
            case '\"':
                fputc('\\', file);
                fputc('"', file);
                break;
            default:
                fputc(*c, file);
                break;
        }
        ++c;
    }
}

static Lisp parse_number_(Lexer* lex, LispContext ctx)
{
    char scratch[128];
    size_t length = lexer_copy_token(lex, 0, 128, scratch);
    scratch[length] = '\0';

    switch (lex->token)
    {
        case TOKEN_INT:  return lisp_parse_int(scratch);
        case TOKEN_FLOAT: return lisp_parse_real(scratch);
        default: assert(0);
    }
}

static Lisp parse_string_(Lexer* lex, LispContext ctx)
{ 
    // -2 length to skip quotes
    size_t size = lex->scan_length - 2;
    Lisp l = lisp_make_buffer(size + 1, ctx);
    char* str = lisp_buffer(l);
    lexer_copy_token(lex, 1, size, str);
    char* out = string_unescape_(str, str + size, str);
    *out = '\0';
    return l;
}

static Lisp parse_symbol_(Lexer* lex, LispContext ctx)
{
    char scratch[LISP_IDENTIFIER_MAX];
    size_t length = lexer_copy_token(lex, 0, LISP_IDENTIFIER_MAX, scratch);
    // always convert symbols to uppercase
    for (int i = 0; i < length; ++i)
        scratch[i] = toupper(scratch[i]);
    return symbol_intern_(ctx.p->symbols, scratch, length, ctx);
}

static const char* ascii_char_name_table_[] =
{
    "EOF",
    "NUL", "SOH", "STX", "ETX", "EOT",
    "ENQ", "ACK", "BEL", "backspace", "tab",
    "newline", "VT", "page", "return", "SO",
    "SI", "DLE", "DC1", "DC2", "DC3",
    "DC4", "NAK", "SYN", "ETB", "CAN",
    "EM", "SUB", "altmode", "FS", "GS", "RS",
    "backnext", "space", NULL
};

static int parse_char_(Lexer* lex)
{
    char scratch[64];
    size_t length = lexer_copy_token(lex, 2, 64, scratch);  
    scratch[length] = '\0';

    if (length == 1)
    {
        return (int)scratch[0];
    }
    else
    {
        const char** name_it = ascii_char_name_table_;
        
        int i = 0;
        while (*name_it)
        {
            if (strcmp(*name_it, scratch) == 0) return i - 1;
            ++name_it;
            ++i;
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
            fprintf(ctx.p->err_port, "%lu. expected closing )\n", lex->position);
            longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
        case TOKEN_DOT:
            fprintf(ctx.p->err_port, "%lu. unexpected .\n", lex->position);
            longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
        case TOKEN_L_PAREN:
        {
            Lisp tail = lisp_null();

            // (
            lexer_next_token(lex);
            while (lex->token != TOKEN_R_PAREN && lex->token != TOKEN_DOT)
            {
                tail = lisp_cons(parse_list_r(lex, error_jmp, ctx), tail, ctx);
                lexer_next_token(lex);
            }

            // A dot at the end of a list assigns the cdr
            if (lex->token == TOKEN_DOT)
            {
                if (lisp_is_null(tail))
                {
                    fprintf(ctx.p->err_port, "%lu. unexpected .\n", lex->position);
                    longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
                }

                lexer_next_token(lex);
                if (lex->token != TOKEN_R_PAREN)
                {
                    Lisp x = parse_list_r(lex, error_jmp, ctx);
                    tail = lisp_list_reverse2(tail, x);
                    lexer_next_token(lex);
                }
            }
            else
            {
                tail = lisp_list_reverse(tail);
            }

            if (lex->token != TOKEN_R_PAREN)
            {
                fprintf(ctx.p->err_port, "%lu. expected closing )\n", lex->position);
                longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
            }
            // )
            return tail;
        }
        case TOKEN_R_PAREN:
            fprintf(ctx.p->err_port, "unexpected )\n");
            longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
        case TOKEN_BOOL:
        {
            char c;
            lexer_copy_token(lex, 1, 1, &c);
            return lisp_make_bool(c == 't' ? 1 : 0);
        }
        case TOKEN_HASH_L_PAREN:
        {
            // #(
            lexer_next_token(lex);

            Lisp* buffer = NULL;
            size_t buffer_cap = 0;

            int n = 0;
            while (lex->token != TOKEN_R_PAREN)
            {
                Lisp x = parse_list_r(lex, error_jmp, ctx);
                lexer_next_token(lex);

                if (buffer_cap <= n + 1)
                {
                    buffer_cap *= 2;
                    if (buffer_cap < 16) buffer_cap = 16;
                    buffer = realloc(buffer, buffer_cap * sizeof(Lisp));
                }
                buffer[n] = x;
                ++n;
            }
            // )
            
            Lisp v =  lisp_make_vector2(buffer, n, ctx);
            if (buffer) free(buffer);
            return v;
        }
        case TOKEN_FLOAT:
        case TOKEN_INT:
            return parse_number_(lex, ctx);
        case TOKEN_STRING:
            return parse_string_(lex, ctx);
        case TOKEN_SYMBOL:
            return parse_symbol_(lex, ctx);
        case TOKEN_CHAR:
        {
            int c = parse_char_(lex);
            if (c <= 0)
            {
                fprintf(ctx.p->err_port, "%lu. unknown character\n", lex->position);
                longjmp(error_jmp, LISP_ERROR_READ_SYNTAX);
            }
            return lisp_make_char(c);
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
             Lisp l = lisp_cons(parse_list_r(lex, error_jmp, ctx), lisp_null(), ctx);
             //lexer_next_token(lex);
             return lisp_cons(get_sym(quote_type, ctx), l, ctx);
        }
        default:
            assert(0);
    }
}

static Lisp parse(Lexer* lex, LispError* out_error, LispContext ctx)
{
    jmp_buf error_jmp;
    LispError error = setjmp(error_jmp);

    if (error != LISP_ERROR_NONE)
    {
        if (out_error) *out_error = error;
        return lisp_null();
    }

    lexer_next_token(lex);
    if (lex->token == TOKEN_NONE) return lisp_eof();

    Lisp result = parse_list_r(lex, error_jmp, ctx);
    lexer_next_token(lex);
    
    if (lex->token != TOKEN_NONE)
    {
        // MULTIPLE FORMS
        result = lisp_cons(result, lisp_null(), ctx);
        
        while (lex->token != TOKEN_NONE)
        {
            result = lisp_cons(parse_list_r(lex, error_jmp, ctx), result, ctx);
            lexer_next_token(lex);
        } 

        result = lisp_cons(get_sym(SYM_BEGIN, ctx), lisp_list_reverse(result), ctx);
    }

    if (out_error) *out_error = error;
    return result;
}

Lisp lisp_read(const char *program, LispError* out_error, LispContext ctx)
{
    Lexer lex;
    lexer_init(&lex, program);
    Lisp l = parse(&lex, out_error, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_file(FILE *file, LispError* out_error, LispContext ctx)
{
    Lexer lex;
    lexer_init_file(&lex, file);
    Lisp l = parse(&lex, out_error, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_path(const char *path, LispError* out_error, LispContext ctx)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        *out_error = LISP_ERROR_FILE_OPEN;
        return lisp_eof();
    }
    Lisp l = lisp_read_file(file, out_error, ctx);
    fclose(file);
    return l;
}

Lisp lisp_env_extend(Lisp l, Lisp table, LispContext ctx) { return lisp_cons(table, l, ctx); }

Lisp lisp_env_lookup(Lisp l, Lisp key, int *present)
{
    while (lisp_is_pair(l))
    {
        Lisp x = lisp_table_get(lisp_car(l), key, present);
        if (*present) return x;
        l = lisp_cdr(l);
    }
    
    return lisp_null();
}

void lisp_env_define(Lisp l, Lisp key, Lisp x, LispContext ctx)
{
    lisp_table_set(lisp_car(l), key, x, ctx);
}

int lisp_env_set(Lisp l, Lisp key, Lisp x, LispContext ctx)
{
    int present;
    while (lisp_is_pair(l))
    {
        lisp_table_get(lisp_car(l), key, &present);
        if (present)
        {
            lisp_table_set(lisp_car(l), key, x, ctx);
            return 1;
        }  
        l = lisp_cdr(l);
    }

    return 0; 
}

int lisp_is_env(Lisp l) { return lisp_is_list(l); }

static void lisp_print_r(FILE* file, Lisp l, int human_readable, int is_cdr)
{
    switch (lisp_type(l))
    {
        case LISP_INT: fprintf(file, "%lli", lisp_int(l)); break;
        case LISP_REAL: fprintf(file, "%f", lisp_real(l)); break;
        case LISP_NULL: fputs("NIL", file); break;
        case LISP_SYMBOL: fputs(lisp_symbol_string(l), file); break;
        case LISP_BOOL:
            fprintf(file, "#%c", lisp_bool(l) == 0 ? 'f' : 't');
            break;
        case LISP_STRING:
            if (human_readable)
            {
                fputs(lisp_string(l), file);
            }
            else
            {
                fputc('"', file);
                print_escaped_(lisp_string(l), file);
                fputc('"', file);
            }
            break;
        case LISP_CHAR:
        {
            int c = lisp_int(l);

            if (human_readable)
            {
                if (c >= 0) fputc(c, file);
            }
            else
            {
                if (c >= -1 && c < 33)
                {
                    fprintf(file, "#\\%s", ascii_char_name_table_[c + 1]);
                }
                else if (isprint(c))
                {
                    fprintf(file, "#\\%c", (char)c);
                }
                else
                {
                    fprintf(file, "#\\+%d", c);
                }
            }
            break;
        }
        case LISP_JUMP: fputs("<jump>", file); break;
        case LISP_LAMBDA: fputs("<lambda>", file); break;
        case LISP_PROMISE: fputs("<promise>", file); break;
        case LISP_PTR: fputs("<ptr-%p>", l.val.ptr_val); break;
        case LISP_FUNC: fprintf(file, "<c-func-%p>", l.val.ptr_val); break;
        case LISP_TABLE:
        {
            const Table* table = table_get_(l);
            fprintf(file, "{");

            Lisp keys = { table->keys, LISP_VECTOR };
            Lisp vals = { table->vals, LISP_VECTOR };
            for (int i = 0; i < table->capacity; ++i)
            {
                Lisp key = lisp_vector_ref(keys, i);
                if (!lisp_is_null(key))
                {
                    Lisp val = lisp_vector_ref(vals, i);

                    lisp_print_r(file, key, human_readable, 0);
                    fprintf(file, ": ");
                    lisp_print_r(file, val, human_readable, 0);
                    fprintf(file, " ");
                }
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
                lisp_print_r(file, lisp_vector_ref(l, i), human_readable, 0);
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
            lisp_print_r(file, lisp_car(l), human_readable, 0);

            if (lisp_type(lisp_cdr(l)) != LISP_PAIR)
            {
                if (!lisp_is_null(lisp_cdr(l)))
                { 
                    fprintf(file, " . ");
                    lisp_print_r(file, lisp_cdr(l), human_readable, 0);
                }

                fprintf(file, ")");
            }
            else
            {
                fprintf(file, " ");
                lisp_print_r(file, lisp_cdr(l), human_readable, 1);
            } 
            break;
        }
        default:
            // TODO
            fprintf(stderr, "printing unknown lisp type: %d\n", lisp_type(l));
            break;
    }
}

void lisp_printf(FILE* file, Lisp l) { lisp_print_r(file, l, 0, 0);  }
void lisp_print(Lisp l) {  lisp_printf(stdout, l); }

void lisp_displayf(FILE* file, Lisp l) { lisp_print_r(file, l, 1, 0); }

void lisp_set_stdout(FILE* file, LispContext ctx) { ctx.p->out_port = file; }
void lisp_set_stdin(FILE* file, LispContext ctx) { ctx.p->in_port = file; }
void lisp_set_stderr(FILE* file, LispContext ctx) { ctx.p->err_port = file; }

FILE *lisp_stdin(LispContext ctx) { return ctx.p->in_port; }
FILE *lisp_stderr(LispContext ctx) { return ctx.p->err_port; }
FILE *lisp_stdout(LispContext ctx) { return ctx.p->out_port; }

static void lisp_stack_push(Lisp x, LispContext ctx)
{
#ifdef LISP_DEBUG
    if (ctx.p->stack_ptr + 1 >= ctx.p->stack_depth)
    {
        fprintf(ctx.p->err_port, "stack overflow\n");
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
        fprintf(ctx.p->err_port, "stack underflow\n");
    }
#endif
    return ctx.p->stack[ctx.p->stack_ptr];
}

static Lisp* lisp_stack_peek(size_t i, LispContext ctx)
{
    return ctx.p->stack + (ctx.p->stack_ptr - i);
}

Lisp lisp_call_cc(Lisp proc, LispError* out_error, LispContext ctx)
{
    Lisp j = make_jump_(ctx);
    Jump* jump = jump_get_(j);
    jump->stack_ptr = ctx.p->stack_ptr;

    int has_result = setjmp(jump->jmp);
    if (has_result)
    {
        // restore jump from the stack
        jump = jump_get_(lisp_stack_pop(ctx));
        ctx.p->stack_ptr = jump->stack_ptr;
        return jump->result;
    }
    else
    {
        return lisp_apply(proc, lisp_cons(j, lisp_null(), ctx), out_error, ctx);
    }
}

// returns whether the result is final, or needs to be eval'd.
static int apply(Lisp operator, Lisp args, Lisp* out_result, Lisp* out_env, LispError* error, LispContext ctx)
{
    switch (lisp_type(operator))
    {
        case LISP_LAMBDA: // lambda call (compound procedure)
        {
            Lisp slot_names = lambda_args_(operator);
            *out_env = lisp_lambda_env(operator);

            // make a new environment
            Lisp new_table = lisp_make_table(ctx);
            
            // bind parameters to arguments
            // to pass into function call
            while (lisp_is_pair(slot_names) && lisp_is_pair(args))
            {
                lisp_table_set(new_table, lisp_car(slot_names), lisp_car(args), ctx);
                slot_names = lisp_cdr(slot_names);
                args = lisp_cdr(args);
            }

            if (lisp_type(slot_names) == LISP_SYMBOL)
            {
                // variable length arguments
                lisp_table_set(new_table, slot_names, args, ctx);
            }
            else if (!lisp_is_null(slot_names))
            {
                *error = LISP_ERROR_TOO_FEW_ARGS;
                return 0;
            }
            else if (!lisp_is_null(args))
            {
                *error = LISP_ERROR_TOO_MANY_ARGS;
                return 0;
            }

            // extend the environment
            *out_env = lisp_env_extend(*out_env, new_table, ctx);

            // normally we would eval the body here
            // but while will eval
            *out_result = lisp_lambda_body(operator);
            return 1;
        }
        case LISP_FUNC: // call into C functions
        {
            // no environment required
            LispCFunc f = lisp_func(operator);
            *out_result = f(args, error, ctx);
            return 0;
        }
        case LISP_JUMP:
        {
            Jump* jump = jump_get_(operator);
            jump->result = lisp_car(args);
            // put jump on the stack
            lisp_stack_push(operator, ctx);
            longjmp(jump->jmp, 1);
        }
        default:
        {
            lisp_printf(ctx.p->err_port, operator);
            fprintf(ctx.p->err_port, " is not an operator.\n");

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
        switch (lisp_type(*x))
        {
            case LISP_SYMBOL: // variable reference
            {
                int present;
                Lisp val = lisp_env_lookup(*env, *x, &present);
                if (!present)
                {
                    fprintf(ctx.p->err_port, "%s is not defined.\n", lisp_symbol_string(*x));
                    longjmp(error_jmp, LISP_ERROR_UNDEFINED_VAR); 
                    return lisp_null();
                }
                return val;
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
                    return lisp_null();
                }
                else if (lisp_eq(op_sym, get_sym(SYM_SET, ctx)) && op_valid)
                {
                    assert(!lisp_is_null(*env));
                    // mutablity
                    // like def, but requires existence
                    // and will search up the environment chain
                    
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(lisp_list_ref(*x, 2), ctx);
                    
                    Lisp value = eval_r(error_jmp, ctx);
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    Lisp symbol = lisp_list_ref(*x, 1);
                    if (!lisp_env_set(*env, symbol, value, ctx))
                    { 
                        fprintf(ctx.p->err_port, "error: unknown variable: %s\n", lisp_symbol_string(symbol));
                    }
                    return lisp_null();
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
                    
                    Lisp operator_expr = lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    lisp_stack_push(operator, ctx);
                    lisp_stack_push(operator_expr, ctx);
                    
                    Lisp arg_expr = lisp_cdr(*x);
                    
                    Lisp args = lisp_null();
                    
                    while (lisp_is_pair(arg_expr))
                    {
                        // save next
                        lisp_stack_push(lisp_cdr(arg_expr), ctx);
                        lisp_stack_push(args, ctx);

                        lisp_stack_push(*env, ctx);
                        lisp_stack_push(lisp_car(arg_expr), ctx);
                        Lisp new_arg = eval_r(error_jmp, ctx);
                        lisp_stack_pop(ctx);
                        lisp_stack_pop(ctx);

                        args = lisp_cons(new_arg, lisp_stack_pop(ctx), ctx);
                        arg_expr = lisp_stack_pop(ctx);
                    }
                    
                    operator_expr = lisp_stack_pop(ctx);
                    operator = lisp_stack_pop(ctx);
                    
                    LispError error = LISP_ERROR_NONE;
                    int needs_to_eval = apply(operator, lisp_list_reverse(args), x, env, &error, ctx);
                    if (error != LISP_ERROR_NONE)
                    {
                        if (lisp_type(operator_expr) == LISP_SYMBOL)
                        {
                            fprintf(ctx.p->err_port, "operator: %s\n", lisp_symbol_string(operator_expr));
                        }

                        longjmp(error_jmp, error);
                    }

                    if (!needs_to_eval)
                    {
                        return *x;
                    }
                    // Otherwise while will eval
                }
                break;
            }
            default:
               return *x; // atom
        }
    }
}

static Lisp expand_quasi_r(Lisp l, jmp_buf error_jmp, LispContext ctx)
{
    if (lisp_type(l) != LISP_PAIR)
    {
        Lisp terms[] = { get_sym(SYM_QUOTE, ctx), l };
        return lisp_make_list2(terms, 2, ctx);
    }

    Lisp op = lisp_car(l);
    int op_valid = lisp_type(op) == LISP_SYMBOL;

    if (lisp_eq(op, get_sym(SYM_UNQUOTE, ctx)) && op_valid)
    {
        return lisp_car(lisp_cdr(l));
    }
    else if (lisp_eq(op, get_sym(SYM_UNQUOTE_SPLICE, ctx)) && op_valid)
    {
        fprintf(ctx.p->err_port, "slicing ,@ must be in a backquoted list.\n");
        longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
    }
    else
    {
        Lisp terms[] = {
            get_sym(SYM_CONS, ctx),
            expand_quasi_r(lisp_car(l), error_jmp, ctx),
            expand_quasi_r(lisp_cdr(l), error_jmp, ctx),
        };
        return lisp_make_list2(terms, 3, ctx);
    }
}

static Lisp expand_r(Lisp l, jmp_buf error_jmp, LispContext ctx)
{
    if (lisp_type(l) != LISP_PAIR) return l;

    // 1. expand extended syntax into primitive syntax
    // 2. perform optimizations
    // 3. check syntax    

    Lisp op = lisp_car(l);
    if (lisp_type(op) == LISP_SYMBOL)
    {
        if (lisp_eq(op, get_sym(SYM_QUOTE, ctx)))
        {
            // don't expand quotes
            if (lisp_list_length(l) != 2)
            {
                fprintf(ctx.p->err_port, "(quote x)\n");
                longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
            }
            return l;
        }
        else if (lisp_eq(op, get_sym(SYM_QUASI_QUOTE, ctx)))
        {
            return expand_quasi_r(lisp_car(lisp_cdr(l)), error_jmp, ctx);
        }
        else if (lisp_eq(op, get_sym(SYM_DEFINE_MACRO, ctx)))
        {
            if (lisp_list_length(l) != 3)
            {
                fprintf(ctx.p->err_port, "(define-macro name proc)\n");
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
                fprintf(ctx.p->err_port, "(define-macro name proc) not a procedure\n");
                longjmp(error_jmp, LISP_ERROR_FORM_SYNTAX);
            } 

            lisp_table_set(ctx.p->macros, symbol, lambda, ctx);
            return lisp_null();
        }
        else 
        {
            int present;
            Lisp proc = lisp_table_get(ctx.p->macros, op, &present);

            if (present)
            {
                // EXPAND MACRO

                // TODO: need to make sure collection is not triggered
                // while evaling a macro.
                Lisp result;
                Lisp calling_env;
                LispError error = LISP_ERROR_NONE;
                if (apply(proc, lisp_cdr(l), &result, &calling_env, &error, ctx) == 1)
                {
                    result = lisp_eval2(result, calling_env, &error, ctx);
                }

                if (error != LISP_ERROR_NONE)
                {
                    fprintf(ctx.p->out_port, "macroexpand failed: %s\n", lisp_symbol_string(op));
                    longjmp(error_jmp, error);
                }
                return expand_r(result, error_jmp, ctx);
            }
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
        return lisp_null();
    }
}

Lisp lisp_eval2(Lisp l, Lisp env, LispError* out_error, LispContext ctx)
{
    LispError error;
    Lisp expanded = lisp_macroexpand(l, &error, ctx);
    
    if (error != LISP_ERROR_NONE)
    {
        if (out_error) *out_error = error;
        return lisp_null();
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

        return lisp_null();
    }
}

Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx)
{
    return lisp_eval2(expr, lisp_env(ctx), out_error, ctx);
}

Lisp lisp_apply(Lisp operator, Lisp args, LispError* out_error, LispContext ctx)
{
    // TODO: argument passing is a little more sophisitaed
    // No environment required. procedures always bring their own enviornment
    // to the call.
    Lisp x;
    Lisp env;
    int needs_to_eval = apply(operator, args, &x, &env, out_error, ctx);
    if (*out_error != LISP_ERROR_NONE) return lisp_false();
    return needs_to_eval ? lisp_eval2(x, env, out_error, ctx) : x;
}

static Lisp gc_move(Lisp x, LispContext ctx)
{
    switch (x.type)
    {
        case LISP_PAIR:
        case LISP_STRING:
        case LISP_LAMBDA:
        case LISP_VECTOR:
        case LISP_PROMISE:
        case LISP_TABLE:
        case LISP_SYMBOL:
        {
            Block* block = x.val.ptr_val;
            if (block->gc_state == GC_CLEAR)
            {
                // copy the data to new block
                Block* dest = heap_alloc(block->info.size, block->type, &ctx.p->heap);
                memcpy(dest, block, block->info.size);
                dest->gc_state = GC_NEED_VISIT;
                
                // save forwarding address (offset in to)
                block->info.forward = dest;
                block->gc_state = GC_GONE;
            }

            assert(block->gc_state == GC_GONE);
            
            // return the moved block address
            x.val.ptr_val = block->info.forward;
            return x;
        }
        default:
            return x;
    }
}

static LispVal gc_move_val(LispVal val, LispType type, LispContext ctx)
{
    return gc_move( (Lisp) { val, type}, ctx).val;
}

static void gc_move_v(Lisp* start, int n, LispContext ctx)
{
    for (int i = 0; i < n; ++i) start[i] = gc_move(start[i], ctx);
}

static Lisp gc_move_weak_symbols(Lisp old_table, LispContext ctx)
{
    // move symbol table (weak references)
    Table* from = table_get_(old_table);
    Lisp to_table = lisp_make_table(ctx);
    // preallocate
    int cap = from->capacity;
    table_grow_(to_table, cap, ctx);

    Lisp hashes = { from->keys, LISP_VECTOR };
    Lisp symbols = { from->vals, LISP_VECTOR };

    for (int i = 0; i < cap; ++i)
    {
        Lisp hash = lisp_vector_ref(hashes, i); 
        if (!lisp_is_null(hash))
        {
            Lisp old_symbol = lisp_vector_ref(symbols, i);
            while (old_symbol.val.ptr_val != NULL)
            {
                if (symbol_get_(old_symbol)->block.gc_state == GC_GONE)
                {
                    Lisp to_insert = gc_move(old_symbol, ctx);
                    int present;
                    Lisp existing = lisp_table_get(to_table, hash, &present);
                    symbol_get_(to_insert)->next = existing.val;
                    lisp_table_set(to_table, hash, to_insert, ctx);
                }
                else
                {
#ifdef LISP_DEBUG
                    //printf("losing symbol: %s\n", lisp_symbol_string(old_symbol));
#endif
                }
                old_symbol.val = symbol_get_(old_symbol)->next;
            }
        }
    }
    return to_table;
}

Lisp lisp_collect(Lisp root_to_save, LispContext ctx)
{
    time_t start_time = clock();

    // copy of old heap
    Heap from = ctx.p->heap;

    // make new heap to allocate and copy to
    heap_init(&ctx.p->heap);

    // move root object
    ctx.p->env = gc_move(ctx.p->env, ctx);
    ctx.p->macros = gc_move(ctx.p->macros, ctx);

    gc_move_v(ctx.p->symbol_cache, SYM_COUNT, ctx);
    gc_move_v(ctx.p->stack, ctx.p->stack_ptr, ctx);

    Lisp result = gc_move(root_to_save, ctx);

    // move references
    const Page* page = ctx.p->heap.bottom;
    int page_counter = 0;
    while (page)
    {
        size_t offset = 0;
        while (offset < page->size)
        {
            Block* block = (Block*)(page->buffer + offset);
            if (block->gc_state == GC_NEED_VISIT)
            {
                switch (block->type)
                {
                    // these add &to the buffer!
                    // so lists are handled in a single pass
                    case LISP_PAIR:
                    {
                        // move the CAR and CDR
                        Pair* p = (Pair*)block;
                        p->car = gc_move_val(p->car, p->block.d.pair.car_type, ctx);
                        p->cdr = gc_move_val(p->cdr, p->block.d.pair.cdr_type, ctx);
                        break;
                    }
                    case LISP_VECTOR:
                    {
                        Lisp vector;
                        vector.val.ptr_val = block;
                        vector.type = LISP_VECTOR;

                        Vector* v = (Vector*)block;
                        int n = vector_len_(v);
                        for (int i = 0; i < n; ++i)
                            v->entries[i] = gc_move(lisp_vector_ref(vector, i), ctx).val;
                        break;
                    }
                    case LISP_LAMBDA:
                    {
                        // move the body and args
                        Lambda* l = (Lambda*)block;
                        l->args = gc_move_val(l->args, (LispType)l->block.d.lambda.args_type, ctx);
                        l->body = gc_move_val(l->body, (LispType)l->block.d.lambda.body_type, ctx);
                        l->env = gc_move_val(l->env, l->env.ptr_val == NULL ? LISP_NULL : LISP_PAIR, ctx);
                        break;
                    }
                    case LISP_PROMISE:
                    {
                        Promise* p = (Promise*)block;
                        p->val_or_proc = gc_move_val(p->val_or_proc, (LispType)p->block.d.promise.type, ctx);
                        break;
                    }
                    case LISP_TABLE:
                    {

                        // During garbage collection all pointers change INCLUDING symbols,
                        // so that means if a symbol pointer is being used as a key, it is no
                        // longer in the correct place in the hash table.
                        // So we have to move it to a new place during garbage collection.
                        Lisp table;
                        table.val.ptr_val = block;
                        table.type = LISP_TABLE;

                        Table* t = (Table*)block;
                        int n = t->capacity;

                        Lisp keys = { t->keys, LISP_VECTOR };
                        Lisp vals = { t->vals, LISP_VECTOR };

                        for (int i = 0; i < n; ++i)
                        {
                            // move all the values, but borrow the old table for now.
                            Lisp key = lisp_vector_ref(keys, i); 
                            if (!lisp_is_null(key))
                            {
                                lisp_vector_set(keys, i, gc_move(key, ctx));
                                lisp_vector_set(vals, i, gc_move(lisp_vector_ref(vals, i), ctx));
                            }
                        }
                        // create new table and move the values in place.
                        table_grow_(table, n, ctx);
                        break;
                     }
                    default: break;
                }
                block->gc_state = GC_CLEAR;
            }
            offset += block->info.size;
        }
        page = page->next;
        ++page_counter;
    }
    // check that we visited all the pages
    assert(page_counter == ctx.p->heap.page_count);
    ctx.p->symbols = gc_move_weak_symbols(ctx.p->symbols, ctx);
    
#ifdef LISP_DEBUG
     {
          // DEBUG, check offsets
          const Page* page = ctx.p->heap.bottom;
          while (page)
          {
              size_t offset = 0;
              while (offset < page->size)
              {
                  Block* block = (Block*)(page->buffer + offset);
                  assert(block->gc_state == GC_CLEAR);
                  assert(block->info.size <= page->size);
                  assert(block->info.size % sizeof(LispVal) == 0);
                  offset += block->info.size;
              }
              assert(offset == page->size);
              page = page->next;
          }
    }
#endif
    
    size_t diff = from.size - ctx.p->heap.size;

    // swap the heaps
    heap_shutdown(&from);
    
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
    fprintf(ctx.p->out_port, "\ngc collected: %lu\t time: %lu us\n", ctx.p->gc_stat_freed, ctx.p->gc_stat_time);
    fprintf(ctx.p->out_port, "heap size: %lu\t pages: %lu\n", ctx.p->heap.size, ctx.p->heap.page_count);
    fprintf(ctx.p->out_port, "symbols: %lu \n", (size_t)lisp_table_size(ctx.p->symbols));
}

Lisp lisp_env(LispContext ctx) { return ctx.p->env; }

void lisp_set_env(Lisp env, LispContext ctx)
{
    assert(lisp_is_env(env));
    ctx.p->env = env;
}

Lisp lisp_macro_table(LispContext ctx) { return ctx.p->macros; }

void lisp_set_macro_table(Lisp table, LispContext ctx)
{
    assert(lisp_type(table) == LISP_TABLE);
    ctx.p->macros = table;
}

const char* lisp_error_string(LispError error)
{
    switch (error)
    {
        case LISP_ERROR_NONE:
            return "none";
        case LISP_ERROR_FILE_OPEN:
            return "file error: could not open file";
        case LISP_ERROR_READ_SYNTAX:
            return "read/syntax error.";
        case LISP_ERROR_FORM_SYNTAX:
            return "expand error: bad special form";
        case LISP_ERROR_UNDEFINED_VAR:
            return "eval error: undefined variable";
        case LISP_ERROR_BAD_OP:
            return "eval error: attempt to apply something which was not an operator";
        case LISP_ERROR_ARG_TYPE:
            return "eval error: invalid argument type";
        case LISP_ERROR_TOO_MANY_ARGS:
            return "eval error: too many arguments";
         case LISP_ERROR_TOO_FEW_ARGS:
            return "eval error: missing arguments";
        case LISP_ERROR_OUT_OF_BOUNDS:
            return "eval error: index out of bounds";
        case LISP_ERROR_RUNTIME:
            return "evaluation called (error) and it was not handled";
        default:
            return "unknown error code";
    }
}

LispContext lisp_init(void)
{
    LispContext ctx;
    ctx.p = malloc(sizeof(struct LispImpl));
    if (!ctx.p) return ctx;

    ctx.p->out_port = stdout;
    ctx.p->in_port = stdin;
    ctx.p->err_port = stderr;

    ctx.p->symbol_counter = 0;
    ctx.p->stack_ptr = 0;
    ctx.p->stack_depth = LISP_STACK_DEPTH;
    ctx.p->stack = malloc(sizeof(Lisp) * LISP_STACK_DEPTH);
    ctx.p->gc_stat_freed = 0;
    ctx.p->gc_stat_time = 0;
    
    heap_init(&ctx.p->heap);

    ctx.p->symbols = lisp_make_table(ctx);
    ctx.p->env = lisp_null();
    ctx.p->macros = lisp_make_table(ctx);

    Lisp* c = ctx.p->symbol_cache;
    c[SYM_IF] = lisp_make_symbol("IF", ctx);
    c[SYM_BEGIN] = lisp_make_symbol("BEGIN", ctx);
    c[SYM_QUOTE] = lisp_make_symbol("QUOTE", ctx);
    c[SYM_QUASI_QUOTE] = lisp_make_symbol("QUASIQUOTE", ctx);
    c[SYM_UNQUOTE] = lisp_make_symbol("UNQUOTE", ctx);
    c[SYM_UNQUOTE_SPLICE] = lisp_make_symbol("UNQUOTESPLICE", ctx);
    c[SYM_DEFINE] = lisp_make_symbol("_DEF", ctx);
    c[SYM_DEFINE_MACRO] = lisp_make_symbol("DEFINE-MACRO", ctx);
    c[SYM_SET] = lisp_make_symbol("_SET!", ctx);
    c[SYM_LAMBDA] = lisp_make_symbol("/\\_", ctx);
    c[SYM_CONS] = lisp_make_symbol("CONS", ctx);
    return ctx;
}

void lisp_shutdown(LispContext ctx)
{
    heap_shutdown(&ctx.p->heap);
    free(ctx.p->stack);
    free(ctx.p);
}

#endif

/*
Copyright (c) 2021 Justin Meiners

Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
