#include "App.h"

#define TMP_BUF_SZ       512

#define MAX_VALUE_STR_SZ 100

#define MTYPE_NONE       0
#define MTYPE_SUBMENU    1
#define MTYPE_SUBTABLE   2
#define MTYPE_PARAMETER  3

static const char *PARAM_EDITOR_HELP =
"\033[5C Press digit key to select menu item.\r\n"
"\033[5C <M> - Main menu, <R> - return on prev. level\r\n"
"\033[5C Enter - Accept, Esc - Cancel, ^[H - erase\r\n";

static void     _Goto_to_edit_param(void);
static uint8_t *_Get_mn_caption(void);
static uint8_t  _Get_mn_prevlev(void);
static bool     _Get_selector_caption(const T_NV_parameters_instance *p_pars, int param_index, uint8_t *caption_str, uint32_t max_len);

/*-----------------------------------------------------------------------------------------------------
  Find menu caption by its identifier

  Parameters: none

  Return: pointer to menu caption string
-----------------------------------------------------------------------------------------------------*/
static uint8_t *_Get_mn_caption(void)
{
  int i;
  GET_MCBL;
  const T_NV_parameters_instance *p_pars = mcbl->p_pinst;

  for (i = 0; i < p_pars->menu_items_num; i++)
  {
    if (p_pars->menu_items_array[i].currlev == mcbl->current_level) return (uint8_t *)p_pars->menu_items_array[i].name;
  }
  return (uint8_t *)p_pars->menu_items_array[0].name;
}

