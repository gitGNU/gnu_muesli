// muesli-pipe.c -*- C -*-
/* muesli interface to talking to an evaluator by pipes
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

#ifndef _MUESLI_PIPE_
#define _MUESLI_PIPE_


#include "../config.h"
#include "muesli.h"

#include <limits.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

// #define TRACE_PIPE_EVALS 1
// #define TRACE_PIPE_DATA 1

#define PIPE_VERSION "0.0"

int muesli_to_app_fd;
int muesli_from_app_fd;

// static evaluator_interface *pipe_interface = NULL;

static void
muesli_pipe_error()
{
  exit(1);
}

static void
send_file_down_pipe(const char *filename)
{
  int ff = open(filename, O_RDONLY);
  if (ff < 0) {
    fprintf(stderr, "Could not open file %s to pipe to application\n", filename);
    muesli_pipe_error();
  } else {
    struct stat stat_buf;
    if (fstat(ff, &stat_buf) != 0) {
      fprintf(stderr, "Could not stat file %s to pipe to application\n", filename);
      muesli_pipe_error();
    } else {
      int file_size = stat_buf.st_size;
      char *contents_buf = (char*)malloc(file_size);
      if (read(ff, contents_buf, file_size) != file_size) {
	fprintf(stderr, "Could not read all of file %s to pipe to application\n", filename);
	muesli_pipe_error();
      }
      write(muesli_to_app_fd, contents_buf, file_size);
      free(contents_buf);
      close(ff);
    }
  }
}

static void
read_up_to(evaluator_interface *interface,
	   char *flagstring)
{
  char skipping_buf[1];
  char *matched = flagstring;

  int muesli_flags = interface->flags;

  if (muesli_flags & TRACE_MUESLI_CHILD_PROCESS) {
    fprintf(stderr, "parent process looking for \"%s\" from child\n", flagstring);
  }
  while (1) {
    read(muesli_from_app_fd, skipping_buf, 1);
#ifdef TRACE_PIPE_DATA
    fprintf(stderr, "{%c=%c?}", *skipping_buf, *matched);
#endif
    if (*skipping_buf == *matched) {
      matched++;
#ifdef TRACE_PIPE_DATA
      fprintf(stderr, "{{another}}");
#endif
      if (*matched == '\0') {
#ifdef TRACE_PIPE_DATA
      fprintf(stderr, "{{all}}");
#endif
	return;
      }
    } else {
#ifdef TRACE_PIPE_DATA
      fprintf(stderr, "{{reset}}");
#endif
      matched = flagstring;
    }
  }
}

static float
read_number_from_pipe()
{
#define RESULT_MAX 1024
  char result_buf[RESULT_MAX];
  char reading_buf[1];
  unsigned int i = 0;
  int number_seen = 0;

  while (1) {
    read(muesli_from_app_fd, reading_buf, 1);
#ifdef TRACE_PIPE_DATA
    fprintf(stderr, "[%c]", reading_buf[0]);
#endif
    switch (reading_buf[0]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      result_buf[i++] = reading_buf[0];
      number_seen = 1;
      break;
    case '-':
    case '+':
      result_buf[i++] = reading_buf[0];
      break;
    case 'e': case 'E':
      if (number_seen) {
	result_buf[i++] = reading_buf[0];
      }
      break;
    default:
      if (number_seen) {
	result_buf[i] = '\0';
#ifdef TRACE_PIPE_DATA
	fprintf(stderr, "[[%s]]", result_buf);
#endif
	return atof(result_buf);
      } else {
	i = 0;			// in case we got a - without a number
	continue;
      }
    }
  }

  return 0.0;
}

static void
pipe_await_initialization(evaluator_interface *interface)
{
  if (interface->underlying_startup_string) {
    read_up_to(interface, interface->underlying_startup_string);
    if (interface->flags & TRACE_MUESLI_CHILD_PROCESS) {
      fprintf(stderr, "parent has observed child initialization\n");
    }
  }
}

//Initialises the mapper, and the interpreter

void
pipe_evaluator_init(evaluator_interface *interface)
{
  // Init pipe interface1
  int to_app_fds[2];
  int from_app_fds[2];

  if (pipe(to_app_fds) == -1) { perror("command pipe"); muesli_pipe_error();}
  if (pipe(from_app_fds) == -1) { perror("result pipe"); muesli_pipe_error(); }

  interface->underlying_evaluator_pid = fork();

  if (interface->underlying_evaluator_pid == -1) { perror("fork"); muesli_pipe_error(); }

  if (interface->underlying_evaluator_pid == 0) {
    // child process
    // set up stdin
    if (dup2(to_app_fds[0], 0) == -1) {
      perror("dup command pipe"); muesli_pipe_error();
    }
    close(to_app_fds[1]);
    // set up stdout
    if (dup2(from_app_fds[1], 1) == -1) {
      perror("dup result pipe"); muesli_pipe_error();
    }
    close(from_app_fds[0]);

    char **argv;
    int argc;

    muesli_split_args(interface->underlying_program, NULL, &argv, &argc);
    if (interface->flags & TRACE_MUESLI_CHILD_PROCESS) {
      fprintf(stderr, "child process about to exec %s\n", argv[0]);
    }
    // todo: search $PATH
    execve(argv[0], argv, NULL);
  } else {
    // Continuing in application process
    muesli_to_app_fd = to_app_fds[1];
    muesli_from_app_fd = from_app_fds[0];
    close(to_app_fds[0]);
    close(from_app_fds[1]);
  }
}

static void
pipe_load_file(evaluator_interface *interface,
	       const char* filename)
{
  int muesli_flags = interface->flags;

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
  if (interface->flags & TRACE_MUESLI_CHILD_PROCESS) {
    fprintf(stderr, "parent process piping contents of %s to child\n", filename);
  }
  send_file_down_pipe(filename);
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

static muesli_value_t
pipe_eval_string(evaluator_interface *interface,
		 const char *pipe_c_string,
		 unsigned int string_length,
		 int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (interface->flags & TRACE_MUESLI_CHILD_PROCESS) {
    fprintf(stderr, "parent sending \"%s\" to child for evaluation\n", pipe_c_string);
  }
  if (pipe_c_string) {
    write(muesli_to_app_fd, pipe_c_string, strlen(pipe_c_string));
    if (interface->underlying_prompt_string) {
      read_up_to(interface,
		 interface->underlying_prompt_string);
    }
  }

  // todo: get other types of result too
  result.data.as_float = read_number_from_pipe();
  result.type = muesli_value_float;
  return result;
}

void
external_pipe_setup(evaluator_interface *new_pipe_interface)
{
  new_pipe_interface->eval_string = pipe_eval_string;
  new_pipe_interface->load_file = pipe_load_file;

  new_pipe_interface->version = PIPE_VERSION;
}

#endif

