lisp-interpreter
===============

An embeddable lisp/scheme interpreter written in C.
Includes a small subset of the MIT Scheme library.
I created this while reading [SICP](https://github.com/justinmeiners/sicp-excercises) to improve my knowledge of lisp and to make an implementation that allows me to easily add scripting to my own programs.

### Philosophy

- **Simple**: Interpreters can easily get complicated with fancy features
    This project doesn't aim to be an optimal, fully featured, or standards compliant Scheme implementation.
    It is just a robust foundation for scripting. 

   If you need a more complete implementation try [s7](https://ccrma.stanford.edu/software/snd/snd/s7.html)
    or [chicken](https://www.call-cc.org)

- **Data & Code**: Lisp s-expressions are undervalued as an alternative to JSON or XML.
    This implementation provides first-class support for working with data or code.

- **Unintrusive**: Just copy in the header file.
    Turn on and off major features with build macros.
    It should be portable between major platforms.

- **Unsurprising**: You should be able to read the source code and understand how it works.
  The header API should work how you expect.


### Features

- Core scheme language: if, let, do, lambda, cons, car, eval, symbols, etc.
- Data structures: lists, vectors, hash tables, integers, real numbers, characters, strings, and integers.
- Standard library: subset of [MIT Scheme](https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_toc.html)
  with Common Lisp features (like `push`) mixed in.
- Exact [garbage collection](#garbage-collection) with explicit invocation.
- Common lisp style unhygenic macros: `define-macro`.
- Single header file.
- Easy integration of C functions.
- REPL command line tool.
- Efficient parsing and manipulation of large data files.

### Non-Features

- complex numbers
- rational numbers
- call/cc
- port IO
- unix system library

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
LispContext ctx = lisp_init();

// load lisp program (add 1 and 2)
LispError error;
Lisp program = lisp_read("(+ 1 2)", &error, ctx);

// execute program using global environment
Lisp result = lisp_eval(program, &error, ctx);

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
Looking up keys which are reused is even more efficient due to symbol comparison.

JSON
```json
{
   "name" : "Bob Jones",
   "age" : 54,
   "city" : "SLC",
}
```

Lisp
```scheme
((name . "Bob Jones")
 (age . 54)
 (city . "SLC"))
```
Loading the structure in C.

```c
// setup lisp without any library
LispContext ctx = lisp_init_empty();
// load lisp structure
Lisp data = lisp_read_file(file, ctx);
// get value for age
Lisp age = lisp_vector_for_key(data, lisp_make_symbol("AGE", ctx), ctx);
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
Constants can also be stored in the environment in a similar fashion.

```c
Lisp pi = lisp_make_real(3.1415);
lisp_env_set(env, lisp_make_symbol("PI", ctx), pi, ctx);
```
## Macros

Common Lisp style (`defmacro`) is available with the name `define-macro`.

    (define-macro nil! (lambda (x)
      `(set! ,x '()))

## Garbage Collection

Unlike most languages, the garbage collector is not called automatically. You must call it yourself in C:

    lisp_collect(ctx);

OR in lisp code:

    (gc-flip)

Note that whenever a collect is issued
ANY `Lisp` value which is not accessible
through the global environment may become invalid.
Be careful what variables you hold onto in C.
Most C functions which are callable from Lisp
don't have a problem as `eval` cannot be called within them.

See [internals](INTERNALS.md) for more details.

## Project License

Copyright (c) 2020 Justin Meiners

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

