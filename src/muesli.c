// muesli.c -*- C -*-
/* Interface to multiple interpreters.
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
#ifndef _MUESLI_C_
#define _MUESLI_C_

#include "../config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/times.h>
#include <unistd.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "muesli.h"

const char *muesli_version = PACKAGE_VERSION;

/***********************/
/* Evaluator interface */
/***********************/

static void muesli_default_close_evaluator(struct evaluator_interface *myself);

static evaluator_interface *evaluator_interfaces = NULL;

evaluator_interface *
muesli_make_evaluator_interface(const char* new_name,
				const char *new_extension,
				int new_is_binary,
				void (*setup_func)(evaluator_interface *myself),
				void (*init_func)(evaluator_interface *myself),
				option_handler_t new_option_handler,
				struct option *new_getopt_options,
				void* params,
				void *data)
{
  evaluator_interface *interface = (evaluator_interface*)malloc(sizeof(evaluator_interface));

  interface->name = muesli_malloc_copy_string(new_name);
  interface->next = NULL;
  interface->extension = muesli_malloc_copy_string(new_extension);
  interface->binary = new_is_binary;
  interface->initialized = 0;
  interface->init_evaluator = init_func;
  interface->specializer = NULL;
  interface->close_evaluator = muesli_default_close_evaluator;
  interface->add_function = NULL;
  interface->handle_option = new_option_handler;
  interface->getopt_options = new_getopt_options;
  interface->repl = NULL;
  interface->array_separator = (char*)", ";

  interface->language_verbosity = 0;
  interface->flags = 0;

  interface->underlying_program = NULL;
  interface->underlying_startup_string = NULL;
  interface->underlying_prompt_string = NULL;
  interface->underlying_evaluator_pid = 0;

  interface->app_params = params;
  interface->app_data = data;

  interface->eval_clear = NULL;
  interface->eval_give = NULL;
  interface->eval_given = NULL;

  interface->version = "?";

  (setup_func)(interface);

  return interface;
}

static void
muesli_default_close_evaluator(struct evaluator_interface *myself)
{
  if (myself->language_verbosity > 0) {
    fprintf(stderr, "Closing %s evaluator\n", myself->name);
  }
}

char*
muesli_getName(evaluator_interface *interface)
{
  return interface->name;
}

int
muesli_getBinary(evaluator_interface *interface)
{
  return interface->binary;
}

void
muesli_initialize_evaluator(evaluator_interface *interface)
{
  if (!interface->initialized) {
    if (interface->language_verbosity > 0) {
      fprintf(stderr, "Muesli initializing %s\n", interface->name);
    }
    (interface->init_evaluator)(interface);
    muesli_app_specialize(interface->name, interface);
    interface->initialized = 1;
  }
}

/**********/
/* Naming */
/**********/

evaluator_interface *
muesli_get_evaluator_by_extension(const char *extension)
{
  evaluator_interface *evaluator;
  for (evaluator = evaluator_interfaces;
       evaluator != NULL;
       evaluator = evaluator->next) {
    if ((evaluator->extension != NULL)
	&& strcmp(evaluator->extension, extension) == 0) {

      // Make sure it's ready for use
      muesli_initialize_evaluator(evaluator);

      return evaluator;
    }
  }
  return NULL;
}

const char *
muesli_get_language_by_extension(const char *extension)
{
  evaluator_interface *evaluator = muesli_get_evaluator_by_extension(extension);

  return (evaluator != NULL) ? evaluator->name : NULL;
}

int
muesli_evaluator_is_defined(const char *evaluator_name)
{
  evaluator_interface *evaluator;

  for (evaluator = evaluator_interfaces;
       evaluator != NULL;
       evaluator = evaluator->next) {
    if ((evaluator->extension != NULL)
	&& strcmp(evaluator->name, evaluator_name) == 0) {
      return 1;
    }
  }
  return 0;
}

/* This finds and initializes an evaluator.  As long as you always use
   this function to find yourself an evaluator, you should always get
   an initialized one.
 */
