// muesli-exec.c -*- C -*-
/* muesli interface to exec'ing external programs
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
#ifndef _MUESLI_EXEC_
#define _MUESLI_EXEC_

#include "../config.h"
#include "muesli.h"

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define EXEC_VERSION "0.0"

// todo: TAF functions

// todo: reflection operators? (i.e. allow mutation rate etc to be examined and set from interpreted code)

//Initialises the mapper, and the interpreter

void
exec_error()
{
  fprintf(stderr, "Error in muesli-exec\n");
  exit(1);
}

void
exec_evaluator_init(evaluator_interface *interface)
{

  // Init exec interface

  // todo: process option names
}

static void
exec_load_file(evaluator_interface *interface,
		const char *filename)
{
  int muesli_flags = interface->flags;

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
  fprintf(stderr, "Functions file %s was given to the external evaluator, which does not take functions files\n",
	  filename);
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

#define RESULT_MAX 1024

static muesli_value_t
exec_eval_string(evaluator_interface *interface,
		 const char *exec_c_string,
		 unsigned int string_length,
		 int transient)
{
  char **argv;
  int argc;
  int tracing = (interface->flags & TRACE_MUESLI_CHILD_PROCESS);

  muesli_value_t result;
  ANULL_VALUE(result);

  muesli_split_args(exec_c_string, interface->underlying_program, &argv, &argc);

  fprintf(stderr, "Split \"%s\" into %d parts\n", exec_c_string, argc);

  int pfd[2];
  pid_t cpid;

  // Create a pair of pipes
  if (pipe(pfd) == -1) { perror("pipe"); exec_error(); }

  // Split off subprocess to run command

  cpid = fork();
  if (cpid == -1) { perror("fork"); exec_error(); }

  if (cpid == 0) {

    ///////////////////
    // child process //
    ///////////////////

    // set up stdout
    close(1);			// man dup2 recommends closing the old fd first
    if (dup2(pfd[1], 1) == -1) {
      perror("dup"); exec_error();
    }
    close(pfd[0]);
    close(pfd[1]);

    if (tracing) {
      fprintf(stderr, "Child process about to execve[%s]\n", argv[0]);
    }

    execve(argv[0], argv, NULL);

  } else {

    ////////////////////
    // parent process //
    ////////////////////

    char result_buf[RESULT_MAX];
    char *p = result_buf;
    close(pfd[1]);

    if (tracing) {
      fprintf(stderr, "Parent waiting for child process to finish\n");
    }
    waitpid(cpid, NULL, 0);

    if (tracing) {
      fprintf(stderr, "Child process finished\n");
    }
    free(argv[0]);
    free(argv);
    if (read(pfd[0], result_buf, RESULT_MAX) > 0) {
      while ((!isdigit(*p))
	     && (*p != '-')
	     && (p < result_buf+RESULT_MAX)); // skip
      if (p < result_buf+RESULT_MAX) {
	result.data.as_float = strtof(p, NULL);
	result.type = muesli_value_float;
	close(pfd[0]);
	close(pfd[1]);
	return result;
      } else {
	result_buf[RESULT_MAX - 1] = '\0';
	result.data.as_string = muesli_malloc_copy_string(result_buf);
	result.type = muesli_value_string;
	close(pfd[0]);
	close(pfd[1]);
	return result;
      }
    }
    close(pfd[0]);
    close(pfd[1]);
  }
  return result;
}

void
external_exec_setup(evaluator_interface *new_exec_interface)
{
  new_exec_interface->eval_string = exec_eval_string;
  new_exec_interface->load_file = exec_load_file;

  new_exec_interface->version = EXEC_VERSION;
}

#endif

