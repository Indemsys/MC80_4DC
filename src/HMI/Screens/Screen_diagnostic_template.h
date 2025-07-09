#ifndef SCREEN_DIAGNOSTIC_TEMPLATE_H
#define SCREEN_DIAGNOSTIC_TEMPLATE_H

#define VIEW_STR_SIZE 1024

typedef void (*T_rt_view_callback_t)(GX_RICH_TEXT_VIEW *rt_view);
typedef void (*T_rt_view_close_callback_t)(void);

extern GX_STRING gx_view_string;
extern char     *view_str;
extern GX_WINDOW *diagn_screen;

void Init_diagn_screen(void *p, T_rt_view_callback_t draw_cb, T_rt_view_close_callback_t close_cb, const char *caption);
void _Show_template_info(const char *caption);

#endif  // SCREEN_DIAGNOSTIC_TEMPLATE_H
