#ifndef FREEMASTER_COMMAND_HANDLER_H
#define FREEMASTER_COMMAND_HANDLER_H

extern FMSTR_HPIPE fm_pipe;

uint8_t Freemaster_Command_Manager(uint16_t app_command);
void Request_save_app_settings(void);
void Reset_SoC(void);

#endif // FREEMASTER_COMMAND_HANDLER_H
