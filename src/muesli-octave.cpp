// muesli-octave.cpp -*- C++ -*-
/* Interface to octave evaluators / template for new language interfaces
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

#ifndef _MUESLI_OCTAVE_CPP_
#define _MUESLI_OCTAVE_CPP_

extern "C" {
#include "../config.h"
}

#ifdef HAVE_LIBOCTINTERP

extern "C" {
#include "muesli.h"
}

// We've defined these in our config.h; clear them to stop warnings
// from octave's config.h redefining them:
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "octave/config.h"
#include "octave/octave.h"
#include "octave/symtab.h"
#include "octave/parse.h"
#include "octave/ov-struct.h"
#include "octave/unwind-prot.h"
#include "octave/toplev.h"
#include "octave/error.h" 
#include "octave/quit.h"
#include "octave/variables.h"
#include "octave/sighandlers.h"
#include "octave/sysdep.h"
#include "octave/version.h"
#include "octave/defun.h"
#include "octave/defun-int.h"

using namespace std;

extern "C" {
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>		// todo: do I need this?

#if 0
  void octave_set_parameter();
  void octave_set_parameters();
  void octave_get_parameter();
  void octave_get_parameters();
  void muesli_octave_eval_in_language();
  void octave_eval_custom();
#endif
  void octave_evaluator_init(evaluator_interface *interface);
  void octave_load_file(evaluator_interface *interface,
			const char *filename);
  muesli_value_t octave_eval_string(evaluator_interface *evaluator,
				    const char *octave_c_string,
				    unsigned int string_length,
				    int transient);
  void octave_setup(evaluator_interface *new_octave_interface);
}

// #include "muesli-octave.h"

static int ambient_transient = 0;

typedef struct octave_state {
  char *dummy;
} octave_state;

#ifndef CUSTOMIZED
#if 0
octave_state *octave_create_octave() { return NULL; }

// Getting args from your scripting language stack:
int octave_count_args(octave_state *S) { return 0; }
int octave_number_arg(octave_state *S, int argi) { return 0; }
char *octave_string_arg(octave_state *S, int argi) { return (char*)""; }
int octave_arg_is_string(octave_state *S, int argi) { return 0; }
int octave_arg_is_number(octave_state *S, int argi) { return 0; }

// Putting results back onto your scripting language stack:
void octave_return_string(octave_state *S, const char *string_back) {}
void octave_return_number(octave_state *S, float number_back) {}
void octave_return_boolean(octave_state *S, int int_back) {}
void octave_return_void(octave_state *S) {};

// Handling table / map / alist structures of your scripting language:
octave_table *octave_create_table(octave_state *S, int size) { return NULL; }
void octave_set_table_string(octave_state *S, octave_table *T, char *key, const char *value) {}
void octave_set_table_number(octave_state *S, octave_table *T, char *key, float value) {}
void octave_set_table_boolean(octave_state *S, octave_table *T, char *key, int value) {}

// Make a name / symbol of the scripting language:
octave_symbol *octave_make_symbol(octave_state *S, const char *n) { return NULL; }

// Non-local exits in the scripting language
void octave_throw(octave_state *S, octave_symbol *Y, float n) {}

// Set a global variable of the scripting language to contain a table:
void octave_set_global_variable_table(octave_state *S, octave_symbol *varname, octave_table *table) {}

// Extend the scripting language with an added built-in function:
void octave_register_builtin_function(octave_state *S, const char *funname, void (*function)(octave_state *FS)) {}

// A specialized non-local exit; could either throw, or just quit the
// application run:
static void
octave_report_error(octave_state *S, char *message, char *arg)
{
  fprintf(stderr, "Got error \"%s\", \"%s\"\n", message, arg);
}
#endif
#endif

static evaluator_interface *octave_interface = NULL;

DEFUN(set_parameter, args, , "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} set_parameter(@var{name} @var{value})\n\
Set parameter @var{name} to @var{value}.\n\
@end deftypefn")
{
  octave_value retval;
  int nargin = args.length ();
  if (nargin < 2) {
    fprintf(stderr, "too few args to octave_set_parameter");
    return retval;
  }

  if ((args(0).is_string ()) && ((args(1).is_string ())
				 || (args(1).is_real_scalar ()))) {
    const char *parameter_name = args(0).string_value().c_str();

    char option_code = muesli_find_option_letter(octave_interface->getopt_options, parameter_name);

    if (option_code != -1) {
      if ((args(1).is_string ())) {
	const char *from_octave = args(1).string_value().c_str();
	char *param_str = (char*)malloc(strlen(from_octave));
	strcpy(param_str, from_octave);
	(octave_interface->handle_option)(octave_interface->app_params,
					  option_code, param_str, 0.0, 1,
					  "octave");
      } else if ((args(1).is_real_scalar())) {
	float numeric_value = args(1).scalar_value();
	(octave_interface->handle_option)(octave_interface->app_params,
					  option_code, NULL, numeric_value, 1,
					  "octave");
      } else {
	(octave_interface->handle_option)(octave_interface->app_params,
					  option_code, (char*)"true", 0.0, 1,
					  "octave");
      }
    }
  } else {
    fprintf(stderr, "wrong type args to octave_set_parameter");
    return retval;
  }
  return retval;
}

DEFUN(set_parameters, args, , "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} set_parameters(@var{map})\n\
Set parameters mentioned in @var{map}.\n\
@end deftypefn")
{
  octave_value retval;
  int nargin = args.length ();
  if (nargin < 1) {
    fprintf(stderr, "too few args to octave_get_parameter");
    return retval;
  }

  if (args(0).is_map()) {

    Octave_map table = args(0).map_value();

    string_vector key_list = table.keys ();

    for (octave_idx_type i = 0; i < key_list.length (); i++)
      {
	std::string key = key_list[i];

	const char *parameter_name = key.c_str();

	char option_code = muesli_find_option_letter(octave_interface->getopt_options, parameter_name);

	if (option_code != -1) {

	  Cell val_cell = table.contents(key);

	  const octave_value *ov = val_cell.data ();

	  if (ov->is_string()) {
	    const char *from_octave = ov->string_value().c_str();
	    char *param_str = (char*)malloc(strlen(from_octave));
	    strcpy(param_str, from_octave);
	    (octave_interface->handle_option)(octave_interface->app_params,
					      option_code, param_str, 0.0, 1,
					      "octave");
	  } else if (ov->is_real_scalar()) {
	    float numeric_value = args(1).scalar_value();
	    (octave_interface->handle_option)(octave_interface->app_params,
					      option_code, NULL, numeric_value, 1,
					      "octave");
	  } else {
	    (octave_interface->handle_option)(octave_interface->app_params,
					      option_code, (char*)"true", 0.0, 1,
					      "octave");
	  }
	}
      }

  }  else {
    fprintf(stderr, "wrong type arg to octave_set_parameters");
  }

  return retval;
}

DEFUN(get_parameter, args, , "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} get_parameter(@var{name})\n\
Get parameter @var{name}.\n\
@end deftypefn")
{
  octave_value retval;
  int nargin = args.length ();
  if (nargin < 1) {
    fprintf(stderr, "too few args to octave_get_parameter");
    return retval;
  }

  if (args(0).is_string ()) {
    const char *parameter_name = args(0).string_value().c_str();

    char option_code = muesli_find_option_letter(octave_interface->getopt_options, parameter_name);

    if (option_code != -1) {
      muesli_value_t result = 
	(octave_interface->handle_option)(octave_interface->app_params,
					  option_code,	// option
					  NULL, 0.0,	// value
					  0,	// set
					  "octave");

      switch (result.type) {
      case muesli_value_float:
	retval = octave_value(result.data.as_float);
	break;
      case muesli_value_integer:
	retval = octave_value(result.data.as_int);
	break;
      case muesli_value_string:
	retval = octave_value(result.data.as_string);
	break;
      case muesli_value_boolean:
	retval = octave_value(result.data.as_bool);
	break;
      case muesli_value_none:
      case muesli_value_error:
	// octave_return_void(cs);
	break;
      }
    }
  } else {
    fprintf(stderr, "wrong type arg to octave_get_parameter");
    return retval;
  }
  return retval;    
}

DEFUN(get_parameters, args, , "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} get_parameters()\n\
Return current parameters as a map.\n\
@end deftypefn")
{
  int dims = 0;

  struct option *option_names = octave_interface->getopt_options;
  for (struct option *op = option_names;
       op->name != 0;
       *op++) {
    dims++;
  }

  Octave_map map;

  while (option_names->name != 0) {
    
    muesli_value_t result = (octave_interface->handle_option)(octave_interface->app_params,
							      (option_names->val), // opt
							      NULL, 0.0,		     // value
							      0,		     // set
							      "octave");

    switch (result.type) {
    case muesli_value_string:
      map.assign(option_names->name, octave_value(result.data.as_string));
      break;
    case muesli_value_float:
      map.assign(option_names->name, octave_value(result.data.as_float));
      break;
    case muesli_value_integer:
      map.assign(option_names->name, octave_value(result.data.as_int));
      break;
    case muesli_value_boolean:
      map.assign(option_names->name, octave_value(result.data.as_bool));
      break;
    default:
      map.assign(option_names->name, octave_value(1));
      break;
    }
    option_names++;
  }

  return octave_value(map);
}

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

DEFUN(eval_in_language, args, ,   "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} eval_in_language (@var{language}) @var{fragment}\n\
Evaluate @var{fragment} using the evaluator for @var{language}.\n\
@end deftypefn")
{
  octave_value retval;
  int nargin = args.length ();
  if (nargin < 2) {
    fprintf(stderr, "too few args to octave_eval_in_language\n");
    // octave_error(cs, (char*)"argcount", (char*)"eval_in_language");
    return retval;
  }

  if ((args(0).is_string ()) && (args(1).is_string ())) {
    const char *language_name = args(0).string_value().c_str();
    const char *evaluand = args(1).string_value().c_str();
    unsigned int evaluand_length = strlen(evaluand);

    fprintf(stderr, "In octave_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

    muesli_value_t result = muesli_eval_in_language(language_name,
						    evaluand,
						    evaluand_length,
						    ambient_transient);

    switch (result.type) {
    case muesli_value_float:
      retval = octave_value(result.data.as_float);
      break;
    case muesli_value_integer:
      retval = octave_value(result.data.as_int);
      break;
    case muesli_value_string:
      retval = octave_value(result.data.as_string);
      break;
    case muesli_value_boolean:
      retval = octave_value(result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      // octave_return_void(cs);
      break;
    }
  } else {
    fprintf(stderr, "wrong type args to octave_eval_in_language\n");
    // octave_error(cs, (char*)"argtype", (char*)"eval_in_language");
    return retval;
  }
  return retval;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

DEFUN(custom, args, ,   "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} custom (@var{x})\n\
Evaluate @var{x} using the custom evaluator.\n\
@end deftypefn")
{
  octave_value retval;
  int nargin = args.length ();

  if ((nargin >= 1) && (args(0).is_string ())) {

    evaluator_interface *custom_interface = muesli_get_named_evaluator("custom", 1);

    const char *string_arg = args(0).string_value().c_str();
    muesli_value_t result = (custom_interface->eval_string)(custom_interface,
							    string_arg, strlen(string_arg),
							    ambient_transient);
    switch (result.type) {
    case muesli_value_float:
      retval = octave_value(result.data.as_float);
      break;
    case muesli_value_integer:
      retval = octave_value(result.data.as_int);
      break;
    case muesli_value_string:
      retval = octave_value(result.data.as_string);
      break;
    case muesli_value_boolean:
      retval = octave_value(result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      // octave_return_void(cs);
      break;
    }
  } else {
    fprintf(stderr, "octave_eval_function must be given a string\n");
    // octave_error(cs, (char*)"argtype", (char*)"octave_eval");
  }

  return retval;
}

////////////////
// Initialize //
////////////////
#define XDEFUN_INTERNAL(name, args_name, nargout_name, is_text_fcn, doc) \
  extern DECLARE_FUN (name, args_name, nargout_name); \
  install_builtin_function (F ## name, #name, doc, is_text_fcn); \

void
octave_evaluator_init(evaluator_interface *interface)
{
  // Init octave interface

  char *oargv[3];
  int oargc = 2;

  oargv[0] = (char*)"libmuesli";
  oargv[1] = (char*)"--silent";
  oargv[2] = NULL;

  octave_state *our_octave_state = NULL;

  octave_main(oargc,oargv,1);

  interface->state = our_octave_state;
  octave_interface = interface;

  // Extend system

  // Fill in: register all these language extensions (as applicable):
  // Muesli_Add_Fn_1(interface, (char*)"get_parameter", octave_get_parameter);
  // Muesli_Add_Fn_2(interface, (char*)"set_parameter", octave_set_parameter);
  // Muesli_Add_Fn_0(interface, (char*)"evolution_parameters", octave_get_parameters);
  // Muesli_Add_Fn_1(interface, (char*)"modify_evolution_parameters", octave_set_parameters);

  XDEFUN_INTERNAL (custom, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} custom (@var{x})\n\
Evaluate @var{x} using the custom evaluator.\n\
@end deftypefn");
  XDEFUN_INTERNAL (eval_in_language, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} eval_in_language (@var{language}) @var{fragment}\n\
Evaluate @var{fragment} using the evaluator for @var{language}.\n\
@end deftypefn");
  XDEFUN_INTERNAL(set_parameter, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} set_parameter(@var{name} @var{value})\n\
Set parameter @var{name} to @var{value}.\n\
@end deftypefn");
  XDEFUN_INTERNAL(get_parameter, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} get_parameter(@var{name})\n\
Get parameter @var{name}.\n\
@end deftypefn");
  XDEFUN_INTERNAL(set_parameters, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} set_parameters(@var{map})\n\
Set parameters mentioned in @var{map}.\n\
@end deftypefn");
  XDEFUN_INTERNAL(get_parameters, args, , false, "-*- texinfo -*-\n\
@deftypefn {Extended Built-in Function} {} get_parameters()\n\
Return current parameters as a map.\n\
@end deftypefn");
  
  // Set up option names
#if 0
  // Fill in: Create a table of option names.  You may well not need to bother.
  octave_table *option_names_table = octave_create_table(our_octave_state, 48);
  struct option *option_names;
  for (option_names = octave_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    octave_set_table_number(our_octave_state, option_names_table,
			    (char*)(option_names->name), (option_names->val));
  }
  octave_set_global_variable_table(our_octave_state,
				   octave_make_symbol(our_octave_state,
						      option_names_var_name),
				   option_names_table);
#endif
}

void
octave_load_file(evaluator_interface *interface,
		const char *filename)
{
  int muesli_flags = interface->flags;
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }

  source_file(filename, "base");

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

muesli_value_t
octave_eval_string(evaluator_interface *evaluator,
		   const char *octave_c_string,
		   unsigned int string_length,
		   int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (octave_c_string) {


    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;
    // fprintf(stderr, "Octave evaluating string \"%s\"\n", octave_c_string);

    octave_value_list raw_results;

    // Modified slightly from example code on the Octave web site,
    // which in turn is derived from toplev.cc which is part of Octave
    // (GPL3):

    int parse_status;

    octave_save_signal_mask ();
    if (octave_set_current_context) {
#if defined (USE_EXCEPTIONS_FOR_INTERRUPTS)
	panic_impossible ();
#else
	unwind_protect::run_all ();
	raw_mode (0);
	std::cout << "\n";
	octave_restore_signal_mask ();
#endif
      }

    can_interrupt = true;
    octave_catch_interrupts ();
    octave_initialized = true;

    // XXX FIXME XXX need to give caller an opaque pointer
    // so that they can define and use separate namespaces
    // when calling octave, with a shared global namespace.
    // Something like:
    //   int call_octave (const char *string, void *psymtab = NULL) {
    //     ...
    //     curr_sym_tab = psymtab == NULL ? top_level_sym_tab : symbol_table;
    // I suppose to be safe from callbacks (surely we have to
    // provide some way to call back from embedded octave into
    // the user's application), we should push and pop the current
    // symbol table.

    // Note that I'm trying to distinguish exception from 
    // failure in the return codes. I believe failure is 
    // indicated by -1.  I have execution exception (including
    // user interrupt and more dramatic failures) returning -2
    // and memory failure returning -3.  We should formalize
    // this with error codes defined in embed_octave.h.  Maybe
    // a more fine-grained approach could be used within octave
    // proper.
    try {
	curr_sym_tab = top_level_sym_tab;
	reset_error_handler ();
	raw_results = eval_string(octave_c_string, false, parse_status);
      }
    catch (octave_interrupt_exception) {
#if 0
    // todo: re-instate this block and find how to get the definition of recover_from_exception, which is in toplev.h which we already include
	recover_from_exception ();
#endif
	std::cout << "\n"; 
	error_state = -2; 
      }
    catch (std::bad_alloc) {
#if 0
    // todo: re-instate this block and find how to get the definition of recover_from_exception, which is in toplev.h which we already include
	recover_from_exception ();
#endif
	std::cout << "\n"; 
	error_state = -3;
      }
    octave_restore_signal_mask();
    octave_initialized = false;

    if (raw_results.empty()) {
      result.type = muesli_value_none;
    } else {

      octave_value res = raw_results(0);


      if (res.is_undefined() || res.is_empty()) {
	result.type = muesli_value_none;
      } else if (res.is_real_scalar()) {
	result.data.as_float = res.scalar_value(); 
	result.type = muesli_value_float;
      } else if (res.is_bool_scalar()) {
	result.data.as_bool = res.bool_value();
	result.type = muesli_value_boolean;
      } else if (res.is_string()) {
	result.data.as_string = res.string_value().c_str();
	result.type = muesli_value_string;
      } else {
	result.type = muesli_value_none;
      }




    
    }

    // delete raw_results;


    ambient_transient = old_ambient_transient;
  }

  return result;
}

void
octave_setup(evaluator_interface *new_octave_interface)
{
  octave_interface = new_octave_interface;

  octave_interface->eval_string = octave_eval_string;
  octave_interface->load_file = octave_load_file;
  // octave_interface->add_function = octave_add_function;

  octave_interface->version = (char*)OCTAVE_VERSION;
}

#endif
#endif

