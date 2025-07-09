// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.08.28
// 18:55:03
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"
#include <stdarg.h>

#define JSON_DECREF(x) \
  {                    \
    if (x != 0)        \
    {                  \
      json_decref(x);  \
      x = 0;           \
    }                  \
  }

// --- JSON error message templates ---
static const char *ERR_JSON_ALLOC = "json error.";
static const char *ERR_JSON_INDEX = "json error. Indx %d ";

// --- Macro for typical JSON error handling ---
#define JSON_ERR_AND_GOTO         \
  do                              \
  {                               \
    APPLOG("%s", ERR_JSON_ALLOC); \
    goto error;                   \
  } while (0)

// --- Macro for typical JSON error handling with index ---
#define JSON_INDEX_ERR_AND_GOTO(i) \
  do                               \
  {                                \
    APPLOG(ERR_JSON_INDEX, (i));   \
    goto error;                    \
  } while (0)

typedef enum
{
  CLEANUP_TYPE_JSON,
  CLEANUP_TYPE_STR
} cleanup_type_t;

/*-----------------------------------------------------------------------------------------------------
  Universal resource cleanup function for json_t* and char*.

  Parameters:
    type - resource type (CLEANUP_TYPE_JSON or CLEANUP_TYPE_STR)
    ...  - pointer addresses for cleanup, terminated by -1

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static inline void cleanup_json_resources(int type, ...)
{
  va_list args;
  void **ptr;
  va_start(args, type);
  while (type != -1)
  {
    ptr = va_arg(args, void **);
    if (ptr && *ptr)
    {
      if (type == CLEANUP_TYPE_JSON)
      {
        json_decref(*((json_t **)ptr));
      }
      else if (type == CLEANUP_TYPE_STR)
      {
        App_free(*ptr);
      }
      *ptr = 0;
    }
    type = va_arg(args, int);
  }
  va_end(args);
}

/*-----------------------------------------------------------------------------------------------------
  Serialize device description to JSON object.

  Parameters:
    obj      - JSON object to fill
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_device_description_to_obj(json_t *obj, char *obj_name)
{
  json_t *main_obj = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  item_obj = json_object();
  if (!item_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (json_object_set_new(item_obj, "HW_Ver", json_string((char *)wvar.product_name)) != 0) goto error;
  if (json_object_set_new(item_obj, "CompDate", json_string(__DATE__)) != 0) goto error;
  if (json_object_set_new(item_obj, "CompTime", json_string(__TIME__)) != 0) goto error;

  if (json_object_set(obj, obj_name, item_obj) != 0) goto error;
  json_decref(item_obj);
  item_obj = 0;

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize device description to JSON file.

  Parameters:
    p_file   - file descriptor
    flags    - JSON formatting flags
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Serialize_device_description_to_file(FX_FILE *p_file, size_t flags, char *obj_name)
{
  json_t *main_obj = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  main_obj = json_object();
  if (!main_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (Serialize_device_description_to_obj(main_obj, obj_name) != RES_OK) goto error;

  if (json_dumpf(main_obj, p_file, flags) != 0) goto error;
  json_decref(main_obj);
  main_obj = 0;

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize main parameters schema to JSON object.

  Parameters:
    p_pars   - pointer to parameters structure
    obj      - JSON object to fill
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_main_params_schema_to_obj(const T_NV_parameters_instance *p_pars, json_t *obj, char *obj_name)
{
  int32_t i = 0;
  const T_NV_parameters *pp;
  char *str = 0;
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  str = App_malloc_pending(MAX_PARAMETER_STRING_LEN, 10);
  if (str == NULL)
  {
    JSON_INDEX_ERR_AND_GOTO(i);
  }
  jarray = json_array();
  if (!jarray)
  {
    JSON_ERR_AND_GOTO;
  }

  // Create an array describing the parameters

  for (i = 0; i < p_pars->items_num; i++)
  {
    pp = &p_pars->items_array[i];
    item_obj = json_object();
    if (json_object_set_new(item_obj, "name", json_string((char const *)pp->var_name)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "alias", json_string((char const *)pp->var_alias)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "descr", json_string((char const *)pp->var_description)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }

    Convert_parameter_to_str(p_pars, (uint8_t *)str, MAX_PARAMETER_STRING_LEN, i);
    if (json_object_set_new(item_obj, "value", json_string(str)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "type", json_string(Convrt_var_type_to_str(pp->vartype))) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "max_len", json_integer(pp->varlen)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "max_value", json_real((double)pp->maxval)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "min_value", json_real((double)pp->minval)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "c_format", json_string(pp->format)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "attributes", json_integer(pp->attr)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "level", json_integer(pp->parmnlev)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "num", json_integer(pp->menu_pos)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    if (json_object_set_new(item_obj, "selector", json_integer(pp->selector_id)) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }

    if (json_array_insert(jarray, i, item_obj) != 0)
    {
      JSON_INDEX_ERR_AND_GOTO(i);
    }
    JSON_DECREF(item_obj);
  }
  if (json_object_set(obj, obj_name, jarray) != 0)
  {
    JSON_INDEX_ERR_AND_GOTO(i);
  }
  JSON_DECREF(jarray);
  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_STR, &str, CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize main parameters schema to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    p_file   - file descriptor
    flags    - JSON formatting flags
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_main_params_schema_to_file(const T_NV_parameters_instance *p_pars, FX_FILE *p_file, size_t flags, char *obj_name)
{
  char *str = 0;
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  main_obj = json_object();
  if (!main_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (Serialize_main_params_schema_to_obj(p_pars, main_obj, obj_name) != RES_OK) goto error;
  // Write the created object and close it to free dynamic memory
  if (json_dumpf(main_obj, p_file, flags) != 0) goto error;
  JSON_DECREF(main_obj);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_STR, &str, CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter values to JSON object.

  Parameters:
    p_pars   - pointer to parameters structure
    obj      - JSON object to fill
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_params_vals_to_obj(const T_NV_parameters_instance *p_pars, json_t *obj, char *obj_name)
{
  const T_NV_parameters *pp;
  char *str = 0;
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  str = App_malloc_pending(MAX_PARAMETER_STRING_LEN, 0);
  if (str == NULL)
  {
    JSON_ERR_AND_GOTO;
  }

  jarray = json_array();
  if (!jarray)
  {
    JSON_ERR_AND_GOTO;
  }

  // Create an array describing the parameters

  for (int32_t i = 0; i < p_pars->items_num; i++)
  {
    pp = &p_pars->items_array[i];
    item_obj = json_array();
    if (json_array_append_new(item_obj, json_string((char const *)pp->var_name)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }

    Convert_parameter_to_str(p_pars, (uint8_t *)str, MAX_PARAMETER_STRING_LEN, i);
    if (json_array_append_new(item_obj, json_string(str)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }

    if (json_array_insert(jarray, i, item_obj) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    JSON_DECREF(item_obj);
  }
  if (json_object_set(obj, obj_name, jarray) != 0)
  {
    JSON_ERR_AND_GOTO;
  }
  JSON_DECREF(jarray);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_STR, &str, CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter values to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    p_file   - file descriptor
    flags    - JSON formatting flags
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_params_vals_to_file(const T_NV_parameters_instance *p_pars, FX_FILE *p_file, size_t flags, char *obj_name)
{
  char *str = 0;
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  main_obj = json_object();
  if (!main_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (Serialize_params_vals_to_obj(p_pars, main_obj, obj_name) != RES_OK) goto error;
  // Write the created object and close it to free dynamic memory
  if (json_dumpf(main_obj, p_file, flags) != 0) goto error;
  JSON_DECREF(main_obj);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_STR, &str, CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter tree to JSON object.

  Parameters:
    p_pars   - pointer to parameters structure
    obj      - JSON object to fill
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_params_tree_to_obj(const T_NV_parameters_instance *p_pars, json_t *obj, char *obj_name)
{
  const T_parmenu *pm;
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  jarray = json_array();
  if (!jarray)
  {
    JSON_ERR_AND_GOTO;
  }

  // Create an array describing the parameter tree

  for (int32_t i = 0; i < p_pars->menu_items_num; i++)
  {
    pm = &p_pars->menu_items_array[i];
    item_obj = json_object();
    if (json_object_set_new(item_obj, "level", json_integer(pm->currlev)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    if (json_object_set_new(item_obj, "parent", json_integer(pm->prevlev)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    if (json_object_set_new(item_obj, "name", json_string(pm->name)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    if (json_object_set_new(item_obj, "attributes", json_integer(pm->visible)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }

    if (json_array_insert(jarray, i, item_obj) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    JSON_DECREF(item_obj);
  }
  if (json_object_set(obj, obj_name, jarray) != 0)
  {
    JSON_ERR_AND_GOTO;
  }
  JSON_DECREF(jarray);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter tree to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    p_file   - file descriptor
    flags    - JSON formatting flags
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_params_tree_to_file(const T_NV_parameters_instance *p_pars, FX_FILE *p_file, size_t flags, char *obj_name)
{
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj = 0;
  uint32_t res = RES_ERROR;

  main_obj = json_object();
  if (!main_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (Serialize_params_tree_to_obj(p_pars, main_obj, obj_name) != RES_OK) goto error;
  // Write the created object and close it to free dynamic memory
  if (json_dumpf(main_obj, p_file, flags) != 0) goto error;
  JSON_DECREF(main_obj);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &item_obj, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter selectors to JSON object.

  Parameters:
    p_pars   - pointer to parameters structure
    obj      - JSON object to fill
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_selectors_to_obj(const T_NV_parameters_instance *p_pars, json_t *obj, char *obj_name)
{
  const T_selectors_list *sl;

  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj1 = 0;
  json_t *item_obj2 = 0;
  json_t *jarray_items = 0;
  uint32_t res = RES_ERROR;

  jarray = json_array();
  if (!jarray)
  {
    JSON_ERR_AND_GOTO;
  }

  for (int32_t i = 0; i < p_pars->selectors_num; i++)
  {
    sl = &p_pars->selectors_array[i];
    item_obj1 = json_object();
    if (!item_obj1)
    {
      JSON_ERR_AND_GOTO;
    }

    if (json_object_set_new(item_obj1, "name", json_string((char const *)sl->name)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    int32_t num = sl->items_cnt;
    if (json_object_set_new(item_obj1, "count", json_integer(num)) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    if (num > 0)
    {
      // Insert an array of selector item values and names
      jarray_items = json_array();
      if (!jarray_items)
      {
        JSON_ERR_AND_GOTO;
      }

      for (int32_t j = 0; j < num; j++)
      {
        item_obj2 = json_object();
        if (!item_obj2)
        {
          JSON_ERR_AND_GOTO;
        }

        if (json_object_set_new(item_obj2, "v", json_integer(sl->items_list[j].val)) != 0)
        {
          JSON_ERR_AND_GOTO;
        }
        if (json_object_set_new(item_obj2, "n", json_string((char const *)sl->items_list[j].caption)) != 0)
        {
          JSON_ERR_AND_GOTO;
        }

        json_array_insert(jarray_items, j, item_obj2);
        JSON_DECREF(item_obj2);
      }
      json_object_set(item_obj1, "items", jarray_items);
      JSON_DECREF(jarray_items);

      uint8_t all_same = 1;

      // Check if all image indices are the same
      int32_t first = sl->items_list[0].img_indx;
      for (int32_t j = 0; j < num; j++)
      {
        if (sl->items_list[j].img_indx != first)
        {
          all_same = 0;
          break;
        }
      }

      if (all_same == 0)
      {
        // Insert an array of selector item image indices if there are different indices
        jarray_items = json_array();
        if (!jarray_items)
        {
          JSON_ERR_AND_GOTO;
        }

        for (int32_t j = 0; j < num; j++)
        {
          if (json_array_insert_new(jarray_items, j, json_integer(sl->items_list[j].img_indx)) != 0)
          {
            JSON_ERR_AND_GOTO;
          }
        }
        json_object_set(item_obj1, "imgs", jarray_items);
        JSON_DECREF(jarray_items);
      }
      else
      {
        // If all indices are the same, insert only one index object
        if (json_object_set_new(item_obj1, "imgs", json_integer(first)) != 0)
        {
          JSON_ERR_AND_GOTO;
        }
      }
    }
    if (json_array_insert(jarray, i, item_obj1) != 0)
    {
      JSON_ERR_AND_GOTO;
    }
    JSON_DECREF(item_obj1);
  }
  if (json_object_set(obj, obj_name, jarray) != 0)
  {
    JSON_ERR_AND_GOTO;
  }
  JSON_DECREF(jarray);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &jarray_items, CLEANUP_TYPE_JSON, &item_obj1, CLEANUP_TYPE_JSON, &item_obj2, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize parameter selectors to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    p_file   - file descriptor
    flags    - JSON formatting flags
    obj_name - key name for the object

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
static uint32_t Serialize_selectors_to_file(const T_NV_parameters_instance *p_pars, FX_FILE *p_file, size_t flags, char *obj_name)
{
  json_t *main_obj = 0;
  json_t *jarray = 0;
  json_t *item_obj1 = 0;
  json_t *item_obj2 = 0;
  json_t *jarray_items = 0;
  uint32_t res = RES_ERROR;

  main_obj = json_object();
  if (!main_obj)
  {
    JSON_ERR_AND_GOTO;
  }

  if (Serialize_selectors_to_obj(p_pars, main_obj, obj_name) != RES_OK) goto error;
  // Write the created object and close it to free dynamic memory
  if (json_dumpf(main_obj, p_file, flags) != 0) goto error;
  JSON_DECREF(main_obj);

  res = RES_OK;
error:
  cleanup_json_resources(CLEANUP_TYPE_JSON, &jarray_items, CLEANUP_TYPE_JSON, &item_obj1, CLEANUP_TYPE_JSON, &item_obj2, CLEANUP_TYPE_JSON, &jarray, CLEANUP_TYPE_JSON, &main_obj, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Write JSON string to file.

  Parameters:
    p_file - file descriptor
    flags  - JSON formatting flags
    str    - string to write

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Write_json_str_to_file(FX_FILE *p_file, size_t flags, char *str)
{
  uint32_t sz;
  sz = strlen(str);
  if (fx_file_write(p_file, str, sz) != FX_SUCCESS) return RES_ERROR;
  if ((flags & JSON_MAX_INDENT) != 0)
  {
    if (fx_file_write(p_file, "\n", 1) != FX_SUCCESS) return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Save device parameter schema to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    filename - file name to save to
    flags    - JSON formatting flags

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Serialze_settings_schema_to_JSON_file(const T_NV_parameters_instance *p_pars, char *filename, size_t flags)
{
  uint32_t res;
  FX_FILE f;

  // Open file for writing
  res = Recreate_file_for_write(&f, (CHAR *)filename);
  if (res != FX_SUCCESS)
  {
    EAPPLOG("Error %d", res);
    return RES_ERROR;
  }

  do
  {
    res = RES_ERROR;

    if (Write_json_str_to_file(&f, flags, "[") != RES_OK) break;

    // Create JSON with an array describing the parameters
    if (Serialize_device_description_to_file(&f, flags, DEVICE_HEADER_KEY) != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, ",") != RES_OK) break;

    if (Serialize_main_params_schema_to_file(p_pars, &f, flags, MAIN_PARAMETERS_KEY) != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, ",") != RES_OK) break;

    if (Serialize_params_tree_to_file(p_pars, &f, flags, PARAMETERS_TREE_KEY) != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, ",") != RES_OK) break;

    // Create JSON with an array describing the selectors in the parameters
    if (Serialize_selectors_to_file(p_pars, &f, flags, "Selectors") != RES_OK) break;

    // if (Serialize_din_list(&f, flags, TYPE_RECORDS_LIST, "RECORDS_LIST") != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, "]") != RES_OK) break;

    res = RES_OK;
    break;

  } while (0);

  if (fx_file_close(&f) != RES_OK) return RES_ERROR;

  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Save device parameter values to JSON file.

  Parameters:
    p_pars   - pointer to parameters structure
    filename - file name to save to
    flags    - JSON formatting flags

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Serialze_settings_to_JSON_file(const T_NV_parameters_instance *p_pars, char *filename, size_t flags)
{
  uint32_t res;
  FX_FILE f;

  // Open file for writing
  res = Recreate_file_for_write(&f, (CHAR *)filename);
  if (res != FX_SUCCESS)
  {
    EAPPLOG("%s", ERR_JSON_ALLOC);
    return res;
  }

  res = RES_ERROR;
  Write_json_str_to_file(&f, flags, "[");

  do
  {
    // Create JSON with an array describing the parameters
    if (Serialize_device_description_to_file(&f, flags, DEVICE_HEADER_KEY) != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, ",") != RES_OK) break;

    if (Serialize_params_vals_to_file(p_pars, &f, flags, MAIN_PARAMETERS_KEY) != RES_OK) break;
    if (Write_json_str_to_file(&f, flags, ",") != RES_OK) break;

    res = RES_OK;
    break;

  } while (0);

  Write_json_str_to_file(&f, flags, "]");

  fx_file_close(&f);
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize device parameters to memory (JSON string).
  Used for editing settings and saving them.

  Parameters:
    p_pars   - pointer to parameters structure
    mem      - pointer to pointer for allocated memory with JSON string (the memory must be freed by the caller, not inside this function)
    str_size - pointer to the size of the resulting string
    flags    - JSON formatting flags

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Serialze_settings_to_mem(const T_NV_parameters_instance *p_pars, char **mem, uint32_t *str_size, uint32_t flags)
{
  char *ptr = NULL;
  json_t *main_arr = 0;
  json_t *arr_item = 0;
  uint32_t res = RES_ERROR;
  size_t size = 0;

  main_arr = json_array();
  if (!main_arr)
  {
    JSON_ERR_AND_GOTO;
  }

  arr_item = json_object();
  if (!arr_item)
  {
    JSON_ERR_AND_GOTO;
  }
  if (Serialize_device_description_to_obj(arr_item, DEVICE_HEADER_KEY) != RES_OK) goto error;
  if (json_array_append(main_arr, arr_item) != 0) goto error;
  JSON_DECREF(arr_item);

  arr_item = json_object();
  if (!arr_item)
  {
    JSON_ERR_AND_GOTO;
  }
  if (Serialize_params_vals_to_obj(p_pars, arr_item, MAIN_PARAMETERS_KEY) != RES_OK) goto error;
  if (json_array_append(main_arr, arr_item) != 0) goto error;
  JSON_DECREF(arr_item);

  // Calculate required memory size for serialization
  size = json_dumpb(main_arr, NULL, 0, flags);
  if (size == 0) goto error;

  *mem = NULL;
  // Allocate required memory
  ptr = App_malloc_pending(size, 10);
  if (ptr == NULL) goto error;

  // Serialize
  size = json_dumpb(main_arr, ptr, size, flags);

  *str_size = size;
  *mem = ptr;
  res = RES_OK;
error:
  // Do not free ptr here, as it is returned to the caller and must be freed by the caller
  cleanup_json_resources(CLEANUP_TYPE_JSON, &main_arr, CLEANUP_TYPE_JSON, &arr_item, -1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Serialize current device state to memory (JSON string).

  Parameters:
    mem      - pointer to pointer for allocated memory with JSON string (the memory must be freed by the caller, not inside this function)
    str_size - pointer to the size of the resulting string

  Return:
    RES_OK on success, RES_ERROR on error
-----------------------------------------------------------------------------------------------------*/
uint32_t Serialze_device_state_to_mem(char **mem, uint32_t *str_size)
{
  char *ptr = NULL;
  json_t *main_arr = 0;
  json_t *arr_item = 0;
  uint32_t flags;
  uint32_t res = RES_ERROR;
  size_t size = 0;
  uint32_t up_time = 0;

  main_arr = json_array();
  if (!main_arr)
  {
    JSON_ERR_AND_GOTO;
  }

  arr_item = json_object();
  if (!arr_item)
  {
    JSON_ERR_AND_GOTO;
  }
  if (Serialize_device_description_to_obj(arr_item, DEVICE_HEADER_KEY) != RES_OK) goto error;
  if (json_array_append(main_arr, arr_item) != 0) goto error;
  JSON_DECREF(arr_item);

  arr_item = json_object();
  if (!arr_item)
  {
    JSON_ERR_AND_GOTO;
  }
  up_time = _tx_time_get() / TX_TIMER_TICKS_PER_SECOND;
  if (json_object_set_new(arr_item, "UpTime_sec", json_integer(up_time)) != 0) goto error;
  if (json_array_append(main_arr, arr_item) != 0) goto error;
  JSON_DECREF(arr_item);

  flags = JSON_ENSURE_ASCII | JSON_COMPACT;

  // Calculate required memory size for serialization
  size = json_dumpb(main_arr, NULL, 0, flags);
  if (size == 0) goto error;

  // Allocate required memory
  *mem = NULL;
  ptr = App_malloc_pending(size, 10);
  if (ptr == NULL) goto error;

  // Serialize
  size = json_dumpb(main_arr, ptr, size, flags);

  *str_size = size;
  *mem = ptr;
  res = RES_OK;
error:
  // Do not free ptr here, as it is returned to the caller and must be freed by the caller
  cleanup_json_resources(CLEANUP_TYPE_JSON, &main_arr, CLEANUP_TYPE_JSON, &arr_item, -1);
  return res;
}
