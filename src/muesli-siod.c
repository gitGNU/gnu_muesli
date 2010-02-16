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

#ifndef _MUESLI_SIOD_
#define _MUESLI_SIOD_

#include "../config.h"

// We only use plain SIOD if our own version of SIOD is not available.
// This is because plain SIOD doesn't use a prefix for its names, and
// clashes with at least one other evaluator package ("load" is also
// defined by TCC).
#ifndef HAVE_LIBULSIOD
#ifdef HAVE_LIBSIOD
#ifdef HAVE_LIBTCC
// check with whether we're going to clash with TCC over the symbol "load"
#error This combination will not work, as SIOD and TCC clash over the symbol "load".
#else

#include "muesli.h"

#include <limits.h>
#include <getopt.h>

#include "siod.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
  extern LISP lthrow(LISP tag,LISP value);

  // from siodp.h
  extern int rfs_getc(unsigned char **p);
  extern void rfs_ungetc(unsigned char c,unsigned char **p);

  extern LISP print_to_string(LISP exp,LISP str,LISP nostart);


#include "lang-extns-cpp.h"


extern LISP sym_t;

/********************************/
/* Integer divide and remainder */
/********************************/

static LISP
IntQuotient(LISP x,LISP y)
{if NFLONUMP(x) err("wta(1st) to quotient",x);
 if NULLP(y)
   return(flocons(1/floor(FLONM(x))));
 else
   {if NFLONUMP(y) err("wta(2nd) to intquotient",y);
    return(flocons((int)floor(FLONM(x))/(int)floor(FLONM(y))));}}

static LISP
Remainder(LISP x,LISP y)
{if NFLONUMP(x) err("wta(1st) to remainder",x);
 if NULLP(y)
   return(flocons(1/(int)floor(FLONM(x))));
 else
   {if NFLONUMP(y) err("wta(2nd) to remainder",y);
    return(flocons((int)floor(FLONM(x))%(int)floor(FLONM(y))));}}

/************************/
/* Identifying closures */
/************************/

/* We want this in simplify-expr.scm */

static LISP
ClosureP(LISP x)
{if TYPEP(x, tc_closure) return(sym_t); else return(NIL);}

// Interface from the rest of the world:
static evaluator_interface *siod_interface = NULL;

static LISP option_names_var = NIL;
static LISP option_names_table = NIL;

#define NUMBER_BUF_SIZE 64
static char number_buf[NUMBER_BUF_SIZE];

static LISP
set_parameter_siod(LISP option, LISP value)
{
  int ok = 0;
  if ((TYPEP(value, tc_string)) || (TYPEP(value, tc_symbol))) {
    // note that `nil' comes through as the symbol named "nil"
    ok = muesli_set_parameter(siod_interface, get_c_string(option), get_c_string(value));
  } else if (TYPEP(value, tc_flonum)) {
    snprintf(number_buf, NUMBER_BUF_SIZE, "%f", FLONM(value));
    ok = muesli_set_parameter(siod_interface, get_c_string(option), number_buf);
  } else {
    ok = muesli_set_parameter(siod_interface, get_c_string(option), (char*)"true");
  }
  if (ok) {
    return cintern((char*)"t");
  } else {
    return NIL;
  }

}

static LISP
set_parameters_siod(LISP changes)
{
  while (changes != NIL) {
    LISP change = car(changes);
    if (CONSP(change)) {
      set_parameter_siod(car(change), cdr(change));
    }
    changes = cdr(changes);
  }
  return NIL;
}

