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
    int length;  
    const char* start;
} Token;

static int in_string(char c, const char* string)
{
    while (*string)
    {
        if (*string == c) return 1;
        ++string;
    }
    return 0;
}

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

static int match_symbol(const char* c, const char** oc)
{
    // which special characters are allowed?
    const char* specials = "=?!#$+-.*^%_-/";
    if (!isalpha(*c) && !in_string(*c, specials)) return 0;
    ++c;
    while (*c &&
            (isalpha(*c) || 
             isdigit(*c) || 
             in_string(*c, specials))) ++c;
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
    tok->length = nc - c;
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

enum
{
    GC_MOVED = (1 << 0), // has this block been moved to the to-space?
    GC_VISITED = (1 << 1), // has this block's pointers been moved?
};

typedef struct
{
    int identifier;
    LispWord args;
    LispWord body;
    LispEnv* env;
} Lambda;

static void heap_init(LispHeap* heap, int capacity)
{
    heap->capacity = capacity;
    heap->size = 0;
    heap->buffer = malloc(heap->capacity);
}

static LispBlock* block_alloc(size_t data_size, LispType type, LispHeap* heap)
{
    LispBlock* block = (LispBlock*)(heap->buffer + heap->size);
    block->gc_flags = 0;
    block->data_size = data_size;
    block->type = type;
    heap->size += sizeof(LispBlock) + data_size;
    return block;
}

LispType lisp_type(LispWord word)
{
    return word.type;
}

int lisp_is_null(LispWord word)
{
    return lisp_type(word) == LISP_NULL;
}

LispWord lisp_null()
{
    LispWord word;
    word.type = LISP_NULL;
    return word;
}

LispWord lisp_create_int(int n)
{
    LispWord word;
    word.type = LISP_INT;
    word.int_val = n;
    return word;
}

LispWord lisp_create_float(float x)
{
    LispWord word;
    word.type = LISP_FLOAT;
    word.float_val = x;
    return word;
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
    int length = strlen(string) + 1;
    LispBlock* block = block_alloc(length, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);

    LispWord word;
    word.type = block->type;
    word.val = block; 
    return word;
}

LispWord lisp_create_symbol(const char* symbol, LispContext* ctx)
{
    LispWord word = lisp_create_string(symbol, ctx);
    word.type = LISP_SYMBOL;

    LispBlock* block = word.val;
    block->type = word.type;

    // always convert symbols to uppercase
    char* c = block->data;

    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }

    return word;
}

LispWord lisp_create_proc(LispProc func)
{
    LispWord word;
    word.type = LISP_PROC;
    word.val = (LispBlock*)func;
    return word;
}

const char* lisp_string(LispWord word)
{
    LispBlock* block = word.val;
    return block->data;
}

const char* lisp_symbol(LispWord word)
{
    return lisp_string(word);
}

static int lambda_identifier = 0;

LispWord lisp_create_lambda(LispWord args, LispWord body, LispEnv* env, LispContext* ctx)
{
    LispBlock* block = block_alloc(sizeof(Lambda), LISP_LAMBDA, &ctx->heap);

    Lambda data;
    data.identifier = lambda_identifier++;
    data.args = args;
    data.body = body;
    data.env = env;
    memcpy(block->data, &data, sizeof(Lambda));

    LispWord word;
    word.type = block->type;
    word.val = block;
    return word;
}

Lambda lisp_word_GetLambda(LispWord lambda)
{
    LispBlock* block = lambda.val;
    return *(const Lambda*)block->data;
}

static LispWord string_from_view(const char* string, int length, LispContext* ctx)
{ 
    LispBlock* block = block_alloc(length + 1, LISP_STRING, &ctx->heap);
    memcpy(block->data, string, length);
    block->data[length] = '\0';

    LispWord word;
    word.type = block->type;
    word.val = block; 
    return word;
}

static LispWord symbol_from_view(const char* string, int length, LispContext* ctx)
{
    LispWord word = string_from_view(string, length, ctx);
    word.type = LISP_SYMBOL;

    LispBlock* block = word.val; 
    block->type = word.type;

    // always convert symbols to uppercase
    char* c = block->data;

    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }

    return word;
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
         start = lisp_cons(lisp_create_symbol("quote", ctx), word, ctx);
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

