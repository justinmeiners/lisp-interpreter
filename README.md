lisp-interpreter
===============

### About ###

A single header + source Lisp intepreter. I wrote this to improve my knowledge of lisp and to make an implementation that is easily embedded into C programs.

### Features ###

- One header and source file.
- Closures
- Cheney garbage collection with explicit invocation.
- Symbol table
- Easy integration of C functions.
- REPL command line tool.

### Example ###

```
$ ./lisp_i
> (define sqr (lambda (x) (* x x)))
> (define length 40)
> (define area 0)
> (set! area (sqr length))
1600
```

### Status ###

This project is in progress and is not stable. However, most of the lisp features have been implemented and work properly. You can find a list of todos on the project page.

