// muesli-perl.c -*- C -*-
/* muesli interface to Perl
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
#ifndef _MUESLI_PERL_
#define _MUESLI_PERL_

#include "../config.h"

#ifdef HAVE_LIBPERL

#include "../config.h"
#include "muesli.h"

#include <limits.h>

#include <unistd.h>
#include <getopt.h>
#include <EXTERN.h>
#include <perl.h>

#include "muesli-perl.h"

#if 0
extern "C" {
void XS_MuesliPerl_c_get_parameter(PerlInterpreter*, CV*);
void XS_MuesliPerl_c_set_parameter(PerlInterpreter*, CV*);
void XS_MuesliPerl_c_add_to_grammar(PerlInterpreter*, CV*);
}
#endif

static int ambient_transient = 0;

#ifndef CUSTOMIZED
// How to customize:

// Replace this whole lot of "perl_*" things, wherever they are
// called, with the corresponding thing from your language or
// application.  This collection of typedefs, variable and functions
// defines the interface between your application and a programming
// language or specialized evaluation library.

// If you're adding another interpreted programming language, you
// should probably define equivalents for all of them; if doing a
// specialized application that isn't a general programming language,
// you can probably get away with leaving them as they are, and just
// filling in the function perl_eval_individual.

// Other things to do when adding a language are to take a copy of
// this file rather than doing it in-place (as the core system expects
// the original to be there) and do s/perl_/$MYLANGUAGE_/g on the
// copy (except for the built-in function called "custom", and its
// call to perl_eval_individual), and to wrap this file in an "ifdef
// WITH_$MYLANGUAGE" so that it can be included conditionally at
// compile time.  You might as well take this comment block out of the
// copy, too, to avoid confusing others later.

// Really lazy hacking technique: switch this block of definitions off
// by using the ifndef above; try compiling, and fix the errors it
// gives you by replacing the call to each missing function by a call
// to a function in your new interpreter.

PerlInterpreter *my_perl = NULL;

int
perl_count_args(PerlInterpreter *S)
{
  // todo: write this
  return 0;
}

int
perl_number_arg(PerlInterpreter *S, int argi)
{
  // todo: write this
  return 0;
}

char *perl_string_arg(PerlInterpreter *S, int argi)
{
  // todo: write this
  return (char*)"";
}

int
perl_arg_is_string(PerlInterpreter *S, int argi)
{
  // todo: write this
  return 0;
}

int
perl_arg_is_number(PerlInterpreter *S, int argi)
{
  // todo: write this
  return 0;
}

void
perl_return_string(PerlInterpreter *S, const char *string_back)
{
  // todo: write this
}

void
perl_return_number(PerlInterpreter *S, float number_back)
{
  // todo: write this
}

void
perl_return_integer(PerlInterpreter *S, float number_back)
{
  // todo: write this
}

void
perl_return_boolean(PerlInterpreter *S, int int_back) 
{
  // todo: write this
}

perl_table *perl_create_table(PerlInterpreter *S, int size) 
{
  // todo: write this
  return NULL;
}

void
perl_set_table_string(PerlInterpreter *S, perl_table *T, char *key, const char *value)
{
  // todo: write this
}

void
perl_set_table_number(PerlInterpreter *S, perl_table *T, char *key, float value)
{
  // todo: write this
}

void
perl_set_table_boolean(PerlInterpreter *S, perl_table *T, char *key, int value)
{
  // todo: write this
}

perl_symbol *perl_make_symbol(PerlInterpreter *S, const char *n)
{
  // todo: write this
  return NULL;
}

void
perl_throw(PerlInterpreter *S, perl_symbol *Y, float n)
{
  // todo: write this
}

void
perl_set_global_variable_table(PerlInterpreter *S, perl_symbol *varname, perl_table *table)
{
  // todo: write this
}
#endif

void
perl_register_builtin_function(char *funname,
			       void (*function)(PerlInterpreter *FS,
						CV *thing))
{
  newXS(funname, function, "muesli-perl.c");
// >     newXS("DataLoader::boot_DataLoader", boot_DataLoader, __FILE__);
}

// Interface from the rest of the world
static evaluator_interface *perl_interface = NULL;

static char *option_names_var_name = (char*)"option_names";

static void
muesli_perl_throw(PerlInterpreter *cs, const char *message)
{
  fprintf(stderr, message);
  exit(1);
}

static void
set_parameter_perl(PerlInterpreter *cs)
{
  if (perl_count_args(cs)) {
    muesli_perl_throw(cs, "too few args to set_parameter_perl\n");
  }

  char option_code = muesli_find_option_letter(perl_interface->getopt_options, perl_string_arg(cs, 0));

  if (option_code != -1) {

    if (perl_arg_is_string(cs, 1)) {
      (perl_interface->handle_option)(perl_interface->app_params,
				      option_code,
				      muesli_malloc_copy_string(perl_string_arg(cs, 1)),
				      0.0,
				      1,
				      (char *)"perl");
    } else if (perl_arg_is_number(cs, 1)) {
      (perl_interface->handle_option)(perl_interface->app_params,
				      option_code,
				      NULL, perl_number_arg(cs, 2),
				      1,
				      (char *)"perl");
    } else {
      (perl_interface->handle_option)(perl_interface->app_params,
				      option_code,
				      (char*)"true", 0.0,
				      1,
				      (char *)"perl");
    }
  }
}

static void
set_parameters_perl(PerlInterpreter *cs)
{
  if (perl_count_args(cs) < 1)  {
    muesli_perl_throw(cs, "too few args to set_parameters_perl\n");
  }

#if 0
  // Fill in: use a table iterator from your language
  perl_table table = perl_table_arg(cs, 0);

  perl_table_iteration_start(cs, table);
  while (perl_table_iteration_next(cs, table) != 0) {
    set_parameter_perl(cs,
		       perl_table_iteration_current_key(cs, table),
		       perl_table_iteration_current_value(cs, table));
  }

#endif
}

static void
get_parameter_perl(PerlInterpreter *cs)
{
  if (perl_count_args(cs) < 1)  {
    muesli_perl_throw(cs, "too few args to get_parameter_perl\n");
  }

  char option_code = muesli_find_option_letter(perl_interface->getopt_options, perl_string_arg(cs, 0));

  if (option_code != -1) {

    muesli_value_t result = (perl_interface->handle_option)(perl_interface->app_params,
							    option_code,	// option
							    NULL, 0.0,	// value
							    0,	// set
							    (char *)"perl");

    switch (result.type) {
    case muesli_value_string:
      perl_return_string(cs, result.data.as_string);
      break;
    case muesli_value_float:
      perl_return_number(cs, result.data.as_float);
      break;
    case muesli_value_boolean:
      perl_return_boolean(cs, result.data.as_int);
      break;
    default:
      perl_return_boolean(cs, 0);
      break;
    }
  }
}

static void
get_parameters_perl(PerlInterpreter *cs)
{
  perl_table *table = perl_create_table(cs, 48);

  struct option *option_names = perl_interface->getopt_options;

  while (option_names->name != 0) {

    muesli_value_t result = (perl_interface->handle_option)(perl_interface->app_params,
							    (option_names->val), // opt
							    NULL, 0.0,		     // value
							    0,		     // set
							    (char *)"perl");

    switch (result.type) {
    case muesli_value_string:
      perl_set_table_string(cs, table, (char*)(option_names->name), result.data.as_string);
      break;
    case muesli_value_float:
      perl_set_table_number(cs, table, (char*)(option_names->name), result.data.as_float);
      break;
    case muesli_value_boolean:
      perl_set_table_boolean(cs, table, (char*)(option_names->name), result.data.as_int);
      break;
    default:
      perl_set_table_boolean(cs, table, (char*)(option_names->name), 0);
      break;
    }
    option_names++;
  }

}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives Perl access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a Perl muesli setup program.

static void
custom_eval_function_perl(PerlInterpreter *cs)
{
  if (perl_arg_is_string(cs, 0)) {
    char *str = perl_string_arg(cs, 0);
    evaluator_interface *evaluator = muesli_get_named_evaluator("custom", 0);
    if (evaluator == NULL) {
      fprintf(stderr, "Custom evaluator not found");
      // todo: some kind of throw?
      return;
    }
    muesli_value_t result = (evaluator->eval_string)(evaluator,
						     str,
						     strlen(str),
						     ambient_transient);
    switch (result.type) {
    case muesli_value_float:
      perl_return_number(cs, result.data.as_float);
      break;
    case muesli_value_integer:
      perl_return_integer(cs, result.data.as_int);
      break;
    case muesli_value_string:
      perl_return_string(cs, result.data.as_string);
      break;
    case muesli_value_boolean:
      if (result.data.as_bool) {
	perl_return_number(cs, 1.0);
      } else {
	perl_return_number(cs, 0.0);
      }
      break;
    case muesli_value_none:
      // probably some other way to do this
      perl_return_number(cs, 0.0);
      break;
    case muesli_value_error:
      // todo: some kind of throw here
      break;
    }
  } else {
    muesli_perl_throw(cs, "custom_eval_function_perl must be given a string\n");
  }
}

////////////////
// Initialize //
////////////////

static void
perl_parse_callback(pTHX)
{


// > Now in my DataDownloader.exe I have an xs_init() function that does a
// >     newXS("DataLoader::boot_DataLoader", boot_DataLoader, __FILE__);
// > 
// > But my extension still does not get registered 'properly' as my
// > getdata.pl script cannot see the DataLoader extension routines (i.e.
// > Cache()) and complains of an 'undefined subroutine'.
// > 
// > Are you saying I should explicitly call boot_DataLoader() in my C++
// > code ?
// > If so, then what does the newXS() in xs_init() do ?  If I do call
// > boot_DataLoader() manually, what arguments should I provide it with ?
// > I looked at the perl headers and dont know what pTHX_ is ?

// You need to pass your own xs_init() as an argument to perl_parse().
// That way when the Perl interpreter is initialized you'll already
// have the DataLoader::boot_DataLoader() function defined in Perl
// space.


  dXSUB_SYS;

#if 0
  // Fill in: register all these language extensions (as applicable):
#if 0
  perl_register_builtin_function((char*)"MuesliPerl::taf", XS_MuesliPerl_c_taf_perl);
#endif
  perl_register_builtin_function((char*)"MuesliPerl::GetParameter", XS_MuesliPerl_c_get_parameter);
  perl_register_builtin_function((char*)"MuesliPerl::SetParameter", XS_MuesliPerl_c_set_parameter);
#if 0
  perl_register_builtin_function((char*)"MuesliPerl::EvolutionParameters", XS_MuesliPerl_c_get_parameters_perl);
  perl_register_builtin_function((char*)"MuesliPerl::ModifyEvolutionParameters", XS_MuesliPerl_c_set_parameters_perl);
#endif
  perl_register_builtin_function((char*)"MuesliPerl::AddToGrammar", XS_MuesliPerl_c_add_to_grammar);
#if 0
  perl_register_builtin_function((char*)"MuesliPerl::Custom", XS_MuesliPerl_c_custom_eval_function_perl);
  perl_register_builtin_function((char*)"MuesliPerl::Data", XS_MuesliPerl_c_data_item_perl);
  perl_register_builtin_function((char*)"MuesliPerl::DataRows", XS_MuesliPerl_c_data_rows_loaded_perl);
  perl_register_builtin_function((char*)"MuesliPerl::Sample", XS_MuesliPerl_c_sample_perl);
#endif
#endif
}

void
perl_load_file(evaluator_interface *interface,
	       const char *filename)
{
  int i;

  int muesli_flags = interface->flags;
#if 1
  // it doesn't work if this isn't there! todo: sort out why
  char *perl_argv0[1] = {
    NULL
  };
#endif
  char **perl_argv = (char**)malloc(6 * sizeof(char*));
  perl_table *option_names_table = perl_create_table(my_perl, 48);
  struct option *option_names;

  perl_interface = interface;
  perl_argv[0] = (char*)"muesli";
  perl_argv[1] = (char*)"-w";		// perhaps make this optional?
  int perl_argc = 2;

  if (filename == NULL) {
    // If we don't give it any files to start up with, it reads stdin,
    // and just sits there until you type something at it...; but
    // giving -e (which tells Perl that the script is on the command
    // line) we get it not to wait for any files.
    perl_argv[perl_argc++] = (char*)"-e";
    perl_argv[perl_argc++] = (char*)"";
  }

#if 0
  for (f_f = functionsFiles.begin();
       f_f != functionsFiles.end();
       f_f++) {
    // fprintf(stderr, "setting argv[%d] to %s\n", perl_argc, *f_f);
    perl_argv[perl_argc++] = *f_f;
  }
#else
  if (filename != NULL) {
    perl_argv[perl_argc++] = (char*)filename;
  }
#endif
  perl_argv[perl_argc] = NULL;
  char *perl_env0[] = {};
  char **perl_env = perl_env0;
  PERL_SYS_INIT3(&perl_argc,
		 &perl_argv,&perl_env);
  my_perl = perl_alloc();
  perl_construct(my_perl);
  PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
  fprintf(stderr, "doing perl_parse with %d args\n", perl_argc);
  for (i = 0; i < perl_argc; i++) {
    fprintf(stderr, "  [%d]: %s\n", i, perl_argv[i]);
  }
  perl_parse(my_perl, perl_parse_callback, perl_argc, perl_argv, perl_env);
  fprintf(stderr, "done perl_parse\n");

  // Extend system

  // Set up option names

  for (option_names = perl_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    perl_set_table_number(my_perl, option_names_table,
			  (char*)(option_names->name), (option_names->val));
    option_names++;
  }
  perl_set_global_variable_table(my_perl,
				 perl_make_symbol(my_perl,
						  option_names_var_name),
				 option_names_table);


  if (muesli_flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "initialized perl interpreter\n");
  }
}

void
perl_evaluator_init(evaluator_interface *interface)
{
  perl_interface = interface;
  // This one is weird.  Perl seems to well-nigh insist on getting the
  // files when it initializes.  It must be possible to work round
  // this.  todo: do so.
  char *dummyFunctionsFile = "";
  perl_load_file(interface, dummyFunctionsFile);
}

static muesli_value_t
perl_eval_string(evaluator_interface *interface,
		 const char *perl_c_string,
		 unsigned int string_length,
		 int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  perl_interface = interface;

  if (perl_c_string) {
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;

    // fprintf(stderr, "Evaluating string \"%s\"\n", perl_c_string);

    SV *val = eval_pv(perl_c_string,
		      0 // 1
		      );
    char *string_back = SvPV_nolen(val);

    ambient_transient = old_ambient_transient;

    if (isdigit(string_back[0])
	|| ((string_back[0] == '-')
	    && isdigit(string_back[1]))) {
      result.data.as_float = SvNV(val);
      result.type = muesli_value_float;
    } else {
      result.data.as_string = string_back;
      result.type = muesli_value_string;
    }
  }
  return result;
}

void
perl_setup(evaluator_interface *new_perl_interface)
{
  perl_interface = new_perl_interface;

  perl_interface->eval_string = perl_eval_string;
  perl_interface->load_file = perl_load_file;
}

#endif
#endif
