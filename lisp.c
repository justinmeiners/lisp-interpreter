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
#include "lisp.h"

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

// Lexer tokens
typedef struct
{
    TokenType type;
    unsigned int length;
    const char* start;
} Token;

static const char* skip_ignored(const char* c)
{
    while (*c)
    {
        // skip whitespace
        while (*c && isspace(*c)) ++c;

        // skip comments to end of line
        if (*c == ';')
        {
            while (*c && *c != '\n') ++c; 
        }
        else
        {
            break;
        }
    }
    return c;
}

int is_symbol(char c)
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

static int match_symbol(const char* c, const char** oc)
{   
    if (!is_symbol(*c)) return 0;
    ++c;
    while (*c &&
            (is_symbol(*c))) ++c;
    *oc = c;
    return 1;
}

static int match_string(const char* c, const char** oc)
{
    if (*c != '"') return 0;

    ++c; 
    while (*c != '"')
    {
        if (*c == '\0' || *c == '\n')
            return 0;
        ++c;
    }
    ++c;
    *oc = c;
    return 1;
}

static int match_int(const char* c, const char** oc)
{
    if (!isdigit(*c)) return 0;
    ++c;
    while (isdigit(*c)) ++c;
    *oc = c;
    return 1;
}

static int match_float(const char* c, const char** oc)
{
    if (!isdigit(*c)) return 0;

    int found_decimal = 0;
    while (isdigit(*c) || *c == '.')
    {
        if (*c == '.') found_decimal = 1;
        ++c;
    }

    if (!found_decimal) return 0;
    *oc = c;
    return 1;
}

static int read_token(const char* c,
                      const char** oc,
                      Token* tok)
{
    c = skip_ignored(c);

    const char* nc = NULL; 

    if (*c == '(')
    {
        tok->type = TOKEN_L_PAREN;
        nc = c + 1;
    }
    else if (*c == ')')
    {
        tok->type = TOKEN_R_PAREN;
        nc = c + 1;
    }
    else if (*c == '\'')
    {
        tok->type = TOKEN_QUOTE;
        nc = c + 1;
    }
    else if (match_string(c, &nc))
    {
        tok->type = TOKEN_STRING;
    }
    else if (match_float(c, &nc))
    {
        tok->type = TOKEN_FLOAT;
    } 
    else if (match_int(c, &nc))
    {
        tok->type = TOKEN_INT;
    } 
    else if (match_symbol(c, &nc))
    {
        tok->type = TOKEN_SYMBOL;
    }
    else
    {
        tok->type = TOKEN_NONE;
        return 0;
    }
    // save pointers to token text
    tok->start = c;
    tok->length = (unsigned int)(nc - c);
    *oc = nc;   
    return 1;
}

// lexical analyis of program text
static Token* tokenize(const char* program, int* count)
{
    // growing vector of tokens
    int buffer_size = 512;
    Token* tokens = malloc(sizeof(Token) * buffer_size);
    int token_count = 0;

    const char* c = program;

    while (*c != '\0' && read_token(c, &c, tokens + token_count))
    {
        ++token_count; 
        if (token_count + 1 == buffer_size)
        {
            buffer_size = (buffer_size * 3) / 2;
            tokens = realloc(tokens, sizeof(Token) * buffer_size);
        } 
    }

    *count = token_count;
    return tokens;
}

LispType lisp_type(Lisp l) { return l.type; }
int lisp_is_null(Lisp l) { return lisp_type(l) == LISP_NULL; }

Lisp lisp_null()
{
    Lisp l;
    l.type = LISP_NULL;
    l.int_val = 0;
    return l;
}

Lisp lisp_create_int(int n)
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

Lisp lisp_create_float(float x)
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

static void heap_init(LispHeap* heap, unsigned int capacity)
{
    heap->capacity = capacity;
    heap->size = 0;
    heap->buffer = malloc(heap->capacity);
}

static void heap_shutdown(LispHeap* heap) { free(heap->buffer); }

