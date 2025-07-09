//----------------------------------------------------------------------
// File created on 2025-04-14
//----------------------------------------------------------------------
#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  Find the index of a parameter by its alias.

  Parameters:
    p_pars - Pointer to the parameters instance structure
    alias  - Pointer to the alias string

  Return:
    Index of the parameter if found, -1 otherwise.
-----------------------------------------------------------------------------------------------------*/
int32_t Find_param_by_alias(const T_NV_parameters_instance *p_pars, char *alias)
{
  int i;
  for (i = 0; i < p_pars->items_num; i++)
  {
    if (strcmp((char *)p_pars->items_array[i].var_alias, alias) == 0)
    {
      return i;
    }
  }
  return -1;
}

/*-----------------------------------------------------------------------------------------------------
  Find the index of a parameter by its name.

  Parameters:
    p_pars - Pointer to the parameters instance structure
    name   - Pointer to the name string

  Return:
    Index of the parameter if found, -1 otherwise.
-----------------------------------------------------------------------------------------------------*/
int32_t Find_param_by_name(const T_NV_parameters_instance *p_pars, char *name)
{
  int i;
  for (i = 0; i < p_pars->items_num; i++)
  {
    if (strcmp((char *)p_pars->items_array[i].var_name, name) == 0)
    {
      return i;
    }
  }
  return -1;
}

/*-----------------------------------------------------------------------------------------------------
  Find the index of a parameter by its value pointer.

  Parameters:
    p_pars - Pointer to the parameters instance structure
    ptr    - Pointer to the value

  Return:
    Index of the parameter if found, -1 otherwise.
-----------------------------------------------------------------------------------------------------*/
int32_t Find_param_by_ptr(const T_NV_parameters_instance *p_pars, void *ptr)
{
  int i;
  for (i = 0; i < p_pars->items_num; i++)
  {
    if (p_pars->items_array[i].val == ptr)
    {
      return i;
    }
  }
  return -1;
}

