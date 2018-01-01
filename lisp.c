#include "lisp.h"
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <assert.h>

LispCell lisp_cons(LispCell car, LispCell cdr)
{  
    int length = sizeof(LispCell) * 2;
    Block* block = malloc(sizeof(BlockHeader) + length);
    block->header.length = length;

    LispCell cell;
    cell.type = LISP_CELL_PAIR;
    cell.val = block;

    lisp_car(cell) = car;
    lisp_cdr(cell) = cdr;      
    return cell;
}       

LispCell lisp_cell_at_index(LispCell list, int i)
{
    while (i > 0)
    {
        list = lisp_cdr(list);
        --i;
    }

    return lisp_car(list);
}

LispCell lisp_null()
{
    LispCell cell;
    cell.type = LISP_CELL_NULL;
    return cell;
}

LispCell lisp_cell_CreateStringView(const char* string, int length)
{ 
    Block* block = malloc(sizeof(BlockHeader) + length + 1);
    block->header.length = length + 1;
    memcpy(block->data, string, length);
    block->data[length] = '\0';

    LispCell cell;
    cell.type = LISP_CELL_STRING;
    cell.val = block; 
    return cell;
}

LispCell lisp_cell_CreateSymbolView(const char* string, int length)
{
    LispCell cell = lisp_cell_CreateStringView(string, length);
    cell.type = LISP_CELL_SYMBOL;

    Block* block = cell.val; 
    char* c = block->data;

    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }

    return cell;
}

LispCell lisp_cell_create_proc(LispCell (*func)(LispCell))
{
    LispCell cell;
    cell.type = LISP_CELL_PROC;
    cell.val = func;
    return cell;
}

LispCell lisp_cell_CreateInt(int n)
{
    LispCell cell;
    cell.type = LISP_CELL_INT;
    cell.int_val = n;
    return cell;
}

LispCell lisp_cell_CreateFloat(float x)
{
    LispCell cell;
    cell.type = LISP_CELL_FLOAT;
    cell.float_val = x;
    return cell;
}

LispCell lisp_cell_CreateString(const char* string)
{
    int length = strlen(string);
    Block* block = malloc(sizeof(BlockHeader) + length + 1);
    block->header.length = length + 1;
    memcpy(block->data, string, length + 1);

    LispCell cell;
    cell.type = LISP_CELL_STRING;
    cell.val = block; 
    return cell;
}

LispCell lisp_cell_CreateSymbol(const char* string)
{
    LispCell cell = lisp_cell_CreateString(string);
    cell.type = LISP_CELL_SYMBOL;

    Block* block = cell.val;
    char* c = block->data;

    while (*c)
    {
        *c = toupper(*c);
        ++c;
    }

    return cell;
}

const char* lisp_cell_GetString(LispCell cell)
{
    Block* block = cell.val;
    return block->data;
}

typedef struct
{
    int identifier;
    LispCell args;
    LispCell body;
    LispEnv* env;
} Lambda;

static int lambda_identifier = 0;

LispCell lisp_cell_CreateLambda(LispCell args, LispCell body, LispEnv* env)
{
    Block* block = malloc(sizeof(BlockHeader) + sizeof(Lambda));
    block->header.length = sizeof(Lambda);

    Lambda data;
    data.identifier = lambda_identifier++;
    data.args = args;
    data.body = body;
    data.env = env;
    memcpy(block->data, &data, sizeof(Lambda));

    LispCell cell;
    cell.type = LISP_CELL_LAMBDA; 
    cell.val = block;
    return cell;
}

