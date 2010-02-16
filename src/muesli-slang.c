// muesli-slang.c -*- C -*-
/* Interface to slang
   Copyright (C) 2008, 2009, 2010 University of Limerick

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

#ifndef _MUESLI_SLANG_
#define _MUESLI_SLANG_

#include "../config.h"
#include "muesli.h"

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>		// todo: do I need this?
#include <slang.h>

static int ambient_transient = 0;

#ifndef CUSTOMIZED

typedef struct slang_state {
  int fred;
} slang_state;

typedef struct slang_table {
  int count;
} slang_table;

typedef struct slang_symbol {
  char *pname;
} slang_symbol;

slang_state *slang_create_custom() { return NULL; }

// Getting args from your scripting language stack:
int slang_count_args(slang_state *S) { return 0; }
int slang_number_arg(slang_state *S, int argi) { return 0; }
char *slang_string_arg(slang_state *S, int argi) { return (char*)""; }
int slang_arg_is_string(slang_state *S, int argi) { return 0; }
int slang_arg_is_number(slang_state *S, int argi) { return 0; }

// Putting results back onto your scripting language stack:
void slang_return_string(slang_state *S, const char *string_back) {}
void slang_return_number(slang_state *S, float number_back) {}
void slang_return_boolean(slang_state *S, int int_back) {}
void slang_return_void(slang_state *S) {};

// Handling table / map / alist structures of your scripting language:
slang_table *slang_create_table(slang_state *S, int size) { return NULL; }
void slang_set_table_string(slang_state *S, slang_table *T, char *key, const char *value) {}
void slang_set_table_number(slang_state *S, slang_table *T, char *key, float value) {}
void slang_set_table_boolean(slang_state *S, slang_table *T, char *key, int value) {}

// Make a name / symbol of the scripting language:
slang_symbol *slang_make_symbol(slang_state *S, const char *n) { return NULL; }

// Non-local exits in the scripting language
void slang_throw(slang_state *S, slang_symbol *Y, float n) {}

// Set a global variable of the scripting language to contain a table:
void slang_set_global_variable_table(slang_state *S, slang_symbol *varname, slang_table *table) {}

// Extend the scripting language with an added built-in function:
void slang_register_builtin_function(slang_state *S, const char *funname, void (*function)(slang_state *FS)) {}
#endif

// A specialized non-local exit; could either throw, or just quit the
// application run:

static evaluator_interface *slang_interface = NULL;

static char *option_names_var_name = (char*)"option_names";

static void
slang_set_parameter(char *name, char *value)
{
  char option_code = muesli_find_option_letter(slang_interface->getopt_options, name);

  if (option_code != -1) {
    (slang_interface->handle_option)(slang_interface->app_params,
				     option_code, muesli_malloc_copy_string(value),
				     0.0, 1, "custom");
  }
}

#if 0
static void
slang_set_parameters(slang_state *cs)
{
  if (slang_count_args(cs) < 1)  {
    fprintf(stderr, "too few args to slang_set_parameters\n");
    slang_error(cs, (char*)"argcount", (char*)"set_parameters");
  }

#if 0
  // Fill in: use a table iterator from your language
  slang_table table = slang_table_arg(cs, 0);

  slang_table_iteration_start(cs, table);
  while (slang_table_iteration_next(cs, table) != 0) {
    slang_set_parameter(cs,
			 slang_table_iteration_current_key(cs, table),
			 slang_table_iteration_current_value(cs, table));
  }

#endif
}
#endif

static void
slang_get_parameter(char *name)
{
  char option_code = muesli_find_option_letter(slang_interface->getopt_options,
					       name);

  if (option_code != -1) {

    muesli_value_t result = (slang_interface->handle_option)(slang_interface->app_params,
							     option_code,	// option
							     NULL, 0.0,	// value
							     0,	// set
							     "custom");

    switch (result.type) {
    case muesli_value_string:
      SLang_push_malloced_string(muesli_malloc_copy_string(result.data.as_string));
      break;
    case muesli_value_float:
      SLang_push_float(result.data.as_float);
      break;
    case muesli_value_boolean:
      SLang_push_int(result.data.as_int);
      break;
    default:
      SLang_push_int(result.data.as_bool);
      break;
    }
  }
}

#if 0
static void
slang_get_parameters(slang_state *cs)
{
  slang_table *table = slang_create_table(cs, 48);

  // todo: fix and re-instate -- I have to get long_options across to it somehow
  struct option *option_names = slang_interface->getopt_options;

  while (option_names->name != 0) {
    
    muesli_value_t result = (slang_interface->handle_option)(slang_interface->app_params,
							     (option_names->val), // opt
							     NULL, 0.0,		     // value
							     0,		     // set
							     "custom");

    switch (result.type) {
    case muesli_value_string:
      slang_set_table_string(cs, table, (char*)(option_names->name), result.data.as_string);
      break;
    case muesli_value_float:
      slang_set_table_number(cs, table, (char*)(option_names->name), result.data.as_float);
      break;
    case muesli_value_boolean:
      slang_set_table_boolean(cs, table, (char*)(option_names->name), result.data.as _int);
      break;
    default:
      slang_set_table_boolean(cs, table, (char*)(option_names->name), 0);
      break;
    }
    option_names++;
  }
}
#endif

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static void
slang_eval_in_language(char *language_name, char *evaluand)
{
  unsigned int evaluand_length = strlen(evaluand);

  fprintf(stderr, "In slang_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

  muesli_value_t result = muesli_eval_in_language(language_name,
						  evaluand,
						  evaluand_length,
						  ambient_transient);

  switch (result.type) {
  case muesli_value_string:
    SLang_push_malloced_string(muesli_malloc_copy_string(result.data.as_string));
    break;
  case muesli_value_float:
    SLang_push_float(result.data.as_float);
    break;
  case muesli_value_boolean:
    SLang_push_int(result.data.as_int);
    break;
  default:
    SLang_push_int(result.data.as_bool);
    break;
  }
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This isn't meant as self-referentially as it looks (pity!).  It's
// really there so that if you use this file as a template for another
// interpreter interface, the language you're adding will get access
// to custom built-in functions just like the existing languages do.

static void
slang_eval_custom(char *string_arg)
{
  muesli_value_t result = (slang_interface->eval_string)(slang_interface,
							 string_arg, strlen(string_arg),
							 ambient_transient);

  switch (result.type) {
  case muesli_value_string:
    SLang_push_malloced_string(muesli_malloc_copy_string(result.data.as_string));
    break;
  case muesli_value_float:
    SLang_push_float(result.data.as_float);
    break;
  case muesli_value_boolean:
    SLang_push_int(result.data.as_int);
    break;
  default:
    SLang_push_int(result.data.as_bool);
    break;
  }
}

////////////////
// Initialize //
////////////////

void
slang_evaluator_init(evaluator_interface *interface)
{
  // Init slang interface

  if (-1 == SLang_init_all ()) {
    fprintf(stderr, "Could not initialize slang\n");
    exit(1);
  }

  // Fill in: initialize your custom application
  slang_state *our_slang_state = slang_create_custom();
  interface->state = our_slang_state;
  slang_interface = interface;

  // Extend system

  SLadd_intrinsic_function((char*)"get_parameter", (FVOID_STAR)slang_get_parameter, SLANG_STRING_TYPE, 1, SLANG_STRING_TYPE);
  SLadd_intrinsic_function((char*)"set_parameter", (FVOID_STAR)slang_set_parameter, SLANG_VOID_TYPE, 2, SLANG_STRING_TYPE, SLANG_STRING_TYPE);
  SLadd_intrinsic_function((char*)"custom_eval", (FVOID_STAR)slang_eval_custom, SLANG_STRING_TYPE, 1, SLANG_STRING_TYPE);
  SLadd_intrinsic_function((char*)"eval_in_language", (FVOID_STAR)slang_eval_in_language, SLANG_VOID_TYPE, 2, SLANG_STRING_TYPE, SLANG_STRING_TYPE);

  // SLadd_intrinsic_function((char*)"parameters", (FVOID_STAR), slang_get_parameters, 0);
  // SLadd_intrinsic_function((char*)"modify_parameters", (FVOID_STAR), slang_set_parameters, 1, ?);

#if 0
  // Set up option names

  // Fill in: Create a table of option names.  You may well not need to bother.
  slang_table *option_names_table = slang_create_table(our_slang_state, 48);
  struct option *option_names;
  for (option_names = slang_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    slang_set_table_number(our_slang_state, option_names_table,
			   (char*)(option_names->name), (option_names->val));
  }
  slang_set_global_variable_table(our_slang_state,
				  slang_make_symbol(our_slang_state,
						    option_names_var_name),
				  option_names_table);
#endif
}

static void
slang_load_file(evaluator_interface *interface,
		const char *filename)
{
  int muesli_flags = interface->flags;
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }

  if (-1 == SLang_load_file(filename)) {
    /* Clear the error and reset the interpreter */
    SLang_restart (1);
  }

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

static muesli_value_t
slang_eval_string(evaluator_interface *evaluator,
		   const char *slang_c_string,
		   unsigned int string_length,
		   int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (slang_c_string) {
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;

    if (-1 == SLang_load_string (slang_c_string)) {
      SLang_restart (1);
    }
    // Fill in: evaluate slang_c_string, decide what kind of result
    // it returns, and fill in the result pointer and the result type
    // accordingly.

    ambient_transient = old_ambient_transient;
  }

  return result;
}

void
slang_setup(evaluator_interface *new_slang_interface)
{
  slang_interface = new_slang_interface;

  slang_interface->eval_string = slang_eval_string;
  slang_interface->load_file = slang_load_file;

  slang_interface->version = SLANG_VERSION_STRING;
}

#endif

