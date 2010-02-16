/* Muesli header for common bits for language extensions
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

/* 
   This file declares the functions to be wrapped by h2xs, SWIG or equivalent.

   These are the "C" versions of the functions, and if you #include
   this file in a C++ file you should do so inside an "extern C".
   However, you shouldn't normally be doing such an inclusion at all
   at all.  There is another file, lang-extns-cpp.h, for
   inclusion from C++ files.

   The C++ and C versions of the functions are declared in separate
   header files, to avoid confusing the likes of h2xs with `ifdef
   cplusplus' and `extern C'.

*/

extern const char *muesli_get_parameter(evaluator_interface *interface,
					struct option *options,
					char* option);
extern int muesli_set_parameter(evaluator_interface *interface,
				struct option *options,
				char* option,
				char *value);
