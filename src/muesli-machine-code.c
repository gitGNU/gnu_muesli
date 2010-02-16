// muesli-machine_code.c -*- C -*-
/* muesli interface to machine_code evaluators / template for new language interfaces
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

#ifndef _MUESLI_MACHINE_CODE_
#define _MUESLI_MACHINE_CODE_


#include "../config.h"
#include "muesli.h"
#include <stdlib.h>

#define MACHINE_CODE_VERSION "0.0"

// A block of useful stuff; we pass the machine code individuals a
// pointer to one of these:
typedef struct machine_code_state {
  void *data;
} machine_code_state_t;

machine_code_state_t *machine_code_global_state = NULL;

static  machine_code_state_t *
machine_code_create_machine_code()
{
  return (machine_code_state_t*)malloc(sizeof(machine_code_state_t));
}

// Extend the scripting language with an added built-in function:
static void machine_code_register_builtin_function(machine_code_state_t *S, const char *funname, void (*function)(machine_code_state_t *FS)) {}

typedef float (*machine_code_function_t)(machine_code_state_t*, void*);

void
muesli_machine_code_add_function(int fn_number, float (*function)(float))
{
  // todo: complete this, and merge it into machine_code_register_builtin_function
}

static float
machine_code_eval(machine_code_state_t *state,
		  const char *scratch,
		  unsigned int length,
		  void *arg)
{
  return ((machine_code_function_t)(scratch))(state, arg);
}

static float
machine_code_eval_string_no_arg(evaluator_interface *interface,
				const char *scratch,
				unsigned int length)
{
  return machine_code_eval((machine_code_state_t*)interface->state,
			   scratch, length,
			   interface->app_data);
}

static muesli_value_t
machine_code_eval_string(evaluator_interface *interface,
			 const char *scratch, unsigned int length,
			 int transient)
{
  muesli_value_t result;
  result.type = muesli_value_float;
  result.data.as_float = machine_code_eval((machine_code_state_t*)interface->state,
					   scratch, length,
					   interface->app_data);
  return result;
}

////////////////
// Initialize //
////////////////

void
machine_code_evaluator_init(evaluator_interface *interface)
{
  machine_code_state_t *state = (machine_code_state_t*)interface->state;
  state->data = interface->app_data;
}

void
machine_code_setup(evaluator_interface *new_machine_code_interface)
{
  evaluator_interface *machine_code_interface = new_machine_code_interface;

  machine_code_global_state = machine_code_create_machine_code();
  machine_code_interface->state = machine_code_global_state;

  machine_code_interface->eval_string = machine_code_eval_string;
  machine_code_interface->load_file = NULL;  // not used

  machine_code_interface->eval_0 = machine_code_eval_string_no_arg;

  machine_code_interface->version = MACHINE_CODE_VERSION;
}

#endif

