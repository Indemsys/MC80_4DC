#ifndef __FREEMASTER_LOOP
  #define __FREEMASTER_LOOP

  #define FMCMD_RESET_DEVICE                   1
  #define FMCMD_CHECK_LOG_PIPE                 7
  #define FMCMD_SAVE_APP_PARAMS                8
  #define FMCMD_RESET_DRIVER_FAULTS            21

  // Motor control commands - Forward direction
  #define FMCMD_MOTOR_1_FORWARD                30
  #define FMCMD_MOTOR_2_FORWARD                31
  #define FMCMD_MOTOR_3_FORWARD                32
  #define FMCMD_MOTOR_4_FORWARD                33

  // Motor control commands - Reverse direction
  #define FMCMD_MOTOR_1_REVERSE                34
  #define FMCMD_MOTOR_2_REVERSE                35
  #define FMCMD_MOTOR_3_REVERSE                36
  #define FMCMD_MOTOR_4_REVERSE                37

  // Motor control commands - Soft stop
  #define FMCMD_MOTOR_1_SOFT_STOP              38
  #define FMCMD_MOTOR_2_SOFT_STOP              39
  #define FMCMD_MOTOR_3_SOFT_STOP              40
  #define FMCMD_MOTOR_4_SOFT_STOP              41

  // Motor control commands - Emergency stop
  #define FMCMD_MOTOR_EMERGENCY_STOP_ALL       42
  #define FMCMD_MOTOR_1_EMERGENCY_STOP         43
  #define FMCMD_MOTOR_2_EMERGENCY_STOP         44
  #define FMCMD_MOTOR_3_EMERGENCY_STOP         45
  #define FMCMD_MOTOR_4_EMERGENCY_STOP         46

  // Motor control commands - Coast stop
  #define FMCMD_MOTOR_COAST_ALL                47

  // Motor control commands - Driver settings
  #define FMCMD_MOTOR_TOGGLE_DRV1_EN           49
  #define FMCMD_MOTOR_TOGGLE_DRV2_EN           50
  #define FMCMD_MOTOR_CALIBRATE_CURRENT        51

  // CAN command processing control
  #define FMCMD_DISABLE_CAN_COMMANDS           52
  #define FMCMD_ENABLE_CAN_COMMANDS            53

  #define FMCMD_MOTOR_RESET_MAX_CURRENT        54

  #define FREEMASTER_ON_NET                    0
  #define FREEMASTER_ON_SERIAL                 1
  #define FREEMASTER_NO_INTERFACE              2

#include "FreeMaster_USB_drv.h"

uint32_t Thread_FreeMaster_create(void);
void     FreeMaster_task_delete(void);
void     Task_FreeMaster(uint32_t initial_data);

#endif
