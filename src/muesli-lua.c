// muesli-lua.c -*- C -*-
/* muesli interface to Lua
   Copyright (C) 2008, 2009, 2010 University of Limerick

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
#ifndef _MUESLI_LUA_
#define _MUESLI_LUA_

#include "../config.h"

#ifdef HAVE_LIBLUA

#include "muesli.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <unistd.h>
#include <getopt.h>

// #include "lang-extns-cpp.h"


static evaluator_interface *lua_interface = NULL;

static int ambient_transient = 0;

static char *option_names_var_name = (char*)"option_names";

static unsigned int scratch_len = 0;
static char* scratch_buf = NULL;

void
muesli_lua_error(lua_State *lua_state, char *message)
{
  fprintf(stderr, "muesli lua error: %s\n", message);
  exit(1);
}

#if 0
static void
show_lua_stack(char *label)
{
  fprintf(stderr, "At %s, %d items on stack; ",
	  label,
	  lua_gettop(lua_state));
  switch (lua_type (lua_state, -1))
    {
    case LUA_TNIL: fprintf(stderr, "TOS is nil\n"); break;
    case LUA_TNUMBER: { lua_Number n = lua_tonumber(lua_state, -1); fprintf(stderr, "TOS is number %f\n", (float)n);} break;
    case LUA_TBOOLEAN: fprintf(stderr, "TOS is boolean\n"); break;
    case LUA_TSTRING: fprintf(stderr, "TOS is string\n"); break;
    case LUA_TTABLE: fprintf(stderr, "TOS is table\n"); break;
    case LUA_TFUNCTION: fprintf(stderr, "TOS is function\n"); break;
    case LUA_TUSERDATA: fprintf(stderr, "TOS is userdata\n"); break;
    case LUA_TTHREAD: fprintf(stderr, "TOS is thread\n"); break;
    case LUA_TLIGHTUSERDATA: fprintf(stderr, "TOS is lightuserdata\n"); break;
    default: fprintf(stderr, "TOS is unknown\n"); break;
    }
}
#endif

static int
muesli_lua_set_parameter(lua_State *L_state)
{
  if (lua_gettop(L_state) < 2) {
    muesli_lua_error(L_state, (char*)"too few args to muesli_lua_set_parameter\n");
  }

  char option_code = muesli_find_option_letter(lua_interface->getopt_options, lua_tostring(L_state, -2));

  // fprintf(stderr, "muesli_lua_set_parameter(%s->%c)\n", lua_tostring(L_state, -2), option_code);

  if (option_code != -1) {
    if (lua_isstring(L_state, -1)) {
      const char *from_lua = lua_tostring(L_state, -1);
      if (strlen(from_lua)+1 > scratch_len) {
	scratch_len = strlen(from_lua)+1;
	free(scratch_buf);
	scratch_buf = (char*)malloc(scratch_len);
      }
      strcpy(scratch_buf, from_lua);
      // fprintf(stderr, "muesli_lua_set_parameter(%s->%c, \"%s\")\n", lua_tostring(L_state, -2), option_code, scratch_buf);
      (lua_interface->handle_option)(lua_interface->app_params,
				     option_code,
				     muesli_malloc_copy_string(lua_tostring(L_state, -1)),
				     0.0, 1,
				     (char *)"lua");
    } else if (lua_isnumber(L_state, -1)) {
      // fprintf(stderr, "muesli_lua_set_parameter(%s->%c, %d)\n", lua_tostring(L_state, -2), option_code, (int)lua_tonumber(L_state, -1));
      (lua_interface->handle_option)(lua_interface->app_params,
				     option_code,
				     NULL,
				     lua_tonumber(L_state, -1),
				     1,
				     (char *)"lua");
    } else {
      (lua_interface->handle_option)(lua_interface->app_params,
				     option_code,
				     (char*)"true", 0.0,
				     1,
				     (char *)"lua");
    }
    // todo: also write into any result returned by muesli_lua_get_parameters
  }
  return 0;
}

static int
muesli_lua_set_parameters(lua_State *L)
{
  int table = lua_gettop(L);
  if (table < 1)  {
    muesli_lua_error(L, (char*)"too few args to muesli_lua_set_parameters\n");
  }

  lua_pushnil(L);
  while (lua_next(L, table) != 0) {
    muesli_lua_set_parameter(L);
    lua_pop(L, 1);
  }

  return 0;
}

static int
muesli_lua_get_parameter(lua_State *L)
{
  if (lua_gettop(L) < 1)  {
    muesli_lua_error(L, (char*)"too few args to muesli_lua_get_parameter\n");
  }

  char option_code = muesli_find_option_letter(lua_interface->getopt_options, lua_tostring(L, -1));

  // fprintf(stderr, "muesli_lua_get_parameter(%s->%c)\n", lua_tostring(L, -1), option_code);

  if (option_code != -1) {
    muesli_value_t result = (lua_interface->handle_option)(lua_interface->app_params,
							   option_code,	// option
							   NULL, 0.0,	// value
							   0,	// set
							   (char *)"lua");

    switch (result.type) {
    case muesli_value_string:
      lua_pushstring(L, result.data.as_string);
      break;
    case muesli_value_float:
      lua_pushnumber(L, result.data.as_float);
      break;
    case muesli_value_boolean:
      lua_pushboolean(L, result.data.as_int);
      break;
    default:
      lua_pushboolean(L, 0);
      break;
    }
  }
  return 1;
}

static int
muesli_lua_get_parameters(lua_State *L)
{
  lua_State *lua_state = (lua_State *)lua_interface->state;

  // todo: use meta-tables to make changes to this table change the parameter settings
  lua_createtable(lua_state, 48, 0);

  struct option *option_names = lua_interface->getopt_options;

  while (option_names->name != 0) {
    muesli_value_t result;

    lua_pushstring(lua_state, (char*)(option_names->name));

    result = (lua_interface->handle_option)(lua_interface->app_params,
					    (option_names->val), // opt
					    NULL, 0.0,		     // value
					    0,		     // set
					    (char *)"lua");

    switch (result.type) {
    case muesli_value_string:
      lua_pushstring(L, result.data.as_string);
      break;
    case muesli_value_float:
      lua_pushnumber(L, result.data.as_float);
      break;
    case muesli_value_boolean:
      lua_pushboolean(L, result.data.as_int);
      break;
    default:
      lua_pushboolean(L, 0);
      break;
    }

    lua_settable(lua_state, -3);
    option_names++;
  }

  return 1;
}

///////////////////////////////
// Custom built-in functions //
///////////////////////////////

// This function gives Lua access to `custom_eval_individual' in
// muesli-custom.c, thus letting you write a custom evaluator (in C)
// that can be used from a Lua setup script.

static int
muesli_lua_custom_eval_function(lua_State *L)
{
  int n = lua_gettop(L);    /* number of arguments */
  if ((n == 1) && lua_isstring(L, 1)) {
    const char *str = lua_tostring(L, 1);
    evaluator_interface *custom_evaluator = muesli_get_named_evaluator("custom", 0);
    if (custom_evaluator == NULL) {
    muesli_lua_error(L, (char*)"custom evaluator not found\n");
      return 0;
    }
    muesli_value_t result = (custom_evaluator->eval_string)(custom_evaluator,
								str, strlen(str),
								ambient_transient);
    switch (result.type) {
    case muesli_value_float:
      lua_pushnumber(L, result.data.as_float);
      break;
    case muesli_value_integer:
      lua_pushnumber(L, result.data.as_int);
      break;
    case muesli_value_string:
      lua_pushstring(L, result.data.as_string);
      break;
    case muesli_value_boolean:
      lua_pushboolean(L, result.data.as_bool);
      break;
    case muesli_value_none:
    case muesli_value_error:
      // I don't think there's anything we need to do here
      break;
    }
  } else {
    muesli_lua_error(L, (char*)"custom_eval_function must be given a string\n");
    return 0;
  }
  return 1;
}
///////////////////////////////////////////
// Call stack-based bytecode interpreter //
///////////////////////////////////////////

