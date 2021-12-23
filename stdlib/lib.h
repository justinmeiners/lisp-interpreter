/*
 Created by: Justin Meiners 
 Repo; https://github.com/justinmeiners/lisp-interpreter
 License: MIT (See end if file)

 This file contains the scheme standard library.
 See lisp.h for insructions.
 */

#ifndef LISP_LIB_H
#define LISP_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

void lisp_lib_load(LispContext ctx);

// convenience for init and load
LispContext lisp_init_with_lib(void);

#ifdef __cplusplus
}
#endif

#endif


