// muesli-python.c -*- C -*-
/* Interface to the python interpreter
   Copyright (C) 2010 University of Limerick

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

#ifndef _MUESLI_PYTHON_
#define _MUESLI_PYTHON_

#include "../config.h"
#include "muesli.h"

#ifdef HAVE_LIBPYTHON

#include <Python.h>

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>		// todo: do I need this?

// #include "muesli-python.h"

static int ambient_transient = 0;

static evaluator_interface *python_interface = NULL;

static PyObject *muesli_to_python(muesli_value_t result)
{
  switch (result.type) {
  case muesli_value_string:
    return Py_BuildValue("s", result.data.as_string);
    break;
  case muesli_value_float:
    return Py_BuildValue("f", result.data.as_float);
    break;
  case muesli_value_boolean:
    return Py_BuildValue("i", result.data.as_int);
    break;
  default:
    return Py_BuildValue("i", 0);
    break;
  }
}

static PyObject*
python_set_parameter_key_value(PyObject *self, PyObject *key, PyObject *value)
{
  if (PyString_Check(key)) {
  char *param_name = PyString_AsString(key);
  char option_code = muesli_find_option_letter(python_interface->getopt_options, param_name);

  if (option_code != -1) {
    if (PyString_Check(value)) {
      (python_interface->handle_option)(python_interface->app_params,
					option_code,
					muesli_malloc_copy_string(PyString_AsString(value)),
					0.0, 1, "python");
    } else if (PyFloat_Check(value)) {
      (python_interface->handle_option)(python_interface->app_params,
					option_code, NULL, PyFloat_AsDouble(value), 1, "python");
    } else if (PyInt_Check(value)) {
      (python_interface->handle_option)(python_interface->app_params,
					option_code, NULL, (float)PyInt_AsLong(value), 1, "python");
    } else {
      (python_interface->handle_option)(python_interface->app_params,
					option_code, (char*)"true", 0.0, 1, "python");
      return Py_BuildValue("i", 1);
    }
  }
  }
  return NULL;
}

static PyObject*
python_set_parameter(PyObject *self, PyObject *args)
{
  PyObject *key;
  PyObject *value;

  if(!PyArg_ParseTuple(args, "OO", &key, &value)) {
    return NULL;
  }
  return python_set_parameter_key_value(self, key, value);
}

static PyObject*
python_set_parameters(PyObject *self, PyObject *args)
{
  PyObject *python_dict;

  if (PyArg_ParseTuple(args, "O", &python_dict)) {
    if (PyDict_Check(python_dict)) {
      unsigned int n_set = 0;

      Py_ssize_t pos = 0;
      PyObject *key;
      PyObject *value;

      while (PyDict_Next(python_dict, &pos, &key, &value)) {
	python_set_parameter_key_value(self, key, value);
	n_set++;
      }
      return Py_BuildValue("i", n_set);
    }
  }
  return NULL;
}

static PyObject*
python_get_parameter(PyObject *self, PyObject *args)
{
  char *param_name;

  if(!PyArg_ParseTuple(args, "s", &param_name)) {
    return NULL;
  }

  char option_code = muesli_find_option_letter(python_interface->getopt_options,
					       param_name);

  if (option_code != -1) {

    return muesli_to_python((python_interface->handle_option)(python_interface->app_params,
							      option_code,	// option
							      NULL, 0.0,	// value
							      0,	// set
							      "python"));
  }
  return NULL;
}

static PyObject*
python_get_parameters(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }

  PyObject *table = PyDict_New();

  struct option *option_names = python_interface->getopt_options;

  while (option_names->name != 0) {

    PyDict_SetItemString(table, (char*)(option_names->name),
			 muesli_to_python((python_interface->handle_option)(python_interface->app_params,
									    (option_names->val), // opt
									    NULL, 0.0, // value
									    0, // set
									    "python")));

    option_names++;
  }

  return table;
}

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static PyObject*
python_eval_in_language(PyObject *self, PyObject *args)
{
  const char *language_name;
  const char *evaluand;

  if (PyArg_ParseTuple(args, "ss", &language_name, &evaluand)) {
    unsigned int evaluand_length = strlen(evaluand);

    fprintf(stderr, "In python_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

    muesli_value_t result = muesli_eval_in_language(language_name,
						    evaluand,
						    evaluand_length,
						    ambient_transient);

    switch (result.type) {
    case muesli_value_float:
      return Py_BuildValue("f", result.data.as_float);
      break;
    case muesli_value_string:
      return Py_BuildValue("s", result.data.as_string);
      break;
    case muesli_value_boolean:
      return Py_BuildValue("i", result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      return Py_BuildValue("i", 0);
      break;
    default:
      return NULL;
    }
  }
    return NULL;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This isn't meant as self-referentially as it looks (pity!).  It's
// really there so that if you use this file as a template for another
// interpreter interface, the language you're adding will get access
// to python built-in functions just like the existing languages do.

static PyObject*
python_custom_eval_function(PyObject *self, PyObject *args)
{
  char *string_arg;

  if (PyArg_ParseTuple(args, "s", &string_arg)) {
    muesli_value_t result = (python_interface->eval_string)(python_interface,
							    string_arg, strlen(string_arg),
							    ambient_transient);
    switch (result.type) {
    case muesli_value_float:
      return Py_BuildValue("f", result.data.as_float);
      break;
    case muesli_value_string:
      return Py_BuildValue("s", result.data.as_string);
      break;
    case muesli_value_boolean:
      return Py_BuildValue("i", result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      return Py_BuildValue("i", 0);
      break;
    default:
      return NULL;
    }
  }
  return NULL;
}

////////////////
// Initialize //
////////////////

static PyMethodDef MuesliMethods[] =
{
  {"SetParameter", python_set_parameter, METH_VARARGS, "Set a specified application parameter."},
  {"GetParameter", python_get_parameter, METH_VARARGS, "Return a specified application parameter."},
  {"ModifyParameters", python_set_parameters, METH_VARARGS, "Set some application parameters."},
  {"Parameters", python_get_parameters, METH_VARARGS, "Return the application parameters."},
  {"EvalInLanguage", python_eval_in_language, METH_VARARGS, "Evaluate a string in a specified language."},
  {"CustomEval", python_custom_eval_function, METH_VARARGS, "Call the custom evaluator."},
  {NULL, NULL, 0, NULL}
};

void
python_evaluator_init(evaluator_interface *interface)
{
  // Init python interface
  Py_Initialize();

  python_interface = interface;

  // Extend system

  // register all these language extensions (as applicable):
  Py_InitModule("Muesli", MuesliMethods);

#if 0
  // Set up option names

  // Fill in: Create a table of option names.  You may well not need to bother.
  python_table *option_names_table = python_create_table(our_python_state, 48);
  struct option *option_names;
  for (option_names = python_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    python_set_table_number(our_python_state, option_names_table,
			    (char*)(option_names->name), (option_names->val));
  }
  python_set_global_variable_table(our_python_state,
				   python_make_symbol(our_python_state,
						      option_names_var_name),
				   option_names_table);
#endif
}

static void
python_load_file(evaluator_interface *interface,
		const char *filename)
{
  int muesli_flags = interface->flags;
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }

  PyObject *pname = PyString_FromString(filename);
  PyImport_Import(pname);
  Py_DECREF(pname);

  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }
}

static muesli_value_t
python_eval_string(evaluator_interface *evaluator,
		   const char *python_c_string,
		   unsigned int string_length,
		   int transient)
{
  muesli_value_t result;
  ANULL_VALUE(result);

  if (python_c_string) {
    PyObject *resulting_object;
    PyObject *globals = PyEval_GetGlobals(); // todo: check this is what I want 
    PyObject *locals = PyEval_GetLocals(); // todo: check this is what I want 
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;

    fprintf(stderr, "Python evaluating string \"%s\"\n", python_c_string);

    resulting_object = PyRun_String(python_c_string, 0, globals, locals);

    ambient_transient = old_ambient_transient;

    if (PyString_Check(resulting_object)) {
      result.data.as_string = PyString_AsString(resulting_object);
      result.type = muesli_value_string;
    } else if (PyFloat_Check(resulting_object)) {
      result.data.as_float = PyFloat_AsDouble(resulting_object);
      result.type = muesli_value_float;
    } else if (PyInt_Check(resulting_object)) {
      result.data.as_float = (float)PyInt_AsLong(resulting_object);
      result.type = muesli_value_float;
    }
  }
  return result;
}

void
python_setup(evaluator_interface *new_python_interface)
{
  python_interface = new_python_interface;

  python_interface->eval_string = python_eval_string;
  python_interface->load_file = python_load_file;

  python_interface->version = PY_VERSION;
}

#endif
#endif
