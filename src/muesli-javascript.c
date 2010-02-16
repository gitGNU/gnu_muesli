// muesli-javascript.c -*- C -*-
/* Interface to javascript evaluators / template for new language interfaces
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

#ifndef _MUESLI_JAVASCRIPT_
#define _MUESLI_JAVASCRIPT_

#include "../config.h"
#include "muesli.h"

#ifdef HAVE_LIBMOZJS

#include <js/jsapi.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>		// todo: do I need this?

static int ambient_transient = 0;

JSClass global_class = {
        "global",0,
        JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
        JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
    };

typedef struct javascript_state {
  JSRuntime *rt;
  JSContext *cx;
  JSObject *global; 
} javascript_state;

#if 0

javascript_state *javascript_create_javascript() { return NULL; }

// Getting args from your scripting language stack:
int javascript_count_args(javascript_state *S) { return 0; }
int javascript_number_arg(javascript_state *S, int argi) { return 0; }
char *javascript_string_arg(javascript_state *S, int argi) { return (char*)""; }
int javascript_arg_is_string(javascript_state *S, int argi) { return 0; }
int javascript_arg_is_number(javascript_state *S, int argi) { return 0; }

// Putting results back onto your scripting language stack:
void javascript_return_string(javascript_state *S, const char *string_back) {}
void javascript_return_number(javascript_state *S, float number_back) {}
void javascript_return_boolean(javascript_state *S, int int_back) {}
void javascript_return_void(javascript_state *S) {};

// Handling table / map / alist structures of your scripting language:
javascript_table *javascript_create_table(javascript_state *S, int size) { return NULL; }
void javascript_set_table_string(javascript_state *S, javascript_table *T, char *key, const char *value) {}
void javascript_set_table_number(javascript_state *S, javascript_table *T, char *key, float value) {}
void javascript_set_table_boolean(javascript_state *S, javascript_table *T, char *key, int value) {}

// Make a name / symbol of the scripting language:
javascript_symbol *javascript_make_symbol(javascript_state *S, const char *n) { return NULL; }

// Non-local exits in the scripting language
void javascript_throw(javascript_state *S, javascript_symbol *Y, float n) {}

// Set a global variable of the scripting language to contain a table:
void javascript_set_global_variable_table(javascript_state *S, javascript_symbol *varname, javascript_table *table) {}

// Extend the scripting language with an added built-in function:
void javascript_register_builtin_function(javascript_state *S, const char *funname, void (*function)(javascript_state *FS)) {}
#endif

// A specialized non-local exit; could either throw, or just quit the
// application run:
void javascript_error(javascript_state *S, char *message, char *arg)
{
  fprintf(stderr, "Got error \"%s\", \"%s\"\n", message, arg);
}

static evaluator_interface *javascript_interface = NULL;

static char *option_names_var_name = (char*)"option_names";

static JSBool
javascript_set_parameter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char *parameter_name = NULL;
  char *parameter_value_string = NULL;
  jsdouble parameter_value_number = 0.0;
  int got_string = 0;
  int got_number = 0; 

  if (JS_ConvertArguments(cx, argc, argv, "ss", &parameter_name, &parameter_value_string)) {
    got_string = 1;
  } else if (JS_ConvertArguments(cx, argc, argv, "sd", &parameter_name, &parameter_value_number)) {
    got_number = 1;
  } else {
    return JS_FALSE;
  }

  char option_code = muesli_find_option_letter(javascript_interface->getopt_options, parameter_name);

  if (option_code != -1) {
    if (got_string) {
      char *value_copy = (char*)malloc(strlen(parameter_value_string));
      strcpy(value_copy, parameter_value_string);
      (javascript_interface->handle_option)(javascript_interface->app_params,
					    option_code,
					    muesli_malloc_copy_string(parameter_value_string),
					    0.0, 1,
					    "javascript");
    } else if (got_number) {
      (javascript_interface->handle_option)(javascript_interface->app_params,
					    option_code, NULL, parameter_value_number, 1,
					    "javascript");
    } else {
      (javascript_interface->handle_option)(javascript_interface->app_params,
					    option_code, (char*)"true", 0.0, 1,
					    "javascript");
    }
  }
  *rval = JSVAL_VOID;
  return JS_TRUE;
}

#if 0
static JSBool
javascript_set_parameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSObject table;

  if (!JS_ConvertArguments(cx, argc, argv, "O", &table)) {
    return JS_FALSE;
  }

#if 0
  // Fill in: use a table iterator from your language
  javascript_table table = javascript_table_arg(cs, 0);

  javascript_table_iteration_start(cs, table);
  while (javascript_table_iteration_next(cs, table) != 0) {
    javascript_set_parameter(cs,
			 javascript_table_iteration_current_key(cs, table),
			 javascript_table_iteration_current_value(cs, table));
  }

#endif
  *rval = JSVAL_VOID;
  return JS_TRUE;
}
#endif

static JSBool
javascript_get_parameter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char *parameter_name = NULL;

  if (!JS_ConvertArguments(cx, argc, argv, "s", &parameter_name)) {
    return JS_FALSE;
  }

  char option_code = muesli_find_option_letter(javascript_interface->getopt_options, parameter_name);

  if (option_code != -1) {

    muesli_value_t result = (javascript_interface->handle_option)(javascript_interface->app_params,
								  option_code,	// option
								  NULL, 0.0,	// value
								  0,	// set
								  "javascript");

    switch (result.type) {
    case muesli_value_string:
      *rval = JS_NewString(cx, result.data.as_string, strlen(result.data.as_string));
      return JS_TRUE;
    case muesli_value_float:
      return JS_NewNumberValue(cx, result.data.as_float, rval);
#if 0
    case muesli_value_boolean:
      *rval = (cs, result.data.as_int);
      return JS_TRUE;
      break;
#endif
    default:
      return JS_FALSE;
      break;
    }
  }
  return JS_FALSE;
}

#if 0
static JSBool
javascript_get_parameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  javascript_table *table = javascript_create_table(cs, 48);

  // todo: fix and re-instate -- I have to get long_options across to it somehow
  struct option *option_names = javascript_interface->getopt_options;

  while (option_names->name != 0) {
    
    result_type_t result_type;
    muesli_value_eval_t result;

    (javascript_interface->handle_option)(javascript_interface->app_params,
				      (option_names->val), // opt
				      NULL, 0.0,		     // value
				      0,		     // set
				      &result_type, &result,
				      "javascript");

    switch (result.type) {
    case muesli_value_string:
      javascript_set_table_string(cs, table, (char*)(option_names->name), result.data.as_string);
      break;
    case muesli_value_float:
      javascript_set_table_number(cs, table, (char*)(option_names->name), result.data.as_float);
      break;
    case muesli_value_boolean:
      javascript_set_table_boolean(cs, table, (char*)(option_names->name), result.data.as_int);
      break;
    default:
      javascript_set_table_boolean(cs, table, (char*)(option_names->name), 0);
      break;
    }
    option_names++;
  }
}
#endif

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static int
javascript_eval_in_language(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char *language_name = NULL;
  char *evaluand = NULL;

  if (!JS_ConvertArguments(cx, argc, argv, "ss", &language_name, &evaluand)) {
    return JS_FALSE;
  }

  unsigned int evaluand_length = strlen(evaluand);

  fprintf(stderr, "In javascript_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

  muesli_value_t result = muesli_eval_in_language(language_name,
						  evaluand,
						  evaluand_length,
						  ambient_transient);


  switch (result.type) {
  case muesli_value_string:
    *rval = JS_NewString(cx, result.data.as_string, strlen(result.data.as_string));
    return JS_TRUE;
  case muesli_value_float:
    return JS_NewNumberValue(cx, result.data.as_float, rval);
#if 0
  case muesli_value_boolean:
    *rval = (cs, result.data.as_int);
    return JS_TRUE;
    break;
#endif
  default:
    return JS_FALSE;
    break;
  }
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

static JSBool
javascript_eval_custom(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  char *evaluand = NULL;

  if (!JS_ConvertArguments(cx, argc, argv, "s", &evaluand)) {
    return JS_FALSE;
  }

  fprintf(stderr, "In javascript_eval_custom(\"%s\")\n", evaluand);

  muesli_value_t result = (javascript_interface->eval_string)(javascript_interface,
							      evaluand, strlen(evaluand),
							      ambient_transient);

  switch (result.type) {
  case muesli_value_string:
    *rval = JS_NewString(cx, result.data.as_string, strlen(result.data.as_string));
    return JS_TRUE;
  case muesli_value_float:
    return JS_NewNumberValue(cx, result.data.as_float, rval);
#if 0
  case muesli_value_boolean:
    *rval = (cs, result.data.as_int);
    return JS_TRUE;
    break;
#endif
  default:
    return JS_FALSE;
    break;
  }
}

////////////////
// Initialize //
////////////////

void
javascript_evaluator_init(evaluator_interface *interface)
{
  // Init javascript interface

  // Fill in: initialize your javascript application
  javascript_state *our_javascript_state = (javascript_state*)malloc(sizeof(javascript_state));

  our_javascript_state->rt = JS_NewRuntime(0x100000);
  our_javascript_state->cx = JS_NewContext(our_javascript_state->rt, 0x1000);
  our_javascript_state->global = JS_NewObject(our_javascript_state->cx, &global_class, NULL, NULL);
  JS_InitStandardClasses(our_javascript_state->cx, our_javascript_state->global);

  interface->state = our_javascript_state;
  javascript_interface = interface;

  javascript_interface->version = muesli_malloc_copy_string(JS_VersionToString(JS_GetVersion(our_javascript_state->cx)));

  // Extend system

  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"GetParameter", javascript_get_parameter, 1, 0);
  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"SetParameter", javascript_set_parameter, 2, 0);
#if 0
  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"Parameters", javascript_get_parameters, 0, 0);
  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"ModifyParameters", javascript_set_parameters, 1, 0);
#endif
  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"Custom", javascript_eval_custom, 1, 0);
  JS_DefineFunction(our_javascript_state->cx, our_javascript_state->global, (char*)"EvalInLanguage", javascript_eval_in_language, 2, 0);

#if 0
  // Set up option names

  // Fill in: Create a table of option names.  You may well not need to bother.
  javascript_table *option_names_table = javascript_create_table(our_javascript_state, 48);
  struct option *option_names;
  for (option_names = javascript_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    javascript_set_table_number(our_javascript_state, option_names_table,
				(char*)(option_names->name), (option_names->val));
  }
  javascript_set_global_variable_table(our_javascript_state,
				       javascript_make_symbol(our_javascript_state,
							      option_names_var_name),
				       option_names_table);
#endif
}

static void
javascript_load_file(evaluator_interface *interface,
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
javascript_eval_string(evaluator_interface *evaluator,
		   const char *javascript_c_string,
		   unsigned int string_length,
		   int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (javascript_c_string) {
    javascript_state *our_state = (javascript_state*)(evaluator->state);
    int old_ambient_transient = ambient_transient;
    int ok;
    jsval rval;
    ambient_transient = transient;

    // fprintf(stderr, "Javascript evaluating string \"%s\"\n", javascript_c_string);

    ok = JS_EvaluateScript(our_state->cx, our_state->global,
			   javascript_c_string, strlen(javascript_c_string),
			   "muesli API", 0, &rval);

    if (JSVAL_IS_STRING(rval)) {
      result.data.as_string = JS_GetStringBytesZ(our_state->cx, JSVAL_TO_STRING(rval));
      result.type = muesli_value_string;
    } else if (JSVAL_IS_DOUBLE(rval)) {
      jsdouble *d = JSVAL_TO_DOUBLE(rval);
      result.data.as_float = (float)(*d);
      result.type = muesli_value_float;
    } else if (JSVAL_IS_INT(rval)) {
      result.data.as_int = JSVAL_TO_INT(rval);
      result.type = muesli_value_integer;
    } else if (JSVAL_IS_BOOLEAN(rval)) {
      result.data.as_bool = JSVAL_TO_BOOLEAN(rval);
      result.type = muesli_value_boolean;
    } else {
      result.type = muesli_value_none;
    }

    ambient_transient = old_ambient_transient;
  }

  return result;
}

void
javascript_setup(evaluator_interface *new_javascript_interface)
{
  javascript_interface = new_javascript_interface;

  javascript_interface->eval_string = javascript_eval_string;
  javascript_interface->load_file = javascript_load_file;

  javascript_interface->version = NULL;
}

#endif
#endif