static void lisp_print_r(FILE* file, LispWord word, int is_cdr)
{
    switch (lisp_type(word))
    {
        case LISP_INT:
            fprintf(file, "%i", word.int_val);
            break;
        case LISP_FLOAT:
            fprintf(file, "%f", word.float_val);
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
            fprintf(file, "lambda");
            break;
        case LISP_PROC:
            fprintf(file, "procedure-%x", (unsigned int)word.val); 
            break;
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

void lisp_printf(FILE* file, LispWord word)
{
    lisp_print_r(file, word, 0); 
}

void lisp_print(LispWord word)
{
    lisp_printf(stdout, word);
}

static const char* lisp_type_name[] = {
    "FLOAT",
    "INT",
    "PAIR",
    "SYMBOL",
    "STRING",
    "LAMBDA",
    "PROCEDURE",
    "NULL",
};

static int hash(const char *buffer, int length) 
{
    // adler 32
     int s1 = 1;
     int s2 = 0;

     for (size_t n = 0; n < length; ++n) 
     {
        s1 = (s1 + buffer[n]) % 65521;
        s2 = (s2 + s1) % 65521;
     }     

     return (s2 << 16) | s1;
}

void lisp_env_init(LispEnv* env, LispEnv* parent, int capacity)
{
    env->count = 0;
    env->capacity = capacity;
    env->parent = parent;
    env->table = malloc(sizeof(LispEnvEntry) * capacity);
    env->retain_count = 1;

    // clear the keys
    for (int i = 0; i < capacity; ++i)
        env->table[i].symbol = lisp_null();
}

void lisp_env_retain(LispEnv* env)
{
    ++env->retain_count;
}

void lisp_env_release(LispEnv* env)
{
    if (--env->retain_count < 1)
    {
        free(env->table);
    }
}

static int lisp_env_empty(LispEnv* env, int index)
{
    return lisp_is_null(env->table[index].symbol);
}

static int lisp_env_search(LispEnv* env, const char* key, int* out_index)
{
    int found = 0;
    int index = hash(key, strlen(key)) % env->capacity;

    for (int i = 0; i < env->capacity - 1; ++i)
    {
        if (lisp_env_empty(env, index)) 
        {
            found = 1;
            break;
        }
        else if (strcmp(lisp_symbol(env->table[index].symbol), key) == 0)
        {
            found = 1;
            break;
        }

        index = (index + 1) % env->capacity;
    }

    *out_index = index;
    return found;
}

void lisp_env_set(LispEnv* env, LispWord symbol, LispWord value)
{
    assert(symbol.type == LISP_SYMBOL);

    int index;
    if (!lisp_env_search(env, lisp_symbol(symbol), &index))
    {
        // hash table full
        // this should never happen because of resizing
        assert(0); 
    }

    if (lisp_env_empty(env, index))
    {
        // new value
        ++env->count;

        // replace with a bigger table
        if (env->count * 2 > env->capacity)
        {
            printf("resizing\n");
            LispEnv new_env;
            lisp_env_init(&new_env, env->parent, env->capacity * 3);

            // insert entries
            for (int i = 0; i < env->capacity; ++i)
            {
                if (lisp_is_null(env->table[i].symbol))
                {
                    lisp_env_set(&new_env, env->table[i].symbol, env->table[i].value);
                } 
            }

            //TODO:
            
            free(env->table);
            *env = new_env;
        }
    }

    env->table[index].value = value;
    env->table[index].symbol = symbol;
}

int lisp_env_get(LispEnv* env, LispWord symbol, LispWord* result)
{
    assert(symbol.type == LISP_SYMBOL);

    LispEnv* current = env;
    while (current)
    {
        int index;
        if (lisp_env_search(current, lisp_symbol(symbol), &index))
        {
            if (!lisp_env_empty(current, index))
            {
                *result = current->table[index].value;
                return 1;
            }
        }

        current = current->parent;
   }

   *result = lisp_null(); 
   return 0;
}

static LispEnv* find_env(LispEnv* env, LispWord symbol)
{
    LispEnv* current = env;
    while (current)
    {
        int index;
        if (lisp_env_search(current, lisp_symbol(symbol), &index))
        {
            if (!lisp_env_empty(current, index))
            {
                return env;
            }
        }

        current = current->parent;
   }

   return NULL;
}

void lisp_env_print(LispEnv* env)
{
    printf("{");
    for (int i = 0; i < env->capacity; ++i)
    {
        if (!lisp_env_empty(env, i))
            printf("%s ", lisp_symbol(env->table[i].symbol));
    }
    printf("}");
}

static LispWord lisp_apply(LispWord proc, LispWord args, LispContext* ctx)
{
    switch (lisp_type(proc))
    {       
        case LISP_LAMBDA:
        {  
            // lambda call (compound procedure)
            // construct a new environment
            Lambda lambda = lisp_word_GetLambda(proc);

            LispEnv new_env;
            lisp_env_init(&new_env, lambda.env, 64); 

            // bind parameters to arguments
            // to pass into function call
            LispWord keyIt = lambda.args;
            LispWord valIt = args;

            while (!lisp_is_null(keyIt))
            {
                lisp_env_set(&new_env, lisp_car(keyIt), lisp_car(valIt));

                keyIt = lisp_cdr(keyIt);
                valIt = lisp_cdr(valIt);
            }

            LispWord result = lisp_eval(lambda.body, &new_env, ctx); 
            lisp_env_release(&new_env); 
            return result;
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

static LispWord eval_list(LispWord list, LispEnv* env, LispContext* ctx)
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

LispWord lisp_eval(LispWord word, LispEnv* env, LispContext* ctx)
{
    if (!env) env = &ctx->global;
    assert(env);

    LispType type = lisp_type(word);
    if (type == LISP_SYMBOL)
    {
        // variable reference

        LispWord result;
        if (!lisp_env_get(env, word, &result))
        {
            fprintf(stderr, "cannot find variable: %s\n", lisp_symbol(word));
        }
        return result;
    }
    else if (type == LISP_INT || 
             type == LISP_FLOAT ||
             type == LISP_STRING)
    {
        // atom
        return word;
    }
    else if (type == LISP_PAIR && 
             lisp_type(lisp_car(word)) == LISP_SYMBOL)
    { 
        const char* opSymbol = lisp_symbol(lisp_car(word));

        if (strcmp(opSymbol, "IF") == 0)
        {
            // if conditional statemetns
            LispWord predicate = lisp_at_index(word, 1);
            LispWord conseq = lisp_at_index(word, 2);
            LispWord alt = lisp_at_index(word, 3);

            if (lisp_eval(predicate, env, ctx).int_val != 0)
            {
                return lisp_eval(conseq, env, ctx);
            } 
            else
            {
                return lisp_eval(alt, env, ctx);
            }
        }
        else if (strcmp(opSymbol, "BEGIN") == 0)
        {
            LispWord it = lisp_cdr(word);

            LispWord result = lisp_null();

            while (!lisp_is_null(it))
            {
                result = lisp_eval(lisp_car(it), env, ctx);
                it = lisp_cdr(it);
            } 

            return result;
        }
        else if (strcmp(opSymbol, "QUOTE") == 0)
        {
            return lisp_at_index(word, 1);
        }
        else if (strcmp(opSymbol, "DEFINE") == 0)
        {
            // variable definitions
            LispWord symbol = lisp_at_index(word, 1); 
            LispWord value = lisp_eval(lisp_at_index(word, 2), env, ctx);
            lisp_env_set(env, symbol, value);
            return value;
        }
        else if (strcmp(opSymbol, "SET!") == 0)
        {
            // mutablity
            // like define, but requires existance
            // and will search up the environment chain
            LispWord symbol = lisp_at_index(word, 1);

            LispEnv* targetLispEnv = find_env(env, symbol);

            if (targetLispEnv)
            {
                LispWord value = lisp_eval(lisp_at_index(word, 2), env, ctx);
                lisp_env_set(targetLispEnv, symbol, value);
                return value;
            }
            else
            {
                fprintf(stderr, "error: unknown env\n");
                return lisp_null();
            }
        }
        else if (strcmp(opSymbol, "LAMBDA") == 0)
        {
            // lambda defintions (compound procedures)
            LispWord args = lisp_at_index(word, 1);
            LispWord body = lisp_at_index(word, 2);
            return lisp_create_lambda(args, body, env, ctx);
        }
        else
        {
            // operator application
            LispWord proc = lisp_eval(lisp_car(word), env, ctx);
            LispWord args = eval_list(lisp_cdr(word), env, ctx);
            return lisp_apply(proc, args, ctx);
       }
    }
    else
    {
        fprintf(stderr, "eval error: invalid form\n");
        return lisp_null();
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

static LispWord proc_display(LispWord args, LispContext* ctx)
{
    lisp_print(lisp_car(args));
    return lisp_null();
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
    heap_init(&ctx->heap, 4096);
    lisp_env_init(&ctx->global, NULL, 2048);

    lisp_add_proc("CONS", proc_cons, ctx);
    lisp_add_proc("CAR", proc_car, ctx);
    lisp_add_proc("CDR", proc_cdr, ctx);
    lisp_add_proc("DISPLAY", proc_display, ctx);
    lisp_add_proc("=", proc_equals, ctx);
    lisp_add_proc("+", proc_add, ctx);
    lisp_add_proc("-", proc_sub, ctx);
    lisp_add_proc("*", proc_mult, ctx);
    return 1;
}

void lisp_add_proc(const char* name, LispProc proc, LispContext* ctx)
{
    lisp_env_set(&ctx->global, lisp_create_symbol(name, ctx), lisp_create_proc(proc)); 
}

static inline LispWord gc_move_word(LispWord word, LispHeap* to)
{
    if (word.type == LISP_PAIR ||
        word.type == LISP_SYMBOL ||
        word.type == LISP_STRING ||
        word.type == LISP_LAMBDA)
    {
        LispBlock* block = word.val;

        if (!(block->gc_flags & GC_MOVED))
        {
            // copy the data
            size_t size = sizeof(LispBlock) + block->data_size;
            LispBlock* dest = (LispBlock*)(to->buffer + to->size);
            memcpy(dest, block, size);
            to->size += size;
            
            // save forwarding address
            block->data_size = (intptr_t)dest;
            block->gc_flags |= GC_MOVED;
        }

        // assign block address back to word
        word.val = (LispBlock*)block->data_size;
    }

    return word;
}

static void gc_move_env(LispEnv* env, LispHeap* to)
{ 
    for (int i = 0; i < env->capacity; ++i)
    {
        if (!lisp_env_empty(env, i))
            env->table[i].value = gc_move_word(env->table[i].value, to);
    }
}

static void gc_move_references(LispHeap* to)
{
    int iterations = 0;
    while (1)
    {
        ++iterations;
        int done = 1;
        size_t offset = 0;
        while (offset < to->size)
        {
            LispBlock* block = (LispBlock*)(to->buffer + offset);

            if (!(block->gc_flags & GC_VISITED))
            {
                // these add to the buffer!
                // so lists are handled in a single pass
                if (block->type == LISP_PAIR)
                {
                    // move the CAR and CDR
                    LispWord* ptrs = (LispWord*)block->data;
                    ptrs[0] = gc_move_word(ptrs[0], to);
                    ptrs[1] = gc_move_word(ptrs[1], to);
                    done = 0;
                }
                else if (block->type == LISP_LAMBDA)
                {
                    // move the body and args
                    Lambda* lambda = (Lambda*)block->data;
                    lambda->args = gc_move_word(lambda->args, to);
                    lambda->body = gc_move_word(lambda->body, to);
                    // TODO: freeing environments?
                    done = 0;
                }

                block->gc_flags |= GC_VISITED;
            }

            offset += sizeof(LispBlock) + block->data_size;
        }
        assert(offset == to->size);

        if (done) break;
    }
    printf("%i\n", iterations);
}

static void gc_clear_flags(LispHeap* to)
{
    // clear the flags
    size_t offset = 0;
    while (offset < to->size)
    {
        LispBlock* block = (LispBlock*)(to->buffer + offset);
        block->gc_flags = 0;
        offset += sizeof(LispBlock) + block->data_size;
    }
    assert(offset == to->size);
}

size_t lisp_collect(LispContext* ctx)
{
    LispHeap* from = &ctx->heap;

    LispHeap to;
    heap_init(&to, from->size);

    // begin by saving all blocks
    // pointed to by words
   
    // move global enviornment
    gc_move_env(&ctx->global, &to);
    gc_move_references(&to);

    gc_clear_flags(&to);

    size_t result = from->size - to.size;

    free(from->buffer);
    *from = to;

    // we need to save blocks referenced
    // by other blocks now
    return result;
}