static LISP
get_parameter_siod(LISP option)
{
  char option_code = FindOptionLetter(get_c_string(option));
  if (option_code != -1) {
    
    
    
    

    fprintf(stderr, "handling option %s\n", get_c_string(option));

    handle_option(siod_interface->app_params,
		  option_code,	// option
		  NULL, 0.0,	// value
		  0,	// set
		  &result_type, &result,
		  1);

    switch (result_type) {
    case muesli_value_string:
      return strcons(strlen(result.as_string), result.as_string);
    case muesli_value_float:
      return flocons(result.as_float);
    case muesli_value_boolean:
      if (result.as_int) {
	return cintern((char*)"t");
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

static LISP
get_parameters_siod()
{
  LISP names = option_names_table;
  LISP results = NIL;

  while (names != NIL) {
    if (CONSP(names)) {
      LISP pair = car(names);
      LISP result;
      
      
      
      

      handle_option(siod_interface->app_params,
		    (int)(FLONM(cdr(pair))), // opt
		    NULL, 0.0,		     // value
		    0,		     // set
		    &result_type, &result,
		    1);

      switch (result_type) {
      case muesli_value_string:
	if (result.as_string != NULL) {
	  result = strcons(strlen(result.as_string), result.as_string);
	} else {
	  result = NIL;
	}
	break;
      case muesli_value_float:
	result = flocons(result.as_float);
	break;
      case muesli_value_boolean:
	if (result.as_int) {
	  result = cintern((char*)"t");
	} else {
	  result = NIL;
	}
	break;
      default:
	result = NIL;
	break;
      }
      if (result != NIL) {
	results = cons(cons(car(pair), result), results);
      }
    }
    names = cdr(names);
  }
  return results;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives SIOD access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a SIOD muesli setup program.

static LISP
custom_eval_function_siod(LISP eval_in)
{
  if ((TYPEP(eval_in, tc_string)) || (TYPEP(eval_in, tc_symbol))) {
    evaluator_interface *custom_evaluator = muesli_get_named_evaluator("custom");
    return flocons((custom_evaluator->eval_individual)(custom_evaluator,
						       get_c_string(eval_in), 0));
  } else {
    fprintf(stderr, "custom_eval_function_siod must be given a string\n");
    exit(EXIT_USER_ERROR);
  }
}

///////////////////////////////////////////
// Call stack-based bytecode interpreter //
///////////////////////////////////////////

static LISP
siod_eval_bytecode_string(LISP args)
{
  LISP program = car(args);
  char *str = get_c_string(program);
  int nargs = 2;
  LISP l;
  evaluator_interface *stack_evaluator = muesli_get_named_evaluator("stack-code");

  (stack_evaluator->eval_clear)();

  for (l = cdr(args); NNULLP(l); l = cdr(l)) {
    LISP s = car(l);
    if (FLONUMP(s)) {
      (stack_evaluator->eval_give)(stack_evaluator, FLONM(s));
    } else {
      (stack_evaluator->eval_give)(stack_evaluator, 0.0);
    }
  }

  return flocons((stack_evaluator->eval_given)(stack_evaluator, str, strlen(str)));
}

/////////////////////////////////////////
// Call interpreter of hidden binaries //
/////////////////////////////////////////

static LISP
siod_binary(LISP args)
{
  // (LISP program, LISP arg1, LISP arg2)
  LISP program = car(args);
  char *str = get_c_string(program);
  LISP l;

  binary_eval_clear_args();

  for (l = cdr(args); NNULLP(l); l = cdr(l)) {
    LISP s = car(l);
    if (FLONUMP(s)) {
      binary_eval_give_arg(FLONM(s));
    } else {
      binary_eval_give_arg(0.0);
    }
  }
  return flocons(binary_eval_given_args());
}

////////////////
// Initialize //
////////////////

static int lisp_buffer_length = 4096;
static char *lisp_buffer;

void
siod_app_init(evaluator_interface *interface)
{
  int muesli_flags = interface->flags;

  if (muesli_flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "Initializing SIOD\n");
  }

  lisp_buffer = new char[lisp_buffer_length];

  // Init siod interface
  char *sargv[4];
  sargv[0] = (char*)"siod";
  sargv[1] = (char*)"-v0";
  sargv[2] = (char*)"-g0";
  sargv[3] = (char*)"-h100000:10";

  siod_init(4,sargv);

  // set up option names

  option_names_var = cintern((char*)"option-names");

  setvar(option_names_var, NIL, NIL);

  init_subr_2n((char*)"//",IntQuotient);
  init_subr_2n((char*)"%",Remainder);
  init_subr_1((char*)"closure?",ClosureP);
  init_subr_1((char*)"get-parameter", get_parameter_siod);
  init_subr_2((char*)"set-parameter", set_parameter_siod);
  init_subr_0((char*)"parameters", get_parameters_siod);
  init_subr_1((char*)"modify-parameters", set_parameters_siod);
  init_subr_1((char*)"custom", custom_eval_function_siod);
  init_lsubr((char*)"bytecode", siod_eval_bytecode_string);
  init_lsubr((char*)"binary", siod_binary);
  // todo: move these out, they belong in the application
  init_subr_1((char*)"add-to-grammar", add_to_grammar_siod);
  init_subr_0((char*)"get-grammar-string", get_grammar_string_siod);
  init_subr_2((char*)"data", data_item_siod);
  init_subr_0((char*)"data-rows", data_rows_loaded_siod);
  init_subr_2((char*)"sample", sample_siod);
  init_subr_3((char*)"make-data-block", make_data_block_siod);
  init_subr_3((char*)"set-data", set_data_siod);
  init_fsubr((char*)"taf", taf_siod);

  for (struct option *option_names = long_options;
       option_names->name != 0;
       option_names++) {
    option_names_table = cons(cons(cintern((char*)(option_names->name)),
				   flocons(option_names->val)),
			      option_names_table);
  }
  setvar(option_names_var, option_names_table, NIL);

  siod_verbose(cons(flocons(siod_interface->getLanguageVerbosity()), NIL));

  if (muesli_flags & TRACE_MUESLI_INIT) {
    fprintf(stderr, "Initialized SIOD\n");
  }
}

static void
siod_load_files(evaluator_interface *interface,
		vector<char *>functionsFiles)
{
  // Load functions needed by the individuals
  vector<char *>::iterator f_f;
  int muesli_flags = interface->flags;

  for (f_f = functionsFiles.begin();
       f_f != functionsFiles.end();
       f_f++) {
    if (muesli_flags & TRACE_MUESLI_LOAD) {
      fprintf(stderr, "Loading %s\n", *f_f);
    }
#if 0
#define LOAD_FORMAT "(*catch 'errobj (load \"%s\"))"
    int load_buf_len = strlen(*f_f) + strlen(LOAD_FORMAT);
    char *load_buf = (char*)alloca(load_buf_len);
    snprintf(load_buf, load_buf_len, LOAD_FORMAT, *f_f);
    fprintf(stderr, "evaluating %s\n", load_buf);
    char *string_back = NULL;
    int string_back_used = 0;
    siod_eval_string(load_buf, NULL, NULL, &string_back, &string_back_used, NULL, NULL);
    if (string_back_used) {
      fprintf(stderr, "Got back string %s\n", string_back);
    }
#else
    // todo: protect this with a catch frame
    fprintf(stderr, "calling plain siod vload\n");
    vload(*f_f, 0, 1);
#endif
    if (muesli_flags & TRACE_MUESLI_LOAD) {
      fprintf(stderr, "Loaded %s\n", *f_f);
    }
  }
}

////////////////
// Evaluation //
////////////////

static result_type_t
siod_eval_string(const char *scratch,
		 unsigned int string_length,
		 eval_result_t *result_p)
{
  if (verbosity > standard) {
    cout << "SIOD eval form: " << scratch << "\n";
  }

  // Construct a lisp reader that will read from the scratch string
  const char *p;
  struct gen_readio s;
  p = scratch;
  s.getc_fcn = (int (*)(void *))rfs_getc;
  s.ungetc_fcn = (void (*)(int,void *))rfs_ungetc;
  s.cb_argument = (char *) &p;

  // Use the reader we have made to make a Lisp value from the string
  LISP read_result = (readtl(&s));

  // Evaluate that Lisp value
  // possible todo: wrap this in a "catch frame" (see leval_catch, leval_catch_1 in siod/slib.c)
  LISP eval_result = leval(read_result, NIL);

  switch (TYPE(eval_result)) {
  case tc_string:
    if (result_p != NULL) {
      *result_p->as_string = get_c_string(eval_result);
    }
    return muesli_value_string;
  case tc_nil:
    if (result_p != NULL) {
      *result_p->as_int = 0;
    }
    return muesli_value_int;
  case tc_symbol:
    if (result_p != NULL) {
      *result_p->as_int = 1;
    }
    return muesli_value_int;
  case tc_flonum:
    if (result_p != NULL) {
      *result_p->as_float = FLONM(eval_result);
    }
    return muesli_value_float;
  }
}

static float
siod_eval_individual(evaluator_interface *interface,
		     const char *scratch, unsigned int length, beast_id_t id)
{
  score_t siod_fitness;

  int in_len = strlen(scratch);
  if ((in_len + 1) > lisp_buffer_length) {
    lisp_buffer_length = in_len + 1;
    /* todo: fix valgrind's complaint about this delete
       ==4467== Mismatched free() / delete / delete []
       ==4467==    at 0x401BCBC: operator delete(void*) (vg_replace_malloc.c:244)
       ==4467==    by 0x806D288: siod_eval_individual(char const*, unsigned) (grevo-siod.cpp:504)
       ==4467==    by 0x8063968: generic_objfunc(int*, char*, unsigned short*, unsigned) (main.cpp:1338)
       ==4467==    by 0x806B2ED: UlgaInitFuncSI(beast*, ULGAparams*, unsigned char**) (initfunc.cpp:89)
       ==4467==    by 0x806AA19: ULGAGeneration::initialize(unsigned) (ULGAGeneration.cpp:132)
       ==4467==    by 0x805F6C5: late_setup(populationHolder*, GAParameterList&, GEListGenome) (main.cpp:3018)
       ==4467==    by 0x8064AD7: main (main.cpp:3285)
       ==4467==  Address 0x44AF320 is 0 bytes inside a block of size 4,096 alloc'd
       ==4467==    at 0x401C7C1: operator new[](unsigned) (vg_replace_malloc.c:195)
       ==4467==    by 0x806D0F0: __static_initialization_and_destruction_0(int, int) (grevo-siod.cpp:421)
       ==4467==    by 0x80B5104: (within /home/jcgs/common/open-projects/grevo/grevo-0.0/src/grevo)
       ==4467==    by 0x805D0E8: (within /home/jcgs/common/open-projects/grevo/grevo-0.0/src/grevo)
       ==4467==    by 0x80B5098: __libc_csu_init (in /home/jcgs/common/open-projects/grevo/grevo-0.0/src/grevo)
       ==4467==    by 0x4384E5C: (below main) (in /lib/tls/i686/cmov/libc-2.3.6.so)
    */
    delete lisp_buffer;
    lisp_buffer = new char[lisp_buffer_length];
  }

  strcpy(lisp_buffer, scratch);

  if (verbosity > standard) {
    cout << "SIOD form: " << scratch << "\n";
  }

  // todo: use `leval' directly, or possibly via `leval_catch', as in siod_eval_string
  if (repl_c_string(lisp_buffer, 1, 0, lisp_buffer_length - 1)) {
    // Siod error processing text in buffer
    cout << "SIOD error in: ";
    cout << lisp_buffer << endl;
    siod_fitness=0.0;
  } else {
    if (verbosity > standard) {
      cout << "SIOD result: " << lisp_buffer << "\n";
    }
    siod_fitness = atof(lisp_buffer);
  }
  return siod_fitness;
}

void
siod_setup(evaluator_interface *new_siod_interface)
{
  siod_interface = new_siod_interface;

  siod_interface->eval_individual = siod_eval_first_individual;
  siod_interface->eval_string = siod_eval_string;
  siod_interface->load_files = siod_load_files;
  siod_interface->array_separator = (char*)" ";
}

#endif
#endif
#endif
#endif
