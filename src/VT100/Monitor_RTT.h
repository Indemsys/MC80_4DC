#ifndef MONITOR_RTT_H
#define MONITOR_RTT_H

extern const T_VT100_Menu MENU_RTT;

void Test_basic_rtt_output(uint8_t keycode);
void Test_rtt_performance(uint8_t keycode);
void Test_rtt_buffer_overflow(uint8_t keycode);
void Test_rtt_multiple_buffers(uint8_t keycode);
void Test_rtt_stress_test(uint8_t keycode);
void Show_rtt_buffer_status(uint8_t keycode);
void Show_rtt_addresses(uint8_t keycode);
void Reset_rtt_buffer_menu(uint8_t keycode);
void RTT_simple_test(void);
void RTT_reset_buffer(void);

#endif // MONITOR_RTT_H
