#ifndef MONITOR_LITTLEFS_H
#define MONITOR_LITTLEFS_H

extern const T_VT100_Menu MENU_LittleFS;

void Do_LittleFS_init(uint8_t keycode);
void Do_LittleFS_format(uint8_t keycode);
void Do_LittleFS_info(uint8_t keycode);
void Do_LittleFS_test_file_ops(uint8_t keycode);
void Do_LittleFS_dir_ops(uint8_t keycode);
void Do_LittleFS_benchmark(uint8_t keycode);
void Do_LittleFS_mount_test(uint8_t keycode);

extern const T_VT100_Menu MENU_LittleFS;

#endif // MONITOR_LITTLEFS_H
