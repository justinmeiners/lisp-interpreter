/*

  References:
 Looks just like how mine ended up :) - http://piumarta.com/software/lysp/
  
  Eval - http://norvig.com/lispy.html
  Enviornments - https://mitpress.mit.edu/sicp/full-text/book/book-Z-H-21.html#%_sec_3.2
  Cons Representation - http://www.more-magic.net/posts/internals-data-representation.html
  GC - http://www.more-magic.net/posts/internals-gc.html
  http://home.pipeline.com/~hbaker1/CheneyMTA.html
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

LispType lisp_type(LispWord word) { return word.type; }
int lisp_is_null(LispWord word) { return lisp_type(word) == LISP_NULL; }

LispWord lisp_null()
{
    LispWord word;
    word.type = LISP_NULL;
    word.int_val = 0;
    return word;
}

LispWord lisp_create_int(int n)
{
    LispWord word;
    word.type = LISP_INT;
    word.int_val = n;
    return word;
}

int lisp_int(LispWord word)
{
    if (word.type == LISP_FLOAT)
        return (int)word.float_val;
    return word.int_val;
}

LispWord lisp_create_float(float x)
{
    LispWord word;
    word.type = LISP_FLOAT;
    word.float_val = x;
    return word;
}

float lisp_float(LispWord word)
{
    if (word.type == LISP_INT)
        return(float)word.int_val;
    return word.float_val;
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

LispWord lisp_cons(LispWord car, LispWord cdr, LispContext* ctx)
{
    LispBlock* block = block_alloc(sizeof(LispWord) * 2, LISP_PAIR, &ctx->heap);
    LispWord word;
    word.type = block->type;
    word.val = block;
    lisp_car(word) = car;
    lisp_cdr(word) = cdr;
    return word;
}

LispWord lisp_at_index(LispWord list, int i)
{
    while (i > 0)
    {
        list = lisp_cdr(list);
        --i;
    }
    return lisp_car(list);
}

LispWord lisp_create_string(const char* string, LispContext* ctx)
{
    unsigned int length = (unsigned int)strlen(string) + 1;
    LispBlock* block = block_alloc(length, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    
    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

static LispWord string_from_view(const char* string, unsigned int length, LispContext* ctx)
{
    LispBlock* block = block_alloc(length + 1, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    block->data[length] = '\0';
    
    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

const char* lisp_string(LispWord word)
{
    assert(word.type == LISP_STRING);
    LispBlock* block = word.val;
    return block->data;
}

typedef struct
{
    unsigned int hash;
    char string[];
} Symbol;

const char* lisp_symbol(LispWord word)
{
    assert(word.type == LISP_SYMBOL);
    LispBlock* block = word.val;
    Symbol* symbol = (Symbol*)block->data;
    return symbol->string;
}

static unsigned int symbol_hash(LispWord word)
{
    assert(word.type == LISP_SYMBOL);
    LispBlock* block = word.val;
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

LispWord lisp_create_symbol(const char* string, LispContext* ctx)
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
    
    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

static LispWord symbol_from_view(const char* string, unsigned int string_length, LispContext* ctx)
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
    
    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

LispWord lisp_create_proc(LispProc func)
{
    LispWord word;
    word.type = LISP_PROC;
    word.val = (LispBlock*)func;
    return word;
}

typedef struct
{
    int identifier;
    LispWord args;
    LispWord body;
    LispWord env;
} Lambda;

LispWord lisp_create_lambda(LispWord args, LispWord body, LispWord env, LispContext* ctx)
{
    LispBlock* block = block_alloc(sizeof(Lambda), LISP_LAMBDA, &ctx->heap);

    Lambda data;
    data.identifier = ctx->lambda_counter++;
    data.args = args;
    data.body = body;
    data.env = env;
    memcpy(block->data, &data, sizeof(Lambda));

    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

Lambda lisp_lambda(LispWord lambda)
{
    LispBlock* block = lambda.val;
    return *(const Lambda*)block->data;
}

#define SCRATCH_MAX 128

static LispWord read_atom(const Token** pointer, LispContext* ctx) 
{
    const Token* token = *pointer;
    
    char scratch[SCRATCH_MAX];
    LispWord word = lisp_null();

    switch (token->type)
    {
        case TOKEN_INT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            word = lisp_create_int(atoi(scratch));
            break;
        case TOKEN_FLOAT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            word = lisp_create_float(atof(scratch));
            break;
        case TOKEN_STRING:
            word = string_from_view(token->start + 1, token->length - 2, ctx);
            break;
        case TOKEN_SYMBOL:
            word = symbol_from_view(token->start, token->length, ctx);
            break;
        default: 
            fprintf(stderr, "read error - unknown word: %s\n", token->start);
            break;
    }

    *pointer = (token + 1); 
    return word;
}

// read tokens and construct S-expresions
static LispWord read_list_r(const Token** begin, const Token* end, LispContext* ctx)
{ 
    const Token* token = *begin;
    
    if (token == end)
    {
        fprintf(stderr, "read error - expected: )\n");
    }

    LispWord start = lisp_null();

    if (token->type == TOKEN_L_PAREN)
    {
        ++token;
        LispWord previous = lisp_null();

        while (token->type != TOKEN_R_PAREN)
        {
            LispWord word = lisp_cons(read_list_r(&token, end, ctx), lisp_null(), ctx);

            if (!lisp_is_null(previous))
            {
                lisp_cdr(previous) = word;
            }
            else
            {
                start = word;
            }
 
            previous = word;
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
         LispWord word = lisp_cons(read_list_r(&token, end, ctx), lisp_null(), ctx);
         start = lisp_cons(lisp_create_symbol("QUOTE", ctx), word, ctx);
    }
    else
    {
        start = read_atom(&token, ctx);
    }

    *begin = token;
    return start;
}

LispWord lisp_read(const char* program, LispContext* ctx)
{
    int token_count;

    Token* tokens = tokenize(program, &token_count);

    const Token* begin = tokens;
    const Token* end = tokens + token_count;

    LispWord result = read_list_r(&begin, end, ctx);

    if (begin != end)
    {
        LispWord previous = lisp_cons(result, lisp_null(), ctx);
        LispWord list = lisp_cons(lisp_create_symbol("BEGIN", ctx), previous, ctx);

        while (begin != end)
        {
            LispWord next_result = lisp_cons(read_list_r(&begin, end, ctx), lisp_null(), ctx);

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
    LispWord parent;

    struct EnvEntry
    {
       LispWord symbol;
       LispWord val;
    } table[];
} Env;

Env* lisp_env(LispWord word)
{
    assert(word.type == LISP_ENV);
    LispBlock* block = word.val;
    return (Env*)block->data;
}

LispWord lisp_create_env(LispWord parent, int capacity, LispContext* ctx)
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

    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

void lisp_env_set(LispWord word, LispWord symbol, LispWord value, LispContext* ctx)
{
    Env* env = lisp_env(word);
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

                LispWord dest = lisp_create_env(env->parent, env->capacity * 2, ctx);
                for (int j = 0; j < env->capacity; ++j)
                    lisp_env_set(dest, env->table[i].symbol, env->table[j].val, ctx);

                env = lisp_env(dest);
                word = dest;
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

LispWord lisp_env_get(LispWord bottom, LispWord symbol, LispWord* holding_env)
{
    LispWord current = bottom;
  
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

static void lisp_print_r(FILE* file, LispWord word, int is_cdr)
{
    switch (lisp_type(word))
    {
        case LISP_INT:
            fprintf(file, "%i", lisp_int(word));
            break;
        case LISP_FLOAT:
            fprintf(file, "%f", lisp_float(word));
            break;
        case LISP_NULL:
            fprintf(file, "NIL");
            break;
        case LISP_SYMBOL:
            fprintf(file, "%s", lisp_symbol(word));
            break;
        case LISP_STRING:
            fprintf(file, "\"%s\"", lisp_string(word));
            break;
        case LISP_LAMBDA:
            fprintf(file, "lambda-%i", lisp_lambda(word).identifier);
            break;
        case LISP_PROC:
            fprintf(file, "procedure-%x", (unsigned int)word.val); 
            break;
        case LISP_ENV:
        {
            Env* env = lisp_env(word);
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
            lisp_print_r(file, lisp_car(word), 0);

            if (lisp_cdr(word).type != LISP_PAIR)
            {
                if (!lisp_is_null(lisp_cdr(word)))
                { 
                    fprintf(file, " . ");
                    lisp_print_r(file, lisp_cdr(word), 0);
                }

                fprintf(file, ")");
            }
            else
            {
                fprintf(file, " ");
                lisp_print_r(file, lisp_cdr(word), 1);
            } 
            break; 
    }
}

void lisp_printf(FILE* file, LispWord word) { lisp_print_r(file, word, 0);  }
void lisp_print(LispWord word) {  lisp_printf(stdout, word); }


static LispWord eval_list(LispWord list, LispWord env, LispContext* ctx)
{
    LispWord start = lisp_null();
    LispWord previous = lisp_null();

    while (!lisp_is_null(list))
    {
        LispWord new_word = lisp_cons(lisp_eval(lisp_car(list), env, ctx), lisp_null(), ctx);

        if (lisp_is_null(previous))
        {
            start = new_word;
            previous = new_word;
        }
        else
        {
            lisp_cdr(previous) = new_word;
            previous = new_word;
        }

        list = lisp_cdr(list);
    }

    return start;
}

LispWord lisp_eval(LispWord x, LispWord env, LispContext* ctx)
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
            LispWord found_env;
            LispWord result = lisp_env_get(env, x, &found_env);
            
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
                LispWord predicate = lisp_at_index(x, 1);
                LispWord conseq = lisp_at_index(x, 2);
                LispWord alt = lisp_at_index(x, 3);

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
                LispWord it = lisp_cdr(x);
                LispWord result = lisp_null();

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
                LispWord symbol = lisp_at_index(x, 1); 
                LispWord value = lisp_eval(lisp_at_index(x, 2), env, ctx);
                lisp_env_set(env, symbol, value, ctx);
                return lisp_null();
            }
            else if (op && strcmp(op, "SET!") == 0)
            {
                // mutablity
                // like define, but requires existance
                // and will search up the environment chain
                LispWord symbol = lisp_at_index(x, 1);

                LispWord found_env;
                lisp_env_get(env, symbol, &found_env);

                if (!lisp_is_null(found_env))
                {
                    LispWord value = lisp_eval(lisp_at_index(x, 2), env, ctx);
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
                LispWord args = lisp_at_index(x, 1);
                LispWord body = lisp_at_index(x, 2);
                return lisp_create_lambda(args, body, env, ctx);
            }
            else
            {
                // operator application
                LispWord proc = lisp_eval(lisp_car(x), env, ctx);
                LispWord args = eval_list(lisp_cdr(x), env, ctx);
                
                switch (lisp_type(proc))
                {
                    case LISP_LAMBDA:
                    {
                        // lambda call (compound procedure)
                        // construct a new environment
                        Lambda lambda = lisp_lambda(proc);
                        LispWord new_env = lisp_create_env(lambda.env, 128, ctx);
                        
                        // bind parameters to arguments
                        // to pass into function call
                        LispWord keyIt = lambda.args;
                        LispWord valIt = args;
                        
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
                        LispWord (*func)(LispWord,LispContext*) = proc.val;
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

static LispWord proc_cons(LispWord args, LispContext* ctx)
{
    return lisp_cons(lisp_car(args), lisp_car(lisp_cdr(args)), ctx);
}

static LispWord proc_car(LispWord args, LispContext* ctx)
{
    return lisp_car(lisp_car(args));
}

static LispWord proc_cdr(LispWord args, LispContext* ctx)
{
    return lisp_cdr(lisp_car(args));
}

static LispWord proc_eq(LispWord args, LispContext* ctx)
{
    void* a = lisp_car(args).val;
    void* b = lisp_car(lisp_cdr(args)).val;

    return lisp_create_int(a == b);
}

static LispWord proc_display(LispWord args, LispContext* ctx)
{
    lisp_print(lisp_car(args)); return lisp_null();
}

static LispWord proc_newline(LispWord args, LispContext* ctx)
{
    printf("\n"); return lisp_null(); 
}

static LispWord proc_equals(LispWord args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val == lisp_car(lisp_cdr(args)).int_val);
}

static LispWord proc_add(LispWord args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val + lisp_car(lisp_cdr(args)).int_val);
}

static LispWord proc_sub(LispWord args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val - lisp_car(lisp_cdr(args)).int_val);
}

static LispWord proc_mult(LispWord args, LispContext* ctx)
{
    return lisp_create_int(lisp_car(args).int_val * lisp_car(lisp_cdr(args)).int_val);
}

int lisp_init(LispContext* ctx)
{
    ctx->lambda_counter = 0;
    ctx->debug = 0;
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
        proc_equals,
        proc_add,
        proc_sub,
        proc_mult,
        NULL,
    };

    lisp_env_add_procs(ctx->env, names, funcs, ctx);
    return 1;
}

void lisp_env_add_procs(LispWord env, const char** names, LispProc* funcs, LispContext* ctx)
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

static inline LispWord gc_move_word(LispWord word, LispHeap* to)
{
    if (word.type == LISP_PAIR ||
        word.type == LISP_SYMBOL ||
        word.type == LISP_STRING ||
        word.type == LISP_LAMBDA ||
        word.type == LISP_ENV)
    {
        LispBlock* block = word.val;

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

        // assign block address back to word
        word.val = (LispBlock*)(to->buffer + block->data_size);
    }

    return word;
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
    // pointed to by words
   
    // move global environment
    ctx->env = gc_move_word(ctx->env, &to);

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
                    LispWord* ptrs = (LispWord*)block->data;
                    ptrs[0] = gc_move_word(ptrs[0], &to);
                    ptrs[1] = gc_move_word(ptrs[1], &to);
                    done = 0;
                }
                else if (block->type == LISP_LAMBDA)
                {
                    // move the body and args
                    Lambda* lambda = (Lambda*)block->data;
                    lambda->args = gc_move_word(lambda->args, &to);
                    lambda->body = gc_move_word(lambda->body, &to);
                    lambda->env = gc_move_word(lambda->env, &to);
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
                            env->table[i].val = gc_move_word(env->table[i].val, &to);
                            env->table[i].symbol = gc_move_word(env->table[i].symbol, &to);
                        }
                    }

                    env->parent = gc_move_word(env->parent, &to);
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


