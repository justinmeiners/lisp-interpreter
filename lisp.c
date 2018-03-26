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
#include "lisp.h"

typedef struct
{
    char* buffer;
    unsigned int size;
    unsigned int capacity;
} Heap;

typedef struct
{
    unsigned int hash;
    char string[];
} Symbol;



static void heap_init(Heap* heap, unsigned int capacity)
{
    heap->capacity = capacity;
    heap->size = 0;
    heap->buffer = malloc(heap->capacity);
}

static void heap_shutdown(Heap* heap) { free(heap->buffer); }

static LispBlock* heap_alloc(unsigned int data_size, LispType type, Heap* heap)
{
    if (heap->size + data_size + sizeof(LispBlock) > heap->capacity)
    {
        fprintf(stderr, "heap full\n");
        return NULL;
    }
    
    LispBlock* block = (LispBlock*)(heap->buffer + heap->size);
    block->gc_flags = 0;
    block->data_size = data_size;
    block->type = type;
    heap->size += sizeof(LispBlock) + data_size;
    return block;
}

static Lisp heap_cons(Lisp car, Lisp cdr, Heap* heap)
{
    LispBlock* block = heap_alloc(sizeof(Lisp) * 2, LISP_PAIR, heap);
    Lisp l;
    l.type = block->type;
    l.val = block;
    lisp_set_car(l, car);
    lisp_set_cdr(l, cdr);
    return l;
}

static LispBlock* heap_alloc_symbol(const char* string, size_t string_length, unsigned int hash, Heap* heap)
{
    // allocate a new block
    LispBlock* block = heap_alloc(sizeof(Symbol) + string_length, LISP_SYMBOL, heap);
    
    Symbol* symbol = (Symbol*)block->data;
    symbol->hash = hash;
    memcpy(symbol->string, string, string_length);

    // always convert symbols to uppercase
    char* c = symbol->string;
    
    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }

    return block;
}

// hash table
// linked list chaining
typedef struct 
{
    unsigned short size;
    unsigned short capacity;
    Lisp entries[];
} Table;

static Table* lisp_table(Lisp l)
{
    assert(lisp_type(l) == LISP_TABLE);
    LispBlock* block = l.val;
    return (Table*)block->data;
}

struct LispContext
{
    Heap heap;
    Lisp symbol_table;
    int lambda_counter;
};

