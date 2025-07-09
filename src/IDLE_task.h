#ifndef IDLE_TASK_H
#define IDLE_TASK_H

// External variables
extern volatile uint32_t g_aver_cpu_usage;
extern volatile uint32_t g_cpu_usage;
extern volatile uint32_t idle_counter;

// Function prototypes
void Init_save_params_mutex(void);
void Request_save_settings(uint8_t ptype, uint8_t media_type, char *file_name);
bool Check_save_operation_status(uint32_t *result, uint32_t timeout_ms);
void IDLE_thread_create(void);

#endif  // IDLE_TASK_H
