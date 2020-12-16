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
#include <time.h>
#include "lisp.h"

#define SCRATCH_MAX 1024

enum
{
    GC_CLEAR = 0,
    GC_MOVED = (1 << 0), // has this block been moved to the to-space?
    GC_VISITED = (1 << 1), // has this block's pointers been moved?
};

typedef struct Page
{
    size_t size;
    size_t capacity;
    struct Page* next;
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
    union
    {
        struct Block* forward_address;
        size_t size;
    };
    unsigned char gc_flags;
    unsigned char type;
} Block;

struct LispImpl
{
    Heap heap;
    Heap to_heap;

    Lisp* stack;
    size_t stack_ptr;
    size_t stack_depth;

    Lisp symbol_table;
    Lisp global_env;
    int lambda_counter;
    int symbol_counter;
    
    size_t gc_stat_freed;
    size_t gc_stat_time;
};

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

static void* heap_alloc(size_t alloc_size, LispType type, Heap* heap)
{
    Page* to_use;
    
    assert(alloc_size > 0);
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
    block->size = alloc_size;
    block->type = type;
    return address;
}

static void* gc_alloc(size_t size, LispType type, LispContext ctx)
{
    return heap_alloc(size, type, &ctx.impl->heap);
}

typedef struct
{
    Block block;
    Lisp car;
    Lisp cdr;
} Pair;

typedef struct
{
    Block block;
    char string[];
} String;

typedef struct
{
    Block block;
    unsigned int hash;
    char string[];
} Symbol;

// hash table
// linked list chaining
typedef struct
{
    Block block;
    unsigned int size;
    unsigned int capacity;
    Lisp entries[];
} Table;

Lisp lisp_make_null()
{
    Lisp l;
    l.type = LISP_NULL;
    l.val.int_val = 0;
    return l;
}

int lisp_eq_r(Lisp a, Lisp b)
{
    if (a.type != b.type)
    {
        return 0;
    }

    switch (a.type)
    {
        case LISP_NULL:
            return 1;
        case LISP_CHAR:
            return lisp_char(a) == lisp_char(b);
        case LISP_INT:
            return lisp_int(a) == lisp_int(b);
        case LISP_REAL:
            return lisp_real(a) == lisp_real(b);
        case LISP_VECTOR:
        {
            int N = lisp_vector_length(a);
            if (lisp_vector_length(b) != N) return 0;
            
            int i;
            for (i = 0; i < N; ++i)
            {
                if (!lisp_eq_r(lisp_vector_ref(a, i), lisp_vector_ref(b, i)))
                    return 0;
            }
            
            return 1;
        }
        case LISP_PAIR:
        {
            while (lisp_is_pair(a) && lisp_is_pair(b))
            {
                if (!lisp_eq_r(lisp_car(a), lisp_car(b)))
                {
                    return 0;
                }
                
                a = lisp_cdr(a);
                b = lisp_cdr(b);
            }
            
            return lisp_eq_r(a, b);
        }
        default:
            return a.val.ptr_val == b.val.ptr_val;
    }
}

static Table* lisp_table(Lisp t)
{
    assert(lisp_type(t) == LISP_TABLE);
    return t.val.ptr_val;
}

Lisp lisp_make_int(int n)
{
    Lisp l = lisp_make_null();
    l.type = LISP_INT;
    l.val.int_val = n;
    return l;
}

int lisp_int(Lisp x)
{
    if (x.type == LISP_REAL)
        return (int)x.val.real_val;
    return x.val.int_val;
}

Lisp lisp_make_real(float x)
{
    Lisp l = lisp_make_null();
    l.type = LISP_REAL;
    l.val.real_val = x;
    return l;
}

float lisp_real(Lisp x)
{
    if (x.type == LISP_INT)
        return(float)x.val.int_val;
    return x.val.real_val;
}

Lisp lisp_car(Lisp p)
{
    assert(p.type == LISP_PAIR);
    const Pair* pair = p.val.ptr_val;
    return pair->car;
}

Lisp lisp_cdr(Lisp p)
{
    assert(p.type == LISP_PAIR);
    const Pair* pair = p.val.ptr_val;
    return pair->cdr;
}

void lisp_set_car(Lisp p, Lisp x)
{
    assert(p.type == LISP_PAIR);
    Pair* pair = p.val.ptr_val;
    pair->car = x;
}

void lisp_set_cdr(Lisp p, Lisp x)
{
    assert(p.type == LISP_PAIR);
    Pair* pair = p.val.ptr_val;
    pair->cdr = x;
}

