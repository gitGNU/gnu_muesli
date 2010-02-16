/* muesli main header file
   Copyright (C) 2008, 2009 University of Limerick

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

#ifndef MUESLI_INT_H
#define MUESLI_INT_H

#include <stdio.h>
#include <unistd.h>

extern const char *muesli_version;

#define TRACE_MUESLI_INIT              0x0010
#define TRACE_MUESLI_LOAD              0x0020
#define TRACE_MUESLI_EVAL              0x0040
#define TRACE_MUESLI_CHILD_PROCESS     0x0080

typedef enum {
  muesli_value_none,
  muesli_value_float,
  muesli_value_integer,
  muesli_value_string,
  muesli_value_boolean,
  muesli_value_error
} value_type_tag_t;

typedef enum {
  private_storage,
  malloced_by_language,
  malloced_by_muesli
} value_storage_t;

typedef struct muesli_value_t {
  union {
    float as_float;
    const char *as_string;
    int as_int;
    int as_bool;
  } data;
  value_type_tag_t type;
  value_type_tag_t element_type;
  value_storage_t storage;
} muesli_value_t;

#define ANULL_VALUE(_v_) \
  (_v_).type = muesli_value_none; \
  (_v_).element_type = muesli_value_none; \
  (_v_).storage = private_storage;

typedef enum {
  silent,
  quiet,
  standard,
  verbose 
} verbosity_level;

extern char *muesli_result_type_name(value_type_tag_t enumerated);

/* Pre-declare the main structure: */
struct evaluator_interface;

/* Access to `binary' (non-textual) languages.  You can't supply
   arguments to program fragments for them by substituting them in
   with sprintf etc, so we provide a generic way to give them
   arguments on a stack.
 */
typedef void (*binary_clear_args_function_t)(struct evaluator_interface *interface);
typedef void (*binary_give_arg_function_t)(struct evaluator_interface *interface,
					   float arg);
typedef float (*binary_eval_given_args_function_t)(struct evaluator_interface *interface,
						   const char *scratch,
						   unsigned int length);
typedef float (*binary_0_evaluator_function_t)(struct evaluator_interface *interface,
					       const char *scratch,
					       unsigned int length);
typedef float (*binary_1_evaluator_function_t)(struct evaluator_interface *interface,
					       const char *scratch,
					       unsigned int length,
					       float);
typedef float (*binary_2_evaluator_function_t)(struct evaluator_interface *interface,
					       const char *scratch,
					       unsigned int length,
					       float, float);

/* Types of functions for setting up callbacks to your application. */

typedef void (*evaluator_specializer_t)(struct evaluator_interface *interface);
typedef void (*evaluator_function_adder_t)(struct evaluator_interface *interface,
					   int arity,
					   const char *name,
					   void *function);

/* Interface to command-line-like options */

typedef muesli_value_t (*option_handler_t)(void *params_block,
					   char opt,
					   /* const */ char *value_string, float value_number,
					   int set,
					   const char *from_language);

/* The main interface to evaluating bits of program text: */

typedef muesli_value_t (*string_evaluator_function_t)(struct evaluator_interface *evaluator_interface,
						      const char *scratch,
						      unsigned int string_length,
						      int transient);

/* Our main data structure: */

typedef struct evaluator_interface {
  struct evaluator_interface *next;

  void *state;

  void *app_params;
  void *app_data;

  /* This is called when we want to initialize the interpreter itself
     (rather than just the interface that holds the interpreter): */
  void (*init_evaluator)(struct evaluator_interface *myself);

  /* This is like an extra initializer, but it filled in by the
     application: */
  evaluator_specializer_t specializer;

  /* This adds a primitive / native function to the interpreter: */
  evaluator_function_adder_t add_function;

  /* A function to shut the evaluator down: */
  void (*close_evaluator)(struct evaluator_interface *myself);

  /* The main callables: */
  string_evaluator_function_t eval_string;
  void (*load_file)(struct evaluator_interface *interface, const char *filename);

  /* The Read-Eval-Print Loop for this interpreter, if it has one: */
  void (*repl)(struct evaluator_interface *myself);

  /* How array elements are separated: */
  char *array_separator;

  /* The application parameter handler.  Each interpreter interface
     defines functions for getting and setting parameters, either
     singly or in a group; this is the hook those functions call to
     get the parameter data back to the application: */
  option_handler_t handle_option;
  struct option *getopt_options;

  /* These are used only for binary evaluators */
  binary_0_evaluator_function_t eval_0;
  binary_clear_args_function_t eval_clear;
  binary_give_arg_function_t eval_give;
  binary_eval_given_args_function_t eval_given;

  /* For the piped evaluator: */
  char *underlying_program;
  char *underlying_startup_string;
  char *underlying_prompt_string;
  pid_t underlying_evaluator_pid;

  char *name;
  char *extension;
  int binary;
  int initialized;
  int language_verbosity;
  int flags;
  char *version;
} evaluator_interface;

#define CLI_BUF_SIZE 1024

extern evaluator_interface *muesli_make_evaluator_interface(const char* new_name,
							    const char *new_extension,
							    int new_is_binary,
							    void (*setup_func)(evaluator_interface *myself),
							    void (*init_func)(evaluator_interface *myself),
							    option_handler_t new_option_handler,
							    struct option *new_getopt_options,
							    void* params,
							    void *data);

extern evaluator_interface *muesli_define_evaluator(const char *new_name,
						    const char *new_extension,
						    int new_is_binary,
						    void (*new_setup)(evaluator_interface *myself),
						    void (*new_init)(evaluator_interface *myself),
						    option_handler_t new_option_handler,
						    struct option *new_getopt_options,
						    void *new_params,
						    void *new_data);

