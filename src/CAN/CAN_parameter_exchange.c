#include "App.h"

// Module memory allocation
#define CAN_PARAM_RESPONSE_BUFFER_SIZE 32

// Global variables for parameter exchange state
static bool    g_param_exchange_initialized = false;
static uint8_t g_response_buffer[CAN_PARAM_RESPONSE_BUFFER_SIZE];

// Static function declarations
static uint32_t _Send_can_response(uint32_t can_id, const uint8_t *data, uint8_t data_length);
static uint32_t _Get_parameter_data_type(uint16_t param_index, uint8_t *data_type);
static uint32_t _Validate_parameter_range(uint16_t param_index, const uint8_t *data_ptr, uint8_t data_length);
static uint32_t _Perform_controller_reset(uint8_t reset_type, uint8_t delay_ms);
static uint32_t _Can_param_convert_to_can_format(uint16_t param_index, uint8_t *data_type, uint8_t *data_ptr, uint8_t *data_length);
static uint32_t _Can_param_convert_from_can_format(uint16_t param_index, uint8_t data_type, const uint8_t *data_ptr, uint8_t data_length);

/*-----------------------------------------------------------------------------------------------------
  Initialize the parameter exchange module for slave device operation

  Parameters:
    None

  Return:
    RES_OK if initialization successful, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_exchange_init(void)
{
  // Clear response buffer
  memset(g_response_buffer, 0, sizeof(g_response_buffer));

  g_param_exchange_initialized = true;

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Process parameter read request from central controller

  Parameters:
    can_data - Pointer to received CAN message data (8 bytes)

  Return:
    RES_OK if request processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_read_request(const uint8_t *can_data)
{
  const T_can_param_read_req *request;
  T_can_param_response        response;
  uint16_t                    param_index;
  uint8_t                     data_type;
  uint8_t                     data_buffer[4];
  uint8_t                     data_length;

  if (!g_param_exchange_initialized || can_data == NULL)
  {
    return RES_ERROR;
  }

  request     = (const T_can_param_read_req *)can_data;
  // Find parameter by hash
  param_index = Find_param_by_hash(request->param_hash);
  if (param_index == 0xFFFF)
  {
    // Parameter not found - send error response
    response.param_hash  = request->param_hash;
    response.status      = PARAM_STATUS_NOT_FOUND;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Convert parameter to CAN format
  if (_Can_param_convert_to_can_format(param_index, &data_type, data_buffer, &data_length) != RES_OK)
  {
    // Conversion error - send error response
    response.param_hash  = request->param_hash;
    response.status      = PARAM_STATUS_INVALID_DATA_TYPE;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }
  // Send successful response with parameter data
  response.param_hash  = request->param_hash;
  response.status      = PARAM_STATUS_SUCCESS;
  // Encode data type and length into single byte: bits 0-3 = type, bits 4-7 = length
  response.type_length = ENCODE_TYPE_LENGTH(data_type, data_length);

  // Copy data (can now fit 4 bytes in CAN frame)
  uint8_t copy_length  = (data_length <= 4) ? data_length : 4;
  memcpy(response.data, data_buffer, copy_length);
  if (copy_length < 4)
  {
    memset(&response.data[copy_length], 0, 4 - copy_length);
  }

  return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
}

/*-----------------------------------------------------------------------------------------------------
  Process parameter write request from central controller

  Parameters:
    can_data - Pointer to received CAN message data (8 bytes)

  Return:
    RES_OK if request processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_write_request(const uint8_t *can_data)
{
  const T_can_param_write_req *request;
  T_can_param_response         response;
  uint16_t                     param_index;

  if (!g_param_exchange_initialized || can_data == NULL)
  {
    return RES_ERROR;
  }

  request     = (const T_can_param_write_req *)can_data;
  // Find parameter by hash
  param_index = Find_param_by_hash(request->param_hash);
  if (param_index == 0xFFFF)
  {
    // Parameter not found - send error response
    response.param_hash  = request->param_hash;
    response.status      = PARAM_STATUS_NOT_FOUND;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Validate data length
  if (request->data_length == 0 || request->data_length > 4)
  {
    response.param_hash  = request->param_hash;
    response.status      = PARAM_STATUS_INVALID_DATA_LENGTH;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Convert and validate parameter data
  if (_Can_param_convert_from_can_format(param_index, request->data_type, request->data, request->data_length) != RES_OK)
  {
    // Conversion or validation error
    uint8_t error_status = PARAM_STATUS_VALUE_OUT_OF_RANGE;  // Default error

    // Try to determine specific error type
    uint8_t expected_type;
    if (_Get_parameter_data_type(param_index, &expected_type) == RES_OK && expected_type != request->data_type)
    {
      error_status = PARAM_STATUS_INVALID_DATA_TYPE;
    }

    response.param_hash  = request->param_hash;
    response.status      = error_status;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }
  // Parameter written successfully
  response.param_hash  = request->param_hash;
  response.status      = PARAM_STATUS_SUCCESS;
  // Encode data type and length into single byte: bits 0-3 = type, bits 4-7 = length
  response.type_length = ENCODE_TYPE_LENGTH(request->data_type, request->data_length);

  // Echo back the written data (can now fit 4 bytes)
  uint8_t copy_length  = (request->data_length <= 4) ? request->data_length : 4;
  memcpy(response.data, request->data, copy_length);
  if (copy_length < 4)
  {
    memset(&response.data[copy_length], 0, 4 - copy_length);
  }

  return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
}

/*-----------------------------------------------------------------------------------------------------
  Process parameter save request from central controller

  Parameters:
    can_data - Pointer to received CAN message data (8 bytes)

  Return:
    RES_OK if request processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_save_request(const uint8_t *can_data)
{
  const T_can_param_save_req *request;
  T_can_param_response        response;

  if (!g_param_exchange_initialized || can_data == NULL)
  {
    return RES_ERROR;
  }

  request = (const T_can_param_save_req *)can_data;
  // Validate magic word
  if (request->magic_word != PARAM_SAVE_MAGIC_WORD)
  {
    response.param_hash  = 0x0000;
    response.status      = PARAM_STATUS_INVALID_MAGIC_WORD;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }  // Perform parameter save operation
  Request_save_settings(APPLICATION_PARAMS, MEDIA_TYPE_DATAFLASH, NULL);

  // Wait for save operation to complete (timeout 5 seconds)
  uint32_t save_result;
  bool     operation_completed = Check_save_operation_status(&save_result, 1000);

  response.param_hash          = 0x0000;
  if (operation_completed && save_result == RES_OK)
  {
    response.status = PARAM_STATUS_SAVE_SUCCESS;
  }
  else if (operation_completed && save_result != RES_OK)
  {
    response.status = PARAM_STATUS_SAVE_ERROR;
  }
  else
  {
    // Timeout occurred
    response.status = PARAM_STATUS_SAVE_ERROR;
  }
  response.type_length = ENCODE_TYPE_LENGTH(0, 0);
  memset(response.data, 0, sizeof(response.data));

  return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
}

/*-----------------------------------------------------------------------------------------------------
  Process controller reset request from central controller

  Parameters:
    can_data - Pointer to received CAN message data (8 bytes)

  Return:
    RES_OK if request processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_reset_request(const uint8_t *can_data)
{
  const T_can_reset_req *request;
  T_can_param_response   response;

  if (!g_param_exchange_initialized || can_data == NULL)
  {
    return RES_ERROR;
  }

  request = (const T_can_reset_req *)can_data;
  // Validate magic word
  if (request->magic_word != RESET_MAGIC_WORD)
  {
    response.param_hash  = 0x0000;
    response.status      = PARAM_STATUS_RESET_INVALID_MAGIC;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Validate reset type
  if (request->reset_type < RESET_TYPE_SOFT || request->reset_type > RESET_TYPE_SAFE_MODE)
  {
    response.param_hash  = 0x0000;
    response.status      = PARAM_STATUS_RESET_INVALID_TYPE;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Send acknowledgment before reset
  response.param_hash  = 0x0000;
  response.status      = PARAM_STATUS_RESET_ACCEPTED;
  response.type_length = ENCODE_TYPE_LENGTH(0, 0);
  memset(response.data, 0, sizeof(response.data));

  _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));

  // Perform controller reset (delayed)
  return _Perform_controller_reset(request->reset_type, request->delay_ms);
}

/*-----------------------------------------------------------------------------------------------------
  Process incoming CAN parameter message based on CAN ID

  Parameters:
    rx_msg - Pointer to received CAN message structure

  Return:
    RES_OK if message processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_message(const T_can_msg *rx_msg)
{
  if (!g_param_exchange_initialized || rx_msg == NULL)
  {
    return RES_ERROR;
  }

  // Route message based on CAN ID
  switch (rx_msg->can_id)
  {
    case MC80_PARAM_RD:
      return Can_param_process_read_request(rx_msg->data);

    case MC80_PARAM_WR:
      return Can_param_process_write_request(rx_msg->data);

    case MC80_PARAM_SAVE:
      return Can_param_process_save_request(rx_msg->data);

    case MC80_RESET:
      return Can_param_process_reset_request(rx_msg->data);

    case MC80_CLEAR_MOTOR_ERRORS:
      return Can_param_process_clear_motor_errors_request(rx_msg->data);

    default:
      // Unknown parameter message ID
      return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Convert parameter value from local format to CAN transmission format

  Parameters:
    param_index - Index of parameter in MC80_Params array
    data_type - Pointer to store parameter data type
    data_ptr - Pointer to buffer for converted data
    data_length - Pointer to store data length

  Return:
    RES_OK if conversion successful, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Can_param_convert_to_can_format(uint16_t param_index, uint8_t *data_type, uint8_t *data_ptr, uint8_t *data_length)
{
  if (param_index >= wvar_inst.items_num || data_type == NULL || data_ptr == NULL || data_length == NULL)
  {
    return RES_ERROR;
  }

  const T_NV_parameters *param       = &wvar_inst.items_array[param_index];
  void                  *param_value = param->val;

  if (param_value == NULL)
  {
    return RES_ERROR;
  }

  // Convert based on parameter variable type
  switch (param->vartype)
  {
    case tint8u:
    {
      *data_type   = PARAM_TYPE_UINT8;
      *data_length = 1;
      data_ptr[0]  = *(uint8_t *)param_value;
      break;
    }

    case tint32u:
    {
      *data_type     = PARAM_TYPE_UINT32;
      *data_length   = 4;
      uint32_t value = *(uint32_t *)param_value;
      data_ptr[0]    = (uint8_t)(value & 0xFF);
      data_ptr[1]    = (uint8_t)((value >> 8) & 0xFF);
      data_ptr[2]    = (uint8_t)((value >> 16) & 0xFF);
      data_ptr[3]    = (uint8_t)((value >> 24) & 0xFF);
      break;
    }

    case tfloat:
    {
      *data_type   = PARAM_TYPE_FLOAT;
      *data_length = 4;
      float value  = *(float *)param_value;
      memcpy(data_ptr, &value, sizeof(float));
      break;
    }

    default:
      return RES_ERROR;  // Unsupported data type for CAN transmission
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Convert parameter value from CAN transmission format to local format

  Parameters:
    param_index - Index of parameter in MC80_Params array
    data_type - Parameter data type from CAN message
    data_ptr - Pointer to CAN data
    data_length - Length of CAN data

  Return:
    RES_OK if conversion successful, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Can_param_convert_from_can_format(uint16_t param_index, uint8_t data_type, const uint8_t *data_ptr, uint8_t data_length)
{
  if (param_index >= wvar_inst.items_num || data_ptr == NULL)
  {
    return RES_ERROR;
  }

  const T_NV_parameters *param       = &wvar_inst.items_array[param_index];
  void                  *param_value = param->val;

  if (param_value == NULL)
  {
    return RES_ERROR;
  }

  // Validate data type compatibility
  uint8_t expected_type;
  if (_Get_parameter_data_type(param_index, &expected_type) != RES_OK || expected_type != data_type)
  {
    return RES_ERROR;
  }

  // Convert based on data type
  switch (data_type)
  {
    case PARAM_TYPE_UINT8:
    {
      if (data_length != 1)
      {
        return RES_ERROR;
      }
      uint8_t value = data_ptr[0];
      if (_Validate_parameter_range(param_index, (const uint8_t *)&value, sizeof(value)) == RES_OK)
      {
        *(uint8_t *)param_value = value;
      }
      else
      {
        return RES_ERROR;
      }
      break;
    }

    case PARAM_TYPE_UINT32:
    {
      if (data_length != 4)
      {
        return RES_ERROR;
      }
      uint32_t value = ((uint32_t)data_ptr[0]) |
                       ((uint32_t)data_ptr[1] << 8) |
                       ((uint32_t)data_ptr[2] << 16) |
                       ((uint32_t)data_ptr[3] << 24);
      if (_Validate_parameter_range(param_index, (const uint8_t *)&value, sizeof(value)) == RES_OK)
      {
        *(uint32_t *)param_value = value;
      }
      else
      {
        return RES_ERROR;
      }
      break;
    }

    case PARAM_TYPE_FLOAT:
    {
      if (data_length != 4)
      {
        return RES_ERROR;
      }
      float value;
      memcpy(&value, data_ptr, sizeof(float));
      if (_Validate_parameter_range(param_index, (const uint8_t *)&value, sizeof(value)) == RES_OK)
      {
        *(float *)param_value = value;
      }
      else
      {
        return RES_ERROR;
      }
      break;
    }

    default:
      return RES_ERROR;
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Send CAN response message (static helper function)

  Parameters:
    can_id - CAN message identifier
    data - Pointer to message data
    data_length - Length of message data

  Return:
    RES_OK if message sent successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Send_can_response(uint32_t can_id, const uint8_t *data, uint8_t data_length)
{
  uint32_t result;

  // Validate parameters
  if (data_length > 8)
  {
    return RES_ERROR;
  }
  // Send CAN message using existing CAN driver
  result = Can_send_extended_data(can_id, (uint8_t *)data, data_length);

  if (result == CAN_SEND_SUCCESS)
  {
    return RES_OK;
  }
  else
  {
    APPLOG("CAN Parameter Exchange: Failed to send response, error code: %u", result);
    return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Perform controller reset (static helper function)

  Parameters:
    reset_type - Type of reset to perform
    delay_ms - Delay before reset in milliseconds

  Return:
    RES_OK if reset initiated successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Perform_controller_reset(uint8_t reset_type, uint8_t delay_ms)
{
  APPLOG("CAN Parameter Exchange: Reset requested, type: %u, delay: %u ms", reset_type, delay_ms);

  // Add delay before reset
  if (delay_ms > 0)
  {
    tx_thread_sleep(delay_ms);
  }

  switch (reset_type)
  {
    case RESET_TYPE_SOFT:
      APPLOG("CAN Parameter Exchange: Performing software reset");
      // Use existing system reset function
      Reset_system();
      break;

    case RESET_TYPE_HARD:
      APPLOG("CAN Parameter Exchange: Performing hardware reset");
      // Same as soft reset - hardware reset is handled by Reset_system()
      Reset_system();
      break;

    case RESET_TYPE_FACTORY:
      APPLOG("CAN Parameter Exchange: Performing factory reset");
      // Restore factory defaults then reset
      Return_def_params(APPLICATION_PARAMS);
      // Save default parameters to NV memory
      Save_settings(APPLICATION_PARAMS, MEDIA_TYPE_DATAFLASH, NULL);
      // Reset system
      Reset_system();
      break;

    case RESET_TYPE_SAFE_MODE:
      APPLOG("CAN Parameter Exchange: Performing safe mode reset");
      // For safe mode, just perform normal reset - safe mode handled by application startup
      Reset_system();
      break;
    default:
      APPLOG("CAN Parameter Exchange: Unknown reset type: %u", reset_type);
      return RES_ERROR;
  }

  // This code will never be reached due to system reset, but keep for completeness
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Get parameter data type (static helper function)

  Parameters:
    param_index - Index of parameter in MC80_Params array
    data_type - Pointer to store data type

  Return:
    RES_OK if data type retrieved, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Get_parameter_data_type(uint16_t param_index, uint8_t *data_type)
{
  if (param_index >= wvar_inst.items_num || data_type == NULL)
  {
    return RES_ERROR;
  }

  const T_NV_parameters *param = &wvar_inst.items_array[param_index];

  switch (param->vartype)
  {
    case tint8u:
      *data_type = PARAM_TYPE_UINT8;
      break;
    case tint32u:
      *data_type = PARAM_TYPE_UINT32;
      break;
    case tfloat:
      *data_type = PARAM_TYPE_FLOAT;
      break;
    default:
      return RES_ERROR;
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Validate parameter range (static helper function)

  Parameters:
    param_index - Index of parameter in MC80_Params array
    data_ptr - Pointer to parameter data
    data_length - Length of parameter data

  Return:
    RES_OK if value is within range, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Validate_parameter_range(uint16_t param_index, const uint8_t *data_ptr, uint8_t data_length)
{
  if (param_index >= wvar_inst.items_num || data_ptr == NULL)
  {
    return RES_ERROR;
  }

  const T_NV_parameters *param = &wvar_inst.items_array[param_index];

  switch (param->vartype)
  {
    case tint8u:
    {
      if (data_length != sizeof(uint8_t))
      {
        return RES_ERROR;
      }
      uint8_t value = *(const uint8_t *)data_ptr;
      if (value < param->minval || value > param->maxval)
      {
        return RES_ERROR;
      }
      break;
    }

    case tint32u:
    {
      if (data_length != sizeof(uint32_t))
      {
        return RES_ERROR;
      }
      uint32_t value = *(const uint32_t *)data_ptr;
      if (value < param->minval || value > param->maxval)
      {
        return RES_ERROR;
      }
      break;
    }

    case tfloat:
    {
      if (data_length != sizeof(float))
      {
        return RES_ERROR;
      }
      float value = *(const float *)data_ptr;
      if (value < param->minval || value > param->maxval)
      {
        return RES_ERROR;
      }
      break;
    }

    default:
      return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Process clear all system errors request from central controller

  Parameters:
    can_data - Pointer to received CAN message data (8 bytes)

  Return:
    RES_OK if request processed successfully, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_param_process_clear_motor_errors_request(const uint8_t *can_data)
{
  const T_can_clear_motor_errors_req *request;
  T_can_param_response                response;

  if (!g_param_exchange_initialized || can_data == NULL)
  {
    return RES_ERROR;
  }

  request = (const T_can_clear_motor_errors_req *)can_data;

  // Validate magic word
  if (request->magic_word != CLEAR_MOTOR_ERRORS_MAGIC_WORD)
  {
    response.param_hash  = 0x0000;
    response.status      = PARAM_STATUS_CLEAR_ERRORS_INVALID_MAGIC;
    response.type_length = ENCODE_TYPE_LENGTH(0, 0);
    memset(response.data, 0, sizeof(response.data));

    return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
  }

  // Clear all system error flags
  App_clear_all_error_flags();

  // Request TMC6200 fault reset for both drivers
  uint32_t tmc6200_reset_result = Tmc6200_request_fault_reset(0);  // 0 = both drivers
  if (tmc6200_reset_result == RES_OK)
  {
    // Wait for TMC6200 fault reset to complete using event flags (with timeout)
    bool reset_completed = Tmc6200_wait_fault_reset_completion(500);  // 0.5 second timeout

    if (reset_completed)
    {
      APPLOG("CAN Parameter Exchange: TMC6200 drivers reset command completed");
    }
    else
    {
      APPLOG("CAN Parameter Exchange: TMC6200 drivers reset command timeout - continuing anyway");
    }
  }
  else
  {
    APPLOG("CAN Parameter Exchange: Failed to request TMC6200 drivers reset");
  }

  // Send success response
  response.param_hash  = 0x0000;
  response.status      = PARAM_STATUS_CLEAR_ERRORS_SUCCESS;
  response.type_length = ENCODE_TYPE_LENGTH(0, 0);
  memset(response.data, 0, sizeof(response.data));

  APPLOG("CAN Parameter Exchange: All system error flags cleared via CAN command");

  return _Send_can_response(MC80_PARAM_ANS, (const uint8_t *)&response, sizeof(response));
}
