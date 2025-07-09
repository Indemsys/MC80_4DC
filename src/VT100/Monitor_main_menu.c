#include "App.h"

static void Do_Reset(uint8_t keycode);
static void Do_date_time_set(uint8_t keycode);
static void Do_APP_Params_editor(uint8_t keycode);

extern const T_VT100_Menu MENU_MAIN;
extern const T_VT100_Menu MENU_PARAMETERS;
extern const T_VT100_Menu MENU_DIAGNOSTIC;

static int32_t Lookup_menu_item(T_VT100_Menu_item **item, uint8_t b);

//--------------------------------------------------------------------------------------------
//
//    Menu items that have their own submenu are placed at the next nesting level.
//    Their function replaces the default key press handler in the main loop and should
//    periodically return control to the main loop.
//
//    Items without a function simply transition to the next submenu level.
//
//    Items without a submenu take full control and do not transition to the next level.
//
//    Items without a submenu and function mean returning to the previous menu level.
//
//--------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
// Menu item structure for main menu
//
//  but   - key code for menu item selection
//  func  - pointer to the function to execute for this menu item
//  psubmenu - pointer to the submenu structure (if any)
//-------------------------------------------------------------------------------------
// clang-format off
const T_VT100_Menu_item MENU_MAIN_ITEMS[] =
{
  { '1', Do_APP_Params_editor, (void *)&MENU_PARAMETERS },
  { '2', Do_show_event_log,    0                        },
  { '3', Do_date_time_set,     0                        },
  { '4', 0,                    (void *)&MENU_DIAGNOSTIC },
  { '5', Do_Reset,             0                        },
  { 'R', 0,                    0                        },
  { 'M', 0,                    (void *)&MENU_MAIN       },
  { 0 } // End of menu
};
// clang-format on

const T_VT100_Menu MENU_MAIN =
{
 "Main menu",
 "\033[5C Main menu\r\n"
 "\033[5C <1> - Application parameters\r\n"
 "\033[5C <2> - Log viewer\r\n"
 "\033[5C <3> - Set date and time\r\n"
 "\033[5C <4> - Diagnostic menu\r\n"
 "\033[5C <5> - Reset\r\n",
 MENU_MAIN_ITEMS,
};

//-------------------------------------------------------------------------------------
// Menu item structure for parameters menu
//
//  but   - key code for menu item selection
//  func  - pointer to the function to execute for this menu item
//  psubmenu - pointer to the submenu structure (if any)
//-------------------------------------------------------------------------------------
const T_VT100_Menu_item MENU_PARAMETERS_ITEMS[] =
{
  { 0 } // No items, placeholder
};

const T_VT100_Menu MENU_PARAMETERS =
{
  "Parameters menu",
  "",
  MENU_PARAMETERS_ITEMS,
};

/*-----------------------------------------------------------------------------------------------------
  Assigns a new monitor function handler.

  Parameters:
    func   Pointer to the new handler function.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Set_monitor_func(void (*func)(unsigned char))
{
  GET_MCBL;

  mcbl->Monitor_func = func;
}

/*-----------------------------------------------------------------------------------------------------
  Outputs the current menu to the screen.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Display_menu(void)
{
  GET_MCBL;

  uint8_t *str;

  MPRINTF(VT100_CLEAR_AND_HOME);

  if (mcbl->menu_trace[mcbl->menu_nesting] == 0) return;

  str = (uint8_t *)wvar.product_name;
  // Output menu header
  // VT100_send_str_to_pos((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_header, 1, Find_str_center((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_header));
  VT100_send_str_to_pos((uint8_t *)str, 1, VT100_find_str_center((uint8_t *)str));
  VT100_send_str_to_pos(DASH_LINE, 2, 0);
  // Output menu body string
  VT100_send_str_to_pos((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_body, 3, 0);
  MPRINTF("\r\n");
  MPRINTF(DASH_LINE);
}
/*-----------------------------------------------------------------------------------------------------
  Searches for a menu item in the current menu by the command code.

  Parameters:
    item   Pointer to the found menu item (output).
    b      Code of the command that calls the menu item.

  Return:
    1 if found, 0 otherwise.
-----------------------------------------------------------------------------------------------------*/
int32_t Lookup_menu_item(T_VT100_Menu_item **item, uint8_t b)
{
  int16_t i;
  GET_MCBL;

  if (isalpha(b) != 0) b = toupper(b);

  i = 0;
  do
  {
    *item = (T_VT100_Menu_item *)mcbl->menu_trace[mcbl->menu_nesting]->menu_items + i;
    if ((*item)->but == b) return (1);
    if ((*item)->but == 0) break;
    i++;
  } while (1);

  return (0);
}

