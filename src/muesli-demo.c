/* Demo / test program for Muesli 
   Time-stamp: <2010-02-15 16:21:27 jcgs>
   
   Copyright (C) 2010 University of Limerick

   This file is part of muesli.
   
   muesli is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your
   option) any later version.
   
   muesli is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.
   
   You should have received a copy of the GNU General Public License
   along with muesli.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "muesli.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

/*********************************************/
/* Parameter and (optional) variable blocks. */
/*********************************************/

typedef struct app_params_t {
  int verbose;
  char *language_name;
  char *prompt_format;
} app_params_t;

typedef struct app_vars_t {
  int count;
  int running;
  evaluator_interface *evaluator;
} app_vars_t;

app_params_t app_params;
app_vars_t app_vars;

/***************************************************************/
/* This represents the main logic of your application.  It can */
/* interface as many functions to muesli as you want.	       */
/***************************************************************/

void
app_hello_world()
{
  printf("hello, world\n");
}

/******************************************************************/
/* An interface to the application from each language. You could  */
/* probably generate these with a wrapper generator such as SWIG. */
/******************************************************************/

#if defined(HAVE_LIBSIOD) || defined(HAVE_LIBULSIOD)
#ifdef HAVE_LIBULSIOD
#include "siod/ulsiod.h"
#else
#include "siod/siod.h"
#endif
/* This section would typically be in another file; for good
   information-hiding, it has a static for the interface (that holds
   the language state) although that's probably a bit fussy for most
   people.  */
static evaluator_interface *siod_interface = NULL;

siod_LISP
app_siod_hello_world()
{
  app_hello_world();
  return NIL;
}

void
siod_specialize_grevo(evaluator_interface *interface)
{
  siod_interface = interface;
  // grevo-specific functions:
  Muesli_Add_Fn_0(siod_interface, "hello-world", app_siod_hello_world);
}
#endif

#ifdef HAVE_LIBLUA
/* This section would typically be in yet another file; for good
   information-hiding, it has a static for the interface (that holds
   the language state) although that's probably a bit fussy for most
   people. */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
static evaluator_interface *lua_interface = NULL;

int
app_lua_hello_world(lua_State *L)
{
  app_hello_world();
  return 0;
}

void
lua_specialize_grevo(evaluator_interface *interface)
{
  lua_interface = interface;
  lua_State *l_s = (lua_State*)(lua_interface->state);
  // todo: use Muesli interface for these
  lua_register(l_s, (char*)"hello_world", app_lua_hello_world);
}
#endif

/******************************************/
/* Command-line / scripted option control */
/******************************************/

static const char *short_options = "vVqhl:p:";

struct option long_options_data[] = {
  {"help", no_argument, 0, 'h'},
  {"verbose", no_argument, 0, 'v'},
  {"quiet", no_argument, 0, 'q'},
  {"language", required_argument, 0, 'l'},
  {"prompt", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {0, 0, 0, 0}
};

struct option *long_options = long_options_data;

// This is in a separate function from the loop for going through
// argv, so it can be used from the scripting languages to set
// parameters from there.  Also for such use, it returns values, too.
// To avoid making it a yet longer, stragglier function which looks to
// see whether the pointers are non-NULL, it requires pointers for
// results to be given whenever the `set' argument is false.

muesli_value_t
handle_demo_app_option(void *param_block,
		       char opt,
		       /* const */ char *value_string,
		       float value_number,
		       int set,
		       const char *from_language)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  app_params_t *run_params = (app_params_t*)param_block;
  switch (opt) {
  case 'l':
    if (set) {
      run_params->language_name = value_string;
    } else {
      result.data.as_string = run_params->language_name;
      result.type = muesli_value_string;
    }
    break;
  case 'p':
    if (set) {
      run_params->prompt_format = value_string;
    } else {
      result.data.as_string = run_params->prompt_format;
      result.type = muesli_value_string;
    }
    break;
  case 'v':
    if (set) {
      run_params->verbose = 1;
    } else {
      result.data.as_bool = run_params->verbose;
      result.type = muesli_value_boolean;
    }
    break;
  case 'q':
    if (set) {
      run_params->verbose = 0;
    } else {
      result.data.as_bool = !(run_params->verbose);
      result.type = muesli_value_boolean;
    }
    break;
  default:
    return result;
    break;
  }
  return result;
}

static char *demo_app_version = "1.0";

