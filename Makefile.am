# Top-level Makefile.am for libmuesli

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

SUBDIRS = . src doc
dist_doc_DATA = README

AM_CPPFLAGS = -Wall -pg -ffast-math @PERL_INC@ @PYTHON_INC@ @OCTAVE_INC@

###############################################################################
# LIBRARY SOURCE FILES
###############################################################################

lib_LTLIBRARIES = libmuesli.la
include_HEADERS = \
    src/muesli.h \
    src/muesli-custom.h \
    src/muesli-perl.h \
    src/muesli-stack-bytecode.h \
    src/muesli-tcc.h

libmuesli_la_SOURCES = \
    $(include_HEADERS) \
    src/muesli.c \
    src/muesli-tests.c \
    src/muesli-custom.c \
    src/muesli-zero.c \
    src/muesli-exec.c \
    src/muesli-lua.c \
    src/muesli-python.c \
    src/muesli-machine-code.c \
    src/muesli-perl.c \
    src/muesli-pipe.c \
    src/muesli-siod.c \
    src/muesli-stack-bytecode.c \
    src/muesli-tcc.c \
    src/muesli-tcl.c \
    src/muesli-slang.c \
    src/muesli-ulsiod.c \
    src/muesli-javascript.c \
    src/muesli-octave.cpp

# libmuesli_la_PERL_CXXFLAGS = @PERL_INC@
# libmuesli_la_PERL_LDADD = @PERL_LIB@

###############################################################################
# DEMO PROGRAM
###############################################################################

bin_PROGRAMS = muesli-demo
muesli_demo_SOURCES = src/muesli-demo.c
muesli_demo_LDADD = -lmuesli -lutil

###############################################################################
# DOCUMENTATION FILES
###############################################################################
info_TEXINFOS = doc/muesli.texi
EXTRA_DIST = doc/*.texi