evaluator_interface *
muesli_get_named_evaluator(const char *evaluator_name, int exit_if_not_found)
{
  evaluator_interface *evaluator;

  for (evaluator = evaluator_interfaces;
       evaluator != NULL;
       evaluator = evaluator->next) {
    if ((evaluator->name != NULL)
	&& strcmp(evaluator->name, evaluator_name) == 0) {
      break;
    }
  }

  if (evaluator == NULL) {
    if (exit_if_not_found) {
      fprintf(stderr, "Evaluator \"%s\" not found\n", evaluator_name);
      exit(1);
    } else {
    return NULL;
    }
  }

  // Make sure it's ready for use
  muesli_initialize_evaluator(evaluator);

  return evaluator;
}

char *
muesli_result_type_name(value_type_tag_t enumerated)
{
  switch (enumerated) {
  case muesli_value_none: return (char*)"void";
  case muesli_value_float: return (char*)"number";
  case muesli_value_string: return (char*)"string";
  case muesli_value_boolean: return (char*)"boolean";
  case muesli_value_error: return (char*)"error";
  default: return (char*)"unknown";
  }
}

muesli_value_t
muesli_eval_in_language(const char *language,
			const char *fragment,
			unsigned int fragment_length,
			int transient)
{
  evaluator_interface *evaluator_interface = muesli_get_named_evaluator(language, 0);

  if (evaluator_interface == NULL) {
    muesli_value_t unresult;
    fprintf(stderr, "No such evaluator: %s\n", language);
    ANULL_VALUE(unresult);
    return unresult;
  }

  return (evaluator_interface->eval_string)(evaluator_interface,
					    fragment,
					    fragment_length,
					    transient);
}

int
muesli_load_file(const char *filename)
{
  const char *extension = strrchr(filename, '.');
  if (extension == NULL) {
    fprintf(stderr,
	    "Could not find extension of \"%s\" -- I do not know how to load this file\n",
	    filename);
    return 0;
  }
  extension++;
  if (*extension == '\0') {
    fprintf(stderr,
	    "Empty extension in \"%s\" -- I do not know how to load this file\n",
	    filename);
    return 0;
  }

  evaluator_interface *interface = muesli_get_evaluator_by_extension(extension);

  if (interface == NULL) {
    fprintf(stderr,
	    "I could not work out how to load \"%s\" from the extension \"%s\"\n",
	    filename, extension);
    return 0;
  }

  (interface->load_file)(interface, filename);
  return 1;
}

void
muesli_print_interpreter_names(FILE *stream,
			       const char *format,
			       const char *selected_name,
			       const char *selected_flag)
{
  evaluator_interface *eval;

  for (eval = evaluator_interfaces;
       eval != NULL;
       eval = eval->next) {
    fprintf(stream, format, eval->name,
	   (((selected_name != NULL)
	     && (strcmp(eval->name, selected_name) == 0))
	    ? (selected_flag ? selected_flag : " (selected)") : ""),
	   eval->version);
  }
}

/*******************/
/* Evaluator setup */
/*******************/

evaluator_interface *no_evaluator = NULL; // might put in a novelty one to print a message

evaluator_interface *scripting_evaluator = NULL;

evaluator_interface *evaluator_zero = NULL;
evaluator_interface *evaluator_siod = NULL;
evaluator_interface *evaluator_lua = NULL;
evaluator_interface *evaluator_external_exec = NULL;
evaluator_interface *evaluator_external_pipe = NULL;
evaluator_interface *evaluator_custom = NULL;
evaluator_interface *evaluator_perl = NULL;
evaluator_interface *evaluator_tcl = NULL;
evaluator_interface *evaluator_python = NULL;
evaluator_interface *evaluator_tcc = NULL;
evaluator_interface *evaluator_octave = NULL;
evaluator_interface *evaluator_slang = NULL;
evaluator_interface *evaluator_ruby = NULL;
evaluator_interface *evaluator_prolog = NULL;
evaluator_interface *evaluator_parrot = NULL;
evaluator_interface *evaluator_jvm = NULL;
evaluator_interface *evaluator_javascript = NULL;
evaluator_interface *evaluator_machine = NULL;
evaluator_interface *evaluator_stack_code = NULL;