/*-----------------------------------------------------------------------------------------------------
  Searches for a menu item by the call code (in the current menu)
  and execute the corresponding function.

  Parameters:
    b      Code of the symbol entered from the keyboard that calls the menu item.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Menu_press_key_handler(uint8_t b)
{
  T_VT100_Menu_item *menu_item;
  GET_MCBL;

  // Search for a record in the list that matches the given symbol in variable b
  if (Lookup_menu_item(&menu_item, b) != 0)
  {
    // Found the corresponding menu item
    if (menu_item->psubmenu != 0)
    {
      // If a submenu is present, display it

      mcbl->menu_nesting++;
      mcbl->menu_trace[mcbl->menu_nesting] = (T_VT100_Menu *)menu_item->psubmenu;

      Display_menu();
      // If the menu item has a function, assign it as the main loop key handler and execute the function.
      if (menu_item->func != 0)
      {
        mcbl->Monitor_func = (T_menu_func)(menu_item->func);  // Set the main loop key handler to the function from the menu item
        menu_item->func(0);                                   // Execute the menu item function
      }
    }
    else
    {
      if (menu_item->func == 0)
      {
        // If there is neither a submenu nor a function, this is a return to the previous menu
        // Control remains with the default main loop handler
        Return_to_prev_menu();
        Display_menu();
      }
      else
      {
        // If the item has no submenu, clear the screen and execute the selected menu item function
        MPRINTF(VT100_CLEAR_AND_HOME);
        menu_item->func(0);
        // Control returns to the default main loop handler
        Display_menu();
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Go to the main menu.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Goto_main_menu(void)
{
  GET_MCBL;

  mcbl->menu_nesting                   = 0;
  mcbl->menu_trace[mcbl->menu_nesting] = (T_VT100_Menu *)&MENU_MAIN;
  Display_menu();
  mcbl->Monitor_func = Menu_press_key_handler;  // Assign the handler function
}

/*-----------------------------------------------------------------------------------------------------
  Return to the previous menu.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Return_to_prev_menu(void)
{
  GET_MCBL;

  if (mcbl->menu_nesting > 0)
  {
    mcbl->menu_trace[mcbl->menu_nesting] = 0;
    mcbl->menu_nesting--;
  }
  mcbl->Monitor_func = Menu_press_key_handler;  // Assign the handler function
}

/*-----------------------------------------------------------------------------------------------------
  Set date and time.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Do_date_time_set(uint8_t keycode)
{
  unsigned char i, k, b;
  uint8_t       buf[EDSTLEN];
  rtc_time_t    rt_time = {0};
  GET_MCBL;

  MPRINTF(VT100_CLEAR_AND_HOME);

  VT100_send_str_to_pos("SYSTEM TIME SETTING", 1, 30);
  VT100_send_str_to_pos("\033[5C <M> - Display main menu, <Esc> - Exit \r\n", 2, 10);
  VT100_send_str_to_pos("Print in form [YY.MM.DD HH.MM.SS]:  .  .     :  :  ", SCR_ITEMS_VERT_OFFS, 1);

  mcbl->beg_pos  = 35;
  k              = 0;
  mcbl->curr_pos = mcbl->beg_pos;
  VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
  MPRINTF((char *)VT100_CURSOR_ON);

  for (i = 0; i < EDSTLEN; i++) buf[i] = 0;

  do
  {
    if (WAIT_CHAR(&b, 200) == RES_OK)
    {
      switch (b)
      {
        case VT100_BCKSP:  // Back Space
          if (mcbl->curr_pos > mcbl->beg_pos)
          {
            mcbl->curr_pos--;
            k--;
            switch (k)
            {
              case 2:
              case 5:
              case 8:
              case 11:
              case 14:
                k--;
                mcbl->curr_pos--;
                break;
            }

            VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
            MPRINTF((char *)" ");
            VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
            buf[k] = 0;
          }
          break;
        case VT100_DEL:  // DEL
          mcbl->curr_pos = mcbl->beg_pos;
          k              = 0;
          for (i = 0; i < EDSTLEN; i++) buf[i] = 0;
          VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->beg_pos);
          MPRINTF((char *)"  .  .     :  :  ");
          VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->beg_pos);
          break;
        case VT100_ESC:  // ESC
          MPRINTF((char *)VT100_CURSOR_OFF);
          return;
        case 'M':        //
        case 'm':        //
          MPRINTF((char *)VT100_CURSOR_OFF);
          return;

        case VT100_CR:  // Enter
          MPRINTF((char *)VT100_CURSOR_OFF);

          rt_time.tm_year = BCD2ToBYTE((buf[0] << 4) + buf[1]) + 2000 - 1900;
          rt_time.tm_mon  = BCD2ToBYTE((buf[3] << 4) + buf[4]) - 1;
          rt_time.tm_mday = BCD2ToBYTE((buf[6] << 4) + buf[7]);
          rt_time.tm_hour = BCD2ToBYTE((buf[9] << 4) + buf[10]);
          rt_time.tm_min  = BCD2ToBYTE((buf[12] << 4) + buf[13]);
          rt_time.tm_sec  = BCD2ToBYTE((buf[15] << 4) + buf[16]);

          RTC_set_system_DateTime(&rt_time);
          return;
        default:
          if (isdigit(b))
          {
            if (k < EDSTLEN)
            {
              uint8_t str[2];
              str[0] = b;
              str[1] = 0;
              MPRINTF((char *)str);
              buf[k] = b - 0x30;
              mcbl->curr_pos++;
              k++;
              switch (k)
              {
                case 2:
                case 5:
                case 8:
                case 11:
                case 14:
                  k++;
                  mcbl->curr_pos++;
                  break;
              }
              VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
            }
          }
          break;

      }  // switch
    }
  } while (1);
}

/*-----------------------------------------------------------------------------------------------------
  Editing module parameters.
  The function is called once when entering the parameter editing mode.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Do_APP_Params_editor(uint8_t keycode)
{
  GET_MCBL;

  mcbl->p_pinst = &wvar_inst;
  if (mcbl->p_pinst == 0) return;
  mcbl->ptype         = 0;
  mcbl->current_level = MAIN_PARAMS_ROOT;
  Show_parameters_menu();                             // Show the parameters menu
  Set_monitor_func(Params_editor_press_key_handler);  // Reassign the key press handler in the main monitor loop
}

/*-----------------------------------------------------------------------------------------------------
  System reset.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Do_Reset(uint8_t keycode)
{
  Reset_system();
}
