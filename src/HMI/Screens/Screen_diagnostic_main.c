//----------------------------------------------------------------------
// File created on 2025-02-17
//----------------------------------------------------------------------
#include "App.h"

#define MAX_CHARS_PER_LINE       29            // Maximum characters per line for selected font
#define MOTOR_STATUS_BUFFER_SIZE 1024          // Size of motor status buffer

static int32_t previous_encoder_value = 0;     // Store the previous encoder value for comparison
static char   *motor_status_buffer    = NULL;  // Dynamic buffer for motor status text

// Static function declarations
static void _Format_motor_status_info(void);
static void _Diagnostic_screen_callback(GX_RICH_TEXT_VIEW *rt_view);
static void _Deinit_diagnostic_screen(void);

/*-----------------------------------------------------------------------------------------------------
  Format motor status information into the buffer

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Format_motor_status_info(void)
{
  if (!motor_status_buffer) return;

  char line_buffer[MAX_CHARS_PER_LINE + 1];  // +1 for null terminator
  bool any_motor_active  = false;

  // Clear buffer
  motor_status_buffer[0] = '\0';

  // Display software version at the top (same as splash screen)
  char version_buffer[32];
  Get_build_date_time(version_buffer, sizeof(version_buffer));
  snprintf(line_buffer, sizeof(line_buffer), "SW: %s", version_buffer);
  strcat(motor_status_buffer, line_buffer);
  strcat(motor_status_buffer, "\n");

  // Display bus voltage
  float bus_voltage = Adc_driver_get_supply_voltage_24v();
  snprintf(line_buffer, sizeof(line_buffer), "Bus Voltage: %.1fV", bus_voltage);
  strcat(motor_status_buffer, line_buffer);
  strcat(motor_status_buffer, "\n\n");

  // Check each motor and display only active ones
  for (uint8_t motor_num = MOTOR_1_; motor_num <= MOTOR_4_; motor_num++)
  {
    uint8_t  enabled;
    uint16_t pwm_level;
    uint8_t  direction;

    // Get motor state
    uint32_t result = Motor_get_state(motor_num, &enabled, &pwm_level, &direction);
    if (result == RES_OK && enabled && pwm_level > 0)
    {
      // Motor is active, add to display
      if (!any_motor_active)
      {
        // Add header only once
        snprintf(line_buffer, sizeof(line_buffer), "Active Motors:");
        strcat(motor_status_buffer, line_buffer);
        strcat(motor_status_buffer, "\n\n");
        any_motor_active = true;
      }

      // Motor name
      const char *motor_names[] = { "", "Traction", "Motor 2", " Motor 3", " Motor 2" };
      snprintf(line_buffer, sizeof(line_buffer), "Motor %d (%s):", motor_num, motor_names[motor_num]);
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n");

      // Direction
      snprintf(line_buffer, sizeof(line_buffer), "  Dir    : %s", direction ? "Forward" : "Reverse");
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n");

      // PWM level
      snprintf(line_buffer, sizeof(line_buffer), "  PWM    : %d%%", pwm_level);
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n");

      // Current - get actual motor current
      float motor_current = Adc_driver_get_dc_motor_current(motor_num);
      snprintf(line_buffer, sizeof(line_buffer), "  Current: %.1fA", motor_current);
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n");

      // Power consumption - current * actual bus voltage
      float motor_power = motor_current * bus_voltage;
      snprintf(line_buffer, sizeof(line_buffer), "  Power  : %.1fW", motor_power);
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n\n");
    }
  }

  // If no motors are active, check for system errors
  if (!any_motor_active)
  {
    if (App_has_any_errors())
    {
      // Display system errors - use remaining buffer space
      uint32_t current_length = strlen(motor_status_buffer);
      uint32_t remaining_space = MOTOR_STATUS_BUFFER_SIZE - current_length;
      App_format_error_list(motor_status_buffer + current_length, remaining_space);
    }
    else
    {
      snprintf(line_buffer, sizeof(line_buffer), "No active motors");
      strcat(motor_status_buffer, line_buffer);
      strcat(motor_status_buffer, "\n");
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Deinitialize diagnostic screen and free resources

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Deinit_diagnostic_screen(void)
{
  previous_encoder_value = 0;  // Reset the previous encoder value when deinitializing

  // Free dynamic memory
  if (motor_status_buffer)
  {
    App_free(motor_status_buffer);
    motor_status_buffer = NULL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Callback for diagnostic screen updates - checks motor status and handles display

  Parameters:
    rt_view - pointer to the rich text view widget

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Diagnostic_screen_callback(GX_RICH_TEXT_VIEW *rt_view)
{
  // Check if all motors have stopped AND no system errors exist - switch back to splash screen
  if (!Are_any_motors_active() && !App_has_any_errors())
  {
    // Stop the timer and detach the diagnostic screen safely
    if (diagn_screen)
    {
      gx_system_timer_stop((GX_WIDGET *)diagn_screen, USER_INPUT_PROC_TIMER_ID);
      gx_widget_detach((GX_WIDGET *)diagn_screen);
    }

    // Clean up diagnostic screen resources
    _Deinit_diagnostic_screen();

    // Show splash screen
    Show_window_splash();

    return;  // Exit early to avoid processing display updates
  }

  // Update motor status information
  _Format_motor_status_info();

  // Set the motor status text to the view
  if (motor_status_buffer)
  {
    gx_view_string.gx_string_length = strlen(motor_status_buffer);
    gx_view_string.gx_string_ptr    = motor_status_buffer;
    gx_multi_line_text_view_text_set_ext((GX_MULTI_LINE_TEXT_VIEW *)rt_view, &gx_view_string);
  }
  else
  {
    gx_view_string.gx_string_length = 0;
    gx_view_string.gx_string_ptr    = 0;
    gx_multi_line_text_view_text_set_ext((GX_MULTI_LINE_TEXT_VIEW *)rt_view, &gx_view_string);
  }

  // Calculate the difference in encoder values
  int32_t encoder_diff = enc_cbl.encoder_counter - previous_encoder_value;

  // Scroll the text view based on the encoder difference
  if (encoder_diff != 0)
  {
    GX_SCROLL_INFO scroll_info;
    UINT           status;

    // Получаем информацию о прокрутке
    status = gx_multi_line_text_view_scroll_info_get((GX_MULTI_LINE_TEXT_VIEW *)rt_view, 0, &scroll_info);
    if (status == GX_SUCCESS)
    {
      // Calculate new scroll value
      int32_t new_scroll_value = scroll_info.gx_scroll_value + encoder_diff * scroll_info.gx_scroll_increment;

      // Limit scroll value to valid range
      if (new_scroll_value < scroll_info.gx_scroll_minimum)
      {
        new_scroll_value = scroll_info.gx_scroll_minimum;
      }
      else if (new_scroll_value > scroll_info.gx_scroll_maximum - scroll_info.gx_scroll_visible + 1)
      {
        new_scroll_value = scroll_info.gx_scroll_maximum - scroll_info.gx_scroll_visible + 1;
        if (new_scroll_value < scroll_info.gx_scroll_minimum)
        {
          new_scroll_value = scroll_info.gx_scroll_minimum;
        }
      }

      // Set new scroll value
      scroll_info.gx_scroll_value                        = new_scroll_value;

      // Обновляем сдвиг текста
      rt_view->gx_multi_line_text_view_text_scroll_shift = scroll_info.gx_scroll_minimum - scroll_info.gx_scroll_value;

      // Redraw the widget to reflect the scroll
      gx_system_dirty_mark((GX_WIDGET *)rt_view);
    }
  }

  // Update the previous encoder value
  previous_encoder_value = enc_cbl.encoder_counter;
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Init_diagnostic_main_screen(void *p)
{
  // Allocate memory for motor status buffer
  if (!motor_status_buffer)
  {
    motor_status_buffer = (char *)App_malloc(MOTOR_STATUS_BUFFER_SIZE);
    if (motor_status_buffer)
    {
      motor_status_buffer[0] = '\0';  // Initialize empty string
    }
  }

  Init_diagn_screen(p, _Diagnostic_screen_callback, _Deinit_diagnostic_screen, "Motor Status");
  previous_encoder_value = enc_cbl.encoder_counter;  // Initialize the previous encoder value on screen initialization
}

/*-----------------------------------------------------------------------------------------------------
  Check if any motor is currently active (rotating)

  Parameters:
    None

  Return:
    true if any motor is active, false otherwise
-----------------------------------------------------------------------------------------------------*/
bool Are_any_motors_active(void)
{
  for (uint8_t motor_num = MOTOR_1_; motor_num <= MOTOR_4_; motor_num++)
  {
    uint8_t  enabled;
    uint16_t pwm_level;
    uint8_t  direction;

    // Get motor state
    uint32_t result = Motor_get_state(motor_num, &enabled, &pwm_level, &direction);
    if (result == RES_OK && enabled && pwm_level > 0)
    {
      return true;  // Found at least one active motor
    }
  }
  return false;     // No active motors found
}
