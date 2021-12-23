
Lisp Interpreter
================

> Any sufficiently complicated C or Fortran program contains an ad hoc, informally-specified, bug-ridden, slow implementation of half of Common Lisp. -- Philip Greenspun

An embeddable lisp/scheme interpreter written in C.
It includes a subset of R5RS with some extensions from MIT Scheme.

I created this while reading [SICP](https://github.com/justinmeiners/sicp-excercises) to improve my knowledge of lisp
and to make an implementation that allows me to easily add scripting to my own programs.

### Philosophy

- **Simple**: This project doesn't aim to be optimal, or fully standards compliant.
    It is just a robust foundation for scripting. 
    It is implemented as a recursive AST walker on the C stack.

    If you need more try [s7](https://ccrma.stanford.edu/software/snd/snd/s7.html) or [chicken](https://www.call-cc.org)

- **Unintrusive**: Just copy in the header file. Turn on and off major features with build macros. It should be portable between major platforms.

- **Unsurprising**: You should be able to read the source code and understand how it works.
  The C API should work how you expect.

- **First class data**: Lisp s-expressions are undervalued as an alternative to JSON or XML.
    Preprocessor flags can remove most scheme features if you just want to read s-expressions
    and manipulate them in C.

### Features

- C99, no dependencies, two files.
- Core lisp language `if`, `let`, `do`, `lambda`, `cons`, `eval`, etc.
- Subset of scheme R5RS library: lists, vectors, hash tables, integers, real numbers, characters, strings, and integers.
- Common lisp goodies: unhygenic macros (`define-macro`), `push`, `dotimes`.
- Easy to integrate C functions.
- Exact [garbage collection](#garbage-collection) with explicit invocation.
- REPL command line tool.
- Efficient parsing and manipulation of large data files.

### Non-Features

- Compiler or VM.
- Full numeric tower.
- Full call/cc. This only supports simple stack jumps.
- syntax rules.
- UNIX system interface/IO library.

### Examples

### Interactive programming with Read, eval, print loop.
```bash
$ ./lisp
> (define (sqr x) (* x x)))
> (define length 40)
> (define area 0)
> (set! area (sqr length))
1600
```

### Quickstart

```c
LispContext ctx = lisp_init();
lisp_load_lib(ctx);

LispError error;
Lisp program = lisp_read("(+ 1 2)", &error, ctx);
Lisp result = lisp_eval(program, &error, ctx);

if (error != LISP_ERROR_NONE)
    lisp_print(result); ; => 3

lisp_shutdown(ctx);
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
#((name . "Bob Jones")
  (age . 54)
  (city . "SLC"))
```
Loading the structure in C.

```c
LispContext ctx = lisp_init();
// load lisp structure
Lisp data = lisp_read_file(file, ctx);
// get value for age
Lisp age_entry = lisp_avector_ref(data, lisp_make_symbol("AGE", ctx), ctx);
// ...
lisp_shutdown(ctx);
```

### Calling C functions

C functions can be used to extend the interpreter, or call into C code.

```c
Lisp integer_range(Lisp args, LispError* e, LispContext ctx)
{
    // first argument
    LispInt start = lisp_int(lisp_car(args));
    args = lisp_cdr(args);
    // second argument
    LispInt end = lisp_int(lisp_car(args));

    if (end < start)
    {
        *e = LISP_ERROR_OUT_OF_BOUNDS;
        return lisp_make_null();
    }

    LispInt n = end - start;
    Lisp numbers = lisp_make_vector_uninitialized(n, ctx);

    for (LispInt i = 0; i < n; ++i)
        lisp_vector_set(numbers, i, lisp_make_int(start + i));

    return numbers;
}

// ...

// wrap in Lisp object
Lisp func = lisp_make_func(integers_in_range);

// add to enviornment with symbol INTEGER-RANGE
Lisp env = lisp_env_global(ctx);
lisp_env_define(env, lisp_make_symbol("INTEGER-RANGE", ctx), func, ctx);
```

In Lisp
```scheme
(integer-range 5 15)
; => #(5 6 7 8 9 10 11 12 13 14)
```
Constants can also be stored in the environment in a similar fashion.

```c
Lisp pi = lisp_make_real(3.141592);
lisp_env_define(env, lisp_make_symbol("PI", ctx), pi, ctx);
```
### Macros

Common Lisp style (`defmacro`) is available with the name `define-macro`.

    (define-macro nil! (lambda (x)
      `(set! ,x '()))

### Garbage Collection

Garbage is only collected if it is explicitly told to.
You can invoke the garbage collector in C:

    lisp_collect(ctx);

OR in lisp code:

    (gc-flip)

Note that whenever a collect is issued
ANY `Lisp` value in `C`which is not accessible
through the global environment may become invalid.
Be careful what variables you hold onto in C.

Don't call `eval` in a custom defined C function unless you know what you are doing.

See [internals](INTERNALS.md) for more details.

## Documentation

For the language refer to [MIT Scheme](https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_toc.html)
with the understanding that not everything is missing.
If we do implement a feature that MIT scheme has, we will try to follow their specificaiton.

For the C API refer to the header and sample programs (`repl.c`, `printer.c`).

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