static int
muesli_lua_eval_bytecode_string(lua_State *L)
{
  int n = lua_gettop(L);    /* number of arguments */
  if ((n >= 1) && lua_isstring(L, 1)) {
    const char *str = lua_tostring(L, 1);

    evaluator_interface *stack_evaluator = muesli_get_named_evaluator("stack-code", 0);

    if (stack_evaluator == NULL) {
      fprintf(stderr, "stack evaluator not found\n");
      return 0;
    } else {
      int i;

      (stack_evaluator->eval_clear)(stack_evaluator);

      for (i = 2; i <= n; i++) {
	if (lua_isnumber(L, i)) {
	  lua_Number a = lua_tonumber(L, i);
	  (stack_evaluator->eval_give)(stack_evaluator, (float)a);
	} else {
	  (stack_evaluator->eval_give)(stack_evaluator, 0.0);
	}
      }

      float result = (stack_evaluator->eval_given)(stack_evaluator, str, strlen(str));
      lua_pushnumber(L, result);
    }
  } else {
    muesli_lua_error(L, (char*)"muesli_lua_eval_bytecode_string must be given a string\n");
    return 0;
  }
  return 1;
}

///////////////////////////////////////
// Call arbitrary evaluators by name //
///////////////////////////////////////

static int
muesli_lua_eval_in_language(lua_State *L)
{
  int n = lua_gettop(L);    /* number of arguments */
  if ((n == 2) && lua_isstring(L, 1) && lua_isstring(L, 2)) {
    const char *language_name = lua_tostring(L, 1);
    const char *evaluand = lua_tostring(L, 2);
    unsigned int evaluand_length = strlen(evaluand);

    fprintf(stderr, "In lua_eval_in_language(\"%s\", \"%s\")\n", language_name, evaluand);

    muesli_value_t result = muesli_eval_in_language(language_name,
						    evaluand,
						    evaluand_length,
						    ambient_transient);

    switch (result.type) {
    case muesli_value_float:
      lua_pushnumber(L, result.data.as_float);
      break;
    case muesli_value_integer:
      lua_pushnumber(L, result.data.as_int);
      break;
    case muesli_value_string:
      lua_pushstring(L, result.data.as_string);
      break;
    case muesli_value_boolean:
      lua_pushboolean(L, result.data.as_int);
      break;
    case muesli_value_none:
    case muesli_value_error:
      return 0;
    }
  }
  return 1;
}

