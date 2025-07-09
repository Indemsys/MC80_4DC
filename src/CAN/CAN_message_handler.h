#ifndef CAN_MESSAGE_HANDLER_H
#define CAN_MESSAGE_HANDLER_H

void Can_message_handler_init(void);
const char* Get_motor_name(uint8_t motor_num);

// CAN command processing control functions
void Disable_can_command_processing(void);
void Restore_can_command_processing(void);

// Global flag indicating CAN command processing state (1 = enabled, 0 = disabled)
extern uint8_t g_can_command_processing_enabled;

#endif  // CAN_MESSAGE_HANDLER_H
