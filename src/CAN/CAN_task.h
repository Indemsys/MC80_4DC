#ifndef CAN_TASK_H
#define CAN_TASK_H

#define CAN_TX_QUEUE_DEPTH     20
#define CAN_TX_QUEUE_MSG_SIZE  4  // Size in ULONG units (16 bytes)
#define CAN_RX_QUEUE_DEPTH     20
#define CAN_RX_QUEUE_MSG_SIZE  4  // Size in ULONG units (16 bytes)

// CAN send message return codes
#define CAN_SEND_SUCCESS       0  // Success
#define CAN_SEND_INVALID_PARAM 1  // Invalid parameter
#define CAN_SEND_QUEUE_FULL    2  // Queue full
#define CAN_SEND_QUEUE_ERROR   3  // Queue send error
#define CAN_SEND_NOT_READY     4  // CAN not ready

// CAN communication monitoring constants
#define CAN_COMMUNICATION_TIMEOUT_MS    500   // 0.5 second timeout for CAN communication monitoring
#define CAN_COMMUNICATION_CHECK_PERIOD  50    // Check communication status every 50ms

// CAN error statistics structure
typedef struct
{                                        // Transmission errors
  uint32_t tx_queue_full_errors;         // TX queue full errors
  uint32_t tx_queue_send_errors;         // TX queue send errors
  uint32_t tx_hardware_errors;           // CAN hardware TX errors
  uint32_t tx_invalid_param_errors;      // TX invalid parameter errors
  uint32_t tx_not_ready_errors;          // TX not ready errors
  uint32_t tx_semaphore_timeout_errors;  // TX semaphore timeout errors

  // Reception errors
  uint32_t rx_queue_full_errors;     // RX queue full errors (from interrupt)
  uint32_t rx_queue_receive_errors;  // RX queue receive errors
  uint32_t rx_invalid_param_errors;  // RX invalid parameter errors
  uint32_t rx_not_ready_errors;      // RX not ready errors
  uint32_t rx_overflow_errors;       // RX buffer overflow errors

  // Hardware/protocol errors
  uint32_t bus_off_errors;       // CAN bus off errors
  uint32_t error_warning_count;  // Error warning events
  uint32_t error_passive_count;  // Error passive events
  uint32_t bus_lock_errors;      // Bus lock errors
  uint32_t tx_aborted_count;     // TX aborted events
  uint32_t message_lost_errors;  // Message lost errors

  // General counters
  uint32_t total_tx_messages;      // Total transmitted messages
  uint32_t total_rx_messages;      // Total received messages
  uint32_t callback_null_pointer;  // Callback called with NULL pointer
} T_can_error_counters;

// CAN message structure for queue transmission/reception
typedef __packed struct
{
  uint32_t can_id;      // CAN ID
  uint8_t  dlc;         // Data length code (0-8)
  uint8_t  frame_type;  // CAN_FRAME_TYPE_DATA or CAN_FRAME_TYPE_REMOTE
  uint8_t  id_mode;     // CAN_ID_MODE_STANDARD or CAN_ID_MODE_EXTENDED
  uint8_t  reserved;    // Reserved byte for alignment
  uint8_t  data[8];     // CAN data bytes (up to 8 bytes)
} T_can_msg;

// Callback function type for received CAN messages
typedef void (*T_can_rx_callback)(const T_can_msg* rx_msg);

extern const can_instance_t       g_CANFD;
extern canfd_instance_ctrl_t      g_CANFD_ctrl;
extern const can_cfg_t            g_CANFD_cfg;
extern const canfd_extended_cfg_t g_CANFD_cfg_extend;
extern canfd_global_cfg_t         g_canfd_global_cfg;
extern bool                       g_can_ready;
extern T_can_error_counters       g_can_error_counters;
extern uint32_t                  g_can_last_rx_time;          // Time of last received CAN message
extern bool                      g_can_communication_active;  // Flag indicating if CAN communication is active

void                  Can_thread_create(void);
void                  Can_rx_thread_create(void);
void                  CANFD_callback(can_callback_args_t* p_args);
uint32_t              Can_send_extended_data(uint32_t can_id, uint8_t* data, uint8_t dlc);
bool                  Can_is_ready(void);
uint32_t              Can_get_rx_queue_count(void);
void                  Can_reset_error_counters(void);
T_can_error_counters* Can_get_error_counters(void);
void                  Can_set_rx_callback(T_can_rx_callback callback);
T_can_rx_callback     Can_get_rx_callback(void);
void                  Can_check_communication_timeout(void);

#endif  // CAN_TASK_H
