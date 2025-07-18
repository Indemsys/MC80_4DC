#ifndef RTT_UTILS_H
#define RTT_UTILS_H

#include "SEGGER_RTT.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Buffer size for formatted messages
#define RTT_FORMAT_BUFFER_SIZE 256

// Universal RTT printf function using sprintf
int RTT_printf(unsigned int channel, const char* format, ...);
int RTT_err_printf(unsigned int channel, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // RTT_UTILS_H
