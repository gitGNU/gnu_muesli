/* muesli interface to tcc (Tiny C Compiler)
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

#ifndef MUESLI_TCC_H
#define MUESLI_TCC_H

typedef struct tcc_table {
  int count;
} tcc_table;
typedef struct tcc_symbol {
  char *name;
} tcc_symbol;

// Getting args from your scripting language stack:
extern int tcc_count_args(TCCState *S);
extern int tcc_number_arg(TCCState *S, int argi);
extern char *tcc_string_arg(TCCState *S, int argi);
extern int tcc_arg_is_string(TCCState *S, int argi);
extern int tcc_arg_is_number(TCCState *S, int argi);

// Putting results back onto your scripting language stack:
extern void tcc_return_string(TCCState *S, const char *string_back);
extern void tcc_return_number(TCCState *S, float number_back);
extern void tcc_return_boolean(TCCState *S, int int_back);

// Handling table / map / alist structures of your scripting language:
extern tcc_table *tcc_create_table(TCCState *S, int size);
extern void tcc_set_table_string(TCCState *S, tcc_table *T, char *key, const char *value);
extern void tcc_set_table_number(TCCState *S, tcc_table *T, char *key, float value);
extern void tcc_set_table_boolean(TCCState *S, tcc_table *T, char *key, int value);

// Make a name / symbol of the scripting language:
extern tcc_symbol *tcc_make_symbol(TCCState *S, const char *n);

// Non-local exit in the scripting language
extern void tcc_throw(TCCState *S, tcc_symbol *Y, float n);

// Set a global variable of the scripting language to contain a table:
extern void tcc_set_global_variable_table(TCCState *S, tcc_symbol *varname, tcc_table *table);

#endif
