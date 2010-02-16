// muesli-tcl.c -*- C -*-
/* muesli interface to Tcl
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
#ifndef _MUESLI_TCL_
#define _MUESLI_TCL_

#include "../config.h"

#ifdef HAVE_LIBTCL

#include "muesli.h"

#include "tcl.h"
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

// The Tcl state with which we were most recently called
Tcl_Interp *tcl_interpreter = NULL;

static int ambient_transient = 0;

// Interface from the rest of the world
static evaluator_interface *tcl_interface = NULL;

// static char *option_names_var_name = (char*)"option_names";

static void
muesli_tcl_error(char *message)
{
  fprintf(stderr, "muesli_tcl_error %s\n", message);
  exit(1);
}

static void
set_one_parameter_tcl(char *name, char *value)
{
  const char option_code = muesli_find_option_letter(tcl_interface->getopt_options, name);

  if (option_code != -1) {

    (tcl_interface->handle_option)(tcl_interface->app_params,
				   option_code,
				   muesli_malloc_copy_string(value), 0.0,
				   1,
				   (char *)"tcl");
  }
}

static int
set_parameter_tcl(ClientData clientData,
		  Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[])
{
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, objc, objv, "SetParameter name value");
    return TCL_ERROR;
  }

  set_one_parameter_tcl(Tcl_GetString(objv[1]), Tcl_GetString(objv[2]));

  return TCL_OK;
}

static int
set_parameters_tcl(ClientData clientData,
		   Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[])
{
  if (objc != 2)  {
    Tcl_WrongNumArgs(interp, objc, objv, "ModifyParameters hash");
    return TCL_ERROR;
  }
#if 0
  // todo: how do I check (or coerce) the type, to make sure I have a hash here?
  if () {
  }
#endif
#if 1

  // http://www.tcl.tk/man/tcl8.4/TclLib/Hash.htm says:
  // Tcl_FirstHashEntry and Tcl_NextHashEntry may be used to scan all
  // of the entries in a hash table. A structure of type
  // ``Tcl_HashSearch'', provided by the client, is used to keep track
  // of progress through the table. Tcl_FirstHashEntry initializes the
  // search record and returns the first entry in the table (or NULL
  // if the table is empty). Each subsequent call to Tcl_NextHashEntry
  // returns the next entry in the table or NULL if the end of the
  // table has been reached. A call to Tcl_FirstHashEntry followed by
  // calls to Tcl_NextHashEntry will return each of the entries in the
  // table exactly once, in an arbitrary order. It is unadvisable to
  // modify the structure of the table, e.g. by creating or deleting
  // entries, while the search is in progress.


  // Todo: fill in the_table with the table passed in as an argument.  I don't yet know how to get a table from a tcl value.
  Tcl_HashTable *the_table;
  Tcl_HashSearch the_search;

  Tcl_HashEntry *the_entry = Tcl_FirstHashEntry(the_table, &the_search);

  while ((the_entry = Tcl_NextHashEntry(&the_search)) != NULL) {
    char *key = Tcl_GetHashKey(the_table, the_entry);
    char *value = Tcl_GetHashValue(the_entry);
    set_one_parameter_tcl(key, value);
  }
#endif
  return TCL_OK;
}

static int
get_parameter_tcl(ClientData clientData,
		  Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[])
{
  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "GetParameter name");
    return TCL_ERROR;
  }

  char option_code = muesli_find_option_letter(tcl_interface->getopt_options, Tcl_GetString(objv[1]));

  if (option_code != -1) {
    Tcl_Obj *result_obj;
    muesli_value_t result = (tcl_interface->handle_option)(tcl_interface->app_params,
							   option_code,	// option
							   NULL, 0.0,	// value
							   0,	// set
							   (char *)"tcl");

    switch (result.type) {
    case muesli_value_string:
      result_obj = Tcl_NewStringObj(result.data.as_string, strlen(result.data.as_string));
      break;
    case muesli_value_float:
      result_obj = Tcl_NewDoubleObj((double)result.data.as_float);
      break;
    case muesli_value_boolean:
      result_obj = Tcl_NewBooleanObj(result.data.as_int);
      break;
    default:
      result_obj = Tcl_NewBooleanObj(0);
      break;
    }
    Tcl_SetObjResult(interp, result_obj);
  }
  return TCL_OK;
}

#if 1

// I want to use associative arrays for the parameter table.  I think
// I do this with things like:
// CONST char * Tcl_SetVar2(interp, name1, name2, newValue, flags)
// The style for this (going by "stat") is for the user program to
// give the name of a variable to be filled in with such an array.

static int
get_parameters_tcl(ClientData clientData,
		   Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[])
{
  if (objc < 2)  {
    muesli_tcl_error((char*)"too few args to get_parameter_tcl\n");
  }
  char *varName = Tcl_GetString(objv[1]);
  Tcl_Obj *var = Tcl_NewStringObj(varName, -1);
  Tcl_Obj *field = Tcl_NewObj();
  Tcl_Obj *value;

  /*
   * Assume Tcl_ObjSetVar2() does not keep a copy of the field name!
   */