/* You should use set_evaluator_app_specializer(string,
   specializer_function) to set a function to add your extensions.
   This is called when the interpreter is initialized, which happens
   when you call get_named_evaluator, so you must call
   set_evaluator_app_specializer before then.
*/
void
muesli_set_evaluator_app_specializer(char *evaluator_name,
			      evaluator_specializer_t specializer)
{
  evaluator_interface *evaluator;

  for (evaluator = evaluator_interfaces;
       evaluator != NULL;
       evaluator = evaluator->next) {
    if ((evaluator->name != NULL)
	&& strcmp(evaluator->name, evaluator_name) == 0) {
      break;
    }
  }

  if (evaluator != NULL) {
    evaluator->specializer = specializer;
  }
}

// This is an internal function; we call it for you.
// set_evaluator_app_specializer is the one you should call to set
// this up.
void
muesli_app_specialize(char *name,
	       evaluator_interface *interface)
{
  if (interface->specializer != NULL) {
    (interface->specializer)(interface);
  }
}

// A wrapper for new evaluator_interface, that registers the name in the map evaluators_by_name
evaluator_interface *
muesli_define_evaluator(const char *new_name,
			const char *new_extension,
			int new_is_binary,
			void (*new_setup)(evaluator_interface *myself),
			void (*new_init)(evaluator_interface *myself),
			option_handler_t new_option_handler,
			struct option *new_getopt_options,
			void *new_params,
			void *new_data)
{
  evaluator_interface *interface = muesli_make_evaluator_interface(new_name, new_extension, new_is_binary,
								   new_setup, new_init,
								   new_option_handler, new_getopt_options,
								   new_params, new_data);

  interface->next = evaluator_interfaces;
  evaluator_interfaces = interface;

#if 0
  fprintf(stderr, "  Added evaluator %p(%s) at %p, next %p\n",
	  interface,
	  interface->name,
	  evaluator_interfaces,
	  evaluator_interfaces->next  );
#endif

  if (new_is_binary
      && (interface->eval_0 == NULL)) {
    fprintf(stderr, "Warning: The evaluator \"%s\" claims to be binary, but cannot be\n         used as a binary fitness evaluator, as it lacks an eval_0 function\n", new_name);
  }

  return interface;
}

