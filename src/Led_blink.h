#ifndef LED_BLINK_H
#define LED_BLINK_H

// LED identifiers
#define LED_BLINK_GREEN             0
#define LED_BLINK_RED               1
#define LED_BLINK_CAN               2
#define LED_BLINK_RS485             3
#define LED_BLINK_COUNT             4  // Total number of LEDs

// LED blink mode
#define LED_BLINK_MODE_CONTINUOUS   0  // Blink indefinitely
#define LED_BLINK_MODE_ONESHOT      1  // Blink N times, then turn off
#define LED_BLINK_MODE_ONESHOT_HOLD 2  // Blink N times, then stay ON

// LED blink pattern structure
typedef struct T_led_blink_pattern
{
  uint32_t period_ticks;  // Total period of one blink cycle (in OS ticks)
  uint32_t on_ticks;      // Duration LED is ON in one cycle (in OS ticks)
  uint32_t repeat_count;  // Number of times to blink (0 = infinite)
  uint8_t  mode;          // Blink mode (continuous, oneshot, oneshot_hold)
  bool     active_high;   // true: ON=1, false: ON=0
} T_led_blink_pattern;

// Predefined patterns for typical system states
extern const T_led_blink_pattern LED_PATTERN_WORK_NORMAL;
extern const T_led_blink_pattern LED_PATTERN_SERVICE;
extern const T_led_blink_pattern LED_PATTERN_ERROR;
extern const T_led_blink_pattern LED_PATTERN_CONFIRM;
extern const T_led_blink_pattern LED_PATTERN_BOOT;
extern const T_led_blink_pattern LED_PATTERN_ON;
extern const T_led_blink_pattern LED_PATTERN_OFF;
extern const T_led_blink_pattern LED_PATTERN_PULSE;
extern const T_led_blink_pattern LED_PATTERN_CAN_ERROR;
extern const T_led_blink_pattern LED_PATTERN_CAN_ACTIVITY;
extern const T_led_blink_pattern LED_PATTERN_RS485_OFF;

// Set blink patterns for both LEDs at once
void Led_blink_set_patterns_GR(const T_led_blink_pattern *pattern_green, const T_led_blink_pattern *pattern_red);

// Set individual LED pattern
void Led_blink_set_pattern(uint8_t led_id, const T_led_blink_pattern *pattern);

// Call periodically from main loop to update LEDs
void Led_blink_func(void);

#endif  // LED_BLINK_H
