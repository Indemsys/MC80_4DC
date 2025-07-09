// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.09.26
// 23:11:19
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

static uint32_t _JSON_Deser_params(uint8_t ptype, json_t *root);
static uint32_t _JSON_find_array(json_t *root, json_t **object, char const *key_name);

/*-----------------------------------------------------------------------------------------------------
  Searches for a JSON array with the specified key name in the root array.

  Parameters:
    root     - Pointer to the root JSON array.
    object   - Double pointer to store the found array object (borrowed reference).
    key_name - Name of the key to search for.

  Return:
    RES_OK if the array is found, RES_ERROR otherwise.
    If the key is found but its value is not an array, returns RES_ERROR.
-----------------------------------------------------------------------------------------------------*/
static uint32_t _JSON_find_array(json_t *root, json_t **object, char const *key_name)
{
  json_t  *item;
  uint32_t err = 1;
  uint32_t n   = 0;
  uint32_t i;

  // Set object to NULL in case not found or root is empty
  *object = NULL;

  // Check if root is a valid array
  if (!json_is_array(root))
  {
    EAPPLOG("Error: Root element provided to _JSON_find_array is not an array.");
    return RES_ERROR;
  }

  err = 1;
  n   = json_array_size(root);
  // Iterate through all elements of the root array
  for (i = 0; i < n; i++)
  {
    item = json_array_get(root, i);  // Get array element (borrowed ref)

    // Check if item is an object
    if (item && json_is_object(item))
    {
      *object = json_object_get(item, key_name);  // Search for object with the given name (borrowed ref)

      if (json_is_array(*object))
      {
        err = 0;
        break;  // Found the key and its value is an array
      }
      // If key exists but value is not array, *object might be non-NULL but json_is_array was false.
      // Continue searching in next item.
    }
    // If item is not an object, json_object_get returns NULL, json_is_array(NULL) is false, loop continues.
  }

  if (err == 1)
  {
    // Key not found, or found but its value was not an array in any element.
    EAPPLOG("Error: Key '%s' not found or its value is not a JSON array.", key_name);
    return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Deserializes parameters from a JSON array and updates the settings instance.

  Parameters:
    ptype - Parameter type (used to select settings instance)
    root  - Pointer to the root JSON array

  Return:
    RES_OK if all parameters are successfully updated, RES_ERROR otherwise.
    Returns RES_ERROR immediately on structural errors or if any conversion fails.
-----------------------------------------------------------------------------------------------------*/
static uint32_t _JSON_Deser_params(uint8_t ptype, json_t *root)
{
  json_t  *params = 0;
  json_t  *item   = 0;
  int32_t  indx;
  char    *var_name;
  char    *val;
  uint32_t i;
  uint32_t err;
  uint32_t hits_cnt                      = 0;
  uint32_t loop_err_cnt                  = 0;  // Counter for conversion errors within the loop

  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (p_pars == NULL)
  {
    return RES_ERROR;
  }

  err = _JSON_find_array(root, &params, MAIN_PARAMETERS_KEY);
  if (err != RES_OK)
  {
    return RES_ERROR;  // Error logged in _JSON_find_array
  }

  for (i = 0; i < json_array_size(params); i++)
  {
    item = json_array_get(params, i);

    // Check for structural error: Item must be an array
    if (!json_is_array(item))
    {
      EAPPLOG("Error: Parameter item at index %d is not an array.", i);
      return RES_ERROR;  // Return immediately on structural error
    }

    // Check for structural error: Item must unpack to [string, string]
    if (json_unpack(item, "[ss]", &var_name, &val) != 0)
    {
      EAPPLOG("Error unpacking parameter at index %d. Expected [string, string].", i);
      return RES_ERROR;  // Return immediately on structural error
    }

    // If structure is valid, proceed with parameter lookup and conversion
    indx = Find_param_by_name(p_pars, var_name);
    if (indx >= 0)
    {
      // Attempt conversion
      if (Convert_str_to_parameter(p_pars, (uint8_t *)val, indx) == RES_OK)
      {
        hits_cnt++;
      }
      else
      {
        EAPPLOG("Error converting parameter '%s' with value '%s'.", var_name, val);  // Log conversion error
        loop_err_cnt++;
      }
    }
    else
    {
      EAPPLOG("Warning: Parameter '%s' not found in settings definition.", var_name);  // Log if parameter name from JSON is not found
    }
  }

  // After processing all items:
  if (hits_cnt > 0)
  {
    EAPPLOG("Successfully updated %d params.", hits_cnt);
  }

  // Return error if any conversion errors occurred during the loop
  if (loop_err_cnt > 0)
  {
    EAPPLOG("Encountered %d conversion errors during parameter deserialization.", loop_err_cnt);
    return RES_ERROR;
  }

  return RES_OK;  // Return OK only if no structural or conversion errors occurred
}

/*-----------------------------------------------------------------------------------------------------
  Deserializes date and time from storage.
  Expected JSON array format: [year, month, day, weekday, hour, minute, second]
  All values must be integers.

  Parameters:
    root - JSON root object containing the settings structure.

  Return:
    RES_OK on success, specific negative error code on failure.
    -1: Not an array
    -2: Incorrect array size
    -3: Missing element
    -4: Element not an integer
    -5: Failed to unpack integer
    RES_ERROR: Key not found (from _JSON_find_array)
-----------------------------------------------------------------------------------------------------*/
uint32_t _JSON_Deser_DT(json_t *root)
{
  json_t    *params   = 0;
  json_t    *item     = 0;
  rtc_time_t rt_time  = {0};
  int       *fields[] = {&rt_time.tm_year, &rt_time.tm_mon, &rt_time.tm_mday, &rt_time.tm_wday, &rt_time.tm_hour, &rt_time.tm_min, &rt_time.tm_sec};
  size_t     i;

#define RES_ERR_DT_NOT_ARRAY    ((uint32_t) - 1)
#define RES_ERR_DT_WRONG_SIZE   ((uint32_t) - 2)
#define RES_ERR_DT_MISSING_ELEM ((uint32_t) - 3)
#define RES_ERR_DT_NOT_INTEGER  ((uint32_t) - 4)
#define RES_ERR_DT_UNPACK_FAIL  ((uint32_t) - 5)

  if (_JSON_find_array(root, &params, DATETIME_SETTINGS_KEY) != RES_OK)
  {
    // Error already logged in _JSON_find_array
    return RES_ERROR;
  }

  // Validate that params is an array
  if (!json_is_array(params))
  {
    return RES_ERR_DT_NOT_ARRAY;
  }

  // Validate array size
  if (json_array_size(params) != 7)
  {
    return RES_ERR_DT_WRONG_SIZE;
  }

  for (i = 0; i < 7; i++)
  {
    item = json_array_get(params, i);
    if (!item)
    {
      return RES_ERR_DT_MISSING_ELEM;
    }

    if (!json_is_integer(item))
    {
      return RES_ERR_DT_NOT_INTEGER;
    }

    if (json_unpack(item, "i", fields[i]) != 0)
    {
      return RES_ERR_DT_UNPACK_FAIL;
    }
  }
  // If loop completed without errors:
  // Adjust year and month according to struct tm standard
  rt_time.tm_year = rt_time.tm_year - 1900;  // Year since 1900
  rt_time.tm_mon  = rt_time.tm_mon - 1;      // Month is 0-11

  // Optional: Add range validation here if needed
  // Example: if (rt_time.tm_mon < 0 || rt_time.tm_mon > 11) { return RES_ERR_DT_INVALID_VALUE; }

  RTC_set_system_DateTime(&rt_time);
  EAPPLOG("DateTime deserialized and set successfully.");  // Log success
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Deserializes parameters from storage.

  Parameters:
    ptype - Parameter type (used to select settings instance)
    text  - Pointer to the JSON text buffer

  Return:
    RES_OK if deserialization is successful, RES_ERROR otherwise.
    Logs errors and success messages accordingly.
-----------------------------------------------------------------------------------------------------*/
uint32_t JSON_Deser_settings(uint8_t ptype, char *text)
{
  json_t      *root;
  json_error_t error;
  uint32_t     res;

  root = json_loads(text, 0, &error);

  if (!root)
  {
    EAPPLOG("JSON decoding error: on line %d: %s", error.line, error.text);
    return RES_ERROR;
  }

  if (!json_is_array(root))
  {
    EAPPLOG("JSON decoding error: root is not array.");
    json_decref(root);
    return RES_ERROR;
  }

  res = _JSON_Deser_params(ptype, root);

  json_decref(root);

  if (res != RES_OK)
  {
    EAPPLOG("Deserialization failed.");
  }
  else
  {
    EAPPLOG("Deserialization done successfully.");
  }

  return res;
}
