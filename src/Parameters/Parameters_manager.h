#ifndef PARAMETERS_MANAGER_H
#define PARAMETERS_MANAGER_H

int32_t     Find_param_by_alias(const T_NV_parameters_instance *p_pars, char *alias);
int32_t     Find_param_by_name(const T_NV_parameters_instance *p_pars, char *name);
int32_t     Find_param_by_ptr(const T_NV_parameters_instance *p_pars, void *ptr);
const char *Convrt_var_type_to_str(enum vartypes vartype);
uint32_t    Convert_str_to_parameter(const T_NV_parameters_instance *p_pars, uint8_t *in_str, uint16_t indx);
uint32_t    Convert_parameter_to_str(const T_NV_parameters_instance *p_pars, uint8_t *buf, uint16_t maxlen, uint16_t indx);

#endif // PARAMETERS_MANAGER_H