Lambda lisp_cell_GetLambda(LispCell lambda)
{
    Block* block = lambda.val;
    return *(const Lambda*)block->data;
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

static const char* TokenType_name_table[] = {
    "NONE", 
    "L_PAREN", 
    "R_PAREN", 
    "QUOTE",
    "SYMBOL", 
    "STRING", 
    "INT", 
    "FLOAT",
};

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
    const char* specials = "?!#$+-.*^%_-/";
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

#define SCRATCH_MAX 128

static LispCell lisp_read_atom(const Token** pointer) 
{
    const Token* token = *pointer;
    
    char scratch[SCRATCH_MAX];
    LispCell cell = lisp_null();

    switch (token->type)
    {
        case TOKEN_INT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            cell = lisp_cell_CreateInt(atoi(scratch));
            break;
        case TOKEN_FLOAT:
            memcpy(scratch, token->start, token->length);
            scratch[token->length] = '\0';
            cell = lisp_cell_CreateFloat(atof(scratch));
            break;
        case TOKEN_STRING:
            cell = lisp_cell_CreateStringView(token->start + 1, token->length - 2);
            break;
        case TOKEN_SYMBOL:
            cell = lisp_cell_CreateSymbolView(token->start, token->length);
            break;
        default: 
            fprintf(stderr, "read error - unknown cell: %s\n", token->start);
            break;
    }

    *pointer = (token + 1); 
    return cell;
}

// read tokens and construct S-expresions
static LispCell lisp_read_list_r(const Token** pointer)
{ 
    const Token* token = *pointer;

    LispCell start = lisp_null();

    if (token->type == TOKEN_L_PAREN)
    {
        ++token;
        LispCell previous = lisp_null();

        while (token->type != TOKEN_R_PAREN)
        {
            LispCell cell = lisp_cons(lisp_read_list_r(&token), lisp_null());

            if (previous.type != LISP_CELL_NULL) 
            {
                lisp_cdr(previous) = cell;
            }
            else
            {
                start = cell;
            }
 
            previous = cell;
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
         LispCell cell = lisp_cons(lisp_read_list_r(&token), lisp_null());
         start = lisp_cons(lisp_cell_CreateSymbol("quote"), cell);
    }
    else
    {
        start = lisp_read_atom(&token);
    }

    *pointer = token;
    return start;
}

LispCell lisp_read(const char* program)
{
    int token_count;

    Token* tokens = tokenize(program, &token_count);
    const Token* pointer = tokens;

    LispCell previous = lisp_null();
    LispCell start = previous;

    while (pointer < tokens + token_count)
    {
        LispCell cell = lisp_cons(lisp_read_list_r(&pointer), lisp_null());

        if (previous.type != LISP_CELL_NULL) 
        {
            lisp_cdr(previous) = cell;
        }
        else
        {
            start = cell;
        }

        previous = cell;
    }

    free(tokens);     

    return start;
}

static void lisp_print_r(FILE* file, LispCell cell, int is_cdr)
{
    switch (cell.type)
    {
        case LISP_CELL_INT:
            fprintf(file, "%i", cell.int_val);
            break;
        case LISP_CELL_FLOAT:
            fprintf(file, "%f", cell.float_val);
            break;
        case LISP_CELL_NULL:
            fprintf(file, "nil");
            break;
        case LISP_CELL_SYMBOL:
            fprintf(file, "%s", lisp_cell_GetString(cell));
            break;
        case LISP_CELL_STRING:
            fprintf(file, "\"%s\"", lisp_cell_GetString(cell));
            break;
        case LISP_CELL_LAMBDA:
            fprintf(file, "lambda #");
            break;
        case LISP_CELL_PROC:
            fprintf(file, "procedure #%x", (unsigned int)cell.val); 
            break;
        case LISP_CELL_PAIR:
            if (!is_cdr) fprintf(file, "(");
            lisp_print_r(file, lisp_car(cell), 0);

            if (lisp_cdr(cell).type != LISP_CELL_PAIR)
            {
                if (lisp_cdr(cell).type != LISP_CELL_NULL)
                { 
                    fprintf(file, " . ");
                    lisp_print_r(file, lisp_cdr(cell), 0);
                }

                fprintf(file, ")");
            }
            else
            {
                fprintf(file, " ");
                lisp_print_r(file, lisp_cdr(cell), 1);
            } 
            break; 
    }
}

void lisp_print(FILE* file, LispCell cell)
{
    lisp_print_r(file, cell, 0); 
}

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
        env->table[i].key[0] = '\0';
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
    return env->table[index].key[0] == '\0';
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
        else if (strncmp(env->table[index].key, key, ENTRY_KEY_MAX) == 0)
        {
            found = 1;
            break;
        }

        index = (index + 1) % env->capacity;
    }

    *out_index = index;
    return found;
}

void lisp_env_set(LispEnv* env, const char* key, LispCell value)
{
    int index;
    if (!lisp_env_search(env, key, &index))
    {
        // hash table full
        // this should never happen because of resizing
        assert(0); 
    }

    if (lisp_env_empty(env, index))
    {
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
                if (env->table[i].key[0] != '\0')
                {
                    lisp_env_set(&new_env, env->table[i].key, env->table[i].value);
                } 
            }

            //TODO:
            
            free(env->table);
            *env = new_env;
        }
    }

    env->table[index].value = value;
    strcpy(env->table[index].key, key);
}

LispCell lisp_env_get(LispEnv* env, const char* key)
{
    LispEnv* current = env;
    while (current)
    {
        int index;
        if (lisp_env_search(current, key, &index))
        {
            if (!lisp_env_empty(current, index))
            {
                return current->table[index].value;
            }
        }

        current = current->parent;
   }
  
   return lisp_null(); 
}

