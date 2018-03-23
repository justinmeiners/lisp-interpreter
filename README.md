lisp-interpreter
===============

## About

An embeddable lisp interepreter written in C. I created this while reading SICP to improve my knowledge of lisp and to make an implementation that allows me to easily add scripting to my own programs.


### Philosophy

- **Simple**: An interpreter is something that can easily get complicated with fancy features. This project doesn't aim to be an optimal, fully featured, or standards compliant Scheme implementation. It is just a robust foundation for scripting.
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


Read, eval, print loop:
```
$ ./lisp_i
> (define (sqr x) (* x x)))
> (define length 40)
> (define area 0)
> (set! area (sqr length))
1600
```


## Embedding in a program

```
// setup lisp with 1 MB of heap
LispContextRef ctx = lisp_init(1048576);    
Lisp env = lisp_make_default_env(ctx);

// load lisp program (add 1 and 2)
Lisp program = lisp_expand(lisp_read("(+ 1 2)", ctx), ctx);    

// execute program
Lisp result = lisp_eval(program, env, ctx); 

// prints 3
lisp_print(result);

// you are responsible for garbage collection
lisp_collect(ctx, env);     
...
// shutdown also garbage collects
lisp_shutdown(ctx, enve); 

```

## Data

Lisp s-expressions can be used as a lightweight substitute to JSON or XML. 

### JSON 
```
{
   "name" : "bob jones",
   "age" : 54,
   "city" : "SLC",
}

```

### Lisp
```
((name "bob jones") 
    (age 54) 
    (city "SLC"))

```

### C Parsing

```
// setup lisp with 1 MB of heap
LispContextRef ctx = lisp_init(1048576); 
// load lisp structure
Lisp data = lisp_read_file(file, ctx); 
// get value for age
Lisp age = lisp_for_key(data, lisp_make_symbol("AGE", ctx), ctx);

```


## Status

This project is in progress and is not stable. However, most of the lisp features have been implemented and work properly. You can find a list of todos on the project page.