void
parse_args(int argc, char **argv,
	   app_params_t *run_params,
	   app_vars_t *variables)
{
  while (1) {
    int option_index = 0;
    char opt = getopt_long(argc, argv,
			   short_options, long_options,
			   &option_index);

    if (opt == -1) {
      break;
    }

    switch (opt) {
      /* First, we handle a couple of responses that we want to work
	 only from the command line, not from a script: */
    case 'h':
      printf("Usage:\n  muesli-demo [options] filenames\n");
      printf("  Options are:\n");
      printf("    --version (-V)         print version and exit\n");
      printf("    --help (-h)            print this message and exit\n");
      printf("    --language (-l) LANG   select LANG\n");
      printf("    --prompt (-p) PR       set prompt format to PR\n");
      printf("    --verbose (-v)         turn on extra output\n");
      printf("    --quiet (-q)           turn off extra output\n");
      exit(0);
      break;
    case 'V':
      printf("This is muesli-demo, version %s\n", demo_app_version);
      exit(0);
      break;
    default:
      {
	handle_demo_app_option(run_params,
			       opt,
			       optarg,
			       0.0,
			       1,
			       NULL);
      }
      break;
    }
  }
  for (; optind < argc; optind++) {
    muesli_load_file(argv[optind]);
  }
}

static void
muesli_internal_tests()
{
  int argc;
  char **argv;
  int i;

  printf("split_args without string0 (should be one .. six):");
  muesli_split_args("one two three four five six", NULL, &argv, &argc);
  for (i = 0; i < argc; i++) {
    printf(" \"%s\"", argv[i]);
  }
  printf("\n");
  printf("split_args with string0 (should be zero one .. six):");
  muesli_split_args("one two three four five six", "zero", &argv, &argc);
  for (i = 0; i < argc; i++) {
    printf(" \"%s\"", argv[i]);
  }
  printf("\n");
}

/********************************/
/* Main program and its helpers */
/********************************/

void
app_help()
{
  printf("You can type any of these demo program commands:\n");
  printf("  help\n  languages\n  mtest\n  quit\n");
  printf("Anything else will be passed to the current interpreter.\n");
  printf("You can use the function set_parameter (or set-parameter or SetParameter,\n");
  printf("according to the syntax of the current language) to set parameters, which\n");
  printf("include:\n  prompt\n  language\n");
  printf("You can use the extension function hello-world / hello_world / HelloWorld\n");
}

#define BUF_SIZE (1024 * 64)

int
main(int argc, char **argv)
{
  app_params.verbose = 0;
  app_params.prompt_format = "muesli[%s]> ";
  app_params.language_name = "siod";

  parse_args(argc, argv, &app_params, &app_vars);

  muesli_register_evaluators(&app_params,
			     &app_vars,
			     handle_demo_app_option,
			     long_options);

#if defined(HAVE_LIBSIOD) || defined(HAVE_LIBULSIOD)
  muesli_set_evaluator_app_specializer((char*)"siod", siod_specialize_grevo);
#endif
#ifdef HAVE_LIBLUA
  muesli_set_evaluator_app_specializer((char*)"lua", lua_specialize_grevo);
#endif

  app_vars.count = 0;
  app_vars.running = 1;

  app_help();

  while (app_vars.running) {
    char buffer[BUF_SIZE];
    char *input;

    /* The user could have changed the language by setting a parameter
       from the fragment they entered last time round the loop, so
       re-examine the parameter each time: */
    app_vars.evaluator
      = muesli_get_named_evaluator(app_params.language_name,
				   1);

    fflush(stdout);
#ifdef HAVE_LIBREADLINE
    snprintf(buffer, BUF_SIZE,
	     app_params.prompt_format,
	     app_vars.evaluator->name);
    input = readline(buffer);
#else
    printf(app_params.prompt_format,
	   app_vars.evaluator->name);
    fflush(stdout);
    fgets(buffer, BUF_SIZE, stdin);
    input = buffer;
#endif

    if (strcmp(input, "quit") == 0) {
      app_vars.running = 0;
    } else if (strcmp(input, "help") == 0) {
      app_help();
    } else if (strcmp(input, "mtest") == 0) {
      muesli_internal_tests();
    } else if (strcmp(input, "languages") == 0) {
      muesli_print_interpreter_names(stdout,
				     "  %s%s\n",
				     app_params.language_name,
				     " (currently selected)");
    } else {
      muesli_value_t result
	= (app_vars.evaluator->eval_string)(app_vars.evaluator,
					    input,
					    strlen(input),
					    0);
      switch (result.type) {
      case muesli_value_none:
	printf("no result (or nothing printable)\n");
	break;
      case muesli_value_float:
	printf("%g\n", result.data.as_float);
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

  }

  exit (0);
}
