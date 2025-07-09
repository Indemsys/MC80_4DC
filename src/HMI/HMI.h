#ifndef HMI_H
  #define HMI_H


  #define  SCREEN_HANDLE             0x12345679

  #define  GX_STRINGS_NUMBER         5
  #define  GX_STRING_MAX_LEN         128

#define USER_INPUT_PROC_TIMER_ID     20
#define USER_INPUT_PROC_INTIT_TICKS  1  // Величина выражается в тиках GX_SYSTEM_TIMER_MS
#define USER_INPUT_PROC_PERIOD_TICKS 1  // Величина выражается в тиках GX_SYSTEM_TIMER_MS

typedef struct
{
    GX_PROMPT *prompt;
} T_prompts;

  #define MENU_ITEM_WINDOW   1
  #define MENU_ITEM_COMMAND  2
  #define MENU_ITEM_RETURN   3


typedef void (*T_hmi_func)(void *);


typedef struct
{
    uint32_t    menu_item_type;
    GX_WIDGET  *prompt;
    void (*item_function)(void *);

} T_gui_menu_content;

typedef struct
{
    uint32_t item_num_prev;
    uint32_t item_num;
    int32_t  enc_cnt;

} T_menu_cbl;

extern char                    gui_strings[GX_STRINGS_NUMBER][GX_STRING_MAX_LEN];
extern GX_STRING               gx_strings[GX_STRINGS_NUMBER];
extern GX_WINDOW_ROOT         *root;



void       GUI_start(void);
GX_STRING* GUI_print_str(uint32_t str_id, const char *fmt, ...);
void       GUI_print_to_prompt(uint32_t str_id, GX_PROMPT **prmts, const char *fmt, ...);

#include "MC80_specifications.h"
#include "MC80_resources.h"
#include "gx_display.h"
#include "gx_multi_line_text_view.h"
#include "gx_rich_text_view.h"

#include "Screen_diagnostic_template.h"
#include "Screen_diagnostic_main.h"
#include "Screen_splash.h"

#endif
