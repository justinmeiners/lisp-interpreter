lisp-interpreter
===============

## About

A single header + source Lisp intepreter, that is closest to the Scheme dialect. I wrote this to improve my knowledge of lisp and to make an implementation that is easily embedded into C programs.


### Philosophy

- **Simple**: An interpreter is something that can get really complicated by cool extra features. This doesn't aim to be an optimal, fully featured, or standards compliant Scheme implementation. Just a robust foundation for embedded scripting.
- **Data & Code**: Lisp is undervalued as a JSON or XML alternative. This implementation provides support for working with data or code.
- **Unintrusive**: Just copy in the header and source file. Source code should be portable between major platforms.
- **Unsuprising**: You should be able to read the source code and understand how it works. The header API should work how you expect.

### Features

- Scheme-like syntax (if, let, and, or, etc)
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

### Reading
```
LispContextRef ctx = lisp_init(...);
// The parse function bypasses the usual syntax expansion phase for code
Lisp data = lisp_parse(program, ctx);

// search for name
Lisp name = lisp_for_key(data, lisp_make_symbol("NAME", ctx));
printf("%s\n", lisp_string(name));

```


## Status

This project is in progress and is not stable. However, most of the lisp features have been implemented and work properly. You can find a list of todos on the project page.