LispEnv* lisp_env_FindWithKey(LispEnv* env, const char* key)
{
    LispEnv* current = env;
    while (current)
    {
        int index;
        if (lisp_env_search(env, key, &index))
        {
            if (!lisp_env_empty(env, index))
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
            printf("%s ", env->table[i].key);
    }
    printf("}");
}

LispCell add(LispCell args)
{
    return lisp_cell_CreateInt(lisp_car(args).int_val + lisp_car(lisp_cdr(args)).int_val);
}

LispCell mult(LispCell args)
{
    return lisp_cell_CreateInt(lisp_car(args).int_val * lisp_car(lisp_cdr(args)).int_val);
}



void lisp_env_init_default(LispEnv* env)
{
    lisp_env_init(env, NULL, 512);
    lisp_env_set(env, "+", lisp_cell_create_proc(add));
    lisp_env_set(env, "*", lisp_cell_create_proc(mult));
}

static LispCell lisp_apply(LispCell proc, LispCell args)
{
    if (proc.type == LISP_CELL_LAMBDA)
    {  
        // lambda call (compound procedure)
        // construct a new environment
        Lambda lambda = lisp_cell_GetLambda(proc);

        LispEnv new_env;
        lisp_env_init(&new_env, lambda.env, 64); 

        // bind parameters to arguments
        // to pass into function call
        LispCell keyIt = lambda.args;
        LispCell valIt = args;

        while (keyIt.type != LISP_CELL_NULL)
        {
            const char* key = lisp_cell_GetString(lisp_car(keyIt));
            lisp_env_set(&new_env, key, lisp_car(valIt));

            keyIt = lisp_cdr(keyIt);
            valIt = lisp_cdr(valIt);
        }

        LispCell result = lisp_eval(lambda.body, &new_env); 
        lisp_env_release(&new_env); // TODO: ref counting?  
        return result;
    }
    else if (proc.type == LISP_CELL_PROC)
    {
        // call into C functions
        // no environment required 
        LispCell (*func)(LispCell) = proc.val;
        return func(args);
    }
    else
    {
        fprintf(stderr, "apply error: not a procedure %i\n", proc.type);
        return lisp_null();
    }
}

static LispCell lisp_eval_list(LispCell list, LispEnv* env)
{
    LispCell start = lisp_null();
    LispCell previous = lisp_null();

    while (list.type != LISP_CELL_NULL)
    {
        LispCell new_cell = lisp_cons(lisp_eval(lisp_car(list), env), lisp_null());

        if (previous.type == LISP_CELL_NULL)
        {
            start = new_cell;
            previous = new_cell;
        }
        else
        {
            lisp_cdr(previous) = new_cell;
            previous = new_cell;
        }

        list = lisp_cdr(list);
    }

    return start;
}

LispCell lisp_eval(LispCell cell, LispEnv* env)
{
    if (cell.type == LISP_CELL_SYMBOL)
    {
        // read variable 
        const char* key = lisp_cell_GetString(cell);
        return lisp_env_get(env, key);
    }
    else if (cell.type == LISP_CELL_INT || 
            cell.type == LISP_CELL_FLOAT ||
            cell.type == LISP_CELL_STRING)
    {
        // atom
        return cell;
    }
    else if (cell.type == LISP_CELL_PAIR && 
             lisp_car(cell).type == LISP_CELL_SYMBOL)
    { 
        const char* opSymbol = lisp_cell_GetString(lisp_car(cell));

        if (strcmp(opSymbol, "IF") == 0)
        {
            // if conditional statemetns
            LispCell cond = lisp_cell_at_index(cell, 1);
            LispCell result = lisp_cell_at_index(cell, 2);
            LispCell alt = lisp_cell_at_index(cell, 3);

            if (lisp_eval(cond, env).int_val != 0)
            {
                return lisp_eval(result, env);
            } 
            else
            {
                return lisp_eval(alt, env);
            }
        }
        else if (strcmp(opSymbol, "QUOTE") == 0)
        {
            return cell;
        }
        else if (strcmp(opSymbol, "DEFINE") == 0)
        {
            // variable definitions
            const char* key = lisp_cell_GetString(lisp_cell_at_index(cell, 1));
            LispCell value = lisp_eval(lisp_cell_at_index(cell, 2), env);
            lisp_env_set(env, key, value);
            return value;
        }
        else if (strcmp(opSymbol, "SET!") == 0)
        {
            // mutablity
            // like define, but requires existance
            // and will search up the environment chain
            const char* key = lisp_cell_GetString(lisp_cell_at_index(cell, 1)); 
            LispEnv* targetLispEnv = lisp_env_FindWithKey(env, key);

            if (targetLispEnv)
            {
                LispCell value = lisp_eval(lisp_cell_at_index(cell, 2), env);
                lisp_env_set(targetLispEnv, key, value);
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
            return lisp_cell_CreateLambda(lisp_cell_at_index(cell, 1), lisp_cell_at_index(cell, 2), env);
        }
        else
        {
            // application
            LispCell proc = lisp_eval(lisp_car(cell), env);
            LispCell args = lisp_eval_list(lisp_cdr(cell), env);
            return lisp_apply(proc, args);
       }
    }
    else
    {
        fprintf(stderr, "eval error: invalid form\n");
        return lisp_null();
    }
}

