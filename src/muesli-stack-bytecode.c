// muesli-stack-bytecode.c -*- C -*-
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

#ifndef _MUESLI_STACK_BYTECODE_
#define _MUESLI_STACK_BYTECODE_

#include "../config.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>

#include "muesli.h"
#include "muesli-stack-bytecode.h"

// stack_bytecode_state *stack_bytecode_global_state = NULL;

static int ambient_transient = 0;

stack_bytecode_state *
stack_bytecode_create_stack_bytecode()
{
  stack_bytecode_state *s = (stack_bytecode_state*)malloc(sizeof(stack_bytecode_state));
  int i;
  
  s->tos = s->stack = (float*)malloc(sizeof(float)*SB_STACK_DEPTH);
  s->just_beyond_stack = s->stack + SB_STACK_DEPTH;
  s->registers = (float*)malloc(sizeof(float)*SB_N_REGISTERS);

  s->verbosity = 0;

  for (i = 0; i < SB_STACK_DEPTH; i++) {
    s->stack[i] = 0.0;
  }

  for (i = 0; i < SB_N_REGISTERS; i++) {
    s->registers[i] = 0.0;
  }

  s->registers[SB_FN_CODE('o')] = 1.0;	// one
  s->registers[SB_FN_CODE('t')] = 2.0;	// two
  s->registers[SB_FN_CODE('e')] = M_E; // mathematical constant `e'
  s->registers[SB_FN_CODE('p')] = M_PI; // mathematical constant `pi'
  s->registers[SB_FN_CODE('r')] = M_SQRT2; // mathematical constant `sqrt(2.0)'

  return s; 
}

// Load and run a scripting language file:
static void
stack_bytecode_load_file(evaluator_interface *interface,
			 const char *filename)
{
  // stack_bytecode_state *S

  struct stat stat_buf;
  char *buffer;

  int muesli_flags = interface->flags;

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }

  if (stat(filename, &stat_buf) != 0) {
    fprintf(stderr, "Could not stat %s\n", filename);
    return;
  }
  int size = stat_buf.st_size;
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s\n", filename);
    return;
  }
  buffer = (char*)malloc(size + 2);
  read(fd, buffer, size);
  close(fd);
  buffer[size] = '.';
  buffer[++size] = '\0';

  int success = 1;
  stack_bytecode_eval(interface, buffer, size, &success);

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

// The latest one passed in from an external call
evaluator_interface *stack_interpreter_interface = NULL;

static void
stack_code_error(const char *message, int arg)
{
  fprintf(stderr, message, arg);
  exit(1);
}

////////////////
// Initialize //
////////////////

void
stack_bytecode_evaluator_init(evaluator_interface *interface)
{
  unsigned int i;

  interface->state = stack_bytecode_create_stack_bytecode();
  ((stack_bytecode_state*)(interface->state))->verbosity = interface->language_verbosity;

  fprintf(stderr, "Set stack-code verbosity of %p to %d\n", interface->state,
	  ((stack_bytecode_state*)(interface->state))->verbosity);

  stack_bytecode_builtin_function *app_fns = ((stack_bytecode_state*)(interface->state))->app_fns;

  for (i = 0; i < N_APP_FNS; i++) {
    app_fns[i] = NULL;
  }
}

void
stack_bytecode_add_app_fn(evaluator_interface *interface,
			  int code,
			  stack_bytecode_builtin_function fn)
{
  if ((code < 0) || (code > N_APP_FNS)) {
    fprintf(stderr, "function code out of range\n");
    exit(1);
  }
  ((stack_bytecode_state*)(interface->state))->app_fns[code] = fn;
}

///////////////////
// The evaluator //
///////////////////