Lisp lisp_cons(Lisp car, Lisp cdr, LispContext ctx)
{
    Pair* pair = gc_alloc(sizeof(Pair), LISP_PAIR, ctx);
    pair->car = car;
    pair->cdr = cdr;
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

Lisp lisp_make_listv(LispContext ctx, Lisp first, ...)
{
    Lisp front = lisp_cons(first, lisp_make_null(), ctx);
    Lisp back = front;

    va_list args;
    va_start(args, first);

    Lisp it = lisp_make_null();
    do
    {
        it = va_arg(args, Lisp);
        if (!lisp_is_null(it))
            lisp_fast_append(&front, &back, it, ctx);
    } while (!lisp_is_null(it));

    va_end(args);

    return front;
}

Lisp lisp_list_append(Lisp l, Lisp tail, LispContext ctx)
{
    // (a b) (c) -> (a b c)

    l = lisp_list_reverse(lisp_list_copy(l, ctx));

    while (lisp_is_pair(l))
    {
        Lisp head = l;
        l = lisp_cdr(l);
        lisp_set_cdr(head, tail);
        tail = head;
    }
    return tail;
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
    Lisp pair = lisp_list_assoc(l, key);
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

Lisp lisp_list_nav(Lisp p, const char* c)
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

Lisp lisp_list_reverse(Lisp l)
{
    Lisp p = lisp_make_null();

    while (lisp_is_pair(l))
    {
        Lisp next = lisp_cdr(l);
        lisp_set_cdr(l, p);
        p = l;
        l = next;        
    }

    return p;
}

typedef struct
{
    Block block;
    unsigned int length;
    Lisp entries[];
} Vector;

Lisp lisp_make_vector(unsigned int n, Lisp x, LispContext ctx)
{
    Vector* vector = gc_alloc(sizeof(Vector) + sizeof(Lisp) * n, LISP_VECTOR, ctx);
    vector->length = n;
    for (unsigned int i = 0; i < n; ++i)
        vector->entries[i] = x;
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
    return lisp_vector(v)->length;
}

Lisp lisp_vector_ref(Lisp v, int i)
{
    const Vector* vector = lisp_vector(v);
    assert(i < vector->length);
    return vector->entries[i];
}

void lisp_vector_set(Lisp v, int i, Lisp x)
{
    Vector* vector = lisp_vector(v);
    assert(i < vector->length);
    vector->entries[i] = x;
}

Lisp lisp_vector_assoc(Lisp v, Lisp key)
{
    const Vector* vector = lisp_vector(v);
    for (int i = 0; i < vector->length; ++i)
    {
        Lisp x = vector->entries[i];
        if (lisp_eq(lisp_car(x), key))
        {
            return x;
        }
    }

    return lisp_make_null();
}

Lisp lisp_subvector(Lisp old, int start, int end, LispContext ctx)
{
    assert(start <= end);
    
    Vector* src = old.val.ptr_val;
    if (end > src->length) end = src->length;
    
    int n = end - start;
    Lisp new_v = lisp_make_vector(n, lisp_make_int(0), ctx);
    Vector* dst = new_v.val.ptr_val;
    memcpy(dst->entries, src->entries, sizeof(Lisp) * n);
    return new_v;
}

Lisp lisp_vector_grow(Lisp v, unsigned int n, LispContext ctx)
{
    const Vector* vector = lisp_vector(v);
    assert(n >= vector->length);

    if (n == vector->length)
    {
        return v;
    }
    else
    {
        Lisp new_v = lisp_make_vector(n, lisp_vector_ref(v, 0), ctx);
        Vector* new_vector = lisp_vector(new_v);
        for (int i = 0; i < vector->length; ++i)
            new_vector->entries[i] = vector->entries[i];
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

const char* lisp_symbol(Lisp l)
{
    assert(l.type == LISP_SYMBOL);
    Symbol* symbol = l.val.ptr_val;
    return symbol->string;
}

Lisp lisp_make_char(int c)
{
    Lisp l = lisp_make_null();
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
    return symbol->hash;
}

// TODO
#if defined(WIN32) || defined(WIN64)
#define strncasecmp _stricmp
#endif

static Lisp table_get_string(Lisp l, const char* string, unsigned int hash)
{
    Table* table = lisp_table(l);
    unsigned int index = hash % table->capacity;

    Lisp it = table->entries[index];

    while (!lisp_is_null(it))
    {
        Lisp pair = lisp_car(it);
        Lisp symbol = lisp_car(pair);

        if (strncasecmp(lisp_symbol(symbol), string, 2048) == 0)
        {
            return pair;
        }

        it = lisp_cdr(it);
    }

    return lisp_make_null();
}

static unsigned int hash_string(const char* c)
{     
    // adler 32
    int s1 = 1;
    int s2 = 0;

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
        snprintf(scratch, SCRATCH_MAX, ":G%d", ctx.impl->symbol_counter++);
        string = scratch;
    }

    unsigned int hash = hash_string(string);
    Lisp pair = table_get_string(ctx.impl->symbol_table, string, hash);
    
    if (lisp_is_null(pair))
    {
        size_t string_length = strlen(string) + 1;
        // allocate a new block
        Symbol* symbol = gc_alloc(sizeof(Symbol) + string_length, LISP_SYMBOL, ctx);
        
        symbol->hash = hash;
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
        lisp_table_set(ctx.impl->symbol_table, l, lisp_make_null(), ctx);
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
    lambda->identifier = ctx.impl->lambda_counter++;
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
    return l.val.ptr_val;
}

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_HASH,
    TOKEN_BSLASH,
    TOKEN_DOT,
    TOKEN_QUOTE,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
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

static void lexer_next_token(Lexer* lex)
{
    lexer_skip_empty(lex);
    lexer_advance_start(lex);

    if (*lex->c == '\0')
    {
        lex->token = TOKEN_NONE;
    }
    else if (*lex->c == '(')
    {
        lex->token = TOKEN_L_PAREN;
        lexer_step(lex);
    }
    else if (*lex->c == ')')
    {
        lex->token = TOKEN_R_PAREN;
        lexer_step(lex);
    }
    else if (*lex->c == '#')
    {
        lex->token = TOKEN_HASH;
        lexer_step(lex);
    }
    else if (*lex->c == '.')
    {
        lex->token = TOKEN_DOT;
        lexer_step(lex);
    }
    else if (*lex->c == '\'')
    {
        lex->token = TOKEN_QUOTE;
        lexer_step(lex);
    }
    else if (*lex->c == '\\')
    {
        lex->token = TOKEN_BSLASH;
        lexer_step(lex);
    }
    else if (lexer_match_string(lex))
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
    else
    {
        lex->token = TOKEN_NONE;
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
    "linefeed", "VT", "page", "return", "SO",
    "SI", "DLE", "DC1", "DC2", "DC3",
    "DC4", "NAK", "SYN", "ETB", "CAN",
    "EM", "SUB", "altmode", "FS", "GS", "RS",
    "backnext", "space", NULL
};

static int parse_char_token(Lexer* lex)
{
    char scratch[SCRATCH_MAX];
    size_t length = lex->scan_length;
    lexer_copy_token(lex, 0, length, scratch);
    
    if (length == 1)
    {
        // TODO: multi chars
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
        case TOKEN_HASH:
        {
            // #
            lexer_next_token(lex);
            if (lex->token == TOKEN_BSLASH)
            {
                lexer_next_token(lex);
                if (lex->token != TOKEN_SYMBOL) longjmp(error_jmp, LISP_ERROR_BAD_TOKEN);
                int c = parse_char_token(lex);
                lexer_next_token(lex);
                return lisp_make_char(c);
            }
            
            if (lex->token != TOKEN_L_PAREN) longjmp(error_jmp, LISP_ERROR_PAREN_EXPECTED);
            lexer_next_token(lex);
            // (

            Lisp v = lisp_make_null();
            int count = 0;

            while (lex->token != TOKEN_R_PAREN)
            {
                Lisp x = parse_list_r(lex, error_jmp, ctx);

                if (lisp_is_null(v))
                {
                    v = lisp_make_vector(16, x, ctx);
                    ++count;
                }
                else
                {
                    int length = lisp_vector_length(v);
                    if (count + 1 >= length)
                    {
                        v = lisp_vector_grow(v, length * 2, ctx);
                    }

                    lisp_vector_set(v, count, x);
                    ++count;
                }
            }
            // )
            lexer_next_token(lex);

            // cutouff length
            Vector* vector = lisp_vector(v);
            vector->length = count;
            return v;
        }
        case TOKEN_QUOTE:
        {
             // '
             lexer_next_token(lex);
             Lisp l = lisp_cons(parse_list_r(lex, error_jmp, ctx), lisp_make_null(), ctx);
             return lisp_cons(lisp_make_symbol("QUOTE", ctx), l, ctx);
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
        Lisp front = lisp_cons(lisp_make_symbol("BEGIN", ctx), back, ctx);
        
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


static Lisp expand_r(Lisp l, jmp_buf error_jmp, LispContext ctx)
{
    // 1. expand extended syntax into primitive syntax
    // 2. perform optimizations
    // 3. check syntax    
    if (lisp_type(l) == LISP_SYMBOL &&
        strcmp(lisp_symbol(l), "QUOTE") == 0)
    {
        // don't expand quotes
        return l;
    }
    else if (lisp_type(l) == LISP_PAIR)
    {
        const char* op = NULL;
        if (lisp_type(lisp_car(l)) == LISP_SYMBOL)
            op = lisp_symbol(lisp_car(l));

        if (op && strcmp(op, "QUOTE") == 0)
        {
            if (lisp_list_length(l) != 2) longjmp(error_jmp, LISP_ERROR_BAD_QUOTE);
            // don't expand quotes
            return l;
        }
        else if (op && strcmp(op, "DEFINE") == 0)
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
                    lambda = lisp_cons(lisp_make_symbol("LAMBDA", ctx), lambda, ctx);

                    lisp_set_cdr(l, lisp_make_listv(ctx,
                                                    name,
                                                    expand_r(lambda, error_jmp, ctx),
                                                    lisp_make_null()));
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
        else if (op && strcmp(op, "SET!") == 0)
        {
            if (lisp_list_length(l) != 3) longjmp(error_jmp, LISP_ERROR_BAD_SET);

            Lisp var = lisp_list_ref(l, 1);
            if (lisp_type(var) != LISP_SYMBOL) longjmp(error_jmp, LISP_ERROR_BAD_SET);
            Lisp expr = expand_r(lisp_list_ref(l, 2), error_jmp, ctx);

            return lisp_make_listv(ctx,
                    lisp_list_ref(l, 0), // SET!
                    var,
                    expr,
                    lisp_make_null());
        }
        else if (op && strcmp(op, "COND") == 0)
        {
            // (COND (<pred0> <expr0>)
            //       (<pred1> <expr1>)
            //        ...
            //        (else <expr-1>)) ->
            //
            //  (IF <pred0> <expr0>
            //      (if <pred1> <expr1>
            //          ....
            //      (if <predN> <exprN> <expr-1>)) ... )

            Lisp conds = lisp_list_reverse(lisp_cdr(l));
            Lisp outer = lisp_make_null();

            Lisp cond_pair = lisp_car(conds);

            // error checks
            if (lisp_type(cond_pair) != LISP_PAIR) longjmp(error_jmp, LISP_ERROR_BAD_COND);
            if (lisp_list_length(cond_pair) != 2) longjmp(error_jmp, LISP_ERROR_BAD_COND);

            Lisp cond_pred = lisp_car(cond_pair);
            Lisp cond_expr = lisp_make_null();

            if ((lisp_type(cond_pred) == LISP_SYMBOL) &&
                    strcmp(lisp_symbol(cond_pred), "ELSE") == 0)
            {
                cond_expr = expand_r(lisp_car(lisp_cdr(cond_pair)), error_jmp, ctx);
                outer = cond_expr;
                conds = lisp_cdr(conds);
            }

            Lisp if_symbol = lisp_make_symbol("IF", ctx);
       
            while (lisp_is_pair(conds))
            {
                cond_pair = lisp_car(conds);

                // error checks
                if (lisp_type(cond_pair) != LISP_PAIR) longjmp(error_jmp, LISP_ERROR_BAD_COND);
                if (lisp_list_length(cond_pair) != 2) longjmp(error_jmp, LISP_ERROR_BAD_COND);

                cond_pred = expand_r(lisp_car(cond_pair), error_jmp, ctx);
                cond_expr = expand_r(lisp_car(lisp_cdr(cond_pair)), error_jmp, ctx);

                outer = lisp_make_listv(ctx,
                                       if_symbol,
                                       cond_pred,
                                       cond_expr,
                                       outer,
                                       lisp_make_null());

                conds = lisp_cdr(conds);
            }

            return outer;
        }
        else if (op && strcmp(op, "AND") == 0)
        {
            // (AND <pred0> <pred1> ... <predN>) 
            // -> (IF <pred0> 
            //      (IF <pred1> ...
            //          (IF <predN> t f)
            if (lisp_list_length(l) < 2) longjmp(error_jmp, LISP_ERROR_BAD_AND);

            Lisp if_symbol = lisp_make_symbol("IF", ctx);

            Lisp preds = lisp_list_reverse(lisp_cdr(l));
            Lisp p = expand_r(lisp_car(preds), error_jmp, ctx);
            
            Lisp outer = lisp_make_listv(ctx,
                                        if_symbol,
                                        p,
                                        lisp_make_int(1),
                                        lisp_make_int(0),
                                        lisp_make_null());

            preds = lisp_cdr(preds);

            while (lisp_is_pair(preds))
            {
                p = expand_r(lisp_car(preds), error_jmp, ctx);

                outer = lisp_make_listv(ctx,
                        if_symbol,
                        p,
                        outer,
                        lisp_make_int(0),
                        lisp_make_null());

                preds = lisp_cdr(preds);
            }
                      
           return outer;
        }
        else if (op && strcmp(op, "OR") == 0)
        {
            // (OR <pred0> <pred1> ... <predN>)
            // -> (IF (<pred0>) t
            //      (IF <pred1> t ...
            //          (if <predN> t f))
            if (lisp_list_length(l) < 2) longjmp(error_jmp, LISP_ERROR_BAD_OR);

            Lisp if_symbol = lisp_make_symbol("IF", ctx);

            Lisp preds = lisp_list_reverse(lisp_cdr(l));
            Lisp p = expand_r(lisp_car(preds), error_jmp, ctx);
            
            Lisp outer = lisp_make_listv(ctx,
                                        if_symbol,
                                        p,
                                        lisp_make_int(1),
                                        lisp_make_int(0),
                                        lisp_make_null());

            preds = lisp_cdr(preds);

            while (lisp_is_pair(preds))
            {
                p = expand_r(lisp_car(preds), error_jmp, ctx);

                outer = lisp_make_listv(ctx,
                                        if_symbol,
                                        p,
                                        lisp_make_int(1),
                                        outer,
                                        lisp_make_null());
                preds = lisp_cdr(preds);
            }
                      
           return outer;

        }
        else if (op && strcmp(op, "LET") == 0)
        {
            // (LET ((<var0> <expr0>) ... (<varN> <expr1>)) <body0> ... <bodyN>)
            //  -> ((LAMBDA (<var0> ... <varN>) (BEGIN <body0> ... <bodyN>)) <expr0> ... <expr1>)            
            Lisp pairs = lisp_list_ref(l, 1);
            if (lisp_type(pairs) != LISP_PAIR) longjmp(error_jmp, LISP_ERROR_BAD_LET);

            Lisp body = lisp_list_advance(l, 2);

            Lisp vars = lisp_make_null();
            Lisp exprs = lisp_make_null();
            
            while (lisp_is_pair(pairs))
            {
                if (!lisp_is_pair(lisp_car(pairs))) longjmp(error_jmp, LISP_ERROR_BAD_LET);

                Lisp var = lisp_list_ref(lisp_car(pairs), 0);
                if (lisp_type(var) != LISP_SYMBOL) longjmp(error_jmp, LISP_ERROR_BAD_LET);
                vars = lisp_cons(var, vars, ctx);

                exprs = lisp_cons(lisp_list_ref(lisp_car(pairs), 1), exprs, ctx);
                pairs = lisp_cdr(pairs);
            }
            vars = lisp_list_reverse(vars);
            exprs = lisp_list_reverse(exprs);
            
            Lisp lambda = lisp_make_listv(ctx, 
                                        lisp_make_symbol("LAMBDA", ctx), 
                                        vars, 
                                        lisp_cons(
                                            lisp_make_symbol("BEGIN", ctx),
                                            expand_r(body, error_jmp, ctx),
                                            ctx
                                        ),
                                        lisp_make_null());

            return lisp_cons(lambda, expand_r(exprs, error_jmp, ctx), ctx);
        }
        else if (op && strcmp(op, "DO") == 0)
        {
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

            Lisp f = lisp_make_symbol(NULL, ctx);
            Lisp lambda_symbol = lisp_make_symbol("LAMBDA", ctx);
            Lisp begin_symbol = lisp_make_symbol("BEGIN", ctx);

            Lisp var_list = lisp_list_ref(l, 1);

            Lisp vars = lisp_make_null();
            Lisp inits = lisp_make_null();
            Lisp steps = lisp_make_null();

            while (lisp_is_pair(var_list))
            {
                Lisp v = lisp_car(var_list);

                vars = lisp_cons(lisp_list_ref(v, 0), vars, ctx);
                if (lisp_type(lisp_car(vars)) != LISP_SYMBOL) longjmp(error_jmp, LISP_ERROR_BAD_DO);

                inits = lisp_cons(lisp_list_ref(v, 1), inits, ctx);
                steps = lisp_cons(lisp_list_ref(v, 2), steps, ctx);
                var_list = lisp_cdr(var_list);
            }

            vars = lisp_list_reverse(vars);
            inits = lisp_list_reverse(inits);
            steps = lisp_list_reverse(steps);

            Lisp loop_test = lisp_car(lisp_list_ref(l, 2));
            Lisp loop_result = lisp_car(lisp_cdr(lisp_list_ref(l, 2)));
            Lisp body = lisp_list_ref(l, 3);

            Lisp lambda = lisp_make_listv(
                    ctx,
                    lambda_symbol,
                    vars, 
                    lisp_make_listv(
                        ctx,
                        lisp_make_symbol("IF", ctx),
                        loop_test,
                        loop_result, 
                        lisp_make_listv(
                            ctx,
                            begin_symbol,
                            body,
                            lisp_cons(f, steps, ctx),
                            lisp_make_null()
                            ),
                        lisp_make_null()
                        ),
                    lisp_make_null()
                    );

            Lisp outer_lambda = lisp_make_listv(
                    ctx,
                    lambda_symbol,
                    lisp_cons(f, lisp_make_null(), ctx),
                    lisp_make_listv(
                        ctx,
                        begin_symbol,
                        lisp_make_listv(
                            ctx,
                            lisp_make_symbol("SET!", ctx),
                            f,
                            lambda,
                            lisp_make_null()
                        ),
                        lisp_cons(f, inits, ctx),
                        lisp_make_null()
                    ),
                    lisp_make_null()
                    );

            Lisp call = lisp_cons(
                    outer_lambda,
                    lisp_cons(lisp_make_null(), lisp_make_null(), ctx),
                    ctx
                    );

            return expand_r(call, error_jmp, ctx);
        }
        else if (op && strcmp(op, "LAMBDA") == 0)
        {
            // (LAMBDA (<var0> ... <varN>) <expr0> ... <exprN>)
            // (LAMBDA (<var0> ... <varN>) (BEGIN <expr0> ... <expr1>)) 
            int length = lisp_list_length(l);

            if (length > 3)
            {
                Lisp body_exprs = expand_r(lisp_list_advance(l, 2), error_jmp, ctx); 
                Lisp begin = lisp_cons(lisp_make_symbol("BEGIN", ctx), body_exprs, ctx);

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
        else if (op && strcmp(op, "ASSERT") == 0)
        {
            Lisp statement = lisp_car(lisp_cdr(l));
            // here we save a quoted version of the code so we can see
            // what happened to trigger the assertion
            Lisp quoted = lisp_make_listv(ctx,
                                         lisp_make_symbol("QUOTE", ctx),
                                         statement,
                                         lisp_make_null());
            return lisp_make_listv(ctx,
                                  lisp_car(l),
                                  expand_r(statement, error_jmp, ctx),
                                  quoted,
                                  lisp_make_null());
                                  
        }
        else
        {
            Lisp it = l;
            while (lisp_is_pair(it))
            {
                lisp_set_car(it, expand_r(lisp_car(it), error_jmp, ctx));
                it = lisp_cdr(it);
            }

            return l;
        }
    } 
    else 
    {
        return l;
    }
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

Lisp lisp_expand(Lisp lisp, LispError* out_error, LispContext ctx)
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

Lisp lisp_make_table(unsigned int capacity, LispContext ctx)
{
    size_t size = sizeof(Table) + sizeof(Lisp) * capacity;
    Table* table = gc_alloc(size, LISP_TABLE, ctx);
    table->size = 0;
    table->capacity = capacity;

    // clear the table
    for (int i = 0; i < capacity; ++i)
        table->entries[i] = lisp_make_null();

    Lisp l;
    l.type = table->block.type;
    l.val.ptr_val = table;
    return l;
}

void lisp_table_set(Lisp t, Lisp key, Lisp value, LispContext ctx)
{
    Table* table = lisp_table(t);
    
    unsigned int index = symbol_hash(key) % table->capacity;
    Lisp pair = lisp_list_assoc(table->entries[index], key);

    if (lisp_is_null(pair))
    {
        // new value. prepend to front of chain
        pair = lisp_cons(key, value, ctx);
        table->entries[index] = lisp_cons(pair, table->entries[index], ctx);
        ++table->size;
    }
    else
    {
        // reassign cdr value (key, val)
        lisp_set_cdr(pair, value);
    }
}

Lisp lisp_table_get(Lisp t, Lisp symbol, LispContext ctx)
{
    const Table* table = lisp_table(t);
    unsigned int index = symbol_hash(symbol) % table->capacity;
    return lisp_list_assoc(table->entries[index], symbol);
}

Lisp lisp_table_to_assoc_list(Lisp t, LispContext ctx)
{
    const Table* table = lisp_table(t);
    Lisp result = lisp_make_null();
    
    int i;
    for (i = 0; i < table->capacity; ++i)
    {
        Lisp it = table->entries[i];
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
        fprintf(stderr, "error: unknown variable: %s\n", lisp_symbol(key));
    }
    lisp_set_cdr(pair, x);
}

static void lisp_print_r(FILE* file, Lisp l, int is_cdr)
{
    switch (lisp_type(l))
    {
        case LISP_INT:
            fprintf(file, "%i", lisp_int(l));
            break;
        case LISP_REAL:
            fprintf(file, "%f", lisp_real(l));
            break;
        case LISP_NULL:
            fprintf(file, "NIL");
            break;
        case LISP_SYMBOL:
            fprintf(file, "%s", lisp_symbol(l));
            break;
        case LISP_STRING:
            fprintf(file, "\"%s\"", lisp_string(l));
            break;
        case LISP_CHAR:
        {
            int c = lisp_int(l);
            
            if (c < 33)
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
                if (lisp_is_null(table->entries[i])) continue;
                lisp_print_r(file, table->entries[i], 0);
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
    }
}

void lisp_printf(FILE* file, Lisp l) { lisp_print_r(file, l, 0);  }
void lisp_print(Lisp l) {  lisp_printf(stdout, l); }


static void lisp_stack_push(Lisp x, LispContext ctx)
{
    if (LISP_DEBUG)
    {
        if (ctx.impl->stack_ptr + 1 >= ctx.impl->stack_depth)
        {
            fprintf(stderr, "stack overflow\n");
        }
    }
    
    ctx.impl->stack[ctx.impl->stack_ptr] = x;
    ++ctx.impl->stack_ptr;
}

static Lisp lisp_stack_pop(LispContext ctx)
{
    ctx.impl->stack_ptr--;
    
    if (LISP_DEBUG)
    {
        if (ctx.impl->stack_ptr < 0)
        {
            fprintf(stderr, "stack underflow\n");
        }
    }
    return ctx.impl->stack[ctx.impl->stack_ptr];
}

static Lisp* lisp_stack_peek(size_t i, LispContext ctx)
{
    return ctx.impl->stack + (ctx.impl->stack_ptr - i);
}

static int eval_apply(Lisp operator, Lisp args, Lisp* out_val, Lisp* out_env, LispError* error, LispContext ctx)
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
            *out_val = lambda->body;
            return 1;
        }
        case LISP_FUNC: // call into C functions
        {
            // no environment required
            LispCFunc func = lisp_func(operator);
            Lisp result = func(args, error, ctx);
            *out_val = result;
            return 0;
        }
        default:
        {
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
                    fprintf(stderr, "cannot find variable: %s\n", lisp_symbol(*x));
                    longjmp(error_jmp, LISP_ERROR_UNKNOWN_VAR); 
                    return lisp_make_null();
                }
                return lisp_cdr(pair);
            }
            case LISP_PAIR:
            {
                const char* op_name = NULL;
                if (lisp_type(lisp_car(*x)) == LISP_SYMBOL)
                    op_name = lisp_symbol(lisp_car(*x));

                if (op_name && strcmp(op_name, "IF") == 0) // if conditional statemetns
                {
                    Lisp predicate = lisp_list_ref(*x, 1);
                    
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(predicate, ctx);
 
                    if (lisp_int(eval_r(error_jmp, ctx)) != 0)
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
                else if (op_name && strcmp(op_name, "BEGIN") == 0)
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
                else if (op_name && strcmp(op_name, "QUOTE") == 0)
                {
                    return lisp_list_ref(*x, 1);
                }
                else if (op_name && strcmp(op_name, "DEFINE") == 0) // variable definitions
                {
                    lisp_stack_push(*env, ctx);
                    lisp_stack_push(lisp_list_ref(*x, 2), ctx);
                    
                    Lisp value = eval_r(error_jmp, ctx);
                    
                    lisp_stack_pop(ctx);
                    lisp_stack_pop(ctx);
                    
                    Lisp symbol = lisp_list_ref(*x, 1);
                    lisp_env_define(*env, symbol, value, ctx);
                    return lisp_make_null();
                }
                else if (op_name && strcmp(op_name, "SET!") == 0)
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
                else if (op_name && strcmp(op_name, "LAMBDA") == 0) // lambda defintions (compound procedures)
                {
                    Lisp args = lisp_list_ref(*x, 1);
                    Lisp body = lisp_list_ref(*x, 2);
                    return lisp_make_lambda(args, body, *env, ctx);
                }
                else // operator application
                {
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
                    int needs_to_eval = eval_apply(operator, args_front, x, env, &error, ctx);
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


Lisp lisp_eval_opt(Lisp l, Lisp env, LispError* out_error, LispContext ctx)
{
    LispError error;
    Lisp expanded = lisp_expand(l, &error, ctx);
    
    if (error != LISP_ERROR_NONE)
    {
        if (out_error) *out_error = error;
        return lisp_make_null();
    }
    
    size_t save_stack = ctx.impl->stack_ptr;
    
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
            ctx.impl->stack_ptr = save_stack;
            *out_error = error;
        }

        return lisp_make_null();
    }
}

Lisp lisp_eval(Lisp expr, LispError* out_error, LispContext ctx)
{
    return lisp_eval_opt(expr, lisp_env_global(ctx), out_error, ctx);
}

static Lisp gc_move(Lisp l, Heap* to)
{
    switch (l.type)
    {
        case LISP_PAIR:
        case LISP_SYMBOL:
        case LISP_STRING:
        case LISP_LAMBDA:
        case LISP_VECTOR:
        {
            Block* block = l.val.ptr_val;
            if (!(block->gc_flags & GC_MOVED))
            {
                // copy the data to new block
                Block* dest = heap_alloc(block->size, block->type, to);
                memcpy(dest, block, block->size);
                dest->gc_flags = GC_CLEAR;
                
                // save forwarding address (offset in to)
                block->forward_address = dest;
                block->gc_flags = GC_MOVED;
            }
            
            // return the moved block address
            l.val.ptr_val = block->forward_address;
            return l;
        }
        case LISP_TABLE:
        {
            Table* table = l.val.ptr_val;
            if (!(table->block.gc_flags & GC_MOVED))
            {
                float load_factor = table->size / (float)table->capacity;
                unsigned int new_capacity = table->capacity;
                
                /// TODO: research these numbers
                if (load_factor > 0.75f || load_factor < 0.1f)
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

                size_t new_size = sizeof(Table) + new_capacity * sizeof(Lisp);
                Table* dest_table = heap_alloc(new_size, LISP_TABLE, to);
                dest_table->size = table->size;
                dest_table->capacity = new_capacity;
                
                // save forwarding address (offset in to)
                table->block.forward_address = &dest_table->block;
                table->block.gc_flags = GC_MOVED;
                
                // clear the table
                for (int i = 0; i < new_capacity; ++i)
                    dest_table->entries[i] = lisp_make_null();
                
                if (LISP_DEBUG && new_capacity != table->capacity)
                {
                    printf("resizing table %i -> %i\n", table->capacity, new_capacity);
                }
                
                for (unsigned int i = 0; i < table->capacity; ++i)
                {
                    Lisp it = table->entries[i];
                    
                    while (lisp_is_pair(it))
                    {
                        unsigned int new_index = i;
                        
                        if (new_capacity != table->capacity)
                            new_index = symbol_hash(lisp_car(lisp_car(it))) % new_capacity;
                        
                        Lisp pair = gc_move(lisp_car(it), to);

                        // allocate a new pair in the to space
                        Pair* cons_block = heap_alloc(sizeof(Pair), LISP_PAIR, to);
                        cons_block->block.gc_flags = GC_VISITED;
                        
                        Lisp new_pair;
                        new_pair.type = cons_block->block.type;
                        new_pair.val.ptr_val = cons_block;
    
                        // (pair, entry)
                        lisp_set_car(new_pair, pair);
                        lisp_set_cdr(new_pair, dest_table->entries[new_index]);
                        dest_table->entries[new_index] = new_pair;
                        it = lisp_cdr(it);
                    }
                }
            }
            
            // return the moved table address
            l.val.ptr_val = table->block.forward_address;
            return l;
        }
        default:
            return l;
    }
}


Lisp lisp_collect(Lisp root_to_save, LispContext ctx)
{
    time_t start_time = clock();

    Heap* to = &ctx.impl->to_heap;
    heap_init(to, ctx.impl->heap.page_size);

    // move root object
    ctx.impl->symbol_table = gc_move(ctx.impl->symbol_table, to);
    ctx.impl->global_env = gc_move(ctx.impl->global_env, to);

    int i;
    for (i = 0; i < ctx.impl->stack_ptr; ++i)
    {
        ctx.impl->stack[i] = gc_move(ctx.impl->stack[i], to);
    }
    
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
                        pair->car = gc_move(pair->car, to);
                        pair->cdr = gc_move(pair->cdr, to);
                        break;
                    }
                    case LISP_VECTOR:
                    {
                        Vector* vector = (Vector*)block;
                        for (int i = 0; i < vector->length; ++i)
                        {
                            vector->entries[i] = gc_move(vector->entries[i], to);
                        }
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
            offset += block->size;
        }
        page = page->next;
        ++page_counter;
    }
    // check that we visited all the pages
    assert(page_counter == to->page_count);
    
    if (LISP_DEBUG)
    {
        // DEBUG, check offsets
        const Page* page = to->bottom;
        while (page)
        {
            size_t offset = 0;
            while (offset < page->size)
            {
                Block* block = (Block*)(page->buffer + offset);
                offset += block->size;
            }
            assert(offset == page->size);
            page = page->next;
        }
    }

    
    size_t diff = ctx.impl->heap.size - ctx.impl->to_heap.size;

    // swap the heaps
    Heap temp = ctx.impl->heap;
    ctx.impl->heap = ctx.impl->to_heap;
    ctx.impl->to_heap = temp;
    
    // reset the heap
    heap_shutdown(&ctx.impl->to_heap);
    
    time_t end_time = clock();
    ctx.impl->gc_stat_freed = diff;
    ctx.impl->gc_stat_time = 1000000 * (end_time - start_time) / CLOCKS_PER_SEC;
    return result;
}

Lisp lisp_env_global(LispContext ctx)
{
    return ctx.impl->global_env;
}

void lisp_env_set_global(Lisp env, LispContext ctx)
{
    ctx.impl->global_env = env;
}

void lisp_shutdown(LispContext ctx)
{
    heap_shutdown(&ctx.impl->heap);
    heap_shutdown(&ctx.impl->to_heap);
    free(ctx.impl->stack);
    free(ctx.impl);
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
        case LISP_ERROR_PAREN_EXPECTED:
            return "syntax error: expected ) paren";
        case LISP_ERROR_DOT_UNEXPECTED:
            return "syntax error: dot . was unexpected";
        case LISP_ERROR_BAD_TOKEN:
            return "syntax error: bad token";
        case LISP_ERROR_BAD_DEFINE:
            return "expand error: bad define (define var x)";
        case LISP_ERROR_BAD_SET:
            return "expand error: bad set (set! var x)";
        case LISP_ERROR_BAD_COND:
            return "expand error: bad cond";
        case LISP_ERROR_BAD_AND:
            return "expand error: bad and (and a b)";
        case LISP_ERROR_BAD_OR:
            return "expand error: bad or (or a b)";
        case LISP_ERROR_BAD_LET:
            return "expand error: bad let";
        case LISP_ERROR_BAD_DO:
            return "expand error: bad do";
        case LISP_ERROR_BAD_LAMBDA:
            return "expand error: bad lambda";
        case LISP_ERROR_UNKNOWN_VAR:
            return "eval error: unknown variable";
        case LISP_ERROR_BAD_OP:
            return "eval error: application was not an operator";
        case LISP_ERROR_UNKNOWN_EVAL:
            return "eval error: got into a bad state";
        case LISP_ERROR_BAD_ARG:
            return "eval error: bad argument type";
        case LISP_ERROR_OUT_OF_BOUNDS:
            return "eval error: index out of bounds";
        default:
            return "unknown error code";
    }
}


LispContext lisp_init_empty_opt(int symbol_table_size, size_t stack_depth, size_t page_size)
{
    LispContext ctx;
    ctx.impl = malloc(sizeof(struct LispImpl));
    if (!ctx.impl) return ctx;

    ctx.impl->lambda_counter = 0;
    ctx.impl->symbol_counter = 0;
    ctx.impl->stack_ptr = 0;
    ctx.impl->stack_depth = stack_depth;
    ctx.impl->stack = malloc(sizeof(Lisp) * stack_depth);
    ctx.impl->gc_stat_freed = 0;
    ctx.impl->gc_stat_time = 0;
    
    heap_init(&ctx.impl->heap, page_size);
    heap_init(&ctx.impl->to_heap, page_size);

    ctx.impl->symbol_table = lisp_make_table(symbol_table_size, ctx);
    ctx.impl->global_env = lisp_make_null();
    return ctx;
}


LispContext lisp_init_empty(void)
{
    return lisp_init_empty_opt(LISP_DEFAULT_SYMBOL_TABLE_SIZE, LISP_DEFAULT_STACK_DEPTH, LISP_DEFAULT_PAGE_SIZE);
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

static Lisp sch_nav(Lisp args, LispError* e, LispContext ctx)
{
    Lisp path = lisp_car(args);
    Lisp l = lisp_car(lisp_cdr(args));

    return lisp_list_nav(l, lisp_string(path));
}

static Lisp sch_exact_eq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    return lisp_make_int(lisp_eq(a, b));
}

static Lisp sch_recursive_eq(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    
    return lisp_make_int(lisp_eq_r(a, b));
}

static Lisp sch_not(Lisp args, LispError* e, LispContext ctx)
{
    Lisp x = lisp_car(args);
    return lisp_make_int(!lisp_int(x));
}

static Lisp sch_is_null(Lisp args, LispError* e, LispContext ctx)
{
    while (!lisp_is_null(args))
    {
        if (!lisp_is_null(lisp_car(args))) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_is_pair(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if (!lisp_is_pair(lisp_car(args))) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_display(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    if (lisp_type(l) == LISP_STRING)
    {
        printf("%s", lisp_string(l));
    }
    else
    {
        lisp_print(l);
    }
    return lisp_make_null();
}

static Lisp sch_newline(Lisp args, LispError* e, LispContext ctx)
{
    printf("\n"); return lisp_make_null();
}

static Lisp sch_assert(Lisp args, LispError* e, LispContext ctx)
{
   if (lisp_int(lisp_car(args)) != 1)
   {
       fprintf(stderr, "assertion: ");
       lisp_printf(stderr, lisp_car(lisp_cdr(args)));
       fprintf(stderr, "\n");
       assert(0);
   }

   return lisp_make_null();
}

static Lisp sch_equals(Lisp args, LispError* e, LispContext ctx)
{
    Lisp to_check  = lisp_car(args);
    if (lisp_is_null(to_check)) return lisp_make_int(1);
    
    args = lisp_cdr(args);
    while (lisp_is_pair(args))
    {
        if (lisp_int(lisp_car(args)) != lisp_int(to_check)) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    
    return lisp_make_int(1);
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

/*
static Lisp sch_map(Lisp args, LispError* e, LispContext ctx)
{
    Lisp op = lisp_car(args);

    if (lisp_type(op) != LISP_FUNC && lisp_type(op) != LISP_LAMBDA)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }

    // multiple lists can be passed in
    Lisp lists = lisp_cdr(args);
    int n = lisp_list_length(lists);
    if (n == 0) return lisp_make_null();

    Lisp result_lists = lisp_make_list(lisp_make_null(), n, ctx);
    Lisp result_it = result_lists;

    while (lisp_is_pair(lists))
    {
        // advance all the lists
        Lisp it = lisp_car(lists);

        Lisp front = lisp_make_null();
        Lisp back = front;

        while (lisp_is_pair(it))
        {
            // TODO: garbage collection might fail
            // TODO: wrong environment
            Lisp expr = lisp_cons(op, lisp_cons(lisp_car(it), lisp_make_null(), ctx), ctx);
            Lisp result = lisp_eval(expr, NULL, ctx);
            lisp_fast_append(&front, &back, result, ctx);
            it = lisp_cdr(it);
        }

        lisp_set_car(result_it, front);
        lists = lisp_cdr(lists);
        result_it = lisp_cdr(result_it);
    }

    if (n == 1)
    {
        return lisp_car(result_lists);
    }
    else
    {
        return result_lists;
    }
} */

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

    return lisp_list_reverse(lisp_car(args));
}

static Lisp sch_assoc(Lisp args, LispError* e, LispContext ctx)
{
    Lisp key = lisp_car(args);
    Lisp l = lisp_car(lisp_cdr(args));
    return lisp_list_assoc(l, key);
}

static Lisp sch_add(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

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
    return lisp_make_int(result);
}

static Lisp sch_greater(Lisp args, LispError* e, LispContext ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);
    int result = 0;

    if (lisp_type(accum) == LISP_INT)
    {
        result = lisp_int(accum) > lisp_int(lisp_car(args));
    }
    else if (lisp_type(accum) == LISP_REAL)
    {
        result = lisp_real(accum) > lisp_real(lisp_car(args));
    }
    else
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
    }
    return lisp_make_int(result);
}

static Lisp sch_less_equal(Lisp args, LispError* e, LispContext ctx)
{
    // a <= b = !(a > b)
    Lisp l = sch_greater(args, e, ctx);
    return  lisp_make_int(!lisp_int(l));
}

static Lisp sch_greater_equal(Lisp args, LispError* e, LispContext ctx)
{
    // a >= b = !(a < b)
    Lisp l = sch_less(args, e, ctx);
    return  lisp_make_int(!lisp_int(l));
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
            
        // TODO: string implementations probably nonstandard
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
            snprintf(scratch, SCRATCH_MAX, "%i", lisp_int(val));
            return lisp_make_string(scratch, ctx);
        case LISP_SYMBOL:
            return lisp_make_string(lisp_symbol(val), ctx);
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
        return lisp_make_string(lisp_symbol(val), ctx);
    }
}

static Lisp sch_is_symbol(Lisp args, LispError* e, LispContext ctx)
{
    return lisp_make_int(lisp_type(lisp_car(args)) == LISP_SYMBOL);
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
        if (lisp_type(lisp_car(args)) != LISP_STRING) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_string_is_null(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    int result = lisp_string(a)[0] == '\0';
    return lisp_make_int(result);
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
    return lisp_make_int(result);
}

static Lisp sch_string_less(Lisp args, LispError* e, LispContext ctx)
{
    Lisp a = lisp_car(args);
    args = lisp_cdr(args);
    Lisp b = lisp_car(args);
    int result = strcmp(lisp_string(a), lisp_string(b)) < 0;
    return lisp_make_int(result);
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

    return lisp_make_int((int)strlen(lisp_string(x)));
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

    return lisp_make_int((int)lisp_string_ref(str, lisp_int(index)));
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
    return lisp_make_int(lisp_type(lisp_car(args)) == LISP_CHAR);
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
    return lisp_make_int(isalnum(c));
}

static Lisp sch_char_is_alpha(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_int(isalpha(c));
}

static Lisp sch_char_is_number(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_int(isdigit(c));
}

static Lisp sch_char_is_white(Lisp args, LispError* e, LispContext ctx)
{
    int c = lisp_char(lisp_car(args));
    return lisp_make_int(isblank(c));
}

static Lisp sch_is_int(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if (lisp_type(lisp_car(args)) != LISP_INT) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_is_real(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if (lisp_type(lisp_car(args)) != LISP_REAL) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_is_even(Lisp args, LispError* e, LispContext ctx)
{
    while (!lisp_is_null(args))
    {
        if ((lisp_int(lisp_car(args)) & 1) == 1) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_is_odd(Lisp args, LispError* e, LispContext ctx)
{
    while (lisp_is_pair(args))
    {
        if ((lisp_int(lisp_car(args)) & 1) == 0) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp sch_exp(Lisp args, LispError* e, LispContext ctx)
{
    float x = expf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_log(Lisp args, LispError* e, LispContext ctx)
{
    float x = logf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_sin(Lisp args, LispError* e, LispContext ctx)
{
    float x = sinf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_cos(Lisp args, LispError* e, LispContext ctx)
{
    float x = cosf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_tan(Lisp args, LispError* e, LispContext ctx)
{
    float x = tanf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_sqrt(Lisp args, LispError* e, LispContext ctx)
{
    float x = sqrtf(lisp_real(lisp_car(args)));
    return lisp_make_real(x);
}

static Lisp sch_modulo(Lisp args, LispError* e, LispContext ctx)
{
    int a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    int b = lisp_int(lisp_car(args));
    return lisp_make_int(a % b);
}

static Lisp sch_abs(Lisp args, LispError* e, LispContext ctx)
{
    switch (lisp_type(lisp_car(args)))
    {
        case LISP_INT:
            return lisp_make_int(abs(lisp_int(lisp_car(args))));
        case LISP_REAL:
            return lisp_make_real(fabs(lisp_real(lisp_car(args))));
        default:
            *e = LISP_ERROR_BAD_ARG;
            return lisp_make_null();
    }
}

static Lisp sch_gcd(Lisp args, LispError* e, LispContext ctx)
{
    if (lisp_is_null(args))
    {
        return lisp_make_int(0);
    }
    
    int a = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    int b = lisp_int(lisp_car(args));
    
    int c;
    while ( a != 0 ) {
       c = a; a = b % a;  b = c;
    }
    return lisp_make_int(abs(b));
}

static Lisp sch_is_vector(Lisp args, LispError* e, LispContext ctx)
{
    if (lisp_type(lisp_car(args)) == LISP_VECTOR)
    {
        return lisp_make_int(1);
    }
    else
    {
        return lisp_make_int(0);
    }
}

static Lisp sch_make_vector(Lisp args, LispError* e, LispContext ctx)
{
    Lisp length = lisp_car(args);
    Lisp val = lisp_car(lisp_cdr(args));

    if (lisp_type(length) != LISP_INT)
    {
        *e = LISP_ERROR_BAD_ARG;
        return lisp_make_null();
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

static Lisp sch_vector_assoc(Lisp args, LispError* e, LispContext ctx)
{
    Lisp key = lisp_car(args);
    Lisp v = lisp_car(lisp_cdr(args));
    return lisp_vector_assoc(v, key);
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
    int needs_to_eval = eval_apply(operator, op_args, &x, &env, e, ctx);
    
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
    if (lisp_type(lisp_car(args)) != LISP_LAMBDA) return lisp_make_int(0);
    return lisp_make_int(1);
}

static Lisp sch_lambda_body(Lisp args, LispError* e, LispContext ctx)
{
    Lisp l = lisp_car(args);
    Lambda* lambda = lisp_lambda(l);
    return lambda->body;
}

static Lisp sch_expand(Lisp args, LispError* e, LispContext ctx)
{
    Lisp expr = lisp_car(args);
    Lisp result = lisp_expand(expr, e, ctx);
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
    Page* page = ctx.impl->heap.bottom;
    
    while (page)
    {
        printf("%lu/%lu ", page->size, page->capacity);
        page = page->next;
    }
    printf("\ngc collected: %lu\t time: %lu us\n", ctx.impl->gc_stat_freed, ctx.impl->gc_stat_time);
    printf("heap size: %lu\t pages: %lu\n", ctx.impl->heap.size, ctx.impl->heap.page_count);
    printf("symbols: %lu \n", lisp_table_size(ctx.impl->symbol_table));
    
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
    { "ASSERT", sch_assert },
    
#ifndef LISP_NO_SYSTEM_LIB
    { "READ-PATH", sch_read_path },
#endif
    
    { "EXPAND", sch_expand },
    
    // TODO: do I want this?
    { "NAV", sch_nav },
    
    // Equivalence Predicates https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Equivalence-Predicates.html
    { "EQ?", sch_exact_eq },
    { "EQV?", sch_exact_eq },
    { "EQUAL?", sch_recursive_eq },
    
    // Booleans https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Booleans.html
    { "NOT", sch_not },
    
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
    { "VECTOR?", sch_is_vector },
    { "MAKE-VECTOR", sch_make_vector },
    { "VECTOR-GROW", sch_vector_grow },
    { "VECTOR-LENGTH", sch_vector_length },
    { "VECTOR-SET!", sch_vector_set },
    { "VECTOR-REF", sch_vector_ref },
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
    // TODO: Non Standard
    { "VECTOR-ASSOC", sch_vector_assoc },

    // Numerical operations https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Numerical-operations.html
    { "=", sch_equals },
    { "+", sch_add },
    { "-", sch_sub },
    { "*", sch_mult },
    { "/", sch_divide },
    { "<", sch_less },
    { ">", sch_greater },
    { "<=", sch_less_equal },
    { ">=", sch_greater_equal },
    { "INTEGER?", sch_is_int },
    { "EVEN?", sch_is_even },
    { "ODD?", sch_is_odd },
    { "REAL?", sch_is_real },
    { "EXP", sch_exp },
    { "LOG", sch_log },
    { "SIN", sch_sin },
    { "COS", sch_cos },
    { "TAN", sch_tan },
    { "SQRT", sch_sqrt },
    { "MODULO", sch_modulo },
    { "ABS", sch_abs },
    { "GCD", sch_gcd },
    
    { "INEXACT", sch_to_inexact },
    { "EXACT", sch_to_exact },
    
    // Symbols https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
    { "SYMBOL?", sch_is_symbol },
    { "STRING->SYMBOL", sch_string_to_symbol },
    { "SYMBOL->STRING", sch_symbol_to_string },
    { "GENERATE-UNINTERNED-SYMBOL", sch_gensym },

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
    { "PROCEDURE?", sch_is_lambda},
    // TOOD: Almost standard
    { "PROCEDURE-BODY", sch_lambda_body },
    
    // Output Procedures https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Output-Procedures.html
    { "DISPLAY", sch_display },
    { "NEWLINE", sch_newline },
    // TODO: { "FLUSH-OUTPUT_PORT", sch_flush },
    
    // Random Numbers https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Random-Numbers.html
    { "RANDOM", sch_pseudo_rand },
    
    // TODO: this is nonstandard
    { "RANDOM-SEED!", sch_pseudo_seed },
    
    // Universl Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
    { "GET-UNIVERSAL-TIME", sch_univeral_time },
    
    // Garbage Collection https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-user/Garbage-Collection.html
    { "GC-FLIP", sch_gc_flip },
    { "PRINT-GC-STATISTICS", sch_print_gc_stats },
    
    { NULL, NULL }
    
};


const char* lib_program_defs = " \
(define (filter pred l) \
  (define (helper l result) \
    (cond ((null? l) result) \
          ((pred (car l)) \
           (helper (cdr l) (cons (car l) result))) \
          (else \
            (helper (cdr l) result)))) \
  (reverse! (helper l '()))) \
\
(define (some? pred l) \
  (cond ((null? l) '()) \
        ((pred (car l)) 1) \
        (else (some? pred (cdr l))))) \
\
(define (map1 proc l result) \
  (if (null? l) \
    (reverse! result) \
    (map1 proc \
          (cdr l) \
          (cons (proc (car l)) result)))) \
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
(define (alist->hash-table alist) \
  (define h (make-hash-table)) \
  (for-each (lambda (pair) \
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
(define (reverse l) (reverse! (list-copy l))) \
(define (vector-head v end) (subvector v 0 end)) \
(define (vector-tail v start) (subvector v start (vector-length v))) \
\
(define (vector-map fn v) \
  (let ((N (vector-length v)) \
        (o (make-vector (vector-length v) '() ))) \
    (do ((i 0 (+ i 1))) \
        ((>= i N) o) \
        (vector-set! o i (fn (vector-ref v i))) \
        ))) \
\
(define (sort l op)  \
  (if (null? l) '()  \
      (append (sort (filter (lambda (x) (op x (car l))) \
                                 (cdr l)) op) \
              (list (car l)) \
              (sort (filter (lambda (x) (not (op x (car l)))) \
                                 (cdr l)) op)))) \
";

LispContext lisp_init_lib(void)
{
    return lisp_init_lib_opt(LISP_DEFAULT_SYMBOL_TABLE_SIZE, LISP_DEFAULT_STACK_DEPTH, LISP_DEFAULT_PAGE_SIZE);
}

LispContext lisp_init_lib_opt(int symbol_table_size, size_t stack_depth, size_t page_size)
{
    LispContext ctx = lisp_init_empty_opt(symbol_table_size, stack_depth, page_size);

    Lisp table = lisp_make_table(300, ctx);
    //lisp_table_set(table, lisp_make_symbol("NULL", ctx), lisp_make_null(), ctx);
    lisp_table_define_funcs(table, lib_cfunc_defs, ctx);
    Lisp system_env = lisp_env_extend(lisp_make_null(), table, ctx);
    ctx.impl->global_env = lisp_env_extend(system_env, lisp_make_table(20, ctx), ctx);
    lisp_eval_opt(lisp_read(lib_program_defs, NULL, ctx), system_env, NULL, ctx);
    return ctx;
}

#endif

