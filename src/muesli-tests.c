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
#ifndef _MUESLI_TESTS_
#define _MUESLI_TESTS_

#include "../config.h"
#include "muesli.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/times.h>
#include <unistd.h>

/**************/
/* Self-tests */
/**************/

static int
show_self_test_results(const char *label,
		       evaluator_interface *evaluator,
		       float reference_value,
		       muesli_value_t result)
{
  fprintf(stderr, "Test of %s evaluator:\n", label);
  if (evaluator == NULL) {
    fprintf(stderr, "  Not present\n");
    return 1;
  } else {
    switch (result.type) {
    case muesli_value_none:
      fprintf(stderr, "The evaluator returned no result\n");
      break;
    case muesli_value_float:
      fprintf(stderr, "  Number result is %f (%s)\n",
	      result.data.as_float,
	      (result.data.as_float == reference_value)
	      ? "right" : "wrong");
      break;
    case muesli_value_string:
      fprintf(stderr, "  String result is %s\n", result.data.as_string);
      break;
    case muesli_value_boolean:
      fprintf(stderr, "  Boolean result is %s\n", result.data.as_int ? "true" : "false");
      break;
    default:
      break;
    }
    return ((result.data.as_float == reference_value)
	    && (result.type == muesli_value_float));
  }
}

typedef float (*mc_test_function_t)(float, float, float);

typedef struct evaluator_test {
  char *name;
  char *test_string;
  int reference;
  int verbosity;
} evaluator_test;

static int
test_evaluator(evaluator_test *test, int seconds, float *evals_per_second_p, float seconds_per_base_loop)
{
  char *name = test->name;
  char *test_string = test->test_string;
  int reference = test->reference;
  int verbosity = test->verbosity;
  evaluator_interface *evaluator = muesli_get_named_evaluator(name, 0);

  muesli_value_t result;
  result.data.as_float = 0.0;

  if (evaluator != NULL) {
    if (seconds > 0) {
      int ticks_per_sec = sysconf(_SC_CLK_TCK);

      printf("Timing %s on \"%s\" for %d seconds\n", name, test_string, seconds);

#if 1
      // You won't often want this one enabled, I'd wager; but it can
      // be useful for tracing gradually leaking resources, etc:
      evaluator->language_verbosity = verbosity;
#endif
      struct  tms eval_time;
       times(&eval_time);
      clock_t start_time = eval_time.tms_utime + eval_time.tms_stime + eval_time.tms_cutime + eval_time.tms_cstime;
      clock_t now_time = start_time;
      clock_t end_time = start_time + seconds * ticks_per_sec;
      int count = 0;
      while (now_time < end_time) {
	result = (evaluator->eval_string)(evaluator,
					  test_string,
					  strlen(test_string),
					  0);
	count++;
	times(&eval_time);
	now_time = eval_time.tms_utime + eval_time.tms_stime + eval_time.tms_cutime + eval_time.tms_cstime;
	if (result.type == muesli_value_none) {
	  printf("Problem in timing \"%s\"; it failed to return a result\n", name);
	  break;
	}
      }
      float evals_per_second = ((float)count) / (float)seconds;
      float seconds_per_eval = 1.0 / evals_per_second;
      float adjusted_seconds_per_eval = seconds_per_eval - seconds_per_base_loop;
      float adjusted_evals_per_second = 1.0 / adjusted_seconds_per_eval;

      printf("%s: %7.0f per second (raw: %7.0f)\n", name, adjusted_evals_per_second, evals_per_second);
      if (evals_per_second_p) {
	*evals_per_second_p = evals_per_second;
      }
    } else {
      printf("Testing %s on \"%s\" at verbosity %d\n", name, test_string, verbosity);
      evaluator->language_verbosity = verbosity;
      result = (evaluator->eval_string)(evaluator,
					test_string,
					strlen(test_string),
					0);
    }
  } else {
    printf("Did not find an evaluator called \"%s\"\n", name);
  }
  return show_self_test_results(name, evaluator, reference, result);
}

evaluator_test evaluator_zero_test = { (char*)"zero", (char*)"", 0, 0};