float
stack_bytecode_eval(evaluator_interface *interface,
		    const char *scratch, unsigned int length,
		    int *success_ptr)
{
  stack_bytecode_state *cs = (stack_bytecode_state*)(interface->state);
  cs->verbosity = interface->language_verbosity;

  const uint8_t *program = (const uint8_t*)scratch;
  const uint8_t *pc = program;
  const uint8_t *pc_end = program + length;

  float *sp = cs->tos;
  float *sp_min = cs->stack - 1; // cs->tos == cs->stack means there
				 // is one item on the stack, because
				 // for speed cs->tos points at the
				 // current item
  float *sp_max = cs->just_beyond_stack;
  float *registers = cs->registers;
  float tmp;

  int verbosity = cs->verbosity;

  if (verbosity >= 1) {
    float *ssp = sp;
    int si = 0;
    fprintf(stderr, "Starting program \"%.*s\" with %d stack items\n", length, scratch, sp - sp_min);
    while (ssp >= sp_min) {
      fprintf(stderr, "%d: %f\n", si++, *ssp--);
    }
  }

  while (pc < pc_end) {
    char c;
    if (verbosity >= 2) { fprintf(stderr, "%04d: %c %f[%d]\n", pc - program, *pc, *sp, sp - sp_min); }
    switch (*pc++) {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
    case 'v': case 'w': case 'x': case 'y': case 'z':
      *++sp = (float)(SB_FN_CODE(*(pc-1))); // push (register/function) number
      break;
    case '<': tmp = *sp--; registers[(unsigned int)tmp] = *sp--; break; // store to register
    case '>': *sp = registers[(unsigned int)*sp]; break;     // load from register
    case '=': tmp = *sp++; *sp = tmp; break; // duplicate
    case '_': sp--; break;	  // drop
    case ':': tmp = *sp; *sp = *(sp-1); *(sp-1) = tmp; break; // exchange
    case '-': tmp = *sp--; *sp -= tmp; break; // arithmetic operations
    case '+': tmp = *sp--; *sp += tmp; break;
    case '*': tmp = *sp--; *sp *= tmp; break;
    case '/': tmp = *sp--; *sp /= tmp; break;
    case '\\': tmp = *sp--; *sp = tmp / *sp; break;
    case '&': tmp = *sp--; *sp = ((tmp != 0.0) && (*sp != 0.0)) ? 1.0 : 0.0; break;
    case '|': tmp = *sp--; *sp = ((tmp != 0.0) || (*sp != 0.0)) ? 1.0 : 0.0; break;
    case '~': *sp = (*sp == 0.0) ? 1.0 : 0.0; break;
    case '!': // functions:
      tmp = (*sp--);
      if (cs->app_fns[((unsigned int)tmp) & APP_FNS_MASK] != NULL) {
	stack_bytecode_builtin_function fn = (cs->app_fns[((unsigned int)tmp) & APP_FNS_MASK]);
	if (fn != NULL) {
	  cs->tos = sp;		// write back in case we recurse evaluator
	  (fn)(interface);
	  sp = cs->tos;		// retrieve in case we recursed evaluator
	}
      } else {
	// We compile with -ffast-math to inline these:
	switch ((unsigned int)tmp) {
	case SB_FN_CODE('a'): *sp = atanf(*sp); break;
	case SB_FN_CODE('c'): *sp = cosf(*sp); break;
	case SB_FN_CODE('d'): *sp = acosf(*sp); break;
	case SB_FN_CODE('e'): *sp = expf(*sp); break;
	case SB_FN_CODE('l'): *sp = logf(*sp); break;
	case SB_FN_CODE('s'): *sp = sinf(*sp); break;
	case SB_FN_CODE('t'): *sp = tanf(*sp); break;
	case SB_FN_CODE('u'): { float y = *sp--; *sp = atan2f(*sp, y); } break;
	case SB_FN_CODE('v'): *sp = asinf(*sp); break;
	  // todo: add call to custom?
	}
      }
      break;
    case ' ': break;
    case '\n': break;
    case '#': while (((c = *pc++) != '\n') && (c != '\0') && (pc < pc_end)) {};
    case '.': tmp = *sp--; cs->tos = sp; if (verbosity >= 1) {fprintf(stderr, "returning %f\n", tmp);} return tmp; break;
    default: break;
    }
    // todo: check for isnan(*sp) and optionally return an error
    if (sp < sp_min) {
      fprintf(stderr, "SP %d below min in program \"%.*s:here:%.*s\"\n", sp_min - sp, pc - program, program, length - (pc - program), pc);
      stack_code_error("Dropped off bottom of stack\n", 0);
      cs->tos = sp;
      return 0;
    }
    if (sp >= sp_max) {
      stack_code_error("SP %d above max\n", sp - sp_max);
      cs->tos = sp;
      return 0;
    }
  }

  tmp = *sp--;
  
  cs->tos = sp;

  if (verbosity >= 1) {
    fprintf(stderr, "Returning %f\n", tmp);
  }

  return tmp;
}

////////////////////////////////////////
// Various wrappings of the evaluator //
////////////////////////////////////////

static void
stack_bytecode_eval_string_give_arg(evaluator_interface *interface,
				    float arg)
{
  stack_bytecode_state *state = (stack_bytecode_state *)interface->state;
  *state->tos = arg;
}

static void
stack_bytecode_eval_string_clear_args(evaluator_interface *interface)
{
  stack_bytecode_state *state = (stack_bytecode_state *)interface->state;
  state->tos = state->stack;
}


static float
stack_bytecode_eval_string_given_args(evaluator_interface *interface,
				      const char *scratch,
				      unsigned int length)
{
  int success = 1;
  stack_interpreter_interface = interface;
  return stack_bytecode_eval(interface, scratch, length, &success);
}

static muesli_value_t
stack_bytecode_eval_string(evaluator_interface *interface,
			   const char *stack_bytecode_c_string,
			   unsigned int string_length,
			   int transient)
{
  muesli_value_t result;
  int success = 1;
  ANULL_VALUE(result);
  stack_interpreter_interface = interface;

  int old_ambient_transient = ambient_transient;
  ambient_transient = transient;

  if (stack_bytecode_c_string) {
    result.data.as_float = stack_bytecode_eval(interface,
					       stack_bytecode_c_string,
					       string_length,
					       &success);

    ambient_transient = old_ambient_transient;

    if (success) {
      result.type = muesli_value_float;
    } else {
      result.type = muesli_value_none;
    }
  } else {
    result.type = muesli_value_error;
  }
  return result;
}

void
stack_code_setup(evaluator_interface *new_stack_code_interface)
{
  stack_interpreter_interface = new_stack_code_interface;

  stack_interpreter_interface->eval_string = stack_bytecode_eval_string;
  stack_interpreter_interface->load_file = stack_bytecode_load_file;

  stack_interpreter_interface->eval_clear = stack_bytecode_eval_string_clear_args;
  stack_interpreter_interface->eval_give = stack_bytecode_eval_string_give_arg;
  stack_interpreter_interface->eval_given = stack_bytecode_eval_string_given_args;

  stack_interpreter_interface->eval_0 = stack_bytecode_eval_string_given_args;

  stack_interpreter_interface->version = STACK_BYTECODE_VERSION;
}

#endif
