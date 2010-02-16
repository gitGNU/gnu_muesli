// muesli-perl.h -*- C -*-
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


#ifndef MUESLI_PERL_H
#define MUESLI_PERL_H

typedef struct perl_table {
  int count;
} perl_table;
typedef struct perl_symbol {
  char *name;
} perl_symbol;

extern PerlInterpreter *my_perl;

extern int perl_count_args(PerlInterpreter *S);
extern int perl_number_arg(PerlInterpreter *S, int argi);
extern char *perl_string_arg(PerlInterpreter *S, int argi);
extern int perl_arg_is_string(PerlInterpreter *S, int argi);
extern int perl_arg_is_number(PerlInterpreter *S, int argi);
extern void perl_return_string(PerlInterpreter *S, const char *string_back);
extern void perl_return_number(PerlInterpreter *S, float number_back);
extern void perl_return_boolean(PerlInterpreter *S, int int_back);
extern perl_table *perl_create_table(PerlInterpreter *S, int size);
extern void perl_set_table_string(PerlInterpreter *S, perl_table *T, char *key, const char *value);
extern void perl_set_table_number(PerlInterpreter *S, perl_table *T, char *key, float value);
extern void perl_set_table_boolean(PerlInterpreter *S, perl_table *T, char *key, int value);
extern perl_symbol *perl_make_symbol(PerlInterpreter *S, const char *n);
extern void perl_throw(PerlInterpreter *S, perl_symbol *Y, float n);
extern void perl_load_file(evaluator_interface *interface,
			   const char *filename);
extern void perl_set_global_variable_table(PerlInterpreter *S, perl_symbol *varname, perl_table *table);

#endif
