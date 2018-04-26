lisp-interpreter
===============

## About

An embeddable lisp interepreter written in C. I created this while reading SICP to improve my knowledge of lisp and to make an implementation that allows me to easily add scripting to my own programs.


### Philosophy

- **Simple**: An interpreter is something that can easily get complicated with fancy features. This project doesn't aim to be an optimal, fully featured, or standards compliant Scheme implementation. It is just a robust foundation for scripting. (If you need a more complete implementation try [s7](https://ccrma.stanford.edu/software/snd/snd/s7.html))
- **Data & Code**: Lisp is undervalued as an alternative to JSON or XML. This implementation provides first-class support for working with data or code.
- **Unintrusive**: Just copy in the header and source file. Source code should be portable between major platforms.
- **Unsurprising**: You should be able to read the source code and understand how it works. The header API should work how you expect.

### Features

- Scheme-like (but not confined to) syntax. if, let, and, or, etc.
- Closures
- Cheney garbage collection with explicit invocation.
- Symbol table
- Easy integration of C functions.
- REPL command line tool.
- Data loading and manipulation.
- Single header and source file.

## Examples


### Interactive programming with Read, eval, print loop.
```bash
$ ./lisp_i
> (define (sqr x) (* x x)))
> (define length 40)
> (define area 0)
> (set! area (sqr length))
1600
```

### Embedding in a program

```c
// setup lisp with 1 MB of heap
LispContext ctx = lisp_init_interpreter();    

// load lisp program (add 1 and 2)
LispError error;
Lisp program = lisp_read_expand("(+ 1 2)", &error, ctx);    

// execute program using global environment
Lisp result = lisp_eval_global(program, &error, ctx); 

// prints 3
lisp_print(result);

// you are responsible for garbage collection
lisp_collect(ctx, env);     
// ...
// shutdown also garbage collects
lisp_shutdown(ctx, env); 
```

### Loading Data

Lisp s-expressions can be used as a lightweight substitute to JSON or XML. 

JSON 
```json
{
   "name" : "bob jones",
   "age" : 54,
   "city" : "SLC",
}
```

Lisp
```scheme
((name "bob jones") 
    (age 54) 
    (city "SLC"))
```
Loading the structure in C.

```c
// setup lisp with 1 MB of heap
LispContext ctx = lisp_init_empty(); 
// load lisp structure
Lisp data = lisp_read_file(file, ctx); 
// get value for age
Lisp age = lisp_for_key(data, lisp_make_symbol("AGE", ctx), ctx);
// ...
lisp_shutdown(ctx);
```

### Calling C functions

C functions can be used to extend the interpreter, or call into C code.

```c
Lisp sum_of_squares(Lisp args, LispError* e, LispContext ctx)
{
    // first argument
    Lisp accum = lisp_car(args);
    // remaining arguments
    Lisp rest = lisp_cdr(args);

    // iterate over the rest of the arguments
    while (!lisp_is_null(rest))
    {
        Lisp val = lisp_car(rest);

        // make this work for integers and floats
        if (lisp_type(accum) == LISP_INT)
        {
            accum.int_val += lisp_int(val) * lisp_int(val);
        }
        else if (lisp_type(accum) == LISP_FLOAT)
        {
            accum.float_val += lisp_float(val) * lisp_float(val);
        }

        rest = lisp_cdr(rest);
    }

    return accum;
}

// wrap in Lisp object
Lisp func = lisp_make_func(sum_of_squares);

// add to enviornment with symbol SUM-OF-SQUARES
lisp_env_set(env, lisp_make_symbol("SUM-OF-SQAURES", ctx), func, ctx);
```

In Lisp
```scheme
(sum-of-squares 1 2 3)
; returns 1 + 4 + 9 = 14
```
Constants can also be stored in the enviornment in a similar fashion.

```c
Lisp pi = lisp_make_float(3.1415);
lisp_env_set(env, lisp_make_symbol("PI", ctx), pi, ctx);
```

## Status

This project is in progress and is not stable. However, most of the lisp features have been implemented and work properly. You can find a list of todos on the project page.