evaluator_test evaluator_tests[] = {
#if 0
  {(char*)"exec", (char*)"dc -e \"4 3 * 2 + p q\"", 14, 0},
#endif
#if 0
  {(char*)"pipe", (char*)"", 14, 0},
  {(char*)"custom", (char*)"", 14, 0},
#endif
#if defined(HAVE_LIBSIOD) || defined(HAVE_LIBULSIOD)
  {(char*)"siod", (char*)"(+ (* 4 3) 2)", 14, 0},
#endif
#ifdef HAVE_LIBPERL
  {(char*)"perl", (char*)"return (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBLUA
  {(char*)"lua", (char*)"return (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIB_SLANG
  {(char*)"s-lang", (char*)"return (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBPYTHON
  {(char*)"python", (char*)"return (4 * 3 + 2)", 14, 0}, // todo: what is the syntax of this?
#endif
#ifdef HAVE_LIBTCL
  {(char*)"tcl", (char*)"expr (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBTCC0
  {(char*)"tcc", (char*)"expr (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBOCTAVE
  {(char*)"octave", (char*)"(4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBMOZJS
  {(char*)"javascript", (char*)"(4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBSLANG
  {(char*)"slang", (char*)"expr (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBPARROT
  {(char*)"parrot", (char*)"expr (4 * 3 + 2)", 14, 0},
#endif
#ifdef HAVE_LIBJVM
  {(char*)"jvm", (char*)"expr (4 * 3 + 2)", 14, 0},
#endif
  {(char*)"stack-code", (char*)"cde*+", 14, 2},
  {NULL, NULL, 0}
};

int
muesli_test_evaluators(int seconds)
{
#if 0
  const char* test_string ="(load \"eval-test.scm\" nil t\n(run-tests eval-tests)\n";
  muesli_eval_in_language("siod", test_string, strlen(test_string), NULL);
#else
  int all_ok = 1;
  int i;

  muesli_value_t result;

  evaluator_interface *evaluator;

  // verbosity = verbose;

  float base_loops_per_second;
  float seconds_per_base_loop;

  test_evaluator(&evaluator_zero_test, seconds, &base_loops_per_second, 0.0);

  seconds_per_base_loop = 1.0 / base_loops_per_second;

  for (i = 0; evaluator_tests[i].name != NULL; i++) {
    test_evaluator(&evaluator_tests[i], seconds, NULL, seconds_per_base_loop);
  }

#if 0
  evaluator = get_named_evaluator("stack-code");

  if (evaluator != NULL) {
    int all_fns_present = 1;
    if (evaluator->eval_clear == NULL) {
      fprintf(stderr, "No eval_clear defined for stack-code\n");
      all_fns_present = 0;
    }
    if (evaluator->eval_give == NULL) {
      fprintf(stderr, "No eval_give defined for stack-code\n");
      all_fns_present = 0;
    }
    if (evaluator->eval_given == NULL) {
      fprintf(stderr, "No eval_given defined for stack-code\n");
      all_fns_present = 0;
    }
    if (all_fns_present) {

      evaluator->setLanguageVerbosity(2);

      evaluator->eval_clear(evaluator);
      evaluator->eval_give(evaluator, 2.0);
      evaluator->eval_give(evaluator, 3.0);
      evaluator->eval_give(evaluator, 4.0);
      result.data.as_float = evaluator->eval_given(evaluator, "*+", 2);
      result.type = muesli_value_float;
    } else {
      result.type = muesli_value_none;
    }

  }
  all_ok &= show_self_test_results("stack-bytecode", evaluator, 14, result);
#endif
  evaluator = muesli_get_named_evaluator("machine", 0);
  if (evaluator != NULL) {
    /*
      float
      test_fn(float a, float b, float c)
      {
      return a * b + c;
      }

      test_fn:
      55       		pushl	%ebp
      89E5     		movl	%esp, %ebp
      D94508   		flds	8(%ebp)
      D84D0C   		fmuls	12(%ebp)
      D84510   		fadds	16(%ebp)
      5D       		popl	%ebp
      C3       		ret
    */
    char machine_eval_test_string[] = {
      0x55,			// pushl %ebp
      0x89, 0xE5,		// movl	%esp, %ebp
      0xD9, 0x45, 0x08,		// flds	8(%ebp)
      0xD8, 0x4D, 0x0C,		// fmuls 12(%ebp)
      0xD8, 0x45, 0x10,		// fadds 16(%ebp)
      0x5D,			// popl	%ebp
      0xC3			// ret
    };
#if 1
    result.data.as_float = ((mc_test_function_t)(machine_eval_test_string))(3.0, 4.0, 2.0);
#else
    result = (evaluator->eval_string)(evaluator,
				      machine_eval_test_string,
				      sizeof(machine_eval_test_string), 0);
#endif
  }
  all_ok &= show_self_test_results("machine", evaluator, 14, result);

  printf(all_ok ? "All tests passed\n" : "At least one test failed\n");
#endif
  return all_ok;
}

#endif

/* muesli-tests.c ends here */