extern muesli_value_t eval_string(evaluator_interface *evaluator_interface,
				  const char *program_fragment,
				  unsigned int fragment_length,
				  int transient);

extern int muesli_load_file(const char *filename);

extern void muesli_initialize_evaluator(evaluator_interface *interface);
extern char* muesli_getName(evaluator_interface *interface);
extern int muesli_getBinary(evaluator_interface *interface);

extern muesli_value_t muesli_eval_in_language(const char *language,
					      const char *fragment,
					      unsigned int fragment_length,
					      int transient);

extern void muesli_print_interpreter_names(FILE *stream,
					   const char *format,
					   const char *selected_name,
					   const char *selected_flag);

extern void muesli_evaluator_repl(const char* evaluator_name);

/* Managing the collection of evaluators */

extern void muesli_register_evaluators(void *new_app_params,
				       void *new_app_data,
				       option_handler_t params_handler,
				       struct option *getopt_options);

extern int muesli_set_parameter(evaluator_interface *interface,
				struct option *options,
				char* option,
				char *value,
				const char *language);
extern const char* muesli_get_parameter(evaluator_interface *interface,
					struct option *options,
					char* option);

extern char *muesli_malloc_copy_string(const char *original);

extern evaluator_interface *muesli_get_named_evaluator(const char *evaluator_name,
						       int exit_if_not_found);
extern int muesli_evaluator_is_defined(const char *evaluator_name);
extern evaluator_interface *muesli_get_evaluator_by_extension(const char *extension);
extern const char *muesli_get_language_by_extension(const char *extension);

extern void muesli_app_specialize(char *name, evaluator_interface *interface);

extern void muesli_set_evaluator_app_specializer(char *evaluator_name,
						 evaluator_specializer_t specializer);

/* Some convenience macros for applications adding functions in their specializers: */

#define ARGS_COUNT_VARIABLE -2
#define ARGS_NO_EVAL -1

#define Muesli_Add_Fn_0(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 0, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_1(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 1, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_2(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 2, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_3(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 3, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_4(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 4, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_5(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), 5, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_V(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), ARGS_COUNT_VARIABLE, (char*)(_name_), (void*)(_function_))
#define Muesli_Add_Fn_N(_interface_,_name_,_function_) ((_interface_)->add_function)((_interface_), ARGS_NO_EVAL, (char*)(_name_), (void*)(_function_))

/* Testing and timing */

extern int muesli_test_evaluators(int seconds);

/* Talking to application option systems */

extern char muesli_find_option_letter(struct option *getopt_options, const char *option_name);
extern void muesli_split_args(const char *string, const char *string0, char ***new_argv, int *new_argc);

/* Customizers for muesli's built-in evaluators */
extern void muesli_machine_code_add_function(int fn_number, float (*function)(float));

/* Things we call in each evaluator to do its internal
   initialization */

extern void exec_evaluator_init(evaluator_interface *interface);
extern void pipe_evaluator_init(evaluator_interface *interface);
extern void custom_evaluator_init(evaluator_interface *interface);
extern void zero_evaluator_init(evaluator_interface *interface);
#if defined(HAVE_LIBSIOD) || defined(HAVE_LIBULSIOD)
extern void siod_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBLUA
extern void lua_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBPYTHON
extern void python_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBTCL
extern void tcl_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBPERL
extern void perl_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBTCC
extern void tcc_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBSLANG
extern void slang_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBOCTINTERP
extern void octave_evaluator_init(evaluator_interface *interface);
#endif
#ifdef HAVE_LIBMOZJS
extern void javascript_evaluator_init(evaluator_interface *interface);
#endif
extern void machine_code_evaluator_init(evaluator_interface *interface);
extern void stack_bytecode_evaluator_init(evaluator_interface *interface);
extern void binary_init();

/* The muesli-<language-name>.c files define the following setup
   functions.  These are NOT initialization functions for the
   associated interpreters; they just make some connections.  The
   interpreters get initialized when they are asked for by name,
   i.e. in get_named_evaluator(string).  Instead of modifying these
   functions to add your application-specific language extensions, you
   should use set_evaluator_app_specializer(string,
   specializer_function) to set a function to add your extensions.
   This is called when the interpreter is initialized, which happens
   when you call get_named_evaluator, so you must call
   set_evaluator_app_specializer before then.  */
extern void siod_setup(evaluator_interface*);
extern void lua_setup(evaluator_interface*);
extern void external_exec_setup(evaluator_interface *new_exec_interface);
extern void external_pipe_setup(evaluator_interface *new_pipe_interface);
extern void custom_setup(evaluator_interface *new_custom_interface);
extern void zero_setup(evaluator_interface *new_custom_interface);
extern void perl_setup(evaluator_interface *new_perl_interface);
extern void python_setup(evaluator_interface *new_python_interface);
extern void tcl_setup(evaluator_interface *new_tcl_interface);
extern void tcc_setup(evaluator_interface *new_tcc_interface);
extern void octave_setup(evaluator_interface *new_octave_interface);
extern void slang_setup(evaluator_interface *new_slang_interface);
extern void parrot_setup(evaluator_interface *new_parrot_interface);
extern void jvm_setup(evaluator_interface *new_jvm_interface);
extern void javascript_setup(evaluator_interface *new_jvm_interface);
extern void machine_code_setup(evaluator_interface *new_machine_interface);
extern void stack_code_setup(evaluator_interface *new_stack_code_interface);

#endif

/* muesli.h ends here */