#define STORE_ARY(fieldName, object)					\
  Tcl_SetStringObj(field, (fieldName), -1);				\
  value = (object);							\
  if (Tcl_ObjSetVar2(interp,var,field,value,TCL_LEAVE_ERR_MSG) == NULL) { \
    Tcl_DecrRefCount(var);						\
    Tcl_DecrRefCount(field);						\
    Tcl_DecrRefCount(value);						\
    return TCL_ERROR;							\
  }

  Tcl_IncrRefCount(var);
  Tcl_IncrRefCount(field);

  //     STORE_ARY("dev",   Tcl_NewLongObj((long)statPtr->st_dev));
  //     /*
  //      * Watch out porters; the inode is meant to be an *unsigned* value,
  //      * so the cast might fail when there isn't a real arithmentic 'long
  //      * long' type...
  //      */
  //     STORE_ARY("ino",   Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_ino));
  //     STORE_ARY("nlink", Tcl_NewLongObj((long)statPtr->st_nlink));
  //     STORE_ARY("gid",   Tcl_NewLongObj((long)statPtr->st_gid));
  //     STORE_ARY("size",  Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_size));
  // #ifdef HAVE_ST_BLOCKS
  //     STORE_ARY("blocks",Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_blocks));
  // #endif
  //     STORE_ARY("atime", Tcl_NewLongObj((long)statPtr->st_atime));
  //     STORE_ARY("mtime", Tcl_NewLongObj((long)statPtr->st_mtime));
  //     STORE_ARY("ctime", Tcl_NewLongObj((long)statPtr->st_ctime));
  //     mode = (unsigned short) statPtr->st_mode;
  //     STORE_ARY("mode",  Tcl_NewIntObj(mode));
  //     return TCL_OK;

  // todo: find how to do Tcl tables

  struct option *option_names = tcl_interface->getopt_options;

  while (option_names->name != 0) {

    muesli_value_t result = (tcl_interface->handle_option)(tcl_interface->app_params,
							   (option_names->val), // opt
							   NULL, 0.0,		     // value
							   0,		     // set
							   (char *)"tcl");

    switch (result.type) {
    case muesli_value_string:
      STORE_ARY(option_names->name,
		Tcl_NewStringObj(result.data.as_string, -1));
      break;
    case muesli_value_float:
      STORE_ARY(option_names->name,
		Tcl_NewLongObj((long)result.data.as_float));
      break;
    case muesli_value_boolean:
      STORE_ARY(option_names->name,
		Tcl_NewLongObj((long) (result.data.as_int ? 1 : 0)));
      break;
    default:
      STORE_ARY(option_names->name,
		Tcl_NewStringObj("<unknown type>", -1));
      break;
    }

    // tcl_settable(tcl_interpreter, -3);
    option_names++;
  }

#undef STORE_ARY

  Tcl_DecrRefCount(var);
  Tcl_DecrRefCount(field);

  return TCL_OK;
}
#endif

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives Tcl access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a Tcl muesli setup program.

static int
custom_eval_function_tcl(ClientData clientData,
			 Tcl_Interp *interp,
			 int objc, Tcl_Obj *CONST objv[])
{
  char *str;

  if (objc < 1)  {
    muesli_tcl_error((char*)"too few args to custom\n");
  }

  str = Tcl_GetString(objv[1]);

  evaluator_interface *custom_evaluator = muesli_get_named_evaluator("custom", 0);

  if (custom_evaluator == NULL) {
    fprintf(stderr, "custom evaluator not found\n");
    return TCL_ERROR;
  } else {
    muesli_value_t result = (custom_evaluator->eval_string)(custom_evaluator,
							    str, strlen(str),
							    ambient_transient);

    switch (result.type) {
    case muesli_value_none:
      break;
    case muesli_value_error:
      return TCL_ERROR;
      break;
    case muesli_value_float:
      Tcl_SetObjResult(interp,
		       Tcl_NewDoubleObj((double)(result.data.as_float)));
      break;
    case muesli_value_integer:
      Tcl_SetObjResult(interp,
		       Tcl_NewDoubleObj((double)(result.data.as_int)));
      break;
    case muesli_value_string:
      Tcl_SetObjResult(interp,
		       Tcl_NewStringObj(result.data.as_string,
					strlen(result.data.as_string)));
      break;
    case muesli_value_boolean:
      Tcl_SetObjResult(interp,
		       Tcl_NewBooleanObj(result.data.as_int));
      break;
    }
  }
  return TCL_OK;
}

