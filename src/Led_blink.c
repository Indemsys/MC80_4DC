#include "App.h"

//-----------------------------------------------------------------------------------------------------
// Internal state for each LED
//-----------------------------------------------------------------------------------------------------
typedef struct T_led_blink_state
{
  T_led_blink_pattern pattern;         // Current pattern
  uint32_t            tick_counter;    // Ticks since start of current cycle
  uint32_t            blinks_done;     // Number of completed blinks
  bool                is_on;           // Current output state
  bool                pattern_active;  // Is this pattern active
} T_led_blink_state;

static T_led_blink_state g_led_blink_states[LED_BLINK_COUNT] = { 0 };

//-----------------------------------------------------------------------------------------------------
// Predefined blink patterns for typical system states
//-----------------------------------------------------------------------------------------------------
const T_led_blink_pattern LED_PATTERN_WORK_NORMAL            = {
             .period_ticks = MS_TO_TICKS(2000),  // 2s period
             .on_ticks     = MS_TO_TICKS(100),   // 100ms ON
             .repeat_count = 0,                  // Infinite
             .mode         = LED_BLINK_MODE_CONTINUOUS,
             .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_SERVICE = {
  .period_ticks = MS_TO_TICKS(400),  // 400ms period
  .on_ticks     = MS_TO_TICKS(200),  // 200ms ON
  .repeat_count = 0,                 // Infinite
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_ERROR = {
  .period_ticks = MS_TO_TICKS(200),  // 200ms period
  .on_ticks     = MS_TO_TICKS(200),  // Always ON (fast blink)
  .repeat_count = 0,                 // Infinite
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_CONFIRM = {
  .period_ticks = MS_TO_TICKS(100),  // 100ms period
  .on_ticks     = MS_TO_TICKS(100),  // 100ms ON
  .repeat_count = 2,                 // Blink 2 times
  .mode         = LED_BLINK_MODE_ONESHOT,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_BOOT = {
  .period_ticks = MS_TO_TICKS(100),  // 100ms period
  .on_ticks     = MS_TO_TICKS(100),  // 100ms ON
  .repeat_count = 5,                 // Blink 5 times
  .mode         = LED_BLINK_MODE_ONESHOT_HOLD,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_ON = {
  .period_ticks = MS_TO_TICKS(100),  // Arbitrary nonzero period
  .on_ticks     = MS_TO_TICKS(100),  // Always ON
  .repeat_count = 0,
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_OFF = {
  .period_ticks = MS_TO_TICKS(100),  // Arbitrary nonzero period
  .on_ticks     = 0,                 // Always OFF
  .repeat_count = 0,
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_PULSE = {
  .period_ticks = MS_TO_TICKS(1000),  // 1s period
  .on_ticks     = MS_TO_TICKS(50),    // 50ms short pulse
  .repeat_count = 0,                  // Infinite
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = true
};

const T_led_blink_pattern LED_PATTERN_CAN_ERROR = {
  .period_ticks = MS_TO_TICKS(100),  // Arbitrary nonzero period
  .on_ticks     = 0,                 // Always OFF (LED disabled)
  .repeat_count = 0,                 // Infinite
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = false
};

const T_led_blink_pattern LED_PATTERN_CAN_ACTIVITY = {
  .period_ticks = MS_TO_TICKS(200),
  .on_ticks     = MS_TO_TICKS(100),
  .repeat_count = 0,                 // Infinite
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = false
};

const T_led_blink_pattern LED_PATTERN_RS485_OFF = {
  .period_ticks = MS_TO_TICKS(100),  // Arbitrary nonzero period
  .on_ticks     = 0,                 // Always OFF
  .repeat_count = 0,
  .mode         = LED_BLINK_MODE_CONTINUOUS,
  .active_high  = false
};

//-----------------------------------------------------------------------------------------------------
// Helper to compare two patterns for equality (to avoid flicker)
//-----------------------------------------------------------------------------------------------------
static bool _Led_blink_pattern_equal(const T_led_blink_pattern *a, const T_led_blink_pattern *b)
{
  if (!a || !b)
  {
    return false;
  }
  if (a->period_ticks == b->period_ticks &&
      a->on_ticks == b->on_ticks &&
      a->repeat_count == b->repeat_count &&
      a->mode == b->mode &&
      a->active_high == b->active_high)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set blink pattern for a specific LED. If the same pattern is already active, does nothing.

  Parameters:
    led_id  - LED_BLINK_GREEN, LED_BLINK_RED, LED_BLINK_CAN, or LED_BLINK_RS485
    pattern - Pointer to the pattern structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Led_blink_set_pattern(uint8_t led_id, const T_led_blink_pattern *pattern)
{
  if (led_id >= LED_BLINK_COUNT || !pattern) return;
  T_led_blink_state *s = &g_led_blink_states[led_id];

  // Check if the same pattern is already running to avoid flicker and rhythm disruption
  if (s->pattern_active && _Led_blink_pattern_equal(&s->pattern, pattern))
  {
    // Same pattern already running, do nothing to preserve timing
    return;
  }

  s->pattern        = *pattern;
  s->tick_counter   = 0;  // Reset timing when pattern changes
  s->blinks_done    = 0;
  s->is_on          = false;
  s->pattern_active = true;

  // Immediately set LED to correct initial state based on pattern
  bool turn_on      = false;
  if (s->pattern.on_ticks > 0)  // If pattern has any ON time, start with ON
  {
    turn_on = true;
  }

  s->is_on = turn_on;
  // Note: Hardware update will be done in Led_blink_func() to batch all LED updates
  // Force an immediate hardware update by calling Led_blink_func
  Led_blink_func();
}

/*-----------------------------------------------------------------------------------------------------
  Set blink patterns for both LEDs at once. If the same pattern is already active, does nothing.

  Parameters:
    pattern_green - Pointer to the pattern structure for green LED
    pattern_red   - Pointer to the pattern structure for red LED

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Led_blink_set_patterns_GR(const T_led_blink_pattern *pattern_green, const T_led_blink_pattern *pattern_red)
{
  const T_led_blink_pattern *patterns[2] = { pattern_green, pattern_red };
  for (uint8_t led = 0; led < 2; led++)
  {
    const T_led_blink_pattern *pattern = patterns[led];
    if (!pattern) continue;
    T_led_blink_state *s = &g_led_blink_states[led];
    if (s->pattern_active && _Led_blink_pattern_equal(&s->pattern, pattern))
    {
      // Same pattern already running, do nothing to avoid flicker
      continue;
    }
    s->pattern        = *pattern;
    s->tick_counter   = 0;
    s->blinks_done    = 0;
    s->is_on          = false;
    s->pattern_active = true;
  }

  // Force an immediate hardware update by calling Led_blink_func
  Led_blink_func();
}

/*-----------------------------------------------------------------------------------------------------
  Call periodically from main loop to update LEDs according to their patterns.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Led_blink_func(void)
{
  static ULONG last_tick = 0;
  ULONG        now       = tx_time_get();
  uint32_t     elapsed;
  if (last_tick == 0)
  {
    elapsed = 1;
  }
  else
  {
    elapsed = (uint32_t)(now - last_tick);
  }
  if (elapsed == 0) return;  // Only update if at least 1 tick passed
  last_tick = now;

  bool state_changed = false;

  for (uint8_t led = 0; led < LED_BLINK_COUNT; led++)
  {
    T_led_blink_state *s = &g_led_blink_states[led];
    if (!s->pattern_active) continue;

    bool old_is_on = s->is_on;

    // Advance tick_counter by elapsed ticks
    s->tick_counter += elapsed;
    if (s->tick_counter >= s->pattern.period_ticks)
    {
      s->tick_counter = s->tick_counter % s->pattern.period_ticks;
      if (s->pattern.mode != LED_BLINK_MODE_CONTINUOUS)
      {
        s->blinks_done++;
        if (s->pattern.repeat_count && s->blinks_done >= s->pattern.repeat_count)
        {
          s->pattern_active = false;
          if (s->pattern.mode == LED_BLINK_MODE_ONESHOT_HOLD)
          {
            s->is_on = true;
          }
          else
          {
            s->is_on = false;
          }
          if (old_is_on != s->is_on)
          {
            state_changed = true;
          }
          continue;
        }
      }
    }

    // Determine if LED should be on based on pattern
    bool turn_on = false;
    if (s->tick_counter < s->pattern.on_ticks && s->pattern_active)
    {
      turn_on = true;
    }

    // Check if LED state changed
    if (s->is_on != turn_on)
    {
      s->is_on = turn_on;
      state_changed = true;
    }
  }

  // Update hardware only if any LED state changed
  if (state_changed)
  {
    // Calculate hardware values for all LEDs
    uint8_t hw_green = 0, hw_red = 0, hw_can = 0, hw_rs485 = 0;

    // LED_BLINK_GREEN = 0
    if (g_led_blink_states[LED_BLINK_GREEN].pattern_active)
    {
      if (g_led_blink_states[LED_BLINK_GREEN].pattern.active_high)
      {
        if (g_led_blink_states[LED_BLINK_GREEN].is_on)
        {
          hw_green = 1;
        }
        else
        {
          hw_green = 0;
        }
      }
      else
      {
        if (g_led_blink_states[LED_BLINK_GREEN].is_on)
        {
          hw_green = 0;
        }
        else
        {
          hw_green = 1;
        }
      }
    }

    // LED_BLINK_RED = 1
    if (g_led_blink_states[LED_BLINK_RED].pattern_active)
    {
      if (g_led_blink_states[LED_BLINK_RED].pattern.active_high)
      {
        if (g_led_blink_states[LED_BLINK_RED].is_on)
        {
          hw_red = 1;
        }
        else
        {
          hw_red = 0;
        }
      }
      else
      {
        if (g_led_blink_states[LED_BLINK_RED].is_on)
        {
          hw_red = 0;
        }
        else
        {
          hw_red = 1;
        }
      }
    }

    // LED_BLINK_CAN = 2
    if (g_led_blink_states[LED_BLINK_CAN].pattern_active)
    {
      if (g_led_blink_states[LED_BLINK_CAN].pattern.active_high)
      {
        if (g_led_blink_states[LED_BLINK_CAN].is_on)
        {
          hw_can = 1;
        }
        else
        {
          hw_can = 0;
        }
      }
      else
      {
        if (g_led_blink_states[LED_BLINK_CAN].is_on)
        {
          hw_can = 0;
        }
        else
        {
          hw_can = 1;
        }
      }
    }

    // LED_BLINK_RS485 = 3
    if (g_led_blink_states[LED_BLINK_RS485].pattern_active)
    {
      if (g_led_blink_states[LED_BLINK_RS485].pattern.active_high)
      {
        if (g_led_blink_states[LED_BLINK_RS485].is_on)
        {
          hw_rs485 = 1;
        }
        else
        {
          hw_rs485 = 0;
        }
      }
      else
      {
        if (g_led_blink_states[LED_BLINK_RS485].is_on)
        {
          hw_rs485 = 0;
        }
        else
        {
          hw_rs485 = 1;
        }
      }
    }

    // Update all LEDs with a single SPI transaction
    Write_multiple_bits_to_IO_extender(hw_green, hw_red, hw_can, hw_rs485);
  }
}