////////////
// Extend //
////////////

static void
lua_add_function(evaluator_interface *interface,
		  int arity, const char *name,
		  void *function)
{
  lua_State *lua_state = (lua_State *)(interface->state);
  lua_register(lua_state, name, function);
}

////////////////
// Initialize //
////////////////

static void lua_clear_stack()
{
  lua_State *lua_state = (lua_State *)lua_interface->state;

  lua_pop (lua_state, lua_gettop(lua_state));
}

void
lua_evaluator_init(evaluator_interface *interface)
{
  // Init lua interface
  lua_State *lua_state = lua_open();
  struct option *option_names;

  lua_interface->state = lua_state;

  luaL_openlibs(lua_state);

  Muesli_Add_Fn_1(interface, (char*)"get_parameter", muesli_lua_get_parameter);
  Muesli_Add_Fn_2(interface, (char*)"set_parameter", muesli_lua_set_parameter);
  Muesli_Add_Fn_0(interface, (char*)"parameters", muesli_lua_get_parameters);
  Muesli_Add_Fn_1(interface, (char*)"modify_parameters", muesli_lua_set_parameters);
  Muesli_Add_Fn_1(interface, (char*)"custom", muesli_lua_custom_eval_function);
  // todo: add "binary", "machine"
  Muesli_Add_Fn_1(interface, (char*)"bytecode", muesli_lua_eval_bytecode_string);
  Muesli_Add_Fn_2(interface, (char*)"eval_in_language", muesli_lua_eval_in_language);

  // set up option names
  lua_createtable(lua_state, 48, 0);
  for (option_names = lua_interface->getopt_options;
       option_names->name != 0;
       option_names++) {
    lua_pushstring(lua_state, (char*)(option_names->name));
    lua_pushnumber(lua_state, (option_names->val));
    lua_settable(lua_state, 1);
  }
  lua_setglobal(lua_state, option_names_var_name);
}

static void
lua_load_file(evaluator_interface *interface,
	      const char *filename)
{
  int muesli_flags = interface->flags;
  lua_State *lua_state = (lua_State *)interface->state;

  fprintf(stderr, "Loading lua files\n");


  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loading %s\n", filename);
  }
  if ((luaL_loadfile(lua_state, filename))
      || lua_pcall(lua_state, 0, 0, 0)) {
    fprintf(stderr, "%s\n", lua_tostring(lua_state, -1));
    fprintf(stderr, "Could not read functions file %s\n", filename);
    fprintf(stderr, "Execution aborted.\n");
    exit(0);
  }
  if (muesli_flags & TRACE_MUESLI_LOAD) {
    fprintf(stderr, "Loaded %s\n", filename);
  }

}

static muesli_value_t
lua_eval_string(evaluator_interface *interface,
		const char *lua_c_string,
		unsigned int string_length,
		int transient)
{
  lua_State *lua_state = (lua_State *)interface->state;
  muesli_value_t result;
  ANULL_VALUE(result);

  lua_clear_stack();

  if (lua_c_string) {
    int old_ambient_transient = ambient_transient;
    ambient_transient = transient;
    // fprintf(stderr, "Lua evaluating string \"%s\"\n", lua_c_string);
    if (luaL_loadbuffer(lua_state,
			lua_c_string,
			strlen(lua_c_string),
			"evalstring")
	|| lua_pcall(lua_state,0,1,0)) {
      // Lua error compiling buffer
      fprintf(stderr, "%s\n", lua_tostring(lua_state,-1));
      ambient_transient = old_ambient_transient;
    }

    int stack_count = lua_gettop(lua_state);

    ambient_transient = old_ambient_transient;

    if (stack_count >= 1) {
      switch (lua_type (lua_state, 1)) {
      case LUA_TNUMBER:
	result.data.as_float = lua_tonumber(lua_state, 1);
	result.type = muesli_value_float;
	break;
      case LUA_TNIL:
	result.data.as_bool = 0;
	result.type = muesli_value_boolean;
	break;
      case LUA_TBOOLEAN:
	result.data.as_bool = lua_toboolean(lua_state, 1);
	result.type = muesli_value_boolean;
	break;
      case LUA_TSTRING:
	result.data.as_string = lua_tostring(lua_state, 1);
	result.type = muesli_value_string;
	break;
      default:
	// I don't think we need fill in the result here
	break;
      }
    }
  }

  return result;
}

void
lua_setup(evaluator_interface *new_lua_interface)
{
  lua_interface = new_lua_interface;

  lua_interface->eval_string = lua_eval_string;
  lua_interface->load_file = lua_load_file;
  lua_interface->add_function = lua_add_function;

  lua_interface->version = LUA_RELEASE;
}

#endif
#endif

