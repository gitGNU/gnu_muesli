#!/usr/bin/autoconf
##############################
# configure.ac for libmuesli #
##############################

# Time-stamp: <2010-02-11 16:18:09 jcgs>

# This configures muesli, a library for multiple utility, evaluation and scripting language interfaces

# Process this file with autoconf to produce a configure script.

# Copyright (C) 2010 University of Limerick

# This file is part of muesli.
# 
# muesli is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your
# option) any later version.
# 
# muesli is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License
# along with muesli.  If not, see <http://www.gnu.org/licenses/>.

# Started 2009-08-25 by John C G Sturdy
# This code was originally part of GrEvo

AC_PREREQ(2.61)
AC_INIT([libmuesli], [0.1], [john.sturdy@ul.ie])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_CONFIG_HEADER([config.h])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])

###########################
# Checks for header files #
###########################

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h unistd.h])

#################################################################################
# Checks for typedefs, structures, and compiler characteristics (from autoscan) #
#################################################################################

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_STRUCT_ST_BLOCKS
AC_TYPE_UINT8_T

################################################
# Checks for library functions (from autoscan) #
################################################

AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 floor sqrt strchr])
AC_CHECK_LIB(readline, readline)

####################################
# Checks for programming languages #
####################################

AC_MSG_NOTICE([checking for languages])
AC_CHECK_LIB(lua, luaL_newstate, , , -lm)
AC_CHECK_LIB(octinterp, octave_sleep)
AC_CHECK_LIB(perl, perl_alloc)
AC_CHECK_LIB(pl, PL_initialise, , , -lrt -lgmp)
AC_CHECK_LIB(python, Py_Initialize, , , -lutil)
AC_CHECK_LIB(siod, siod_init)
AC_CHECK_LIB(slang, SLang_init_all)
AC_CHECK_LIB(tcc, tcc_new)
AC_CHECK_LIB(tcl, Tcl_CreateInterp)
AC_CHECK_LIB(mozjs, JS_Init)
AC_CHECK_LIB(ulsiod, ulsiod_init)

##################
# Perl embedding #
##################

# We ask perl to reify information as to how it was built, where its
# pancreas and kidneys are, etc:
PERL_INC=`perl -MExtUtils::Embed -e ccopts -e perl_inc`
PERL_LIB=`perl -MExtUtils::Embed -e ccopts -e ldopts`

AC_SUBST(PERL_INC)
AC_SUBST(PERL_LIB)

####################
# Python embedding #
####################

PYTHON_INC=-I`find /usr/include /usr/local/include -name Python.h -exec dirname {} \\;`
AC_SUBST(PYTHON_INC)

####################
# Octave embedding #
####################

# OCTAVE_INC=-I`find /usr/include /usr/local/include -name octave.h -exec dirname {} \\;`
OCTAVE_INC=-I`dirname \`find /usr/include /usr/local/include -name octave.h -exec dirname {} \\;\``
AC_SUBST(OCTAVE_INC)

#################
# Write results #
#################

AC_OUTPUT

AC_MSG_NOTICE([************************************************************])
AC_MSG_NOTICE([* Now run `make` and `make install`.                       *])
AC_MSG_NOTICE([************************************************************])
