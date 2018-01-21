lisp-interpreter
===============

## About

A single header + source Lisp intepreter, that is closest to the Scheme dialect. I wrote this to improve my knowledge of lisp and to make an implementation that is easily embedded into C programs.

### Features

- One header and source file.
- Scheme-like syntax (if, let, and, or, etc)
- Closures
- Cheney garbage collection with explicit invocation.
- Symbol table
- Easy integration of C functions.
- REPL command line tool.
- Support for static data

### Example

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

((name "bob jones") (age 54) (city "SLC"))

```

### Reading
```
LispContextRef ctx = lisp_init(...);
Lisp data = lisp_parse(program, ctx);

// search for name
Lisp name = lisp_for_key(data, lisp_make_symbol("NAME", ctx));
printf("%s\n", lisp_string(name));

```


## Status

This project is in progress and is not stable. However, most of the lisp features have been implemented and work properly. You can find a list of todos on the project page.