static int
stack_bytecode_tcl(ClientData clientData,
		   Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[])
{
  // todo: fill in
  return TCL_ERROR;
}

static int
eval_in_language_tcl(ClientData clientData,
		     Tcl_Interp *interp,
		     int objc, Tcl_Obj *CONST objv[])
{
  // todo: fill in
  return TCL_ERROR;
}

////////////////
// Initialize //
////////////////

void
tcl_evaluator_init(evaluator_interface *interface)
{
  // Init tcl interface
  Tcl_Interp *tcl_interpreter = Tcl_CreateInterp();
  interface->state = tcl_interpreter;

  Tcl_CreateObjCommand(tcl_interpreter, (char*)"evolution_parameters", get_parameters_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"modify_evolution_parameters", set_parameters_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"get_parameter", get_parameter_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"set_parameter", set_parameter_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"custom", custom_eval_function_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"bytecode", stack_bytecode_tcl, NULL, NULL);
  Tcl_CreateObjCommand(tcl_interpreter, (char*)"eval_in_language", eval_in_language_tcl, NULL, NULL);

#if 0
  // set up option names

  // todo: how do I make the tablePtr?
  Tcl_InitObjHashTable(tablePtr);

  for (struct option *option_names = long_options;
       option_names->name != 0;
       option_names++) {
    Tcl_HashEntry *entry_ptr = Tcl_CreateHashEntry(tablePtr, (char*)(option_names->name), newPtr);
    Tcl_SetHashValue(entry_ptr, (option_names->val));
  }

  Tcl_ObjSetVar2(interp,
		 Tcl_NewStringObj(option_names_var_name,
				  strlen(option_names_var_name)),
		 NULL,
		 // todo: wrap tablePtr into a Tcl value
		 tablePtr,
		 flags);
#endif
}

static void
tcl_load_file(evaluator_interface *interface,
	       const char *filename)
{
  Tcl_Interp *tcl_interpreter = (Tcl_Interp*)interface->state;

  int muesli_flags = interface->flags;

  
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
  if (Tcl_EvalFile(tcl_interpreter, filename) != TCL_OK) {
    fprintf(stderr, "Could not read functions file %s\n", filename);
    fprintf(stderr, "Execution aborted.\n");
    exit(0);
  }
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
  
}

static muesli_value_t
tcl_eval_string(evaluator_interface *interface,
		const char *tcl_c_string,
		unsigned int string_length,
		int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  // Set the global one, for the sake of the taf caller at least
  tcl_interpreter = (Tcl_Interp*)interface->state;

  if (tcl_c_string) {
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;

    // fprintf(stderr, "Tcl evaluating string \"%s\"\n", tcl_c_string);
    if (Tcl_Eval((Tcl_Interp*)interface->state, tcl_c_string) == TCL_OK) {

      ambient_transient = old_ambient_transient;

      Tcl_Obj *result_obj = Tcl_GetObjResult(tcl_interpreter);

      double result_double = 0.0;

      if (Tcl_GetDoubleFromObj((Tcl_Interp*)interface->state,
			       result_obj,
			       &result_double) == TCL_OK) {
	result.data.as_float = (float)result_double;
	result.type = muesli_value_float;
      } else {
	result.data.as_string = Tcl_GetString(result_obj);
	result.type = muesli_value_string;
      }
    } else {
      ambient_transient = old_ambient_transient;
      fprintf(stderr, "Error from Tcl interpreter while interpreting: %s\n", tcl_c_string);
      result.type = muesli_value_error;
    }
  }
  return result;
}

void
tcl_setup(evaluator_interface *new_tcl_interface)
{
  tcl_interface = new_tcl_interface;
 // maybe use Tcl_GetAssocData, Tcl_SetAssocData, Tcl_DeleteAssocData
 // - manage associations of string keys and user specified data with
 // Tcl interpreters?

  tcl_interface->eval_string = tcl_eval_string;
  tcl_interface->load_file = tcl_load_file;

  tcl_interface->version = TCL_PATCH_LEVEL;
}

#endif
#endif