static LispBlock* block_alloc(unsigned int data_size, LispType type, LispHeap* heap)
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

Lisp lisp_cons(Lisp car, Lisp cdr, LispContext* ctx)
{
    LispBlock* block = block_alloc(sizeof(Lisp) * 2, LISP_PAIR, &ctx->heap);
    Lisp l;
    l.type = block->type;
    l.val = block;
    lisp_car(l) = car;
    lisp_cdr(l) = cdr;
    return l;
}

Lisp lisp_at_index(Lisp list, int i)
{
    while (i > 0)
    {
        list = lisp_cdr(list);
        --i;
    }
    return lisp_car(list);
}

Lisp lisp_create_string(const char* string, LispContext* ctx)
{
    unsigned int length = (unsigned int)strlen(string) + 1;
    LispBlock* block = block_alloc(length, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    
    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

static Lisp string_from_view(const char* string, unsigned int length, LispContext* ctx)
{
    LispBlock* block = block_alloc(length + 1, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    block->data[length] = '\0';
    
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

typedef struct
{
    unsigned int hash;
    char string[];
} Symbol;

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

static void symbol_table_init(SymbolTable* table, int capacity)
{
    table->size = 0;
    table->capacity = capacity;
    table->symbols = malloc(sizeof(LispBlock*) * capacity);

    for (int i = 0; i < table->capacity; ++i)
        table->symbols[i] = NULL;
}

static void symbol_table_shutdown(SymbolTable* table)
{
    free(table->symbols);
}

static LispBlock* symbol_table_get(SymbolTable* table, const char* string, unsigned int max_length, unsigned int hash)
{
    unsigned int index = hash % table->capacity;

    for (int i = 0; i < table->capacity - 1; ++i)
    {
        if (!table->symbols[index])
        {
            return NULL;
        }
        else
        {
            const Symbol* symbol = (const Symbol*)table->symbols[index]->data;
            
            if (strncasecmp(symbol->string, string, max_length) == 0)
            {
                return (LispBlock*)table->symbols[index];
            }
        }

        index = (index + 1) % table->capacity;
    }
 
    return NULL;
}

static void symbol_table_add(SymbolTable* table, const LispBlock* symbol_block)
{
    // this only works if the item has never been added before
    Symbol* symbol = (Symbol*)symbol_block->data;
    unsigned int index = symbol->hash % table->capacity;

    for (int i = 0; i < table->capacity - 1; ++i)
    {
        if (!table->symbols[index])
        {
            table->symbols[index] = symbol_block;
            ++table->size;

            float load_factor = table->size / (float)table->capacity;
           
            if (load_factor > 0.6f)
            {
                SymbolTable dest;
                symbol_table_init(&dest, table->capacity * 2);
                for (int j = 0; j < table->capacity; ++j)
                    symbol_table_add(&dest, table->symbols[j]);

                symbol_table_shutdown(table);
                *table = dest;
            }

            return;
        }

        index = (index + 1) % table->capacity;
    }     

    // table full which should never happen
    // because of resizing
    assert(0);
}

Lisp lisp_create_symbol(const char* string, LispContext* ctx)
{
    unsigned int string_length = (unsigned int)strlen(string) + 1;
    unsigned int hash = hash_string(string, string_length);
    LispBlock* block = (LispBlock*)symbol_table_get(&ctx->symbols, string, 2048, hash);
    
    if (block == NULL)
    {
        // allocate a new block
        block = block_alloc(sizeof(Symbol) + string_length, LISP_SYMBOL, &ctx->heap);
        
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
        
        symbol_table_add(&ctx->symbols, block);
    }
    
    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

static Lisp symbol_from_view(const char* string, unsigned int string_length, LispContext* ctx)
{
    // always convert symbols to uppercase
    unsigned int hash = hash_string(string, string_length);
    LispBlock* block = (LispBlock*)symbol_table_get(&ctx->symbols, string, string_length, hash);
    
    if (block == NULL)
    {
        // allocate a new block
        block = block_alloc(sizeof(Symbol) + string_length + 1, LISP_SYMBOL, &ctx->heap);
        
        Symbol* symbol = (Symbol*)block->data;
        symbol->hash = hash;
        memcpy(symbol->string, string, string_length);
        symbol->string[string_length] = '\0';
        
        // always convert symbols to uppercase
        char* c = symbol->string;
        
        while (*c)
        {
            *c = toupper(*c);
            ++c;
        }
        
        symbol_table_add(&ctx->symbols, block);
    }
    
    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

Lisp lisp_create_proc(LispProc func)
{
    Lisp l;
    l.type = LISP_PROC;
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

Lisp lisp_create_lambda(Lisp args, Lisp body, Lisp env, LispContext* ctx)
{
    LispBlock* block = block_alloc(sizeof(Lambda), LISP_LAMBDA, &ctx->heap);

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

Lambda lisp_lambda(Lisp lambda)
{
    LispBlock* block = lambda.val;
    return *(const Lambda*)block->data;
}

#define SCRATCH_MAX 128

static Lisp read_atom(const Token** pointer, LispContext* ctx) 
{
    const Token* token = *pointer;
    
    char scratch[SCRATCH_MAX];
    Lisp l = lisp_null();

    switch (token->type)
    {
        case TOKEN_INT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            l = lisp_create_int(atoi(scratch));
            break;
        case TOKEN_FLOAT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            l = lisp_create_float(atof(scratch));
            break;
        case TOKEN_STRING:
            l = string_from_view(token->start + 1, token->length - 2, ctx);
            break;
        case TOKEN_SYMBOL:
            l = symbol_from_view(token->start, token->length, ctx);
            break;
        default: 
            fprintf(stderr, "read error - unknown l: %s\n", token->start);
            break;
    }

    *pointer = (token + 1); 
    return l;
}

// read tokens and construct S-expresions
static Lisp read_list_r(const Token** begin, const Token* end, LispContext* ctx)
{ 
    const Token* token = *begin;
    
    if (token == end)
    {
        fprintf(stderr, "read error - expected: )\n");
    }

    Lisp start = lisp_null();

    if (token->type == TOKEN_L_PAREN)
    {
        ++token;
        Lisp previous = lisp_null();

        while (token->type != TOKEN_R_PAREN)
        {
            Lisp l = lisp_cons(read_list_r(&token, end, ctx), lisp_null(), ctx);

            if (!lisp_is_null(previous))
            {
                lisp_cdr(previous) = l;
            }
            else
            {
                start = l;
            }
 
            previous = l;
        }

        ++token;
    }
    else if (token->type == TOKEN_R_PAREN)
    {
        fprintf(stderr, "read error - unexpected: )\n");
    }
    else if (token->type == TOKEN_QUOTE)
    {
         ++token;
         Lisp l = lisp_cons(read_list_r(&token, end, ctx), lisp_null(), ctx);
         start = lisp_cons(lisp_create_symbol("QUOTE", ctx), l, ctx);
    }
    else
    {
        start = read_atom(&token, ctx);
    }

    *begin = token;
    return start;
}

Lisp lisp_read(const char* program, LispContext* ctx)
{
    int token_count;

    Token* tokens = tokenize(program, &token_count);

    const Token* begin = tokens;
    const Token* end = tokens + token_count;

    Lisp result = read_list_r(&begin, end, ctx);

    if (begin != end)
    {
        Lisp previous = lisp_cons(result, lisp_null(), ctx);
        Lisp list = lisp_cons(lisp_create_symbol("BEGIN", ctx), previous, ctx);

        while (begin != end)
        {
            Lisp next_result = lisp_cons(read_list_r(&begin, end, ctx), lisp_null(), ctx);

            lisp_cdr(previous) = next_result;
            previous = next_result;
        } 

       result = list;
    }

    free(tokens);
    return result;
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

// hash table
// open addressing with dynamic resizing
typedef struct 
{
    int depth;
    int size;
    int capacity;
    Lisp parent;

    struct EnvEntry
    {
       Lisp symbol;
       Lisp val;
    } table[];
} Env;

Env* lisp_env(Lisp l)
{
    assert(l.type == LISP_ENV);
    LispBlock* block = l.val;
    return (Env*)block->data;
}

Lisp lisp_create_env(Lisp parent, int capacity, LispContext* ctx)
{
    int size = sizeof(Env) + sizeof(struct EnvEntry) * capacity;
    LispBlock* block = block_alloc(size, LISP_ENV, &ctx->heap);

    Env* env = (Env*)block->data;
    env->size = 0;
    env->capacity = capacity;
    env->parent = parent;

    if (lisp_is_null(parent))
    {
        env->depth = 0;
    }
    else
    {
        Env* parent_env = lisp_env(parent);
        env->depth = parent_env->depth + 1;
    }

    // clear the keys
    for (int i = 0; i < capacity; ++i)
        env->table[i].symbol = lisp_null();

    Lisp l;
    l.type = block->type;
    l.val = block;
    return l;
}

void lisp_env_set(Lisp l, Lisp symbol, Lisp value, LispContext* ctx)
{
    Env* env = lisp_env(l);
    unsigned int index = symbol_hash(symbol) % env->capacity;

    for (int i = 0; i < env->capacity - 1; ++i)
    {
        if (lisp_is_null(env->table[index].symbol))
        {   
            env->table[index].val = value;
            env->table[index].symbol = symbol;
            ++env->size;

            // replace with a bigger table
            float load_factor = env->size / (float)env->capacity;
            if (load_factor > 0.6f)
            {
                printf("resizing\n");

                Lisp dest = lisp_create_env(env->parent, env->capacity * 2, ctx);
                for (int j = 0; j < env->capacity; ++j)
                    lisp_env_set(dest, env->table[i].symbol, env->table[j].val, ctx);

                env = lisp_env(dest);
                l = dest;
            }
            return;
        }
        else if (strcmp(lisp_symbol(env->table[index].symbol), lisp_symbol(symbol)) == 0)
        {
            env->table[index].val = value;
            return;
        }

        index = (index + 1) % env->capacity;
    }

    // hash table full
    // this should never happen because of resizing
    assert(0);
}

Lisp lisp_env_get(Lisp bottom, Lisp symbol, Lisp* holding_env)
{
    Lisp current = bottom;
  
    while (!lisp_is_null(current))
    {
        Env* env = lisp_env(current);
        unsigned int index = symbol_hash(symbol) % env->capacity;

        for (int i = 0; i < env->capacity - 1; ++i)
        {
            if (lisp_is_null(env->table[index].symbol))
            {
                break;
            }
            else if (strcmp(lisp_symbol(env->table[index].symbol), lisp_symbol(symbol)) == 0)
            {
                *holding_env = current;
                return env->table[index].val;
            }

            index = (index + 1) % env->capacity;
        }

        current = env->parent;
    }

   *holding_env = lisp_null(); 
   return lisp_null();
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
        case LISP_PROC:
            fprintf(file, "procedure-%x", (unsigned int)l.val); 
            break;
        case LISP_ENV:
        {
            Env* env = lisp_env(l);
            fprintf(file, "%i-{", env->depth);
            for (int i = 0; i < env->capacity; ++i)
            {
                if (!lisp_is_null(env->table[i].symbol))
                {
                    lisp_print_r(file, env->table[i].symbol, 0);
                    fprintf(file, " ");
                }
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


static Lisp eval_list(Lisp list, Lisp env, LispContext* ctx)
{
    Lisp start = lisp_null();
    Lisp previous = lisp_null();

    while (!lisp_is_null(list))
    {
        Lisp new_l = lisp_cons(lisp_eval(lisp_car(list), env, ctx), lisp_null(), ctx);

        if (lisp_is_null(previous))
        {
            start = new_l;
            previous = new_l;
        }
        else
        {
            lisp_cdr(previous) = new_l;
            previous = new_l;
        }

        list = lisp_cdr(list);
    }

    return start;
}

Lisp lisp_eval(Lisp x, Lisp env, LispContext* ctx)
{
    while (1)
    {
        LispType type = lisp_type(x);
        
        if (type == LISP_INT ||
                type == LISP_FLOAT ||
                type == LISP_STRING ||
                type == LISP_NULL)
        {
            // atom
            return x;
        }
        else if (type == LISP_SYMBOL)
        {
            // variable reference
            Lisp found_env;
            Lisp result = lisp_env_get(env, x, &found_env);
            
            if (lisp_is_null(found_env))
            {
                fprintf(stderr, "cannot find variable: %s\n", lisp_symbol(x));
            }
            return result;
        }
        else if (type == LISP_PAIR) 
        { 
            const char* op = NULL;
            if (lisp_type(lisp_car(x)) == LISP_SYMBOL)
                op = lisp_symbol(lisp_car(x));

            if (op && strcmp(op, "IF") == 0)
            {
                // if conditional statemetns
                Lisp predicate = lisp_at_index(x, 1);
                Lisp conseq = lisp_at_index(x, 2);
                Lisp alt = lisp_at_index(x, 3);

                if (lisp_int(lisp_eval(predicate, env, ctx)) != 0)
                {
                    x = lisp_eval(conseq, env, ctx);
                } 
                else
                {
                    x = lisp_eval(alt, env, ctx);
                }
            }
            else if (op && strcmp(op, "BEGIN") == 0)
            {
                Lisp it = lisp_cdr(x);
                Lisp result = lisp_null();

                while (!lisp_is_null(it))
                {
                    result = lisp_eval(lisp_car(it), env, ctx);
                    it = lisp_cdr(it);
                } 

                x = result;
            }
            else if (op && strcmp(op, "QUOTE") == 0)
            {
                return lisp_at_index(x, 1);
            }
            else if (op && strcmp(op, "DEFINE") == 0)
            {
                // variable definitions
                Lisp symbol = lisp_at_index(x, 1); 
                Lisp value = lisp_eval(lisp_at_index(x, 2), env, ctx);
                lisp_env_set(env, symbol, value, ctx);
                return lisp_null();
            }
            else if (op && strcmp(op, "SET!") == 0)
            {
                // mutablity
                // like define, but requires existance
                // and will search up the environment chain
                Lisp symbol = lisp_at_index(x, 1);

                Lisp found_env;
                lisp_env_get(env, symbol, &found_env);

                if (!lisp_is_null(found_env))
                {
                    Lisp value = lisp_eval(lisp_at_index(x, 2), env, ctx);
                    lisp_env_set(found_env, symbol, value, ctx);
                    return value;
                }
                else
                {
                    fprintf(stderr, "error: unknown variable: %s\n", lisp_symbol(symbol));
                    return lisp_null();
                }
            }
            else if (op && strcmp(op, "LAMBDA") == 0)
            {
                // lambda defintions (compound procedures)
                Lisp args = lisp_at_index(x, 1);
                Lisp body = lisp_at_index(x, 2);
                return lisp_create_lambda(args, body, env, ctx);
            }
            else
            {
                // operator application
                Lisp proc = lisp_eval(lisp_car(x), env, ctx);
                Lisp args = eval_list(lisp_cdr(x), env, ctx);
                
                switch (lisp_type(proc))
                {
                    case LISP_LAMBDA:
                    {
                        // lambda call (compound procedure)
                        // construct a new environment
                        Lambda lambda = lisp_lambda(proc);
                        Lisp new_env = lisp_create_env(lambda.env, 128, ctx);
                        
                        // bind parameters to arguments
                        // to pass into function call
                        Lisp keyIt = lambda.args;
                        Lisp valIt = args;
                        
                        while (!lisp_is_null(keyIt))
                        {
                            lisp_env_set(new_env, lisp_car(keyIt), lisp_car(valIt), ctx);
                            keyIt = lisp_cdr(keyIt);
                            valIt = lisp_cdr(valIt);
                        }
                        
                        x = lisp_eval(lambda.body, new_env, ctx);
                        env = new_env;
                        break;
                    }
                    case LISP_PROC:
                    {
                        // call into C functions
                        // no environment required 
                        Lisp (*func)(Lisp,LispContext*) = proc.val;
                        return func(args, ctx); 
                    }
                    default:
                    {
                        fprintf(stderr, "apply error: not a procedure %s\n", lisp_type_name[proc.type]);
                        return lisp_null();
                    }
                }
            }
        }
        else
        {
            fprintf(stderr,"here: %s\n", lisp_type_name[x.type]);
        }
    }
}

static Lisp proc_cons(Lisp args, LispContext* ctx)
{
    return lisp_cons(lisp_car(args), lisp_car(lisp_cdr(args)), ctx);
}

static Lisp proc_car(Lisp args, LispContext* ctx)
{
    return lisp_car(lisp_car(args));
}

static Lisp proc_cdr(Lisp args, LispContext* ctx)
{
    return lisp_cdr(lisp_car(args));
}

static Lisp proc_eq(Lisp args, LispContext* ctx)
{
    void* a = lisp_car(args).val;
    void* b = lisp_car(lisp_cdr(args)).val;

    return lisp_create_int(a == b);
}

static Lisp proc_display(Lisp args, LispContext* ctx)
{
    lisp_print(lisp_car(args)); return lisp_null();
}

static Lisp proc_newline(Lisp args, LispContext* ctx)
{
    printf("\n"); return lisp_null(); 
}

static Lisp proc_equals(Lisp args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val == lisp_car(lisp_cdr(args)).int_val);
}

static Lisp proc_add(Lisp args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val + lisp_car(lisp_cdr(args)).int_val);
}

static Lisp proc_sub(Lisp args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val - lisp_car(lisp_cdr(args)).int_val);
}

static Lisp proc_mult(Lisp args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val * lisp_car(lisp_cdr(args)).int_val);
}

static Lisp proc_load(Lisp args, LispContext* ctx)
{
    const char* path = lisp_string(lisp_car(args));
    FILE* file = fopen(path, "r");
    
    if (!file)
    {
        fprintf(stderr, "failed to open: %s", path);
        return lisp_null();
    }
    
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* contents = malloc(length);
    
    if (!contents) return lisp_null();
    
    fread(contents, 1, length, file);
    fclose(file);

    Lisp result = lisp_create_string(contents, ctx);
    free(contents);
    
    return result;
}

static Lisp proc_read(Lisp args, LispContext* ctx)
{
    const char* text = lisp_string(lisp_car(args));
    return lisp_read(text, ctx);
}

int lisp_init(LispContext* ctx)
{
    ctx->lambda_counter = 0;
    ctx->debug = 1;
    heap_init(&ctx->heap, 2097152);
    symbol_table_init(&ctx->symbols, 2048);

    ctx->env = lisp_create_env(lisp_null(), 2048, ctx);

    lisp_env_set(ctx->env, lisp_create_symbol("NULL", ctx), lisp_null(), ctx);
    lisp_env_set(ctx->env, lisp_create_symbol("global", ctx), ctx->env, ctx);

    const char* names[] = {
        "CONS",
        "CAR",
        "CDR",
        "EQ?",
        "DISPLAY",
        "NEWLINE",
        "LOAD",
        "READ",
        "=",
        "+",
        "-",
        "*",
        NULL,
    };

    LispProc funcs[] = {
        proc_cons,
        proc_car,
        proc_cdr,
        proc_eq,
        proc_display,
        proc_newline,
        proc_load,
        proc_read,
        proc_equals,
        proc_add,
        proc_sub,
        proc_mult,
        NULL,
    };

    lisp_env_add_procs(ctx->env, names, funcs, ctx);
    return 1;
}

void lisp_env_add_procs(Lisp env, const char** names, LispProc* funcs, LispContext* ctx)
{
    const char** name = names;
    LispProc* func = funcs;

    while (*name)
    {
        lisp_env_set(env, lisp_create_symbol(*name, ctx), lisp_create_proc(*func), ctx);
        ++name;
        ++func;
    }
}

enum
{
    GC_MOVED = (1 << 0), // has this block been moved to the to-space?
    GC_VISITED = (1 << 1), // has this block's pointers been moved?
};

static inline Lisp gc_move(Lisp l, LispHeap* to)
{
    if (l.type == LISP_PAIR ||
        l.type == LISP_SYMBOL ||
        l.type == LISP_STRING ||
        l.type == LISP_LAMBDA ||
        l.type == LISP_ENV)
    {
        LispBlock* block = l.val;

        if (!(block->gc_flags & GC_MOVED))
        {
            unsigned int address = to->size;
            // copy the data
            size_t size = sizeof(LispBlock) + block->data_size;
            LispBlock* dest = (LispBlock*)(to->buffer + to->size);
            memcpy(dest, block, size);
            to->size += size;
            
            // save forwarding address (offset in to)
            block->data_size = address;
            block->gc_flags |= GC_MOVED;
        }

        // assign block address back to l
        l.val = (LispBlock*)(to->buffer + block->data_size);
    }

    return l;
}

size_t lisp_collect(LispContext* ctx)
{
    LispHeap* from = &ctx->heap;

    int capacity = ctx->symbols.capacity;

    float load_factor = ctx->symbols.size / (float)ctx->symbols.capacity;

    if (load_factor < 0.05f)
    {
        capacity /= 2;

        if (ctx->debug)
            printf("shrinking symbol table %i\n", capacity);
    } 
    else if (load_factor > 0.5f)
    {
        capacity *= 2;

        if (ctx->debug)
            printf("growing symbol table %i\n", capacity);
    }

    SymbolTable new_symbols;
    symbol_table_init(&new_symbols, capacity);

    LispHeap to;
    heap_init(&to, from->capacity);

    // begin by saving all blocks
    // pointed to by ls
   
    // move global environment
    ctx->env = gc_move(ctx->env, &to);

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
                    ptrs[0] = gc_move(ptrs[0], &to);
                    ptrs[1] = gc_move(ptrs[1], &to);
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
                else if (block->type == LISP_SYMBOL)
                {
                    // save symbol in new table
                    const Symbol* symbol = (const Symbol*)block->data;
                    const LispBlock* stored_block = symbol_table_get(&new_symbols, symbol->string, block->data_size, symbol->hash);

                    if (!stored_block)
                    {
                        symbol_table_add(&new_symbols, block);
                    }
                    else { assert(stored_block == block); }
                }
                else if (block->type == LISP_ENV)
                {
                    // save all references in environments
                    Env* env = (Env*)block->data;

                    for (int i = 0; i < env->capacity; ++i)
                    {
                        if (!lisp_is_null(env->table[i].symbol))
                        {
                            env->table[i].val = gc_move(env->table[i].val, &to);
                            env->table[i].symbol = gc_move(env->table[i].symbol, &to);
                        }
                    }

                    env->parent = gc_move(env->parent, &to);
                }

                block->gc_flags |= GC_VISITED;
            }

            offset += sizeof(LispBlock) + block->data_size;
        }

        if (done) break;
    }

    if (ctx->debug)
        printf("gc passes: %i\n", passes);

    assert(passes <= 2);
 
    {
        // clear the flags
        size_t offset = 0;
        while (offset < to.size)
        {
            LispBlock* block = (LispBlock*)(to.buffer + offset);
            block->gc_flags = 0;
            offset += sizeof(LispBlock) + block->data_size;
        }
        assert(offset == to.size);
    }
    
    size_t result = from->size - to.size;

    // save new symbol table and heap
    symbol_table_shutdown(&ctx->symbols);
    ctx->symbols = new_symbols;

    heap_shutdown(from);
    *from = to;

    if (ctx->debug)
        printf("gc collected: %lu heap: %i\n", result, ctx->heap.size);

    // we need to save blocks referenced
    // by other blocks now
    return result;
}