/*-----------------------------------------------------------------------------------------------------
  Get previous menu level for current menu

  Parameters: none

  Return: previous menu level identifier
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_mn_prevlev(void)
{
  int i;
  GET_MCBL;
  const T_NV_parameters_instance *p_pars = mcbl->p_pinst;

  for (i = 0; i < p_pars->menu_items_num; i++)
  {
    if (p_pars->menu_items_array[i].currlev == mcbl->current_level) return (p_pars->menu_items_array[i].prevlev);
  }
  return (MAIN_PARAMS_ROOT);
}

/*-----------------------------------------------------------------------------------------------------
  Get selector caption for parameter value if selector exists

  Parameters: p_pars      - pointer to parameters instance
              param_index - parameter index
              caption_str - buffer for caption string
              max_len     - maximum buffer length

  Return: true if caption found and copied, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Get_selector_caption(const T_NV_parameters_instance *p_pars, int param_index, uint8_t *caption_str, uint32_t max_len)
{
  uint32_t                selector_id   = p_pars->items_array[param_index].selector_id;
  const T_selectors_list *selectors     = p_pars->selectors_array;
  int                     selectors_num = p_pars->selectors_num;
  uint32_t                param_value;

  // Check if parameter has a valid selector
  if (selector_id == 0 || selector_id >= selectors_num || selectors[selector_id].items_list == NULL)
  {
    return false;
  }  // Get current parameter value

  switch (p_pars->items_array[param_index].vartype)
  {
    case tint8u:
      param_value = *(uint8_t *)p_pars->items_array[param_index].val;
      break;
    case tint16u:
      param_value = *(uint16_t *)p_pars->items_array[param_index].val;
      break;
    case tint32u:
    case tint32s:
    case tfloat:
      param_value = *(uint32_t *)p_pars->items_array[param_index].val;
      break;
    default:
      return false;  // Unknown variable type
  }

  // Search for matching value in selector items
  for (uint32_t i = 0; i < selectors[selector_id].items_cnt; i++)
  {
    if (selectors[selector_id].items_list[i].val == param_value)
    {
      // Format string as "value - caption" with bounds checking
      snprintf((char *)caption_str, max_len, "%s (%u)", (const char *)selectors[selector_id].items_list[i].caption, param_value);
      caption_str[max_len - 1] = '\0';  // Ensure null termination
      return true;
    }
  }

  return false;  // Value not found in selector
}

/*-----------------------------------------------------------------------------------------------------
  Display parameters menu to screen

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Show_parameters_menu(void)
{
  int      i;
  uint32_t n;
  uint8_t  str[MAX_VALUE_STR_SZ];
  uint8_t *st;
  GET_MCBL;
  const T_NV_parameters_instance *p_pars = mcbl->p_pinst;
  MPRINTF(VT100_CLEAR_AND_HOME);
  // Display parameters menu header
  st = _Get_mn_caption();
  VT100_send_str_to_pos(st, 1, VT100_find_str_center(st));
  VT100_send_str_to_pos(DASH_LINE, 2, 0);
  VT100_send_str_to_pos((uint8_t *)PARAM_EDITOR_HELP, 3, 0);
  MPRINTF("\r\n");
  MPRINTF(DASH_LINE);

  // Display list of submenus at current level
  n = 0;
  for (i = 0; i < p_pars->menu_items_num; i++)
  {
    if (p_pars->menu_items_array[i].prevlev != mcbl->current_level) continue;
    sprintf((char *)str, "%1X - %s", n, p_pars->menu_items_array[i].name);

    if ((strlen((char *)str) + SCR_ITEMS_HOR_OFFS) > COLCOUNT)
    {
      str[COLCOUNT - SCR_ITEMS_HOR_OFFS - 3] = '.';
      str[COLCOUNT - SCR_ITEMS_HOR_OFFS - 2] = '>';
      str[COLCOUNT - SCR_ITEMS_HOR_OFFS - 1] = 0;
    }
    VT100_send_str_to_pos(str, SCR_ITEMS_VERT_OFFS + n, SCR_ITEMS_HOR_OFFS);
    mcbl->item_indexes[n] = i;
    n++;
  }
  mcbl->current_menu_submenus_count = n;

  // Find maximum parameter description length for alignment
  int max_desc_len                  = 0;
  for (i = 0; i < p_pars->items_num; i++)
  {
    if (p_pars->items_array[i].parmnlev != mcbl->current_level) continue;
    int desc_len = strlen((char *)p_pars->items_array[i].var_description);
    if (desc_len > max_desc_len)
    {
      max_desc_len = desc_len;
    }
  }

  // Display list of all parameters at current level
  for (i = 0; i < p_pars->items_num; i++)
  {
    int len;
    int desc_len;
    int spaces_needed;
    if (p_pars->items_array[i].parmnlev != mcbl->current_level) continue;
    VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS + n, SCR_ITEMS_HOR_OFFS);
    sprintf((char *)str, "%1X - ", n);
    MPRINTF((char *)str);
    len = strlen((char *)str) + SCR_ITEMS_HOR_OFFS;
    MPRINTF(VT100_REVERSE_ON);
    MPRINTF((char *)p_pars->items_array[i].var_description);

    // Calculate spaces needed for alignment
    desc_len      = strlen((char *)p_pars->items_array[i].var_description);
    spaces_needed = max_desc_len - desc_len;

    // Add spaces for alignment
    for (int j = 0; j < spaces_needed; j++)
    {
      MPRINTF(" ");
    }

    MPRINTF(" = ");
    len = len + max_desc_len + 3;  // +3 for " = "
    MPRINTF(VT100_REVERSE_OFF);
    // Try to get caption from selector first, otherwise convert parameter to string
    if (!_Get_selector_caption(p_pars, i, str, MAX_VALUE_STR_SZ - 1))
    {
      Convert_parameter_to_str(p_pars, str, MAX_VALUE_STR_SZ - 1, i);
    }
    if ((strlen((char *)str) + len) > COLCOUNT)
    {
      str[COLCOUNT - len - 3] = '.';
      str[COLCOUNT - len - 2] = '>';
      str[COLCOUNT - len - 1] = 0;
    }
    MPRINTF((char *)str);
    mcbl->item_indexes[n] = i;
    n++;
  }
  mcbl->current_menu_items_count = n;
}

/*-----------------------------------------------------------------------------------------------------
  Function periodically called as key press handler from the main monitor loop

  Parameters: b - key code (0-F for menu items, M/R for navigation)

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Params_editor_press_key_handler(uint8_t b)
{
  GET_MCBL;
  const T_NV_parameters_instance *p_pars = mcbl->p_pinst;

  if (b < 0x30) return;  // Ignore service codes

  if ((b == 'M') || (b == 'm'))
  {
    Goto_main_menu();
    return;
  }
  if ((b == 'R') || (b == 'r'))
  {
    if (mcbl->current_level != MAIN_PARAMS_ROOT)
    {
      mcbl->current_level = _Get_mn_prevlev();
      Show_parameters_menu();
      return;
    }
    else
    {
      // Return to the menu item from which the parameter editor was entered
      Return_to_prev_menu();
      Display_menu();
      return;
    }
  }

  b = ascii_to_hex(b);                              // Convert key code to hexadecimal digit
  if (b >= MAX_ITEMS_COUNT) return;

  if (b >= mcbl->current_menu_items_count) return;  // Exit if code is greater than available items

  // Determine the type of menu item for the key code
  uint8_t mtype = MTYPE_NONE;

  if (b < mcbl->current_menu_submenus_count)
  {
    mtype = MTYPE_SUBMENU;
  }
  else
  {
    mtype = MTYPE_PARAMETER;
  }

  switch (mtype)
  {
    case MTYPE_SUBMENU:
      // Submenu item selected
      mcbl->current_level = p_pars->menu_items_array[mcbl->item_indexes[b]].currlev;
      Show_parameters_menu();
      break;

    case MTYPE_PARAMETER:
      // Parameter selected for editing
      mcbl->current_parameter_indx = mcbl->item_indexes[b];
      _Goto_to_edit_param();
      break;
  }
}
/*-----------------------------------------------------------------------------------------------------
  Display possible values for a parameter based on its selector_id

  Parameters:
    p_pars - pointer to the parameters instance
    param_index - index of the parameter in the items_array

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void Display_possible_values(const T_NV_parameters_instance *p_pars, int param_index)
{
  GET_MCBL;
  uint32_t                selector_id   = p_pars->items_array[param_index].selector_id;
  const T_selectors_list *selectors     = p_pars->selectors_array;
  int                     selectors_num = p_pars->selectors_num;

  // Corrected logic to match selector_id with items_list values
  if (selector_id < selectors_num && selectors[selector_id].items_list != NULL)
  {
    for (uint32_t i = 0; i < selectors[selector_id].items_cnt; i++)
    {
      MPRINTF("%d - %s\n", selectors[selector_id].items_list[i].val, selectors[selector_id].items_list[i].caption);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Go to parameter editing mode

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Goto_to_edit_param(void)
{
  GET_MCBL;
  int                             n;
  const T_NV_parameters_instance *p_pars = mcbl->p_pinst;
  mcbl->firstrow                         = ROWCOUNT - 4;
  VT100_set_cursor_pos(mcbl->firstrow - 2, 0);
  MPRINTF("Edited parameter: '%s'\r\n", p_pars->items_array[mcbl->current_parameter_indx].var_description);
  MPRINTF(DASH_LINE);
  Convert_parameter_to_str(p_pars, mcbl->param_str, MONIT_STR_MAXLEN, mcbl->current_parameter_indx);
  VT100_set_cursor_pos(mcbl->firstrow, 0);
  MPRINTF(VT100_CLR_FM_CRSR);
  VT100_set_cursor_pos(mcbl->firstrow, 0);
  MPRINTF("> ");
  MPRINTF(VT100_CURSOR_ON);
  MPRINTF((char *)mcbl->param_str);

  // Display possible values if selector_id is not 0
  if (p_pars->items_array[mcbl->current_parameter_indx].selector_id != 0)
  {
    VT100_set_cursor_pos(mcbl->firstrow + 1, 0);
    MPRINTF("\r\n================ POSSIBLE VALUES ================\r\n");
    Display_possible_values(p_pars, mcbl->current_parameter_indx);
    MPRINTF("=================================================\r\n");
  }
  // Calculate current cursor position
  n = strlen((char *)mcbl->param_str);
  if (n > MONIT_STR_MAXLEN)
  {
    n                                 = MONIT_STR_MAXLEN;
    mcbl->param_str[MONIT_STR_MAXLEN] = 0;  // Ensure null termination
  }

  // Set logical position in buffer (after last character)
  mcbl->current_pos = n++;

  // Calculate screen position: cursor should be after the last character.
  // Assumes mcbl->current_col will be 1-indexed (1 to COLCOUNT) for VT100_set_cursor_pos.
  // mcbl->current_row is 0-indexed relative to mcbl->firstrow.
  uint32_t total_chars_on_line = n + 2; // Number of characters including "> " prefix

  mcbl->current_row = (total_chars_on_line - 1) / COLCOUNT;
  mcbl->current_col = (total_chars_on_line - 1) % COLCOUNT + 1;

  // Set the physical cursor position
  VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);

  Set_monitor_func(Edit_func);
}

/*-----------------------------------------------------------------------------------------------------
  Edit parameter in terminal window

  Parameters:
    b - key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Edit_func(uint8_t b)
{
  GET_MCBL;
  uint32_t res;  // Variable to store the result of conversion

  switch (b)
  {
    case VT100_BCKSP:  // Back Space
      if (mcbl->current_pos > 0)
      {
        mcbl->current_pos--;
        mcbl->param_str[mcbl->current_pos] = 0; // Null-terminate at new position

        // Update screen cursor position
        if (mcbl->current_col == 1) // If at the beginning of a line (column 1)
        {
          if (mcbl->current_row > 0) // And not on the first line of the edit area
          {
            mcbl->current_row--;
            mcbl->current_col = COLCOUNT;
          }
          // If on the first line (mcbl->current_row == 0) and first col (mcbl->current_col == 1),
          // do not move further left than the start of the prompt (col 3 after "> ")
          // However, the prompt itself is not deletable, so current_pos > 0 handles this.
          // The cursor should not go before the "> " prefix, which is at column 1 and 2.
          // The editable area starts at column 3.
          // If current_pos becomes 0, it means we deleted the first char of the param string.
          // The cursor should then be at firstrow + 0, column 3.
        }
        else
        {
          mcbl->current_col--;
        }

        // Set cursor, print space to erase char, then set cursor back
        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);
        MPRINTF(" ");
        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);
      }
      break;

    case VT100_DEL: // Delete key (often sent as 0x7F)
      // Optional: Implement forward delete if needed, or treat as backspace
      break;

    case VT100_ESC: // Escape - Cancel editing without saving
      MPRINTF(VT100_CURSOR_OFF);
      Show_parameters_menu();
      Set_monitor_func(Params_editor_press_key_handler);
      break;

    case VT100_CR: // Enter
      mcbl->param_str[mcbl->current_pos] = 0;
      res                                = Convert_str_to_parameter(mcbl->p_pinst, mcbl->param_str, mcbl->current_parameter_indx);
      if (res == RES_OK)
      {
        // Call function if it exists for the parameter
        if (mcbl->p_pinst->items_array[mcbl->current_parameter_indx].func != 0)
        {
          mcbl->p_pinst->items_array[mcbl->current_parameter_indx].func();
        }
        Request_save_settings(APPLICATION_PARAMS, MEDIA_TYPE_DATAFLASH, NULL);
        MPRINTF(VT100_CURSOR_OFF);
        Show_parameters_menu();
        Set_monitor_func(Params_editor_press_key_handler);
      }
      else
      {
        // Handle conversion error, show a message
        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row + 1, 0);              // Move cursor down
        MPRINTF(VT100_CLR_LINE);                                                      // Clear the line
        MPRINTF("Error: Invalid value entered.");
        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);  // Restore cursor position
        // Keep the editor active to allow correction
      }
      break;

    case VT100_LEFT_ARROW: // Left arrow - move cursor left
      if (mcbl->current_pos > 0)
      {
        mcbl->current_pos--;

        // Update screen cursor position
        if (mcbl->current_col == 1) // If at the beginning of a line (column 1)
        {
          if (mcbl->current_row > 0) // And not on the first line of the edit area
          {
            mcbl->current_row--;
            mcbl->current_col = COLCOUNT;
          }
          // If we're at the very beginning (row 0, col 1), we shouldn't go further left
          // than the start position after "> " (which is col 3)
        }
        else
        {
          mcbl->current_col--;
        }

        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);
      }
      break;

    case VT100_RIGHT_ARROW: // Right arrow - move cursor right
      if (mcbl->current_pos < strlen((char *)mcbl->param_str))
      {
        mcbl->current_pos++;

        // Update screen cursor position
        if (mcbl->current_col == COLCOUNT) // If at the end of a line
        {
          mcbl->current_row++;
          mcbl->current_col = 1;
        }
        else
        {
          mcbl->current_col++;
        }

        VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);
      }
      break;

    case VT100_UP_ARROW: // Up arrow - reserved for future functionality
      // Could be used for command history navigation in the future
      break;

    case VT100_DOWN_ARROW: // Down arrow - reserved for future functionality
      // Could be used for command history navigation in the future
      break;

    default:
      if (isprint(b)) // Use isprint to allow spaces but not other control chars
      {
        if (mcbl->current_pos < (MONIT_STR_MAXLEN -1)) // MONIT_STR_MAXLEN includes null terminator
        {
          // Optional: Implement insert mode vs overwrite mode here if needed.
          // Current implementation is overwrite, but MPRINTF advances cursor.
          // For true insert, would need to shift existing text.
          // Assuming simple append/overwrite at current_pos for now.

          mcbl->param_str[mcbl->current_pos] = (uint8_t)b;
          MPRINTF("%c", b); // Print the character, MPRINTF handles cursor advancement

          mcbl->current_pos++;
          mcbl->param_str[mcbl->current_pos] = 0;  // Always null-terminate

          // Update screen cursor position based on MPRINTF's behavior
          if (mcbl->current_col == COLCOUNT) // If MPRINTF wrapped to next line
          {
            mcbl->current_row++;
            mcbl->current_col = 1;
          }
          else
          {
            mcbl->current_col++;
          }
          // No need to call VT100_set_cursor_pos here if MPRINTF places it correctly.
          // However, to be certain, especially if MPRINTF's behavior is complex:
          VT100_set_cursor_pos(mcbl->firstrow + mcbl->current_row, mcbl->current_col);
        }
      }
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Find menu name by its identifier

  Parameters:
    p_pars   - pointer to parameters instance
    menu_lev - menu level identifier

  Return: pointer to menu name string
-----------------------------------------------------------------------------------------------------*/
uint8_t *Get_mn_name(const T_NV_parameters_instance *p_pars, uint32_t menu_lev)
{
  uint16_t i;
  for (i = 0; i < p_pars->menu_items_num; i++)
  {
    if (p_pars->menu_items_array[i].currlev == menu_lev) return (uint8_t *)p_pars->menu_items_array[i].name;
  }
  return (uint8_t *)p_pars->menu_items_array[0].name;
}
