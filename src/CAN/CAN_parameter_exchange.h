#ifndef CAN_PARAMETER_EXCHANGE_H
#define CAN_PARAMETER_EXCHANGE_H

/*-----------------------------------------------------------------------------------------------------
  CAN PARAMETER EXCHANGE MODULE FOR MC80 4DC SLAVE DEVICE
  ========================================================

  This module implements the parameter exchange protocol for the MC80 4DC system
  operating as a slave device responding to central controller parameter requests.

  PROTOCOL OVERVIEW:
  - Process parameter read requests from central controller
  - Process parameter write requests from central controller
  - Process parameter save requests to store parameters in NV memory
  - Process controller reset requests
  - Send appropriate responses for all operations
  CAN MESSAGE TYPES:
  - MC80_PARAM_RD: Parameter read request
  - MC80_PARAM_WR: Parameter write request
  - MC80_PARAM_SAVE: Save parameters to NV memory
  - MC80_RESET: Controller reset request
  - MC80_PARAM_ANS: Parameter response (all operations)

  PARAMETER ACCESS:
  - All parameters in MC80_Params database are accessible
  - Parameters identified by CRC16 hash values
  - Data type validation and range checking performed
  - Automatic conversion between local and CAN data formats
  USAGE EXAMPLE:
  1. Initialize: Can_param_exchange_init()
  2. Route incoming CAN messages: Can_param_process_message(rx_msg)
  3. Module handles all protocol details automatically
-----------------------------------------------------------------------------------------------------*/

// Parameter data type identifiers
#define PARAM_TYPE_UINT8                 0x01  // 8-bit unsigned integer
#define PARAM_TYPE_UINT32                0x02  // 32-bit unsigned integer
#define PARAM_TYPE_FLOAT                 0x03  // 32-bit floating point

// Parameter operation status codes
#define PARAM_STATUS_SUCCESS             0x00  // Operation successful
#define PARAM_STATUS_NOT_FOUND           0x01  // Parameter hash not found
#define PARAM_STATUS_INVALID_DATA_TYPE   0x02  // Data type mismatch
#define PARAM_STATUS_INVALID_DATA_LENGTH 0x03  // Data length incorrect
#define PARAM_STATUS_VALUE_OUT_OF_RANGE  0x04  // Value outside valid range
#define PARAM_STATUS_WRITE_PROTECTED     0x05  // Parameter is read-only
#define PARAM_STATUS_STORAGE_ERROR       0x06  // Failed to store parameter
#define PARAM_STATUS_SAVE_SUCCESS        0x07  // Parameters saved to NV memory
#define PARAM_STATUS_SAVE_ERROR          0x08  // Failed to save to NV memory
#define PARAM_STATUS_INVALID_MAGIC_WORD  0x09  // Invalid magic word
#define PARAM_STATUS_RESET_ACCEPTED      0x0A  // Reset request accepted
#define PARAM_STATUS_RESET_INVALID_TYPE  0x0B  // Invalid reset type
#define PARAM_STATUS_RESET_INVALID_MAGIC 0x0C  // Invalid reset magic word
#define PARAM_STATUS_CLEAR_ERRORS_SUCCESS 0x0D  // Motor errors cleared successfully
#define PARAM_STATUS_CLEAR_ERRORS_INVALID_MAGIC 0x0E  // Invalid clear errors magic word

// Save operation options
#define SAVE_OPT_ALL_PARAMS              0x01  // Save all parameters in RAM
#define SAVE_OPT_VERIFY_AFTER            0x02  // Verify after saving
#define SAVE_OPT_BACKUP_PREVIOUS         0x04  // Create backup of previous

// Reset types
#define RESET_TYPE_SOFT                  0x01  // Software reset
#define RESET_TYPE_HARD                  0x02  // Hardware reset
#define RESET_TYPE_FACTORY               0x03  // Factory reset
#define RESET_TYPE_SAFE_MODE             0x04  // Safe mode reset

// Type/Length encoding and decoding macros
#define ENCODE_TYPE_LENGTH(type, length) (((length) << 4) | ((type) & 0x0F))
#define DECODE_TYPE(type_length)         ((type_length) & 0x0F)
#define DECODE_LENGTH(type_length)       (((type_length) >> 4) & 0x0F)

// Timing constants
#define PARAM_READ_TIMEOUT_MS            10    // Parameter read timeout
#define PARAM_WRITE_TIMEOUT_MS           10    // Parameter write timeout
#define PARAM_SAVE_TIMEOUT_MS            1000  // NV memory save timeout

// CAN message structures (packed for exact byte layout)
typedef struct __attribute__((packed))
{
  uint16_t param_hash;   // CRC16 hash of parameter name
  uint8_t  data_type;    // Parameter data type (PARAM_TYPE_*)
  uint8_t  data_length;  // Length of parameter data in bytes (1-4)
  uint8_t  data[4];      // Parameter data (little-endian)
} T_can_param_write_req;

// Parameter read request structure (8 bytes)
typedef struct __attribute__((packed))
{
  uint16_t param_hash;   // CRC16 hash of parameter name
  uint8_t  reserved[6];  // Reserved for future use
} T_can_param_read_req;

// Parameter response structure (8 bytes)
typedef struct __attribute__((packed))
{
  uint16_t param_hash;   // CRC16 hash of parameter name
  uint8_t  status;       // Response status (PARAM_STATUS_*)
  uint8_t  type_length;  // Bits 0-3: data_type, Bits 4-7: data_length
  uint8_t  data[4];      // Parameter data (little-endian)
} T_can_param_response;

// Parameter save request structure (8 bytes)
typedef struct __attribute__((packed))
{
  uint32_t magic_word;    // Magic word for save confirmation (PARAM_SAVE_MAGIC_WORD)
  uint8_t  save_options;  // Save options (SAVE_OPT_*)
  uint8_t  reserved[3];   // Reserved for future use
} T_can_param_save_req;

// Controller reset request structure (8 bytes)
typedef struct __attribute__((packed))
{
  uint32_t magic_word;   // Magic word for reset confirmation (RESET_MAGIC_WORD)
  uint8_t  reset_type;   // Reset type (RESET_TYPE_*)
  uint8_t  delay_ms;     // Delay before reset in milliseconds (0-255)
  uint8_t  reserved[2];  // Reserved for future use
} T_can_reset_req;

// Clear motor errors request structure (8 bytes)
typedef struct __attribute__((packed))
{
  uint32_t magic_word;   // Magic word for confirmation (CLEAR_MOTOR_ERRORS_MAGIC_WORD)
  uint8_t  reserved[4];  // Reserved for future use
} T_can_clear_motor_errors_req;

// Parameter transfer operation result
typedef struct
{
  uint8_t  status;             // Operation status (PARAM_STATUS_*)
  uint16_t param_hash;         // Parameter hash that was processed
  uint8_t  retry_count;        // Number of retries performed
  bool     operation_success;  // Overall operation result
} T_param_operation_result;

// Function declarations
uint32_t Can_param_exchange_init(void);
uint32_t Can_param_process_message(const T_can_msg *rx_msg);
uint32_t Can_param_process_clear_motor_errors_request(const uint8_t *can_data);

#endif  // CAN_PARAMETER_EXCHANGE_H
