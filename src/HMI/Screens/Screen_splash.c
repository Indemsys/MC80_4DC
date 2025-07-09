#include "App.h"

#define UPTIME_BUFFER_SIZE  64                                          // Buffer size for uptime string
#define VERSION_BUFFER_SIZE 32                                          // Buffer size for version string
#define TOTAL_BUFFER_SIZE   (UPTIME_BUFFER_SIZE + VERSION_BUFFER_SIZE)  // Total buffer size

static GX_CHAR   *g_uptime_buffer   = GX_NULL;
static GX_CHAR   *g_version_buffer  = GX_NULL;
static GX_CHAR   *g_combined_buffer = GX_NULL;
static GX_WINDOW *g_splash_screen;

/*-----------------------------------------------------------------------------------------------------
  Description: Create and display the splash window on the display

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Show_window_splash(void)
{
  UINT status = gx_studio_named_widget_create("window_splash", 0, (GX_WIDGET **)&g_splash_screen);

  if (status != GX_SUCCESS)
  {
    // Handle creation error
    g_splash_screen = GX_NULL;
    return;
  }

  // Allocate memory for all buffers with one call
  if (g_combined_buffer == GX_NULL)
  {
    g_combined_buffer = (GX_CHAR *)App_malloc(TOTAL_BUFFER_SIZE);

    if (g_combined_buffer == GX_NULL)
    {
      // Handle allocation error - clean up and return
      if (g_splash_screen != GX_NULL)
      {
        gx_widget_delete((GX_WIDGET *)g_splash_screen);
        g_splash_screen = GX_NULL;
      }
      return;
    }

    // Set pointers to different parts of the combined buffer
    g_version_buffer = g_combined_buffer;
    g_uptime_buffer  = g_combined_buffer + VERSION_BUFFER_SIZE;
  }

  gx_widget_attach((GX_WIDGET *)root, (GX_WIDGET *)g_splash_screen);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Free resources when closing the splash window

  Parameters:
    window - pointer to the window being closed

  Return: void
-----------------------------------------------------------------------------------------------------*/
static void _Close_window_splash(GX_WINDOW *window)
{
  // Stop the timer
  gx_system_timer_stop((GX_WIDGET *)window, USER_INPUT_PROC_TIMER_ID);

  // Detach window from parent widget tree before cleanup
  if (window != GX_NULL)
  {
    gx_widget_detach((GX_WIDGET *)window);
  }

  // Free memory with one call
  if (g_combined_buffer != GX_NULL)
  {
    App_free(g_combined_buffer);
    g_combined_buffer = GX_NULL;
    g_uptime_buffer   = GX_NULL;
    g_version_buffer  = GX_NULL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Periodic processing of the splash screen (called by timer)

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
static void _Splash_screen_process(void)
{
  // Check encoder button press and switch to diagnostic main screen if pressed
  if (Get_switch_press_signal())
  {
    // First close splash window and free resources
    _Close_window_splash(g_splash_screen);

    // Clear the pointer to prevent further timer processing
    g_splash_screen = GX_NULL;

    // Switch to diagnostic main screen
    Init_diagnostic_main_screen(GX_NULL);

    return;  // Exit early to avoid processing display updates
  }

  // Check if any motors are active or if there are system errors - switch to diagnostic screen
  if (Are_any_motors_active() || App_has_any_errors())
  {
    // First close splash window and free resources
    _Close_window_splash(g_splash_screen);

    // Clear the pointer to prevent further timer processing
    g_splash_screen = GX_NULL;

    // Switch to diagnostic main screen
    Init_diagnostic_main_screen(GX_NULL);

    return;  // Exit early to avoid processing display updates
  }

  // Update display only if splash screen is still active
  if (g_combined_buffer != GX_NULL && g_splash_screen != GX_NULL)
  {
    // Get firmware version using build date/time
    Get_build_date_time(g_version_buffer, VERSION_BUFFER_SIZE);

    // Set firmware version in version widget
    GX_STRING version_string;
    version_string.gx_string_ptr    = g_version_buffer;
    version_string.gx_string_length = strlen(g_version_buffer);
    gx_prompt_text_set_ext(&window_splash.window_splash_pr_ver, &version_string);

    // Get uptime string and set it in status widget
    Get_and_format_uptime_string(g_uptime_buffer, UPTIME_BUFFER_SIZE);

    GX_STRING uptime_string;
    uptime_string.gx_string_ptr    = g_uptime_buffer;
    uptime_string.gx_string_length = strlen(g_uptime_buffer);
    gx_prompt_text_set_ext(&window_splash.window_splash_pr_status, &uptime_string);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Draw callback for splash window

  Parameters:
    window - pointer to the window

  Return: void
-----------------------------------------------------------------------------------------------------*/
VOID Splash_draw_callback(GX_WINDOW *window)
{
  gx_window_draw(window);
  gx_widget_children_draw(window);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Event handler for splash window

  Parameters:
    window     - pointer to the window
    event_ptr  - pointer to the event structure

  Return: UINT - event processing status
-----------------------------------------------------------------------------------------------------*/
UINT Splash_event_callback(GX_WINDOW *window, GX_EVENT *event_ptr)
{
  UINT status;

  switch (event_ptr->gx_event_type)
  {
    case GX_EVENT_SHOW:
      gx_system_timer_start((GX_WIDGET *)window, USER_INPUT_PROC_TIMER_ID, USER_INPUT_PROC_INTIT_TICKS, USER_INPUT_PROC_PERIOD_TICKS);
      status = gx_window_event_process(window, event_ptr);
      break;

    case GX_EVENT_TIMER:
      if (event_ptr->gx_event_payload.gx_event_timer_id == USER_INPUT_PROC_TIMER_ID)
      {
        // Only process timer if splash screen is still active
        if (g_splash_screen != GX_NULL && window == g_splash_screen)
        {
          _Splash_screen_process();
        }
      }
      status = GX_SUCCESS;
      break;

    default:
      status = gx_window_event_process(window, event_ptr);
      break;
  }

  return status;
}
