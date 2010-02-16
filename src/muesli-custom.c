// muesli-custom.c -*- C -*-
/* Interface to custom evaluators / template for new language interfaces
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

#ifndef _MUESLI_CUSTOM_
#define _MUESLI_CUSTOM_

#include "../config.h"
#include "muesli.h"

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>		// todo: do I need this?

#include "muesli-custom.h"

static int ambient_transient = 0;

#ifndef CUSTOMIZED

custom_state *custom_create_custom() { return NULL; }

// Getting args from your scripting language stack:
int custom_count_args(custom_state *S) { return 0; }
int custom_number_arg(custom_state *S, int argi) { return 0; }
char *custom_string_arg(custom_state *S, int argi) { return (char*)""; }
int custom_arg_is_string(custom_state *S, int argi) { return 0; }
int custom_arg_is_number(custom_state *S, int argi) { return 0; }

// Putting results back onto your scripting language stack:
void custom_return_string(custom_state *S, const char *string_back) {}
void custom_return_number(custom_state *S, float number_back) {}
void custom_return_integer(custom_state *S, int number_back) {}
void custom_return_boolean(custom_state *S, int int_back) {}
void custom_return_void(custom_state *S) {};

// Handling table / map / alist structures of your scripting language:
custom_table *custom_create_table(custom_state *S, int size) { return NULL; }
void custom_set_table_string(custom_state *S, custom_table *T, char *key, const char *value) {}
void custom_set_table_number(custom_state *S, custom_table *T, char *key, float value) {}
void custom_set_table_boolean(custom_state *S, custom_table *T, char *key, int value) {}

// Make a name / symbol of the scripting language:
custom_symbol *custom_make_symbol(custom_state *S, const char *n) { return NULL; }

// Non-local exits in the scripting language
void custom_throw(custom_state *S, custom_symbol *Y, float n) {}

// Set a global variable of the scripting language to contain a table:
void custom_set_global_variable_table(custom_state *S, custom_symbol *varname, custom_table *table) {}

// Extend the scripting language with an added built-in function:
void custom_register_builtin_function(custom_state *S, const char *funname, void (*function)(custom_state *FS)) {}
#endif

// A specialized non-local exit; could either throw, or just quit the
// application run:
void custom_error(custom_state *S, char *message, char *arg)
{
  fprintf(stderr, "Got error \"%s\", \"%s\"\n", message, arg);
}

static evaluator_interface *custom_interface = NULL;

static char *option_names_var_name = (char*)"option_names";

static void
custom_set_parameter(custom_state *cs)
{
  if (custom_count_args(cs) < 2) {
    fprintf(stderr, "too few args to custom_set_parameter\n");
    custom_error(cs, (char*)"argcount", (char*)"set_parameter");
  }

  char option_code = muesli_find_option_letter(custom_interface->getopt_options, custom_string_arg(cs, 0));

  if (option_code != -1) {
    if (custom_arg_is_string(cs, 1)) {
      (custom_interface->handle_option)(custom_interface->app_params,
					option_code,
					muesli_malloc_copy_string(custom_string_arg(cs, 1)),
					0.0, 1,
					"custom");
    } else if (custom_arg_is_number(cs, 1)) {
      (custom_interface->handle_option)(custom_interface->app_params,
					option_code, NULL, custom_number_arg(cs, 2), 1,
					"custom");
    } else {
      (custom_interface->handle_option)(custom_interface->app_params,
					option_code, (char*)"true", 0.0, 1,
					"custom");
    }
  }
}

static void
custom_set_parameters(custom_state *cs)
{
  if (custom_count_args(cs) < 1)  {
    fprintf(stderr, "too few args to custom_set_parameters\n");
    custom_error(cs, (char*)"argcount", (char*)"set_parameters");
  }

#if 0
  // Fill in: use a table iterator from your language
  custom_table table = custom_table_arg(cs, 0);

  custom_table_iteration_start(cs, table);
  while (custom_table_iteration_next(cs, table) != 0) {
    custom_set_parameter(cs,
			 custom_table_iteration_current_key(cs, table),
			 custom_table_iteration_current_value(cs, table));
  }

#endif
}

static void
custom_get_parameter(custom_state *cs)
{
  if (custom_count_args(cs) < 1)  {
    fprintf(stderr, "too few args to custom_get_parameter\n");
    custom_error(cs, (char*)"argcount", (char*)"get_parameter");
  }

  char option_code = muesli_find_option_letter(custom_interface->getopt_options,
					       custom_string_arg(cs, 0));

  if (option_code != -1) {

muesli_value_t result = (custom_interface->handle_option)(custom_interface->app_params,
							  option_code,	// option
							  NULL, 0.0,	// value
							  0,	// set
							  "custom");

    switch (result.type) {
    case muesli_value_string:
      custom_return_string(cs, result.data.as_string);
      break;
    case muesli_value_float:
      custom_return_number(cs, result.data.as_float);
      break;
    case muesli_value_integer:
      custom_return_integer(cs, result.data.as_int);
      break;
    case muesli_value_boolean:
      custom_return_boolean(cs, result.data.as_int);
      break;
    default:
      custom_return_boolean(cs, 0);
      break;
    }
  }
}

static void
custom_get_parameters(custom_state *cs)
{
  custom_table *table = custom_create_table(cs, 48);

  // todo: fix and re-instate -- I have to get long_options across to it somehow
  struct option *option_names = custom_interface->getopt_options;

  while (option_names->name != 0) {
    
    muesli_value_t result = (custom_interface->handle_option)(custom_interface->app_params,
							      (option_names->val), // opt
							      NULL, 0.0, // value
							      0, // set
							      "custom");

    switch (result.type) {
    case muesli_value_string:
      custom_set_table_string(cs, table, (char*)(option_names->name), result.data.as_string);
      break;
    case muesli_value_float:
      custom_set_table_number(cs, table, (char*)(option_names->name), result.data.as_float);
      break;
    case muesli_value_boolean:
      custom_set_table_boolean(cs, table, (char*)(option_names->name), result.data.as_int);
      break;
    default:
      custom_set_table_boolean(cs, table, (char*)(option_names->name), 0);
      break;
    }
    option_names++;
  }
}

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static int
muesli_custom_eval_in_language(custom_state *cs)
{
  if (custom_count_args(cs) < 2) {
    fprintf(stderr, "too few args to custom_eval_in_language\n");
    custom_error(cs, (char*)"argcount", (char*)"eval_in_language");
    return 0;
  }

  if ((custom_arg_is_string(cs, 1)) && (custom_arg_is_string(cs, 2))) {
    const char *language_name = custom_string_arg(cs, 1);
    const char *evaluand = custom_string_arg(cs, 2);
    unsigned int evaluand_length = strlen(evaluand);

    fprintf(stderr, "In custom_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

    muesli_value_t result = muesli_eval_in_language(language_name,
						    evaluand,
						    evaluand_length,
						    ambient_transient);

    switch (result.type) {
    case muesli_value_float:
      custom_return_number(cs, result.data.as_float);
      break;
    case muesli_value_integer:
      custom_return_integer(cs, result.data.as_int);
      break;
    case muesli_value_string:
      custom_return_string(cs, result.data.as_string);
      break;
    case muesli_value_boolean:
      custom_return_boolean(cs, result.data.as_bool);
      break;
    case muesli_value_none:
    case muesli_value_error:
      custom_return_boolean(cs, 0);
      break;
    }
  } else {
    fprintf(stderr, "wrong type args to custom_eval_in_language\n");
    custom_error(cs, (char*)"argtype", (char*)"eval_in_language");
    return 0;
  }
  return 1;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This isn't meant as self-referentially as it looks (pity!).  It's
// really there so that if you use this file as a template for another
// interpreter interface, the language you're adding will get access
// to custom built-in functions just like the existing languages do.

static void
custom_eval_function(custom_state *cs)
{
  if (custom_arg_is_string(cs, 0)) {
    char *string_arg = custom_string_arg(cs, 0);
    muesli_value_t result = (custom_interface->eval_string)(custom_interface,
							    string_arg, strlen(string_arg),
							    ambient_transient);
    switch (result.type) {
    case muesli_value_float:
      custom_return_number(cs, result.data.as_float);
      break;
    case muesli_value_integer:
      custom_return_integer(cs, result.data.as_int);
      break;
    case muesli_value_string:
      custom_return_string(cs, result.data.as_string);
      break;
    case muesli_value_boolean:
      custom_return_boolean(cs, result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      custom_return_void(cs);
      break;
    }
  } else {
    fprintf(stderr, "custom_eval_function must be given a string\n");
    custom_error(cs, (char*)"argtype", (char*)"custom_eval");
  }
}

////////////////
// Initialize //
////////////////

void
custom_evaluator_init(evaluator_interface *interface)
{
  // Init custom interface

  // Fill in: initialize your custom application
  custom_state *our_custom_state = custom_create_custom();
  interface->state = our_custom_state;
  custom_interface = interface;

  // Extend system

  // Fill in: register all these language extensions (as applicable):
  Muesli_Add_Fn_1(interface, (char*)"get_parameter", custom_get_parameter);
  Muesli_Add_Fn_2(interface, (char*)"set_parameter", custom_set_parameter);
  Muesli_Add_Fn_0(interface, (char*)"evolution_parameters", custom_get_parameters);
  Muesli_Add_Fn_1(interface, (char*)"modify_evolution_parameters", custom_set_parameters);
  Muesli_Add_Fn_1(interface, (char*)"custom", custom_eval_function);
  Muesli_Add_Fn_1(interface, (char*)"eval_in_language", muesli_custom_eval_in_language);

  // Set up option names

  // Fill in: Create a table of option names.  You may well not need to bother.
  custom_table *option_names_table = custom_create_table(our_custom_state, 48);
  struct option *option_names;
  for (option_names = custom_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    custom_set_table_number(our_custom_state, option_names_table,
			    (char*)(option_names->name), (option_names->val));
  }
  custom_set_global_variable_table(our_custom_state,
				   custom_make_symbol(our_custom_state,
						      option_names_var_name),
				   option_names_table);
}

static void
custom_load_file(evaluator_interface *interface,
		const char *filename)
{
  int muesli_flags = interface->flags;
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
  // Fill in: load the functions file given as (char*)(filename)
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

static muesli_value_t
custom_eval_string(evaluator_interface *evaluator,
		   const char *custom_c_string,
		   unsigned int string_length,
		   int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (custom_c_string) {
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;
    // fprintf(stderr, "Custom evaluating string \"%s\"\n", custom_c_string);

    // Fill in: evaluate custom_c_string, decide what kind of result
    // it returns, and fill in the result pointer and the result type
    // accordingly.



    ambient_transient = old_ambient_transient;
  }

  return result;
}

static float
custom_eval_0(struct evaluator_interface *interface,
	      const char *scratch,
	      unsigned int length)
{
  muesli_value_t result = custom_eval_string(interface,
					     scratch,
					     length,
					     0);
  switch (result.type) {
  case muesli_value_float:
    return result.data.as_float;
    break;
  default:
    return 0.0;
  }
}

void
custom_setup(evaluator_interface *new_custom_interface)
{
  custom_interface = new_custom_interface;

  custom_interface->eval_string = custom_eval_string;
  custom_interface->eval_0 = custom_eval_0;
  custom_interface->load_file = custom_load_file;

  custom_interface->version = CUSTOM_VERSION;
}

#endif