void
muesli_register_evaluators(void *new_app_params,
			   void *new_app_data,
			   option_handler_t params_handler,
			   struct option *getopt_options)
{
  // These create the interfaces, and give them the information
  // they'll need when the interpreters initialize.  These do not
  // initialize the interpreters; that happens later.
  evaluator_external_exec = muesli_define_evaluator("exec", "sh", 0,
						    external_exec_setup, exec_evaluator_init,
						    params_handler, getopt_options,
						    new_app_params, new_app_data);
  evaluator_external_pipe = muesli_define_evaluator("pipe", "", 0,
						    external_pipe_setup, pipe_evaluator_init,
						    params_handler, getopt_options,
						    new_app_params, new_app_data);
  evaluator_custom = muesli_define_evaluator("custom", "cus", 1,
					     custom_setup, custom_evaluator_init,
					     params_handler, getopt_options,
					     new_app_params, new_app_data);
  evaluator_zero = muesli_define_evaluator("zero", "zero", 1,
					   zero_setup, zero_evaluator_init,
					   params_handler, getopt_options,
					   new_app_params, new_app_data);
#if defined(HAVE_LIBSIOD) || defined(HAVE_LIBULSIOD)
  evaluator_siod = muesli_define_evaluator("siod", "scm", 0,
					   siod_setup, siod_evaluator_init,
					   params_handler, getopt_options,
					   new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBLUA
  evaluator_lua = muesli_define_evaluator("lua", "lua", 0,
					  lua_setup, lua_evaluator_init,
					  params_handler, getopt_options,
					  new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBPERL
  evaluator_perl = muesli_define_evaluator("perl", "pl", 0,
					   perl_setup, perl_evaluator_init,
					   params_handler, getopt_options,
					   new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBTCL
  evaluator_tcl = muesli_define_evaluator("tcl", "tcl", 0,
					  tcl_setup, tcl_evaluator_init,
					  params_handler, getopt_options,
					  new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBPYTHON
  evaluator_python = muesli_define_evaluator("python", "python", 0,
					     python_setup, python_evaluator_init,
					     params_handler, getopt_options,
					     new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBTCC
  evaluator_tcc = muesli_define_evaluator("tcc", "c", 0,
					  tcc_setup, tcc_evaluator_init,
					  params_handler, getopt_options,
					  new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBOCTINTERP
  evaluator_octave = muesli_define_evaluator("octave", "oct", 0,
					     octave_setup, octave_evaluator_init,
					     params_handler, getopt_options,
					     new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBSLANG
  evaluator_slang = muesli_define_evaluator("slang", "sl", 0,
					    slang_setup, slang_evaluator_init,
					    params_handler, getopt_options,
					    new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBRUBY
  evaluator_ruby = muesli_define_evaluator("ruby", "ruby", 0,
					   ruby_setup, ruby_evaluator_init,
					   params_handler, getopt_options,
					   new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBPROLOG
  evaluator_prolog = muesli_define_evaluator("prolog", "pro", 0,
					     prolog_setup, prolog_evaluator_init,
					     params_handler, getopt_options,
					     new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBPARROT
  evaluator_parrot = muesli_define_evaluator("parrot", "peg", 1,
					     parrot_setup, parrot_evaluator_init,
					     params_handler, getopt_options,
					     new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBJVM
  evaluator_jvm = muesli_define_evaluator("jvm", "class", 1,
					  jvm_setup, jvm_evaluator_init,
					  params_handler, getopt_options,
					  new_app_params, new_app_data);
#endif
#ifdef HAVE_LIBMOZJS
  evaluator_javascript = muesli_define_evaluator("javascript", "js", 1,
						 javascript_setup, javascript_evaluator_init,
						 params_handler, getopt_options,
						 new_app_params, new_app_data);
#endif
  evaluator_machine = muesli_define_evaluator("machine", "o", 1,
					      machine_code_setup, machine_code_evaluator_init,
					      params_handler, getopt_options,
					      new_app_params, new_app_data);
  evaluator_stack_code = muesli_define_evaluator("stack-code", "sc", 1,
						 stack_code_setup, stack_bytecode_evaluator_init,
						 params_handler, getopt_options,
						 new_app_params, new_app_data);
}

/**************************/
/* Interactive evaluation */
/**************************/

#define OUR_OWN_REPL 1

void
muesli_evaluator_repl(const char *evaluator_name)
{
  evaluator_interface *evaluator =
    muesli_get_named_evaluator(evaluator_name, 0);

  if (evaluator == NULL) {
    fprintf(stderr, "Could not find an evaluator called %s\n", evaluator_name);
  } else if (OUR_OWN_REPL || evaluator->repl == NULL) {
    int running = 1;
    char *prompt = (char*)malloc(strlen(evaluator->name) + 2);
    sprintf(prompt, "%s>", evaluator->name);
    fprintf(stderr, "Using makeshift Read-Eval-Print loop for %s\n", evaluator->name);
    fprintf(stderr, "Use quit to go back to application run or exit to leave application\n");
    do {
      char *input;
#ifdef HAVE_LIBREADLINE
      input = readline(prompt);
#else
      char input_buffer[CLI_BUF_SIZE];
      puts(prompt); fflush(stdout);
      fgets(input_buffer, CLI_BUF_SIZE, stdin);
      input = input_buffer;
#endif
      if (strcmp(input, "quit") == 0) {
	running = 0;
      } else if (strcmp(input, "exit") == 0) {
	exit(0);
      } else {
	muesli_value_t result = (evaluator->eval_string)(evaluator,
							 input,
							 strlen(input),
							 0);
	switch (result.type) {
	case muesli_value_none:
	  printf("no result (or nothing printable)\n");
	  break;
	case muesli_value_float:
	  printf("%f\n", result.data.as_float);
	  break;
	case muesli_value_integer:
	  printf("%d\n", result.data.as_int);
	  break;
	case muesli_value_string:
	  printf("\"%s\"\n", result.data.as_string);
	  break;
	case muesli_value_boolean:
	  printf("%s\n", result.data.as_bool ? "true" : "false");
	  break;
	case muesli_value_error:
	  printf("error\n");
	  break;
	}
      }
    } while (running);
    free(prompt);
  } else {
    printf("Calling Read-Eval-Print loop of %s\n", evaluator->name);
    evaluator->repl(evaluator);
  }
}

/*************************************/
/* Utility for shell/pipe evaluators */
/*************************************/

/* If the extra string0 is given, take that as supplying the first element. */

void
muesli_split_args(const char *string, const char *string0, char ***new_argv, int *new_argc)
{
  unsigned int n_parts = (string0 == NULL) ? 1 : 2;
  const char *p = string;
  char c;
  int in_quotes = 0;
  int escaped = 0;
  char *scratch = (char*)malloc(strlen(string)
				+ ((string0 == NULL)
				   ? 0
				   : (strlen(string0) + 1))
				+ 1);
  char *q = scratch;

  // count the possible breaks between arguments -- possibly an
  // overestimate as some may be quoted, but that doesn't matter
  while ((c = *p++) != '\0') {
    switch (c) {
    case ' ': case '\t': case '\n': case '\r': n_parts++; break;
    default: break;
    }
  }

  *new_argv = (char**)malloc(n_parts * sizeof(char**));
  *new_argc = 0;
  (*new_argv)[0] = q;

  if (string0 != NULL) {
    strcpy(q, string0);
    q += strlen(q) + 1;
    (*new_argc)++;
    (*new_argv)[1] = q;
  }

  p = string;

  (*new_argc)++;

  while ((c = *p++) != '\0') {
    if (escaped) {
      switch (c) {
      case 'n': *q++ = '\n'; break;
      case 'r': *q++ = '\r'; break;
      case 't': *q++ = '\t'; break;
      case '\\': *q++ = '\\'; break;
      default:  *q++ = c; break;
      }
      escaped = 0;
    } else {
      if (in_quotes) {
	switch (c) {
	case '\\': escaped = 1; break;
	case '"': in_quotes = 0; break;
	default: *q++ = c; break;
	}
      } else {
	switch (c) {
	case '\\': escaped = 1; break;
	case ' ': case '\t': case '\n': case '\r':
	  *q++ = '\0';
	  (*new_argv)[(*new_argc)++] = q;
	  break;
	case '"': in_quotes = 1; break;
	default: *q++ = c; break;
	}
      }
    }
  }
  *q++ = c;			/* write the final null */
  (*new_argv)[(*new_argc)] = NULL;
}

/*******************/
/* Option handling */
/*******************/

char
muesli_find_option_letter(struct option *getopt_options, const char *option_name)
{
  struct option *opt;

  for (opt = getopt_options; opt->name != NULL; opt++) {
    if (strcmp(option_name, opt->name) == 0) {
      return opt->val;
    }
  }
  return (char)(-1);
}

int
muesli_set_parameter(evaluator_interface *interface,
		     struct option *options,
		     char* option,
		     char *value,
		     const char *language)
{
  char option_code = muesli_find_option_letter(options, option);
  option_handler_t handler = interface->handle_option;

  // fprintf(stderr, "option_code=%c (%d)\n", option_code, option_code);

  if (option_code != -1) {
    (handler)(interface->app_params, option_code,
	      value, 0.0, 1, language);
    return 1;
  } else {
    return 0;
  }
}

static char num_buf[64];

const char*
muesli_get_parameter(evaluator_interface *interface,
		     struct option *options,
		     char* option)
{
  option_handler_t handler = interface->handle_option;
  char option_code = muesli_find_option_letter(options, option);
  if (option_code != -1) {
    muesli_value_t result = (handler)(interface->app_params,
				      option_code,	// option
				      NULL, 0.0,	// value
				      0,	// set
				      NULL);

    switch (result.type) {
    case muesli_value_string:
      return result.data.as_string;
    case muesli_value_float:
      sprintf(num_buf, "%f", result.data.as_float);
      return num_buf;
    case muesli_value_integer:
      sprintf(num_buf, "%d", result.data.as_int);
      return num_buf;
    case muesli_value_boolean:
      if (result.data.as_bool) {
	return "true";
      } else {
	return "false";
      }
    default:
      return NULL;
    }
  } else {
    return NULL;
  }
}

char *
muesli_malloc_copy_string(const char *original)
{
  char *copy = (char*)malloc(strlen(original));
  strcpy(copy, original);
  return copy;
}

#endif

/* muesli.c ends here */
