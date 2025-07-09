#ifndef __PARAMS_EDITOR
  #define __PARAMS_EDITOR

#define           MAX_PARAMETER_STRING_LEN 1024


void              Edit_func(uint8_t b);
void              Params_editor_press_key_handler(uint8_t b);
void              Save_str_to_logfile(char* str);
void              Close_logfile(void);
uint8_t*          Get_mn_name(const T_NV_parameters_instance *p_pars, uint32_t menu_lev);
void              Show_parameters_menu(void);
#endif
