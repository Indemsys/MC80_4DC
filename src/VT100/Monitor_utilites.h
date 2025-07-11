﻿#ifndef MONITOR_UTILITES_H
  #define MONITOR_UTILITES_H

// Специальные коды клавиш VT100
#define VT100_UP_ARROW    0xA0  // Специальный код для стрелки вверх
#define VT100_DOWN_ARROW  0xA1  // Специальный код для стрелки вниз
#define VT100_RIGHT_ARROW 0xA2  // Специальный код для стрелки вправо
#define VT100_LEFT_ARROW  0xA3  // Специальный код для стрелки влево

void         VT100_clr_screen(void);
uint8_t      VT100_find_str_center(uint8_t *str);
void         VT100_send_str_to_pos(uint8_t *str, uint8_t row, uint8_t col);
void         VT100_set_cursor_pos(uint8_t row, uint8_t col);

int          VT100_get_string(char *lp, int n);
int32_t      VT100_edit_string_in_pos(char *buf, int buf_len, int row, char *instr);
int32_t      VT100_edit_string(char *buf, uint32_t buf_len, char *instr);
void         VT100_edit_uinteger_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv);
void         VT100_edit_integer_val(uint32_t row, int32_t *value, int32_t minv, int32_t maxv);
void         VT100_edit_float_val(uint32_t row, float *value, float minv, float maxv);
void         VT100_print_dump(uint32_t addr, void *buf, uint32_t buf_len, uint8_t sym_in_str);

int32_t      VT100_edit_uinteger_hex_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv);
int32_t      VT100_edit_uinteger_val_mode(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv, bool hex_mode);

// Функция для обработки специальных клавиш VT100
int          VT100_wait_special_key(uint8_t *key, int timeout);

#endif