/*-----------------------------------------------------------------------------------------------------
  Convert variable type enum to string representation.

  Parameters:
    vartype - Variable type enum value

  Return:
    Pointer to a string representing the variable type.
-----------------------------------------------------------------------------------------------------*/
const char *Convrt_var_type_to_str(enum vartypes vartype)
{
  switch (vartype)
  {
    case tint8u:
      return "tint8u";
    case tint16u:
      return "tint16u";
    case tint32u:
      return "tint32u";
    case tfloat:
      return "tfloat";
    case tarrofdouble:
      return "tarrofdouble";
    case tstring:
      return "tstring";
    case tarrofbyte:
      return "tarrofbyte";
    case tint32s:
      return "tint32s";
    default:
      return "unknown";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Convert a string to a parameter value.
  The input string must be modifiable.

  Parameters:
    p_pars - Pointer to the parameters instance structure
    in_str - Pointer to the input string (modifiable)
    indx   - Index of the parameter in the items array

  Return:
    RES_OK if conversion is successful, RES_ERROR otherwise.
-----------------------------------------------------------------------------------------------------*/
uint32_t Convert_str_to_parameter(const T_NV_parameters_instance *p_pars, uint8_t *in_str, uint16_t indx)
{
  char         *end;
  float         d_tmp;
  unsigned long ulg_tmp;
  long          slg_tmp;
  uint32_t      res = RES_OK;

  // Convert "True" and "False" strings to 1 and 0 for non-string types
  if (p_pars->items_array[indx].vartype != tstring)
  {
    if (strcmp((char *)in_str, "True") == 0)
    {
      in_str[0] = '1';
      in_str[1] = 0;
    }
    else if (strcmp((char *)in_str, "False") == 0)
    {
      in_str[0] = '0';
      in_str[1] = 0;
    }
  }

  switch (p_pars->items_array[indx].vartype)
  {
    case tint8u:
      ulg_tmp = strtoul((char *)in_str, &end, 10);
      if (*end != '\0' || end == (char *)in_str)  // Invalid conversion
      {
        res = RES_ERROR;
      }
      else if (ulg_tmp > (uint32_t)p_pars->items_array[indx].maxval)  // Check against maxval before casting to uint8_t
      {
        res = RES_ERROR;                                              // Value exceeds maximum allowed
      }
      else if (ulg_tmp < (uint32_t)p_pars->items_array[indx].minval)  // Check against minval
      {
        res = RES_ERROR;                                              // Value is less than minimum allowed
      }
      else
      {
        *(uint8_t *)p_pars->items_array[indx].val = (uint8_t)ulg_tmp;
      }
      break;
    case tint16u:
      ulg_tmp = strtoul((char *)in_str, &end, 10);
      if (*end != '\0' || end == (char *)in_str)
      {
        res = RES_ERROR;
      }
      else if (ulg_tmp > (uint32_t)p_pars->items_array[indx].maxval)
      {
        res = RES_ERROR;
      }
      else if (ulg_tmp < (uint32_t)p_pars->items_array[indx].minval)
      {
        res = RES_ERROR;
      }
      else
      {
        *(uint16_t *)p_pars->items_array[indx].val = (uint16_t)ulg_tmp;
      }
      break;
    case tint32u:
      ulg_tmp = strtoul((char *)in_str, &end, 10);
      if (*end != '\0' || end == (char *)in_str)
      {
        res = RES_ERROR;
      }
      // For tint32u, strtoul might return ULONG_MAX on overflow, which could be a valid maxval.
      // We rely on the comparison with maxval defined in parameters.
      // No explicit check for ULONG_MAX needed if maxval is less than ULONG_MAX.
      else if (ulg_tmp > (uint32_t)p_pars->items_array[indx].maxval)
      {
        res = RES_ERROR;
      }
      else if (ulg_tmp < (uint32_t)p_pars->items_array[indx].minval)
      {
        res = RES_ERROR;
      }
      else
      {
        *(uint32_t *)p_pars->items_array[indx].val = ulg_tmp;
      }
      break;
    case tint32s:
      slg_tmp = strtol((char *)in_str, &end, 10);
      if (*end != '\0' || end == (char *)in_str)
      {
        res = RES_ERROR;
      }
      // strtol returns LONG_MAX or LONG_MIN on overflow.
      // Check these cases if they are not the intended max/min values.
      else if (slg_tmp > (int32_t)p_pars->items_array[indx].maxval)
      {
        res = RES_ERROR;
      }
      else if (slg_tmp < (int32_t)p_pars->items_array[indx].minval)
      {
        res = RES_ERROR;
      }
      else
      {
        *(int32_t *)p_pars->items_array[indx].val = slg_tmp;
      }
      break;
    case tfloat:
      d_tmp = strtof((char *)in_str, &end);
      if (*end != '\0' || end == (char *)in_str)
      {
        res = RES_ERROR;
      }
      else
      {
        if (d_tmp > ((float)p_pars->items_array[indx].maxval)) d_tmp = (float)p_pars->items_array[indx].maxval;
        if (d_tmp < ((float)p_pars->items_array[indx].minval)) d_tmp = (float)p_pars->items_array[indx].minval;
        *(float *)p_pars->items_array[indx].val = d_tmp;
      }
      break;
    case tstring:
    {
      uint8_t *st;
      if (strlen((char *)in_str) >= p_pars->items_array[indx].varlen)
      {
        // Truncate if string is too long
      }
      strncpy((char *)p_pars->items_array[indx].val, (char *)in_str, p_pars->items_array[indx].varlen - 1);
      st                                       = (uint8_t *)p_pars->items_array[indx].val;
      st[p_pars->items_array[indx].varlen - 1] = 0;
    }
    break;
    case tarrofbyte:
    case tarrofdouble:
      res = RES_ERROR;
      break;
    default:
      res = RES_ERROR;
      break;
  }
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Convert a parameter value to a string.

  Parameters:
    p_pars - Pointer to the parameters instance structure
    buf    - Pointer to the output buffer
    maxlen - Maximum length of the output buffer
    indx   - Index of the parameter in the items array

  Return:
    RES_OK if conversion is successful, RES_ERROR otherwise.
    The output buffer is always null-terminated. If the type is not supported, "N/A" is written.
-----------------------------------------------------------------------------------------------------*/
uint32_t Convert_parameter_to_str(const T_NV_parameters_instance *p_pars, uint8_t *buf, uint16_t maxlen, uint16_t indx)
{
  void *val;
  int   res;

  if (p_pars == NULL || buf == NULL || maxlen == 0)
  {
    return RES_ERROR;
  }
  if (indx >= p_pars->items_num)
  {
    buf[0] = '\0';
    return RES_ERROR;
  }

  val = p_pars->items_array[indx].val;
  if (val == NULL && p_pars->items_array[indx].vartype != tstring)
  {
    buf[0] = '\0';
    return RES_ERROR;
  }

  switch (p_pars->items_array[indx].vartype)
  {
    case tint8u:
      res = snprintf((char *)buf, maxlen, p_pars->items_array[indx].format, *(uint8_t *)val);
      break;
    case tint16u:
      res = snprintf((char *)buf, maxlen, p_pars->items_array[indx].format, *(uint16_t *)val);
      break;
    case tint32u:
      res = snprintf((char *)buf, maxlen, p_pars->items_array[indx].format, *(uint32_t *)val);
      break;
    case tint32s:
      res = snprintf((char *)buf, maxlen, p_pars->items_array[indx].format, *(int32_t *)val);
      break;
    case tfloat:
    {
      float f;
      f   = *((float *)val);
      res = snprintf((char *)buf, maxlen, p_pars->items_array[indx].format, (double)f);
      break;
    }
    case tstring:
    {
      int len;
      if (val == NULL)
      {
        buf[0] = '\0';
        res    = 0;
      }
      else
      {
        len = p_pars->items_array[indx].varlen;
        if (len >= maxlen)
        {
          len = maxlen - 1;
        }
        strncpy((char *)buf, (char *)val, len);
        buf[len] = 0;
        res      = strlen((char *)buf);
      }
    }
    break;
    case tarrofbyte:
    case tarrofdouble:
    default:
      strncpy((char *)buf, "N/A", maxlen - 1);
      buf[maxlen - 1] = '\0';
      res             = -1;
      break;
  }

  if (res < 0)
  {
    buf[0] = '\0';
    return RES_ERROR;
  }

  return RES_OK;
}
