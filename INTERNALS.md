
## Garbage Collection

The choice to use explicit, rather than automatic garbage collection, was made so that the interpreter does not need to keep track of every lisp object on the stack, only the most important objects.
 If garbage collection was allowed to trigger at any time in the middle of a C function call, then the interpreter would need to be able to "see" all the lisp values on the call stack, in order to prevent them from being collected. Providing this feature would make integrating with C code much more complicated and conflict with the project's goal of being easily embeddable.

This means that when `lisp_collect` is called, all lisp values which are not reachable from the global environment or the function's parameters become invalidated. Be conscious of when and where you call the garbage collector.

(You can learn about an alternative solution in the [Lua Scripting Language](https://www.lua.org/pil/24.2.html). )

The interpreter uses the [Cheney algorithim](https://en.wikipedia.org/wiki/Cheney%27s_algorithm) for garbage collection. Memory is allocated in fixed size pages. When an allocation is request and the current page does not have enough space remaining, a new page will be allocated to fulfill the allocation. So, allocations will continue to use up more memory until garbage collection.
Note that tail call recursion will not overflow the stack, but will use additional memory for each function call.
 
