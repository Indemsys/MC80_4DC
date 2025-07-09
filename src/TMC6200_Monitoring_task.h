#ifndef TMC6200_MONITORING_TASK_H
#define TMC6200_MONITORING_TASK_H

// TMC6200 monitoring thread function declarations
void Tmc6200_monitoring_thread_create(void);
void Motdrv_tmc6200_InitMonitoring(void);
void Motdrv_tmc6200_UpdateInitErrorCodes(void);
uint32_t Tmc6200_request_fault_reset(uint8_t driver_num);
bool Tmc6200_wait_fault_reset_completion(uint32_t timeout_ms);

#endif // TMC6200_MONITORING_TASK_H
