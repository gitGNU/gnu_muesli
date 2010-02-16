// muesli-siod.c -*- C -*-
/* muesli interface to SIOD
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

#ifndef _MUESLI_ULSIOD_
#define _MUESLI_ULSIOD_

#include "../config.h"

#ifdef HAVE_LIBULSIOD

#include "muesli.h"
// #include "lang-extns.h"

#include <limits.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

// #include "lang-extns-cpp.h"

#include "siod/ulsiod.h"

// Forward declaration (to keep normal order of functions but make
// this available to traced file loading):
static muesli_value_t
muesli_siod_eval_string(evaluator_interface *interface,
			const char *scratch,
			unsigned int string_length,
			int transient);

// Interface from the rest of Muesli, set each time Muesli calls us
static evaluator_interface *current_interface = NULL;

static int ambient_transient = 0;

static siod_LISP option_names_var = NIL;
static siod_LISP option_names_table = NIL;

#define NUMBER_BUF_SIZE 64
static char number_buf[NUMBER_BUF_SIZE];

static siod_LISP
muesli_siod_set_parameter(siod_LISP option, siod_LISP value)
{
  int ok = 0;

  if ((TYPEP(value, tc_string)) || (TYPEP(value, tc_symbol))) {
    // note that `nil' comes through as the symbol named "nil"
    ok = muesli_set_parameter(current_interface, current_interface->getopt_options, get_c_string(option), get_c_string(value), (const char*)"siod");
  } else if (TYPEP(value, tc_flonum)) {
    snprintf(number_buf, NUMBER_BUF_SIZE, "%f", FLONM(value));
    ok = muesli_set_parameter(current_interface, current_interface->getopt_options, get_c_string(option), number_buf, (const char*)"siod");
  } else {
    ok = muesli_set_parameter(current_interface, current_interface->getopt_options, get_c_string(option), (char*)"true", (const char*)"siod");
  }
  if (ok) {
    return sym_t;
  } else {
    return NIL;
  }

}

static siod_LISP
muesli_siod_set_parameters(siod_LISP changes)
{
  while (changes != NIL) {
    siod_LISP change = siod_car(changes);
    if (CONSP(change)) {
      muesli_siod_set_parameter(siod_car(change), siod_cdr(change));
    }
    changes = siod_cdr(changes);
  }
  return NIL;
}

static siod_LISP
muesli_siod_get_parameter(siod_LISP option)
{
  char option_code = muesli_find_option_letter(current_interface->getopt_options, get_c_string(option));

  if (option_code != -1) {
    muesli_value_t result = (current_interface->handle_option)(current_interface->app_params,
							       option_code,	// option
							       NULL, 0.0,	// value
							       0,	// set
							       (const char *)"siod");

    switch (result.type) {
    case muesli_value_string:
      return siod_strcons(strlen(result.data.as_string), result.data.as_string);
    case muesli_value_float:
      return siod_flocons(result.data.as_float);
    case muesli_value_integer:
      return siod_flocons((float)result.data.as_int);
    case muesli_value_boolean:
      if (result.data.as_int) {
	return sym_t;
      } else {
	return NIL;
      }
    default:
      return NIL;
    }
  } else {
    return NIL;
  }
}

static siod_LISP
muesli_siod_get_parameters()
{
  siod_LISP names = option_names_table;
  siod_LISP results = NIL;

  while (names != NIL) {
    if (CONSP(names)) {
      siod_LISP pair = siod_car(names);
      siod_LISP lisp_result;

muesli_value_t result = (current_interface->handle_option)(current_interface->app_params,
							   (int)(FLONM(siod_cdr(pair))), // opt
							   NULL, 0.0,		     // value
							   0,		     // set
							   (const char *)"siod");

      switch (result.type) {
      case muesli_value_string:
	if (result.data.as_string != NULL) {
	  lisp_result = siod_strcons(strlen(result.data.as_string), result.data.as_string);
	} else {
	  lisp_result = NIL;
	}
	break;
      case muesli_value_float:
	lisp_result = siod_flocons(result.data.as_float);
	break;
      case muesli_value_integer:
	lisp_result = siod_flocons((float)result.data.as_int);
	break;
      case muesli_value_boolean:
	if (result.data.as_int) {
	  lisp_result = sym_t;
	} else {
	  lisp_result = NIL;
	}
	break;
      default:
	lisp_result = NIL;
	break;
      }
      if (lisp_result != NIL) {
	results = siod_cons(siod_cons(siod_car(pair), lisp_result), results);
      }
    }
    names = siod_cdr(names);
  }
  return results;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives SIOD access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a SIOD program.

static siod_LISP
muesli_siod_custom_eval_function(siod_LISP eval_in)
{
  if ((TYPEP(eval_in, tc_string)) || (TYPEP(eval_in, tc_symbol))) {
    evaluator_interface *custom_evaluator = muesli_get_named_evaluator("custom", 0);
    if (custom_evaluator == NULL) {
      fprintf(stderr, "custom evaluator not found\n");
      return eval_in;
    } else {
      char *str = get_c_string(eval_in);
      muesli_value_t result = (custom_evaluator->eval_string)(custom_evaluator,
							      str, strlen(str),
							      ambient_transient);
      switch (result.type) {
      case muesli_value_float:
	return siod_flocons(result.data.as_float);
      case muesli_value_integer:
	return siod_flocons(result.data.as_int);
      case muesli_value_string:
	return siod_strcons(strlen(result.data.as_string), result.data.as_string);
      case muesli_value_boolean:
	if (result.data.as_int) {
	  return sym_t;
	} else {
	  return NIL;
	}
      case muesli_value_none:
	return NIL;
      case muesli_value_error:
	// todo: should probably throw something
	return NIL;
      }
    }
  } else {
#if 0
    fprintf(stderr, "muesli_siod_custom_eval_function must be given a string\n");
    exit(EXIT_USER_ERROR);
#else
    return eval_in;
#endif
  }
  return eval_in;
}

///////////////////////////////////////////
// Call stack-based bytecode interpreter //
///////////////////////////////////////////

static siod_LISP
muesli_siod_eval_bytecode_string(siod_LISP args)
{
  siod_LISP program = siod_car(args);
  char *str = get_c_string(program);
  siod_LISP l;
  evaluator_interface *stack_evaluator = muesli_get_named_evaluator("stack-code", 0);

  if (stack_evaluator == NULL) {
    fprintf(stderr, "stack evaluator not found\n");
    return NIL;
  } else {
    (stack_evaluator->eval_clear)(stack_evaluator);

    for (l = siod_cdr(args); NNULLP(l); l = siod_cdr(l)) {
      siod_LISP s = siod_car(l);
      if (FLONUMP(s)) {
	(stack_evaluator->eval_give)(stack_evaluator, FLONM(s));
      } else {
	(stack_evaluator->eval_give)(stack_evaluator, 0.0);
      }
    }

    return siod_flocons((stack_evaluator->eval_given)(stack_evaluator, str, strlen(str)));
  }
}

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static siod_LISP
muesli_siod_eval_in_language(siod_LISP language_name, siod_LISP evaluand)
{
  if (((TYPEP(language_name, tc_string)) || (TYPEP(language_name, tc_symbol)))
      && ((TYPEP(evaluand, tc_string)) || (TYPEP(evaluand, tc_symbol)))) {
    const char *language = get_c_string(language_name);
    const char *scratch = get_c_string(evaluand);
    unsigned int string_length = strlen(scratch);

    // fprintf(stderr, "In muesli_siod_eval_in_language(\"%s\", \"%s\")\n", language, scratch);

    muesli_value_t result = muesli_eval_in_language(language,
						    scratch,
						    string_length,
						    ambient_transient);

    switch (result.type) {
    case muesli_value_none:
      return NIL;
    case muesli_value_float:
      return siod_flocons(result.data.as_float);
    case muesli_value_integer:
      return siod_flocons(result.data.as_int);
    case muesli_value_string:
      return siod_strcons(strlen(result.data.as_string), result.data.as_string);
    case muesli_value_boolean:
      if (result.data.as_int) {
	return sym_t;
      } else {
	return NIL;
      }
    case muesli_value_error:
      // todo: should probably throw
      return NIL;
    }   
  } else {
#if 0
    // todo: make this a throw, or something like that
    fprintf(stderr, "Arguments to muesli_siod_eval_in_language must be strings\n");
    exit(EXIT_USER_ERROR);
    return NIL;
#else
    return evaluand;
#endif
  }
  return NIL;
}

////////////////
// Initialize //
////////////////

static int lisp_buffer_length = 4096;
static char *lisp_buffer = NULL;

void
siod_evaluator_init(evaluator_interface *interface)
{
  int muesli_flags = interface->flags;
  struct option *option_names;
  int iarg = 0;

  int each_heap_size = 100000;
  int n_heaps = 10;
  // If using stop_and_copy, you can only GC at top level of the REPL.
  // This isn't usually good.  The documentation says that it is more
  // portable.
  int stop_and_copy = 0;

  if (muesli_flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "Initializing ULSIOD\n");
  }

  lisp_buffer = (char*)malloc(lisp_buffer_length);

  snprintf(lisp_buffer, lisp_buffer_length, "-h%d:%d", each_heap_size, n_heaps);

  current_interface = interface;

  // Init siod interface
  char *sargv[4];
  sargv[iarg++] = (char*)"siod";
  sargv[iarg++] = (char*)"-v0";
  if (stop_and_copy) {
    sargv[iarg++] = (char*)"-g1";
  } else {
    sargv[iarg++] = (char*)"-g0";
  }
  sargv[iarg++] = lisp_buffer;

  ulsiod_init(iarg, sargv);

  // set up option names

  option_names_var = siod_cintern((char*)"option-names");

  siod_setvar(option_names_var, NIL, NIL);

  // general functions:
  Muesli_Add_Fn_1(current_interface, (char*)"get-parameter", muesli_siod_get_parameter);
  Muesli_Add_Fn_2(current_interface, (char*)"set-parameter", muesli_siod_set_parameter);
  Muesli_Add_Fn_0(current_interface, (char*)"parameters", muesli_siod_get_parameters);
  Muesli_Add_Fn_1(current_interface, (char*)"modify-parameters", muesli_siod_set_parameters);
  Muesli_Add_Fn_1(current_interface, (char*)"custom", muesli_siod_custom_eval_function);
  Muesli_Add_Fn_V(current_interface, (char*)"bytecode", muesli_siod_eval_bytecode_string);
  Muesli_Add_Fn_2(current_interface, (char*)"eval-in-language", muesli_siod_eval_in_language);

  for (option_names = current_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    option_names_table = siod_cons(siod_cons(siod_cintern((char*)(option_names->name)),
					     siod_flocons(option_names->val)),
				   option_names_table);
  }
  siod_setvar(option_names_var, option_names_table, NIL);

  siod_verbose(siod_cons(siod_flocons(interface->language_verbosity),
			 NIL));

  if (muesli_flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "Initialized ULSIOD\n");
  }
}

////////////
// Extend //
////////////

static void
siod_add_function(evaluator_interface *interface,
		  int arity, const char *name,
		  void *function)
{
  switch (arity) {
  case ARGS_COUNT_VARIABLE:
    siod_init_lsubr((char*)name, function);
    break;
  case ARGS_NO_EVAL:
    siod_init_fsubr((char*)name, function);
    break;
  case 0:
    siod_init_subr_0((char*)name, function);
    break;
  case 1:
    siod_init_subr_1((char*)name, function);
    break;
  case 2:
    siod_init_subr_2((char*)name, function);
    break;
  case 3:
    siod_init_subr_3((char*)name, function);
    break;
  }
}

static void
siod_muesli_repl(evaluator_interface *interface)
{
  // todo: fix the streams this uses
  siod_repl_driver(1,1,NULL);
}

static void
siod_load_file(evaluator_interface *interface,
	       const char *filename)
{
  int muesli_flags = interface->flags;

  current_interface = interface;

  
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
#if 1
#define LOAD_FORMAT "(*catch 'errobj (load \"%s\"))"
  int load_buf_len = strlen(filename) + strlen(LOAD_FORMAT);
  char *load_buf = (char*)alloca(load_buf_len);
  snprintf(load_buf, load_buf_len, LOAD_FORMAT, filename);
  // fprintf(stderr, "evaluating %s\n", load_buf);
  muesli_value_t result = muesli_siod_eval_string(interface,
						  load_buf, strlen(load_buf),
						  0);

  switch (result.type) {
  case muesli_value_string:
    fprintf(stderr, "Got back string %s\n", result.data.as_string);
    break;
  default:
    break;
  }
#else
  // todo: protect this with a catch frame
  fprintf(stderr, "Calling (siod_vload \"%s\") from ULSIOD\n", filename);
  siod_vload(filename, 0, 1);
#endif
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
  
}

////////////////
// Evaluation //
////////////////

// Currently experimenting: I'd like to use the direct eval, and it
// works a bit... but it all goes horribly wrong at garbage
// collection.  However, the indirect eval (which is slower; it's the
// old eval_individual code) does the garbage collection for us, as it
// uses the REPL which included that.

// #define DIRECT_EVAL 1

#if 0
extern int trace_this_eval_siod;
int trace_this_eval_siod = 0;
#endif

static muesli_value_t
muesli_siod_eval_string(evaluator_interface *interface,
			const char *scratch,
			unsigned int string_length,
			int transient)
{
  int old_ambient_transient = ambient_transient;
  ambient_transient = transient;
  current_interface = interface;
  int verbosity = interface->language_verbosity;

  muesli_value_t result;
  ANULL_VALUE(result);

#if 0
  if (trace_this_eval_siod) {
    fprintf(stderr, "muesli_siod_eval_string got %p = \"%.*s\"\n", scratch, string_length, scratch);
    if (!isascii(scratch[0])) {
      fprintf(stderr, "Bad string?\n");
    }
  }
#endif

#ifdef DIRECT_EVAL

  if (verbosity > standard) {
    cout << "SIOD eval form: " << scratch << "\n";
  }

#if 0
  siod_gc_stop_and_copy();
#endif

  // Construct a lisp reader that will read from the scratch string
  const char *p;
  struct gen_readio s;
  p = scratch;
  s.getc_fcn = (int (*)(void *))siod_rfs_getc;
  s.ungetc_fcn = (void (*)(int,void *))siod_rfs_ungetc;
  s.cb_argument = (char *) &p;

  // Use the reader we have made to make a Lisp value from the string
  siod_LISP read_result = (siod_readtl(&s));

  // Evaluate that Lisp value
  // possible todo: wrap this in a "catch frame" (see leval_catch, leval_catch_1 in siod/slib.c)
  siod_LISP eval_result = siod_leval(read_result, NIL);

  ambient_transient = old_ambient_transient;

  switch (TYPE(eval_result)) {
  case tc_string:
    result.data.as_string = get_c_string(eval_result);
    result.type = muesli_value_string;
    break;
  case tc_nil:
    result.data.as_int = 0;
    result.type = muesli_value_boolean;
    break;
  case tc_symbol:
    result.data.as_int = 1;
    result.type = muesli_value_boolean;
    break;
  case tc_flonum:
    result.data.as_float = FLONM(eval_result);
    result.type = muesli_value_float;
    break;
  }

#else

  int in_len = strlen(scratch);

  current_interface = interface;

  if ((in_len + 1) > lisp_buffer_length) {
    lisp_buffer_length = in_len + 1024;

    free(lisp_buffer);

    lisp_buffer = (char*)malloc(lisp_buffer_length);
  }

  strcpy(lisp_buffer, scratch);

  if (verbosity > standard) {
    fprintf(stderr, "ULSIOD form: %s\n", scratch);
  }

  // fprintf(stderr, "SIOD form: %s\n", scratch);

  // todo: use `siod_leval' directly, or possibly via `leval_catch', as in muesli_siod_eval_string
  if (siod_repl_c_string(lisp_buffer, 1, 0, lisp_buffer_length - 1)) {
    // Siod error processing text in buffer
    fprintf(stderr, "ULSIOD error in: %s\n", lisp_buffer);
#if 1
    printf("Errobj: ");
    siod_lprint(siod_leval(sym_errobj, NIL), NIL);
    printf("End of ULSIOD error output\n");
#else
    strcpy(lisp_buffer, (char*)"(writes nil errojb)\n");
    siod_repl_c_string(lisp_buffer, 1, 0, strlen(lisp_buffer));
#endif
    ambient_transient = old_ambient_transient;

    result.type = muesli_value_error;
  } else {
    ambient_transient = old_ambient_transient;
    if (verbosity > standard) {
      fprintf(stderr, "SIOD result: %s\n", lisp_buffer);
    }

    if (isdigit(lisp_buffer[0]) || (lisp_buffer[0] == '-')) {
      result.data.as_float = atof(lisp_buffer);
      result.type = muesli_value_float;
    } else if (lisp_buffer[0] == '\"') {
      result.data.as_string = lisp_buffer;
      // todo: not really right, as it's got the quotes on it
      result.type = muesli_value_string;
    } else if (strcmp(lisp_buffer, "t") == 0) {
      result.data.as_int = 1;
      result.type = muesli_value_boolean;
    } else if (strcmp(lisp_buffer, "nil") == 0) {
      result.data.as_int = 0;
      result.type = muesli_value_boolean;
    } else {
      result.type = muesli_value_none;
    }

  }

#endif

  return result;
}

void
siod_setup(evaluator_interface *new_siod_interface)
{
  current_interface = new_siod_interface;

  current_interface->eval_string = muesli_siod_eval_string;
  current_interface->add_function = siod_add_function;
  current_interface->load_file = siod_load_file;
  current_interface->repl = siod_muesli_repl;
  current_interface->array_separator = (char*)" ";

  current_interface->version = siod_version();
}

#endif
#endif
