# Internals

Technical design decisions and tricks.


## Memory

We do not use tagged pointers, for simplicity and portability.
`Lisp` objects are fairly large due to alignment requirements (16 bytes).
However, they are usually only stored in this form when interacting
in the C stack. In data structures, we prefer to store `LispVal` (8 bytes)
and pack the types in with the block info.

All allocations are aligned to `sizeof(LispVal)` to avoid unaligned access.

- [Chicken representation](http://www.more-magic.net/posts/internals-data-representation.html)

## Garbage Collection

The choice to use explicit, rather than automatic garbage collection, was made so that the interpreter does not need to keep track of every lisp object on the stack, only the most important objects.
 If garbage collection was allowed to trigger at any time in the middle of a C function call, then the interpreter would need to be able to "see" all the lisp values on the call stack, in order to prevent them from being collected. Providing this feature would make integrating with C code much more complicated and conflict with the project's goal of being easily embeddable.

This means that when `lisp_collect` is called, all lisp values which are not reachable from the global environment or the function's parameters become invalidated. Be conscious of when and where you call the garbage collector.

An alternative solution is used in [Lua][lua-memory].

The interpreter uses the [Cheney algorithim][cheney-mta] for garbage collection. Memory is allocated in fixed size pages. When an allocation is request and the current page does not have enough space remaining, a new page will be allocated to fulfill the allocation. So, allocations will continue to use up more memory until garbage collection.
Note that tail call recursion will not overflow the stack, but will use additional memory for each function call.

[cheney-mta]: https://en.wikipedia.org/wiki/Cheney%27s_algorithm
[mta-info]: http://home.pipeline.com/~hbaker1/CheneyMTA.html
[lua-memory]: https://www.lua.org/pil/24.2.html
[gc-internals]: http://www.more-magic.net/posts/internals-gc.html

## General design

- [Lysp][lysp]
- [Lispy][lispy]

[lysp]: http://piumarta.com/software/lysp/
[lispy]: http://norvig.com/lispy.html


## Environments/Tables

Tables are only resized at garbage collect time.
This leads us to linked list-chaining as opposed to open addresing
because linked list chaining can scale infinitely while
waiting for garbage collection.

- [SICP][sicp-environments]
- [MIT][environment-objects]

[sicp-environments]: https://mitpress.mit.edu/sicp/full-text/book/book-Z-H-21.html#%_sec_3.2
[environment-objects]: https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_14.html

## Symbols

- Reference counting symbol table? - http://sandbox.mc.edu/~bennet/cs404/ex/lisprcnt.html

