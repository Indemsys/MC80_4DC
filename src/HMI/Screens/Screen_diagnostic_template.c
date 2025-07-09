//----------------------------------------------------------------------
// File created on 2025-02-15 (modification for window_diagn)
//----------------------------------------------------------------------
#include "App.h"

GX_WINDOW *diagn_screen;

#define ID_VAL_VER          0
#define ID_VAL_CAP          1
#define DIAGN_INFO_STR_SIZE 64  // Size for diagnostic information string buffer

static volatile T_hmi_func ret_func;

T_rt_view_callback_t       draw_callback;
T_rt_view_close_callback_t close_callback;
const char                *win_caption;

GX_STRING gx_view_string;
char     *view_str;

/*-----------------------------------------------------------------------------------------------------
  Initializes the diagnostic screen

  Parameters:
  p - callback function for return
  draw_cb - callback function for drawing
  close_cb - callback function for closing
  caption - caption text to display

  Return:
-----------------------------------------------------------------------------------------------------*/
void Init_diagn_screen(void *p, T_rt_view_callback_t draw_cb, T_rt_view_close_callback_t close_cb, const char *caption)
{
  if (p != 0)
  {
    ret_func = (T_hmi_func)p;
  }

  if (diagn_screen == 0)
  {
    gx_studio_named_widget_create("window_diagn", 0, (GX_WIDGET **)&diagn_screen);
  }

  win_caption                     = caption;
  draw_callback                   = draw_cb;
  close_callback                  = close_cb;

  view_str                        = App_malloc(VIEW_STR_SIZE);
  gx_view_string.gx_string_ptr    = view_str;
  gx_view_string.gx_string_length = 0;
  // Update caption widget
  gx_prompt_text_set_ext(&window_diagn.window_diagn_pr_caption, GUI_print_str(ID_VAL_CAP, "%s", win_caption));

  gx_widget_attach((GX_WIDGET *)root, (GX_WIDGET *)diagn_screen);
}

/*-----------------------------------------------------------------------------------------------------
  Updates information in the diagnostic template window:
    - Displays memory statistics and CPU usage in window_diagn.window_diagn_pr_status widget
    - Updates caption to show only the product name

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Show_diagn_info(void)
{
  char str[DIAGN_INFO_STR_SIZE];  // Buffer for formatting diagnostic information

  // Update caption widget to show only the product name
  gx_prompt_text_set_ext(&window_diagn.window_diagn_pr_caption, GUI_print_str(ID_VAL_CAP, "%s", win_caption));

  float cpu_usage = (float)g_aver_cpu_usage / 10.0f;

  // Get memory pool statistics
  uint32_t available_mem;
  uint32_t fragments;
  App_get_RAM_pool_statistic(&available_mem, &fragments);

  if (g_file_system_ready)
  {
    snprintf(str, sizeof(str), " Free:%d. Frag: %d. SD: Ok ", available_mem, fragments);
  }
  else
  {
    snprintf(str, sizeof(str), " Free:%d. Frag: %d. SD: Err", available_mem, fragments);
  }
  gx_prompt_text_set_ext(&window_diagn.window_diagn_pr_status, GUI_print_str(ID_VAL_VER, "%s. CPU: %0.1f %%", str, (double)cpu_usage));

  if (draw_callback)
  {
    draw_callback(&window_diagn.window_diagn_rt_view);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Processes timer events for the diagnostic screen

  Parameters:

  Return:
  uint32_t - 0 if the window should be closed, 1 to continue updating
-----------------------------------------------------------------------------------------------------*/
static uint32_t _timer_event_processing(void)
{
  // If return button is pressed (e.g., BTTN_1), close the diagnostic window
  //  if (btns[BTTN_1].pressed)
  //  {
  //    btns[BTTN_1].pressed = 0;
  //    gx_system_timer_stop((GX_WIDGET *)diagn_screen, USER_INPUT_PROC_TIMER_ID);
  //    gx_widget_detach((GX_WIDGET *)diagn_screen);
  //    if (close_callback) close_callback();
  //    App_free(view_str);
  //    if (ret_func)
  //    {
  //      ret_func(0);
  //    }
  //    return 0;
  //  }
  return 1;
}

/*-----------------------------------------------------------------------------------------------------
  Draw callback function for the diagnostic window

  Parameters:
  window - pointer to the window

  Return:
-----------------------------------------------------------------------------------------------------*/
VOID Diagn_draw_callback(GX_WINDOW *window)
{
  gx_window_draw(window);
  gx_widget_children_draw(window);
}

/*-----------------------------------------------------------------------------------------------------
  Event callback function for the diagnostic window

  Parameters:
  window - pointer to the window
  event_ptr - pointer to the event

  Return:
  UINT - event processing status
-----------------------------------------------------------------------------------------------------*/
UINT Diagn_event_callback(GX_WINDOW *window, GX_EVENT *event_ptr)
{
  UINT status;

  switch (event_ptr->gx_event_type)
  {
    case GX_EVENT_SHOW:
      _Show_diagn_info();
      gx_system_timer_start((GX_WIDGET *)window, USER_INPUT_PROC_TIMER_ID, USER_INPUT_PROC_INTIT_TICKS, USER_INPUT_PROC_PERIOD_TICKS);
      status = gx_window_event_process(window, event_ptr);
      break;

    case GX_EVENT_TIMER:
      if (event_ptr->gx_event_payload.gx_event_timer_id == USER_INPUT_PROC_TIMER_ID)
      {
        if (_timer_event_processing())
        {
          _Show_diagn_info();
        }
      }
      break;

    default:
      status = gx_window_event_process(window, event_ptr);
      return status;
  }

  return 0;
}
