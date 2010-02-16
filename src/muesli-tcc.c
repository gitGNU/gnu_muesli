// muesli-tcc.c -*- C -*-
/* muesli interface to tcc (Tiny C Compiler)
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

#ifndef _MUESLI_TCC_
#define _MUESLI_TCC_

#include "../config.h"

#ifdef HAVE_LIBTCC
// check with whether we're going to clash with SIOD over the symbol "load"
#if defined(HAVE_LIBSIOD) && !defined(HAVE_LIBULSIOD)
#error This combination will not work, as SIOD and TCC clash over the symbol "load".
#else

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <libtcc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "muesli.h"

#include "muesli-tcc.h"


// Interface from the rest of Muesli, set each time Muesli calls us
static evaluator_interface *tcc_interface = NULL;

TCCState *tcc_global_state = NULL;

// Getting args from your scripting language stack:
int tcc_count_args(TCCState *S) { return 0; }
int tcc_number_arg(TCCState *S, int argi) { return 0; }
char *tcc_string_arg(TCCState *S, int argi) { return (char*)""; }
int tcc_arg_is_string(TCCState *S, int argi) { return 0; }
int tcc_arg_is_number(TCCState *S, int argi) { return 0; }

// Putting results back onto your scripting language stack:
void tcc_return_string(TCCState *S, const char *string_back) {}
void tcc_return_number(TCCState *S, float number_back) {}
void tcc_return_boolean(TCCState *S, int int_back) {}

// Handling table / map / alist structures of your scripting language:
tcc_table *tcc_create_table(TCCState *S, int size) { return NULL; }
void tcc_set_table_string(TCCState *S, tcc_table *T, char *key, const char *value) {}
void tcc_set_table_number(TCCState *S, tcc_table *T, char *key, float value) {}
void tcc_set_table_boolean(TCCState *S, tcc_table *T, char *key, int value) {}

// Make a name / symbol of the scripting language:
tcc_symbol *tcc_make_symbol(TCCState *S, const char *n) { return NULL; }

// Non-local exit in the scripting language
void tcc_throw(TCCState *S, tcc_symbol *Y, float n) {}

// Set a global variable of the scripting language to contain a table:
void tcc_set_global_variable_table(TCCState *S, tcc_symbol *varname, tcc_table *table) {}

static char *preloaded_functions = NULL;

static void
muesli_tcc_error(char *message)
{
  fprintf(stderr, "muesli_tcc_error %s\n", message);
  exit(1);
}

// Pointers to the live data in main.cpp
static struct option *static_option_names = NULL;
static char *option_names_var_name = (char*)"option_names";

static int ambient_transient = 0;

static void
set_parameter_tcc(TCCState *cs)
{
  if (tcc_count_args(cs)) {
    muesli_tcc_error((char*)"too few args to set_parameter_tcc\n");
  }

  char option_code = muesli_find_option_letter(tcc_interface->getopt_options, tcc_string_arg(cs, 0));

  if (option_code != -1) {

    if (tcc_arg_is_string(cs, 1)) {
      (tcc_interface->handle_option)(tcc_interface->app_params,
				     option_code,
				     muesli_malloc_copy_string(tcc_string_arg(cs, 1)), 0.0,
				     1,
				     (char *)"tcc");
    } else if (tcc_arg_is_number(cs, 1)) {
      (tcc_interface->handle_option)(tcc_interface->app_params,
				     option_code,
				     NULL, tcc_number_arg(cs, 2),
				     1,
				     (char *)"tcc");
    } else {
      (tcc_interface->handle_option)(tcc_interface->app_params,
				     option_code,
				     (char*)"true", 0.0,
				     1,
				     (char *)"tcc");
    }
  }
}

static void
set_parameters_tcc(TCCState *cs)
{
  if (tcc_count_args(cs) < 1)  {
    muesli_tcc_error((char*)"too few args to set_parameters_tcc\n");
  }

#if 0
  // Fill in: use a table iterator from your language
  tcc_table table = tcc_table_arg(cs, 0);

  tcc_table_iteration_start(cs, table);
  while (tcc_table_iteration_next(cs, table) != 0) {
    set_parameter_tcc(cs,
		      tcc_table_iteration_current_key(cs, table),
		      tcc_table_iteration_current_value(cs, table));
  }

#endif
}

static void
get_parameter_tcc(TCCState *cs)
{
  if (tcc_count_args(cs) < 1)  {
    muesli_tcc_error((char*)"too few args to get_parameter_tcc\n");
  }

  char option_code = muesli_find_option_letter(tcc_interface->getopt_options, tcc_string_arg(cs, 0));

  if (option_code != -1) {

    muesli_value_t result = (tcc_interface->handle_option)(tcc_interface->app_params,
							   option_code,	// option
							   NULL, 0.0,	// value
							   0,	// set
							   (char *)"tcc");

    switch (result.type) {
    case muesli_value_string:
      tcc_return_string(cs, result.data.as_string);
      break;
    case muesli_value_float:
      tcc_return_number(cs, result.data.as_float);
      break;
    case muesli_value_boolean:
      tcc_return_boolean(cs, result.data.as_int);
      break;
    default:
      tcc_return_boolean(cs, 0);
      break;
    }
  }
}

static void
get_parameters_tcc(TCCState *cs)
{
  tcc_table *table = tcc_create_table(cs, 48);

  struct option *option_names = static_option_names;

  while (option_names->name != 0) {
    
    muesli_value_t result = (tcc_interface->handle_option)(tcc_interface->app_params,
							   (option_names->val), // opt
							   NULL, 0.0,		     // value
							   0,		     // set
							   (char *)"tcc");

    switch (result.type) {
    case muesli_value_string:
      tcc_set_table_string(cs, table, (char*)(option_names->name), result.data.as_string);
      break;
    case muesli_value_float:
      tcc_set_table_number(cs, table, (char*)(option_names->name), result.data.as_float);
      break;
    case muesli_value_boolean:
      tcc_set_table_boolean(cs, table, (char*)(option_names->name), result.data.as_int);
      break;
    default:
      tcc_set_table_boolean(cs, table, (char*)(option_names->name), 0);
      break;
    }
    option_names++;
  }

}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives tcc access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a tcc muesli program.

static muesli_value_t
tcc_custom_eval_function(TCCState *cs)
{
  if (tcc_arg_is_string(cs, 0)) {
    evaluator_interface *custom_evaluator = muesli_get_named_evaluator("custom", 0);
    if (custom_evaluator == NULL) {
    muesli_tcc_error((char*)"custom evaluator not found\n");    }
    char *str = (char*)"";		// todo: fix this
    return ((custom_evaluator->eval_string)(custom_evaluator,
					    str, strlen(str), ambient_transient));
  } else {
    muesli_tcc_error((char*)"tcc_custom_eval_function must be given a string\n");
    muesli_value_t result;
    result.type = muesli_value_none;
    return result;
  }
}

////////////////
// Initialize //
////////////////

static int tcc_evaluator_initialized = 0;

void
tcc_evaluator_init(evaluator_interface *interface)
{
  if (interface->flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "Initializing tcc interface\n");
  }
  if (!tcc_evaluator_initialized) {
    tcc_evaluator_initialized = 1;
    // todo: complete this
  }
  muesli_tcc_error((char*)"tcc embedding not complete!\n");
}

static void
begin_tcc_instance()
{
  if (tcc_global_state != NULL) {
    muesli_tcc_error((char*)"TCC tangle\n");
  }
  // Init tcc interface

  // Fill in: initialize your tcc application
  tcc_global_state = tcc_new();

  if (tcc_global_state == NULL) {
    muesli_tcc_error((char*)"Could not initialize TCC system\n");
  }
  tcc_set_output_type(tcc_global_state, TCC_OUTPUT_MEMORY);

  // Extend system
#if 0
  // Fill in: register all these language extensions (as applicable),
  // and think about: how do I fit them in with the init/specialize
  // pair?:
  tcc_add_symbol(tcc_global_state, (char*)"get_parameter", get_parameter_tcc);
  tcc_add_symbol(tcc_global_state, (char*)"set_parameter", set_parameter_tcc);
  tcc_add_symbol(tcc_global_state, (char*)"evolution_parameters", get_parameters_tcc);
  tcc_add_symbol(tcc_global_state, (char*)"modify_evolution_parameters", set_parameters_tcc);
  tcc_add_symbol(tcc_global_state, (char*)"custom", tcc_custom_eval_function);
#endif
  // Set up option names
#if 0
  // Fill in: Create a table of option names.  You may well not need to bother.
  tcc_table *option_names_table = tcc_create_table(tcc_global_state, 48);
  for (struct option *option_names = long_options;
       option_names->name != 0;
       option_names++) {
    tcc_set_table_number(tcc_global_state, option_names_table,
			 (char*)(option_names->name), (option_names->val));
  }
  tcc_set_global_variable_table(tcc_global_state,
				tcc_make_symbol(tcc_global_state,
						option_names_var_name),
				option_names_table);
#endif
}

static void
end_tcc_instance()
{
  tcc_delete(tcc_global_state);
  tcc_global_state = NULL;
}

static void
tcc_load_file(evaluator_interface *interface,
	      const char *filename)
{
  tcc_evaluator_init(interface);
  
  int total_size = 1;
  char *place;

  struct stat stat_buf;

  int (*func)();
  unsigned long val;
  
  if (stat(filename, &stat_buf) != 0) {
    // problem -- todo: handle this
  }
  total_size += stat_buf.st_size;

  place = preloaded_functions = (char*)malloc(total_size);
  
  if (stat(filename, &stat_buf) != 0) {
    // problem -- todo: handle this
  }
  int size = stat_buf.st_size;
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    // problem -- todo: handle this
  }
  read(fd, place, size);
  close(fd);
  place += size;
  
  *place = '\0';

#if 0
  // todo: re-write this somehow, so as not to use grevo's params
  if (static_params->initializer) {
    char *init_name = (char*)"internal_initializer";
    char *init_format = (char*)"void %s { %s }";
    int init_size = strlen(init_format) + strlen(init_name) + strlen(static_params->initializer);
    char *init_string = new char[init_size];
    sprintf(init_string, init_format, init_name, static_params->initializer);

    begin_tcc_instance();
    tcc_compile_string(tcc_global_state, preloaded_functions);
    tcc_compile_string(tcc_global_state, init_string);

    tcc_relocate(tcc_global_state);

    tcc_get_symbol(tcc_global_state, &val, init_name);
    func = (int (*)()) val;

    func();
    end_tcc_instance();
  }
#endif
}

static muesli_value_t
tcc_eval_string(evaluator_interface *interface,
		const char *tcc_c_string,
		unsigned int string_length,
		int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (tcc_c_string) {
    int old_transient = ambient_transient;
    ambient_transient = transient;
    // fprintf(stderr, "Tcc evaluating string \"%s\"\n", tcc_c_string);
    int (*func)();
    unsigned long val;



    // todo: make this wrap the string in a function, like I've already done for the initializer; document accordingly





    char *name_end = strchr(tcc_c_string, '(');
    int name_length = 0;
    char *name_buf = NULL;
    if (name_end != NULL) {
      name_length = name_end - tcc_c_string;
    } else {
      muesli_tcc_error((char*)"TCC interface needs evaluated strings to look like single function calls, e.g. \"foo(42)\"\n");
    }
    name_buf = (char*)malloc(name_length + 1);
    strncpy(name_buf, tcc_c_string, name_length);
    name_buf[name_length] = '\0';

    tcc_evaluator_init(interface);
    begin_tcc_instance();

    if (preloaded_functions != NULL) {
      tcc_compile_string(tcc_global_state, preloaded_functions);
    }
    tcc_compile_string(tcc_global_state, tcc_c_string);

    tcc_relocate(tcc_global_state);

    tcc_get_symbol(tcc_global_state, &val, name_buf);
    func = (int (*)())val;

    float muesli_value_float = func();

    result.data.as_float = muesli_value_float;
    result.type = muesli_value_float;

    free(name_buf);

    // Fill in: evaluate tcc_c_string, decide what kind of result it
    // returns, and fill in the result and its type accordingly.
    ambient_transient = old_transient;
  }
  end_tcc_instance();

  return result;		// todo: get result type
}

void
tcc_setup(evaluator_interface *new_tcc_interface)
{
  tcc_interface = new_tcc_interface;

  tcc_interface->eval_string = tcc_eval_string;
  tcc_interface->load_file = tcc_load_file;
}

#endif
#endif
#endif

