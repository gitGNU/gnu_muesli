// muesli-zero.c -*- C -*-
/* Interface to zero evaluator
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

#ifndef _MUESLI_ZERO_
#define _MUESLI_ZERO_

#include "../config.h"
#include "muesli.h"

#define ZERO_VERSION "0.0"

void
zero_evaluator_init(evaluator_interface *interface)
{
}

void
zero_load_file(evaluator_interface *interface,
	       const char *file)
{
}

muesli_value_t
zero_eval_string(evaluator_interface *evaluator,
		 const char *zero_c_string,
		 unsigned int string_length,
		 int transient)
{
  muesli_value_t result;
  result.data.as_float = 0.0;
  result.type = muesli_value_float;
  return result;
}

static float
zero_eval_0(struct evaluator_interface *interface,
	      const char *scratch,
	      unsigned int length)
{
  return 0.0;
}

void
zero_setup(evaluator_interface *new_zero_interface)
{
  new_zero_interface->eval_string = zero_eval_string;
  new_zero_interface->eval_0 = zero_eval_0;
  new_zero_interface->load_file = zero_load_file;

  new_zero_interface->version = ZERO_VERSION;
}

#endif

