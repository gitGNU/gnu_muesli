// muesli-stack-bytecode.h -*- C -*-
/* muesli stack bytecode evaluator
   Copyright (C) 2008, 2009 University of Limerick

   This file is part of Muesli.

   Muesli is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   Muesli is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with Muesli.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef MUESLI_STACK_BYTECODE_H
#define MUESLI_STACK_BYTECODE_H

#define STACK_BYTECODE_VERSION "0.0"

#define N_APP_FNS 256		// make it a power of 2
#define APP_FNS_MASK (N_APP_FNS - 1) // only works for powers of 2

typedef void (*stack_bytecode_builtin_function)(evaluator_interface*);

// An evaluator (interpreter) will typically have a global, or
// nearly-global, state-holder, and some types of things that hang off
// that:

typedef struct stack_bytecode_state {
  float* stack;
  float* tos;
  float *just_beyond_stack;
  float *registers;
  int verbosity;
  stack_bytecode_builtin_function app_fns[N_APP_FNS];
} stack_bytecode_state;

extern void stack_bytecode_add_app_fn(evaluator_interface *interface,
				      int code,
				      stack_bytecode_builtin_function fn);


#define SB_STACK_DEPTH 1024
#define SB_N_REGISTERS 256

#define SB_PUSH(_v_,_s_) { *(++((_s_)->tos)) = (_v_); }
#define SB_POP(_s_) (*((_s_)->tos--))
#define SB_COUNT(_s_) (((_s_)->tos) - ((_s_)->stack))
#define SB_FN_CODE(_c_) ((_c_)-'a')

float stack_bytecode_eval(evaluator_interface*, const char*, unsigned int, int *success_ptr);

#endif