static unsigned int hash_string(const char* buffer, unsigned int length)
{     
    // adler 32
    int s1 = 1;
    int s2 = 0;
    
    for (int i = 0; i < length; ++i)
    {
        s1 = (s1 + toupper(buffer[i])) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    
    return (s2 << 16) | s1;
}

Lisp lisp_null()
{
    Lisp l;
    l.type = LISP_NULL;
    l.int_val = 0;
    return l;
}

Lisp lisp_make_int(int n)
{
    Lisp l;
    l.type = LISP_INT;
    l.int_val = n;
    return l;
}

int lisp_int(Lisp l)
{
    if (l.type == LISP_FLOAT)
        return (int)l.float_val;
    return l.int_val;
}

Lisp lisp_make_float(float x)
{
    Lisp l;
    l.type = LISP_FLOAT;
    l.float_val = x;
    return l;
}

float lisp_float(Lisp l)
{
    if (l.type == LISP_INT)
        return(float)l.int_val;
    return l.float_val;
}

Lisp lisp_cons(Lisp car, Lisp cdr, LispContextRef ctx)
{
    return heap_cons(car, cdr, &ctx->heap);
}

static void back_append(Lisp* front, Lisp* back, Lisp item, LispContextRef ctx)
{
    Lisp new_l = lisp_cons(item, lisp_null(), ctx);
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

Lisp lisp_append(Lisp l, Lisp l2, LispContextRef ctx)
{
    if (lisp_is_null(l)) return l;

    Lisp tail = lisp_cons(lisp_car(l), lisp_null(), ctx);
    Lisp start = tail;
    l = lisp_cdr(l);

    while (!lisp_is_null(l))
    {
        Lisp cell = lisp_cons(lisp_car(l), lisp_null(), ctx);
        lisp_set_cdr(tail, cell);
        tail = cell;
        l = lisp_cdr(l);
    }

    lisp_set_cdr(tail, l2);
    return start;
}

Lisp lisp_at_index(Lisp l, int i)
{
    while (i > 0)
    {
        assert(l.type == LISP_PAIR);
        l = lisp_cdr(l);
        --i;
    }
    return lisp_car(l);
}

int lisp_length(Lisp l)
{
    int count = 0;
    while (!lisp_is_null(l))
    {
        ++count;
        l = lisp_cdr(l);
    }
    return count;
}

Lisp lisp_make_list(LispContextRef ctx, Lisp first, ...)
{
    Lisp front = lisp_cons(first, lisp_null(), ctx);
    Lisp back = front;

    va_list args;
    va_start(args, first);

    Lisp it = lisp_null();
    do
    {
        it = va_arg(args, Lisp);
        if (!lisp_is_null(it))
            back_append(&front, &back, it, ctx);
    } while (!lisp_is_null(it));

    va_end(args);

    return front;
}

Lisp lisp_assoc(Lisp l, Lisp key_symbol)
{
    while (!lisp_is_null(l))
    {
        Lisp pair = lisp_car(l);
        if (lisp_type(pair) == LISP_PAIR && lisp_eq(lisp_car(pair), key_symbol))
        {
            return pair;
        }

        l = lisp_cdr(l);
    }
    return lisp_null();
}

Lisp lisp_for_key(Lisp list, Lisp key_symbol)
{
    Lisp pair = lisp_assoc(list, key_symbol);
    return lisp_car(lisp_cdr(pair));
}

Lisp lisp_make_string(const char* string, LispContextRef ctx)
{
    unsigned int length = (unsigned int)strlen(string) + 1;
    LispBlock* block = heap_alloc(length, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    
    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

const char* lisp_string(Lisp l)
{
    assert(l.type == LISP_STRING);
    LispBlock* block = l.val;
    return block->data;
}

const char* lisp_symbol(Lisp l)
{
    assert(l.type == LISP_SYMBOL);
    LispBlock* block = l.val;
    Symbol* symbol = (Symbol*)block->data;
    return symbol->string;
}

static unsigned int symbol_hash(Lisp l)
{
    assert(l.type == LISP_SYMBOL);
    LispBlock* block = l.val;
    Symbol* symbol = (Symbol*)block->data;
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

    return lisp_null();
}

Lisp lisp_make_symbol(const char* string, LispContextRef ctx)
{
    unsigned int string_length = (unsigned int)strlen(string) + 1;
    unsigned int hash = hash_string(string, string_length);
    Lisp pair = table_get_string(ctx->symbol_table, string, hash);
    
    if (lisp_is_null(pair))
    {
        Lisp l;
        l.type = LISP_SYMBOL;
        l.val = heap_alloc_symbol(string, string_length, hash, &ctx->heap);
        lisp_table_set(ctx->symbol_table, l, lisp_null(), ctx);
        return l;
    }
    else
    {
        return lisp_car(pair);
    }
}

Lisp lisp_make_func(LispFunc func)
{
    Lisp l;
    l.type = LISP_FUNC;
    l.val = (LispBlock*)func;
    return l;
}

typedef struct
{
    int identifier;
    Lisp args;
    Lisp body;
    Lisp env;
} Lambda;

Lisp lisp_make_lambda(Lisp args, Lisp body, Lisp env, LispContextRef ctx)
{
    LispBlock* block = heap_alloc(sizeof(Lambda), LISP_LAMBDA, &ctx->heap);

    Lambda data;
    data.identifier = ctx->lambda_counter++;
    data.args = args;
    data.body = body;
    data.env = env;
    memcpy(block->data, &data, sizeof(Lambda));

    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

static Lambda lisp_lambda(Lisp lambda)
{
    LispBlock* block = lambda.val;
    return *(const Lambda*)block->data;
}

#pragma mark -
#pragma mark Lexer

typedef enum
{
    TOKEN_NONE = 0,
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_QUOTE,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
} TokenType;

/* for debug
static const char* token_type_name[] = {
    "NONE", "L_PAREN", "R_PAREN", "QUOTE", "SYMBOL", "STRING", "INT", "FLOAT",
}; */

typedef struct
{
    FILE* file;

    const char* sc; // start of token
    const char* c;  // scanner
    unsigned int scan_length;
    TokenType token;

    int sc_buff_index;
    int c_buff_index; 

    char* buffs[2];
    int buff_number[2];
    unsigned int buff_size;
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

    lex->buff_size = 4096;

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
    while (*lex->c)
    {
        // skip whitespace
        while (*lex->c && isspace(*lex->c)) lexer_step(lex);

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
        if (*lex->c == '-')
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
        if (*lex->c == '-')
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

    if (!found_decimal) return 0;
    return 1;
}

static int is_symbol(char c)
{
    if (c < '!' || c > 'z') return 0;
    const char* illegal= "()#;";
    const char* x = illegal;

    while (*x)
    {
        if (c == *x) return 0;
        ++x;
    }
    return 1;
}

static int lexer_match_symbol(Lexer* lex)
{   
    lexer_restart_scan(lex);
    // need at least one valid symbol character
    if (!is_symbol(*lex->c)) return 0;
    lexer_step(lex);
    while (*lex->c &&
            (is_symbol(*lex->c))) lexer_step(lex);
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

static void lexer_copy_token(Lexer* lex, unsigned int start_index, unsigned int length, char* dest)
{
    int token_length = lex->scan_length;
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
    else if (*lex->c == '\'')
    {
        lex->token = TOKEN_QUOTE;
        lexer_step(lex);
    }
    else if (lexer_match_string(lex))
    {
        lex->token = TOKEN_STRING;
    }
    else if (lexer_match_float(lex))
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

#pragma mark -
#pragma mark Read

#define SCRATCH_MAX 1024

static Lisp parse_atom(Lexer* lex, LispContextRef ctx)
{ 
    char scratch[SCRATCH_MAX];
    int length = lex->scan_length;
    Lisp l = lisp_null();

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
            l = lisp_make_float(atof(scratch));
            break;
        }
        case TOKEN_STRING:
        {
            // -2 length to skip quotes
            LispBlock* block = heap_alloc(length - 1, LISP_STRING, &ctx->heap);
            lexer_copy_token(lex, 1, length - 2, block->data);
            block->data[length - 2] = '\0';
            
            l.type = block->type;
            l.val = block;
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
        {
            fprintf(stderr, "read error - unknown l: %s\n", lex->sc);
        }
    }
    
    lexer_next_token(lex);
    return l;
}

// read tokens and construct S-expresions
static Lisp parse_list_r(Lexer* lex, LispContextRef ctx)
{  
    switch (lex->token)
    {
        case TOKEN_NONE:
        {
            fprintf(stderr, "parse error - expected: )\n");
            return lisp_null();
        }
        case TOKEN_L_PAREN:
        {
            Lisp front = lisp_null();
            Lisp back = lisp_null();

            // (
            lexer_next_token(lex);
            while (lex->token != TOKEN_R_PAREN)
            {
                Lisp l = parse_list_r(lex, ctx);
                back_append(&front, &back, l, ctx);
            }
            // )
            lexer_next_token(lex);
            return front;
        }
        case TOKEN_R_PAREN:
        {
            fprintf(stderr, "parse error - unexpected: )\n");
            lexer_next_token(lex);
            return lisp_null();
        }
        case TOKEN_QUOTE:
        {
             // '
             lexer_next_token(lex);
             Lisp l = lisp_cons(parse_list_r(lex, ctx), lisp_null(), ctx);
             return lisp_cons(lisp_make_symbol("QUOTE", ctx), l, ctx);
        }
        default:
        {
            return parse_atom(lex, ctx);
        } 
    }
}

static Lisp parse(Lexer* lex, LispContextRef ctx)
{
    lexer_next_token(lex);
    Lisp result = parse_list_r(lex, ctx);
    
    if (lex->token != TOKEN_NONE)
    {
        Lisp back = lisp_cons(result, lisp_null(), ctx);
        Lisp front = lisp_cons(lisp_make_symbol("BEGIN", ctx), back, ctx);
        
        while (lex->token != TOKEN_NONE)
        {
            Lisp next_result = parse_list_r(lex, ctx);
            back_append(&front, &back, next_result, ctx);
        } 

       result = front;
    }
    return result;
}

static Lisp expand_r(Lisp l, LispContextRef ctx)
{
    // 1. expand extended syntax into primitive syntax
    // 2. perform optimizations
    // 3. check syntax    
    if (l.type == LISP_SYMBOL &&
        lisp_eq(l, lisp_make_symbol("QUOTE", ctx)))
    {
        // don't expand quotes
        return l;
    }
    else if (l.type == LISP_PAIR)
    {
        const char* op = NULL;
        if (lisp_type(lisp_car(l)) == LISP_SYMBOL)
            op = lisp_symbol(lisp_car(l));

		if (op && strcmp(op, "QUOTE") == 0)
		{
			// don't expand quotes
			return l;
		}
        else if (op && strcmp(op, "DEFINE") == 0)
        {
            Lisp def = lisp_at_index(l, 1);
            
            if (def.type == LISP_PAIR)
            {
                // (define (<name> <arg0> ... <argn>) <body0> ... <bodyN>)
                // -> (define <name> (lambda (<arg0> ... <argn>) <body> ... <bodyN>))
                Lisp name = lisp_at_index(def, 0);
                Lisp body = lisp_cdr(lisp_cdr(l));
  
                Lisp lambda = lisp_make_list(ctx,
                                             lisp_make_symbol("LAMBDA", ctx),
                                             lisp_cdr(def), // args
                                             lisp_null()); 

                lisp_set_cdr(lisp_cdr(lambda), body);

                lisp_set_cdr(l, lisp_make_list(ctx,
                                              name,
                                              expand_r(lambda, ctx),
                                              body,
                                              lisp_null()));
                return l;
            }
            else
            {
                assert(def.type == LISP_SYMBOL);
                lisp_set_cdr(lisp_cdr(l), expand_r(lisp_cdr(lisp_cdr(l)), ctx));
                return l;
            }
        }
        else if (op && strcmp(op, "AND") == 0)
        {
            // (AND <pred0> <pred1>) -> (IF <pred0> (IF <pred1> t f) f)
            Lisp predicate0 = expand_r(lisp_at_index(l, 1), ctx);
            Lisp predicate1 = expand_r(lisp_at_index(l, 2), ctx);

            Lisp inner = lisp_make_list(ctx, 
                                        lisp_make_symbol("IF", ctx),
                                        predicate1,
                                        lisp_make_int(1), 
                                        lisp_make_int(0),
                                        lisp_null());

            Lisp outer = lisp_make_list(ctx,
                                        lisp_make_symbol("IF", ctx),
                                        predicate0,
                                        inner,
                                        lisp_make_int(0),
                                        lisp_null());

            return outer;
        }
        else if (op && strcmp(op, "OR") == 0)
        {
            // (OR <pred0> <pred1>) -> (IF (<pred0>) t (IF <pred1> t f))
            Lisp predicate0 = expand_r(lisp_at_index(l, 1), ctx);
            Lisp predicate1 = expand_r(lisp_at_index(l, 2), ctx);
            
            Lisp inner = lisp_make_list(ctx,
                                        lisp_make_symbol("IF", ctx),
                                        predicate1,
                                        lisp_make_int(1),
                                        lisp_make_int(0),
                                        lisp_null());
            
            Lisp outer = lisp_make_list(ctx,
                                        lisp_make_symbol("IF", ctx),
                                        predicate0,
                                        lisp_make_int(1),
                                        inner,
                                        lisp_null());
            
            return outer;

        }
        else if (op && strcmp(op, "LET") == 0)
        {
            // (LET ((<var0> <expr0>) ... (<varN> <expr1>)) <body0> ... <bodyN>)
            //  -> ((LAMBDA (<var0> ... <varN>) <body0> ... <bodyN>) <expr0> ... <expr1>)            
            Lisp pairs = lisp_at_index(l, 1);
            Lisp body = lisp_cdr(lisp_cdr(l));

            Lisp vars_front = lisp_null();
            Lisp vars_back = vars_front;
            
            Lisp exprs_front = lisp_null();
            Lisp exprs_back = exprs_front;
            
            while (!lisp_is_null(pairs))
            {
                Lisp var = lisp_at_index(lisp_car(pairs), 0);
                assert(var.type == LISP_SYMBOL);
                back_append(&vars_front, &vars_back, var, ctx);

                Lisp val = expand_r(lisp_at_index(lisp_car(pairs), 1), ctx);
                back_append(&exprs_front, &exprs_back, val, ctx);
                pairs = lisp_cdr(pairs);
            }
            
            Lisp lambda = lisp_make_list(ctx, 
                                        lisp_make_symbol("LAMBDA", ctx), 
                                        vars_front, 
                                        lisp_null());

            lisp_set_cdr(lisp_cdr(lambda), body);

            return lisp_cons(expand_r(lambda, ctx), exprs_front, ctx);
        }
        else if (op && strcmp(op, "LAMBDA") == 0)
        {
            // (LAMBDA (<var0> ... <varN>) <expr0> ... <exprN>)
            // (LAMBDA (<var0> ... <varN>) (BEGIN <expr0> ... <expr1>)) 
            int length = lisp_length(l);
            if (length > 3)
            {
                Lisp body_exprs = expand_r(lisp_cdr(lisp_cdr(l)), ctx); 
                Lisp begin = lisp_cons(lisp_make_symbol("BEGIN", ctx), body_exprs, ctx);
                return lisp_make_list(ctx, 
                                      lisp_at_index(l, 0),  // LAMBDA
                                      lisp_at_index(l, 1),  // vars
                                      begin,
                                      lisp_null()); 
            }
            else
            {
				Lisp body = lisp_cdr(lisp_cdr(l));
                lisp_set_cdr(lisp_cdr(l), expand_r(body, ctx));
                return l;
            }
        }
        else if (op && strcmp(op, "ASSERT") == 0)
        {
            Lisp statement = lisp_car(lisp_cdr(l));
            Lisp quoted = lisp_make_list(ctx,
                                         lisp_make_symbol("QUOTE", ctx),
                                         statement,
                                         lisp_null());
            return lisp_make_list(ctx,
                                  lisp_at_index(l, 0),
                                  expand_r(statement, ctx),
                                  quoted,
                                  lisp_null());
                                  
        }
        else
        {
            Lisp it = l;

            while (!lisp_is_null(it))
            {
                lisp_set_car(it, expand_r(lisp_car(it), ctx));
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

Lisp lisp_read(const char* program, LispContextRef ctx)
{
    Lexer lex;
    lexer_init(&lex, program);
    Lisp l = parse(&lex, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_file(FILE* file, LispContextRef ctx)
{
    Lexer lex;
    lexer_init_file(&lex, file);
    Lisp l = parse(&lex, ctx);
    lexer_shutdown(&lex);
    return l;
}

Lisp lisp_read_path(const char* path, LispContextRef ctx)
{
    FILE* file = fopen(path, "r");

    if (!file)
    {
        return lisp_null();
    }

    Lisp l = lisp_read_file(file, ctx);
    fclose(file);
    return l;
}

Lisp lisp_expand(Lisp lisp, LispContextRef ctx)
{
    return expand_r(lisp, ctx);
}
 
static const char* lisp_type_name[] = {
    "FLOAT",
    "INT",
    "PAIR",
    "SYMBOL",
    "STRING",
    "LAMBDA",
    "PROCEDURE",
    "ENV",
    "NULL",
};

Lisp lisp_make_table(unsigned int capacity, LispContextRef ctx)
{
    unsigned int size = sizeof(Table) + sizeof(Lisp) * capacity;
    LispBlock* block = heap_alloc(size, LISP_TABLE, &ctx->heap);

    Table* table = (Table*)block->data;
    table->size = 0;
    table->capacity = capacity;

    // clear the table
    for (int i = 0; i < capacity; ++i)
        table->entries[i] = lisp_null();

    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

void lisp_table_set(Lisp l, Lisp symbol, Lisp value, LispContextRef ctx)
{
    Table* table = lisp_table(l);
    
    unsigned int index = symbol_hash(symbol) % table->capacity;
    Lisp pair = lisp_assoc(table->entries[index], symbol);

    if (lisp_is_null(pair))
    {
        // new value. prepend to front of chain
        pair = lisp_cons(symbol, value, ctx);
        table->entries[index] = lisp_cons(pair, table->entries[index], ctx);
        ++table->size;
    }
    else
    {
        // reassign cdr value (key, val)
        lisp_set_cdr(pair, value);
    }
}

Lisp lisp_table_get(Lisp l, Lisp symbol, LispContextRef ctx)
{
    Table* table = lisp_table(l);
    unsigned int index = symbol_hash(symbol) % table->capacity;
    return lisp_assoc(table->entries[index], symbol);
}

void lisp_table_add_funcs(Lisp table, const char** names, LispFunc* funcs, LispContextRef ctx)
{
    const char** name = names;
    LispFunc* func = funcs;

    while (*name)
    {
        lisp_table_set(table, lisp_make_symbol(*name, ctx), lisp_make_func(*func), ctx);
        ++name;
        ++func;
    }
}

Lisp lisp_make_env(Lisp table, LispContextRef ctx)
{
    return lisp_cons(table, lisp_null(), ctx);
}

Lisp lisp_env_extend(Lisp env, Lisp table, LispContextRef ctx)
{
    return lisp_cons(table, env, ctx);
}

Lisp lisp_env_lookup(Lisp env, Lisp symbol, LispContextRef ctx)
{
    Lisp it = env;
    while (!lisp_is_null(it))
    {
        Lisp x = lisp_table_get(lisp_car(it), symbol, ctx);
        if (!lisp_is_null(x)) return x;
        it = lisp_cdr(it);
    }
    
    return lisp_null();
}

void lisp_env_define(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx)
{
    lisp_table_set(lisp_car(env), symbol, value, ctx);
}

void lisp_env_set(Lisp env, Lisp symbol, Lisp value, LispContextRef ctx)
{
    Lisp pair = lisp_env_lookup(env, symbol, ctx);
    if (lisp_is_null(pair))
    {
        fprintf(stderr, "error: unknown variable: %s\n", lisp_symbol(symbol));
    }
    lisp_set_cdr(pair, value);
}

static void lisp_print_r(FILE* file, Lisp l, int is_cdr)
{
    switch (lisp_type(l))
    {
        case LISP_INT:
            fprintf(file, "%i", lisp_int(l));
            break;
        case LISP_FLOAT:
            fprintf(file, "%f", lisp_float(l));
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
        case LISP_LAMBDA:
            fprintf(file, "lambda-%i", lisp_lambda(l).identifier);
            break;
        case LISP_FUNC:
            fprintf(file, "function-%x", (unsigned int)l.val); 
            break;
        case LISP_TABLE:
        {
            Table* table = lisp_table(l);
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
        case LISP_PAIR:
            if (!is_cdr) fprintf(file, "(");
            lisp_print_r(file, lisp_car(l), 0);

            if (lisp_cdr(l).type != LISP_PAIR)
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

void lisp_printf(FILE* file, Lisp l) { lisp_print_r(file, l, 0);  }
void lisp_print(Lisp l) {  lisp_printf(stdout, l); }

Lisp lisp_eval(Lisp x, Lisp env, LispContextRef ctx)
{
    while (1)
    {
        assert(!lisp_is_null(env));
        LispType type = lisp_type(x);
        
        if (type == LISP_INT ||
            type == LISP_FLOAT ||
            type == LISP_STRING ||
            type == LISP_LAMBDA ||
            type == LISP_NULL)
        {
            // atom
            return x;
        }
        else if (type == LISP_SYMBOL)
        {
            // variable reference
            Lisp pair = lisp_env_lookup(env, x, ctx);
            
            if (lisp_is_null(pair))
            {
                fprintf(stderr, "cannot find variable: %s\n", lisp_symbol(x));
                return lisp_null();
            }
            return lisp_cdr(pair);
        }
        else if (type == LISP_PAIR) 
        { 
            const char* op_name = NULL;
            if (lisp_type(lisp_car(x)) == LISP_SYMBOL)
                op_name = lisp_symbol(lisp_car(x));

            if (op_name && strcmp(op_name, "IF") == 0) // if conditional statemetns
            {
                Lisp predicate = lisp_at_index(x, 1);
                Lisp conseq = lisp_at_index(x, 2);
                Lisp alt = lisp_at_index(x, 3);

                if (lisp_int(lisp_eval(predicate, env, ctx)) != 0)
                {
                    x = conseq; // while will eval
                } 
                else
                {
                    x = alt; // while will eval
                }
            }
            else if (op_name && strcmp(op_name, "BEGIN") == 0)
            {
                Lisp it = lisp_cdr(x);
                if (lisp_is_null(it)) return it;

                // eval all but last
                while (!lisp_is_null(lisp_cdr(it)))
                {
                    lisp_eval(lisp_car(it), env, ctx);
                    it = lisp_cdr(it);
                }
                
                x = lisp_car(it); // while will eval last
            }
            else if (op_name && strcmp(op_name, "QUOTE") == 0)
            {
                return lisp_at_index(x, 1);
            }
            else if (op_name && strcmp(op_name, "DEFINE") == 0) // variable definitions
            {
                Lisp symbol = lisp_at_index(x, 1); 
                Lisp value = lisp_eval(lisp_at_index(x, 2), env, ctx);
                lisp_env_define(env, symbol, value, ctx);
                return lisp_null();
            }
            else if (op_name && strcmp(op_name, "SET!") == 0)
            {
                // mutablity
                // like def, but requires existence
                // and will search up the environment chain
                Lisp symbol = lisp_at_index(x, 1);
                lisp_env_set(env, symbol, lisp_eval(lisp_at_index(x, 2), env, ctx), ctx);
                return lisp_null();
            }
            else if (op_name && strcmp(op_name, "LAMBDA") == 0) // lambda defintions (compound procedures)

            {
                Lisp args = lisp_at_index(x, 1);
                Lisp body = lisp_at_index(x, 2);
                return lisp_make_lambda(args, body, env, ctx);
            }
            else // operator application
            {
                Lisp operator = lisp_eval(lisp_car(x), env, ctx);
                Lisp arg_expr = lisp_cdr(x);
                
                Lisp args_front = lisp_null();
                Lisp args_back = lisp_null();
                
                while (!lisp_is_null(arg_expr))
                {
                    Lisp new_arg = lisp_eval(lisp_car(arg_expr), env, ctx);
                    back_append(&args_front, &args_back, new_arg, ctx);
                    arg_expr = lisp_cdr(arg_expr);
                }
                
                switch (lisp_type(operator))
                {
                    case LISP_LAMBDA: // lambda call (compound procedure)
                    {
                        Lambda lambda = lisp_lambda(operator);
                        // make a new environment
                        Lisp new_table = lisp_make_table(19, ctx);                       
                        // bind parameters to arguments
                        // to pass into function call
                        Lisp keyIt = lambda.args;
                        Lisp valIt = args_front;
                        
                        while (!lisp_is_null(keyIt))
                        {
                            lisp_table_set(new_table, lisp_car(keyIt), lisp_car(valIt), ctx);
                            keyIt = lisp_cdr(keyIt);
                            valIt = lisp_cdr(valIt);
                        }
 
                        // normally we would eval the body here
                        // but while will eval
                        x = lambda.body;

                        // extend the environment
                        env = lisp_env_extend(lambda.env, new_table, ctx);
                        break;
                    }
                    case LISP_FUNC: // call into C functions
                    {
                        // no environment required 
                        LispFunc func = operator.val;
                        return func(args_front, ctx);
                    }
                    default:
                    {
                        fprintf(stderr, "apply error: not an operator %s\n", lisp_type_name[operator.type]);
                        return lisp_null();
                    }
                }
            }
        }
        else
        {
            fprintf(stderr,"here: %s\n", lisp_type_name[x.type]);
            return lisp_null();
        }
    }
}

LispContextRef lisp_init(unsigned int heap_size)
{
    LispContextRef ctx = malloc(sizeof(struct LispContext));
    ctx->lambda_counter = 0;
    heap_init(&ctx->heap, heap_size);

    ctx->symbol_table = lisp_make_table(2048, ctx);
    return ctx;
}

void lisp_shutdown(LispContextRef ctx)
{
    heap_shutdown(&ctx->heap);
    free(ctx);
}

#pragma Garbage Collection

enum
{
    GC_MOVED = (1 << 0), // has this block been moved to the to-space?
    GC_VISITED = (1 << 1), // has this block's pointers been moved?
};

static inline Lisp gc_move(Lisp l, Heap* to)
{
    switch (l.type)
    {
        case LISP_PAIR:
        case LISP_SYMBOL:
        case LISP_STRING:
        case LISP_LAMBDA:
        {
            LispBlock* block = l.val;
            
            if (!(block->gc_flags & GC_MOVED))
            {
                unsigned int address = to->size;
                
                LispBlock* dest = (LispBlock*)(to->buffer + to->size);
                size_t block_size = sizeof(LispBlock) + block->data_size;
                to->size += block_size;
                
                // copy the data
                memcpy(dest, block, block_size);
                dest->gc_flags = 0;
                
                // save forwarding address (offset in to)
                block->data_size = address;
                block->gc_flags = GC_MOVED;
            }
            
            // assign block address back to l
            l.val = (LispBlock*)(to->buffer + block->data_size);
            return l;
        }
        case LISP_TABLE:
        {
            LispBlock* block = l.val;
            
            if (!(block->gc_flags & GC_MOVED))
            {
                Table* table = (Table*)block->data;
                float load_factor = table->size / (float)table->capacity;
                
                unsigned int new_capacity = table->capacity;
                
                if (load_factor > 0.6f || load_factor < 0.05f)
                {
                    new_capacity = (table->size * 4);
                }

                unsigned int address = to->size;
                LispBlock* dest = (LispBlock*)(to->buffer + to->size);
                dest->type = block->type;
                dest->data_size = sizeof(Table) + new_capacity * sizeof(Lisp);
                dest->gc_flags = 0;
                size_t block_size = sizeof(LispBlock) + dest->data_size;
                to->size += block_size;
                
                Table* dest_table = (Table*)dest->data;
                dest_table->size = table->size;
                dest_table->capacity = new_capacity;
                
                // clear the table
                for (int i = 0; i < new_capacity; ++i)
                    dest_table->entries[i] = lisp_null();
                
                // save forwarding address (offset in to)
                block->data_size = address;
                block->gc_flags = GC_MOVED;
                

                if (LISP_DEBUG && new_capacity != table->capacity)
                {
                    printf("resizing table %i -> %i\n", table->capacity, new_capacity);
                }
                
                for (int i = 0; i < table->capacity; ++i)
                {
                    Lisp it = table->entries[i];
                    
                    while (!lisp_is_null(it))
                    {
                        unsigned int new_index = i;
                        
                        if (new_capacity != table->capacity)
                            new_index = symbol_hash(lisp_car(lisp_car(it))) % new_capacity;
                        
                        Lisp pair = gc_move(lisp_car(it), to);
                        
                        Lisp new_cell = heap_cons(pair, dest_table->entries[new_index], to);
                        dest_table->entries[new_index] = new_cell;
                        
                        LispBlock* block = new_cell.val;
                        block->gc_flags = GC_VISITED;
                        
                        it = lisp_cdr(it);
                    }
                }
            }
            
            // assign block address back to l
            l.val = (LispBlock*)(to->buffer + block->data_size);
            return l;
        }
        default:
            return l;
    }
}

Lisp lisp_collect(Lisp root_to_save, LispContextRef ctx)
{
    Heap* from = &ctx->heap;
    Heap to;
    heap_init(&to, from->capacity);
 
    // move root object
    ctx->symbol_table = gc_move(ctx->symbol_table, &to);
    Lisp result = gc_move(root_to_save, &to);

    // move references
    int passes = 0;
    while (1)
    {
        ++passes;
        int done = 1;
        size_t offset = 0;
        while (offset < to.size)
        {
            LispBlock* block = (LispBlock*)(to.buffer + offset);

            if (!(block->gc_flags & GC_VISITED))
            {
                // these add to the buffer!
                // so lists are handled in a single pass
                if (block->type == LISP_PAIR)
                {
                    // move the CAR and CDR
                    Lisp* ptrs = (Lisp*)block->data;
                    for (int i = 0; i < 2; ++i)
                    {
                        if (!lisp_is_null(ptrs[i]))
                            ptrs[i] = gc_move(ptrs[i], &to);
                    }
                    done = 0;
                }
                else if (block->type == LISP_LAMBDA)
                {
                    // move the body and args
                    Lambda* lambda = (Lambda*)block->data;
                    lambda->args = gc_move(lambda->args, &to);
                    lambda->body = gc_move(lambda->body, &to);
                    lambda->env = gc_move(lambda->env, &to);
                    done = 0;
                }
                
                block->gc_flags |= GC_VISITED;
            }

            offset += sizeof(LispBlock) + block->data_size;
        }

        if (done) break;
    }

    if (LISP_DEBUG)
        printf("gc passes: %i\n", passes);

    assert(passes <= 2);
 
    {
        // clear the flags
        size_t offset = 0;
        while (offset < to.size)
        {
            LispBlock* block = (LispBlock*)(to.buffer + offset);
            offset += sizeof(LispBlock) + block->data_size;
        }
        assert(offset == to.size);
    }
    
    size_t diff = from->size - to.size;
    heap_shutdown(from);
    *from = to;

    if (LISP_DEBUG)
        printf("gc collected: %lu heap: %i\n", diff, ctx->heap.size);

    return result;
}

#pragma mark Default Enviornment

static Lisp func_cons(Lisp args, LispContextRef ctx)
{
    return lisp_cons(lisp_car(args), lisp_car(lisp_cdr(args)), ctx);
}

static Lisp func_car(Lisp args, LispContextRef ctx)
{
    return lisp_car(lisp_car(args));
}

static Lisp func_cdr(Lisp args, LispContextRef ctx)
{
    return lisp_cdr(lisp_car(args));
}

static Lisp func_eq(Lisp args, LispContextRef ctx)
{
    void* a = lisp_car(args).val;
    void* b = lisp_car(lisp_cdr(args)).val;

    return lisp_make_int(a == b);
}

static Lisp func_is_null(Lisp args, LispContextRef ctx)
{
    while (!lisp_is_null(args))
    {
        if (!lisp_is_null(lisp_car(args))) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    return lisp_make_int(1);
}

static Lisp func_display(Lisp args, LispContextRef ctx)
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
    return lisp_null();
}

static Lisp func_newline(Lisp args, LispContextRef ctx)
{
    printf("\n"); return lisp_null(); 
}

static Lisp func_assert(Lisp args, LispContextRef ctx)
{
   if (lisp_int(lisp_car(args)) != 1)
   {
       fprintf(stderr, "assertion: ");
       lisp_printf(stderr, lisp_car(lisp_cdr(args)));
       fprintf(stderr, "\n");
       assert(0);
   }

   return lisp_null();
}

static Lisp func_equals(Lisp args, LispContextRef ctx)
{
    Lisp to_check  = lisp_car(args);
    if (lisp_is_null(to_check)) return lisp_make_int(1);
    
    args = lisp_cdr(args);
    while (!lisp_is_null(args))
    {
        if (lisp_int(lisp_car(args)) != lisp_int(to_check)) return lisp_make_int(0);
        args = lisp_cdr(args);
    }
    
    return lisp_make_int(1);
}

static Lisp func_list(Lisp args, LispContextRef ctx)
{
    return args;
}

static Lisp func_append(Lisp args, LispContextRef ctx)
{
    Lisp l = lisp_car(args);

    args = lisp_cdr(args);
    while (!lisp_is_null(args))
    {
        l = lisp_append(l, lisp_car(args), ctx);
        args = lisp_cdr(args);
    }

    return l;
}

static Lisp func_nth(Lisp args, LispContextRef ctx)
{
    Lisp index = lisp_car(args);
    Lisp list = lisp_car(lisp_cdr(args));
    return lisp_at_index(list, lisp_int(index));
}

static Lisp func_length(Lisp args, LispContextRef ctx)
{
    return lisp_make_int(lisp_length(lisp_car(args)));
}

static Lisp func_assoc(Lisp args, LispContextRef ctx)
{
    return lisp_assoc(lisp_car(args), lisp_car(lisp_cdr(args)));
}

static Lisp func_add(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (!lisp_is_null(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.int_val += lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_FLOAT)
        {
            accum.float_val += lisp_float(lisp_car(args));
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp func_sub(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (!lisp_is_null(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.int_val -= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_FLOAT)
        {
            accum.float_val -= lisp_float(lisp_car(args));
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp func_mult(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (!lisp_is_null(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.int_val *= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_FLOAT)
        {
            accum.float_val *= lisp_float(lisp_car(args));
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp func_divide(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);

    while (!lisp_is_null(args))
    {
        if (lisp_type(accum) == LISP_INT)
        {
            accum.int_val /= lisp_int(lisp_car(args));
        }
        else if (lisp_type(accum) == LISP_FLOAT)
        {
            accum.float_val /= lisp_float(lisp_car(args));
        }
        args = lisp_cdr(args);
    }
    return accum;
}

static Lisp func_less(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);
    int result = 0;

    if (lisp_type(accum) == LISP_INT)
    {
        result = lisp_int(accum) < lisp_int(lisp_car(args));
    }
    else if (lisp_type(accum) == LISP_FLOAT)
    {
        result = lisp_float(accum) < lisp_float(lisp_car(args));
    }
    
    return lisp_make_int(result);
}

static Lisp func_greater(Lisp args, LispContextRef ctx)
{
    Lisp accum = lisp_car(args);
    args = lisp_cdr(args);
    int result = 0;

    if (lisp_type(accum) == LISP_INT)
    {
        result = lisp_int(accum) > lisp_int(lisp_car(args));
    }
    else if (lisp_type(accum) == LISP_FLOAT)
    {
        result = lisp_float(accum) > lisp_float(lisp_car(args));
    }
    
    return lisp_make_int(result);
}

static Lisp func_read_path(Lisp args, LispContextRef ctx)
{
    const char* path = lisp_string(lisp_car(args));
    return lisp_read_path(path, ctx); 
}

static Lisp func_expand(Lisp args, LispContextRef ctx)
{
    Lisp expr = lisp_car(args);
    return lisp_expand(expr, ctx);
}

Lisp lisp_make_default_env(LispContextRef ctx)
{
    Lisp table = lisp_make_table(15, ctx);
    Lisp env = lisp_make_env(table, ctx); 

    lisp_table_set(table, lisp_make_symbol("NULL", ctx), lisp_null(), ctx);
    //lisp_table_set(table, lisp_make_symbol("global-env", ctx), env, ctx);

    const char* names[] = {
        "CONS",
        "CAR",
        "CDR",
        "EQ?",
        "NULL?",
        "LIST",
        "APPEND",
        "NTH",
        "LENGTH",
        "ASSOC",
        "DISPLAY",
        "NEWLINE",
        "ASSERT",
        "READ-PATH",
        "EXPAND",
        "=",
        "+",
        "-",
        "*",
        "/",
        "<",
        ">",
        NULL,
    };

    LispFunc funcs[] = {
        func_cons,
        func_car,
        func_cdr,
        func_eq,
        func_is_null,
        func_list,
        func_append,
        func_nth,
        func_length,
        func_assoc,
        func_display,
        func_newline,
        func_assert,
        func_read_path,
        func_expand,
        func_equals,
        func_add,
        func_sub,
        func_mult,
        func_divide,
        func_less,
        func_greater,
        NULL,
    };

    lisp_table_add_funcs(table, names, funcs, ctx);
    return env;
}

