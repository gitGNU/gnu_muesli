#ifndef MUESLI_CUSTOM_H
#define MUESLI_CUSTOM_H

/* Interface to custom evaluators / template for new language interfaces
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


#define CUSTOM_VERSION "0.0"

// How to customize:

// Either replace this whole lot of "custom_*" things, wherever they
// are called, with the corresponding thing from your language or
// application, or fill them in with the glue code needed.  This
// collection of typedefs, variable and functions defines the
// interface between an application and a programming language or specialized
// evaluator library.

// If you're adding another interpreted programming language, you
// should probably define equivalents for all of them; if doing a
// specialized application that isn't a general programming language,
// you can probably get away with leaving them as they are, and just
// filling in the function custom_eval_individual.

// Other things to do when adding a language are to take a copy of
// this file rather than doing it in-place (as the core system expects
// the original to be there) and do s/custom_/$MYLANGUAGE_/g on the
// copy (except for the built-in function called "custom", and its
// call to custom_eval_individual), and to wrap this file in an "ifdef
// WITH_$MYLANGUAGE" so that it can be included conditionally at
// compile time.  You might as well take this comment block out of the
// copy, too, to avoid confusing others later.

// Really lazy hacking technique: switch this block of definitions off
// by using the ifndef above; try compiling, and fix the errors it
// gives you by replacing the call to each missing function by a call
// to a function in your new interpreter.

// Here are the individual bits you'll have to replace or fill in:

// An evaluator (interpreter) will typically have a global, or
// nearly-global, state-holder, and some types of things that hang off
// that.  We store the state in the slot provided in the
// evaluator_interface.
typedef struct custom_state {
  int nothing;
} custom_state;
typedef struct custom_table {
  int count;
} custom_table;
typedef struct custom_symbol {
  char *name;
} custom_symbol;

custom_state *custom_create_custom();

// Getting args from your scripting language stack:
extern int custom_count_args(custom_state *S);
extern int custom_number_arg(custom_state *S, int argi);
extern char *custom_string_arg(custom_state *S, int argi);
extern int custom_arg_is_string(custom_state *S, int argi);
extern int custom_arg_is_number(custom_state *S, int argi);

// Putting results back onto your scripting language stack:
extern void custom_return_string(custom_state *S, const char *string_back);
extern void custom_return_number(custom_state *S, float number_back);
extern void custom_return_boolean(custom_state *S, int int_back);

// Handling table / map / alist structures of your scripting language:
extern custom_table *custom_create_table(custom_state *S, int size);
extern void custom_set_table_string(custom_state *S, custom_table *T, char *key, const char *value);
extern void custom_set_table_number(custom_state *S, custom_table *T, char *key, float value);
extern void custom_set_table_boolean(custom_state *S, custom_table *T, char *key, int value);

// Make a name / symbol of the scripting language:
extern custom_symbol *custom_make_symbol(custom_state *S, const char *n);

// Non-local exit in the scripting language
extern void custom_throw(custom_state *S, custom_symbol *Y, float n);

// Load and run a scripting language file:
static void custom_load_file(evaluator_interface *interface, const char *filename);

// Set a global variable of the scripting language to contain a table:
extern void custom_set_global_variable_table(custom_state *S, custom_symbol *varname, custom_table *table);

// Extend the scripting language with an added built-in function:
extern void custom_register_builtin_function(custom_state *S, const char *funname, void (*function)(custom_state *FS));

#endif
