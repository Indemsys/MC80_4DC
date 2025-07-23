#ifndef MONITOR_OSPI_H
#define MONITOR_OSPI_H

#include "App.h"

#ifdef __cplusplus
extern "C" {
#endif

// External reference to OSPI test menu
extern const T_VT100_Menu MENU_OSPI;

// Function prototypes for OSPI testing
void OSPI_test_info(uint8_t keycode);
void OSPI_test_status(uint8_t keycode);
void OSPI_test_read(uint8_t keycode);
void OSPI_test_memory_mapped_write(uint8_t keycode);
void OSPI_test_erase(uint8_t keycode);
void OSPI_test_pattern(uint8_t keycode);
void OSPI_test_sector(uint8_t keycode);
void OSPI_test_xip_mode(uint8_t keycode);

#ifdef __cplusplus
}
#endif

#endif // MONITOR_OSPI_H
