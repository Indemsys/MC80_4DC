// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2024-07-02
// 15:17:32
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

const char *days_abbrev[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months_abbrev[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*-------------------------------------------------------------------------------------------------------------
  Clear monitor screen
-------------------------------------------------------------------------------------------------------------*/
void VT100_clr_screen(void)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
}

/*-------------------------------------------------------------------------------------------------------------
  Set cursor to specified position
  Column and row counting starts from zero
-------------------------------------------------------------------------------------------------------------*/
void VT100_set_cursor_pos(uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
}

/*-------------------------------------------------------------------------------------------------------------
  Output string to specified position
-------------------------------------------------------------------------------------------------------------*/
void VT100_send_str_to_pos(uint8_t *str, uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
  SEND_BUF(str, strlen((char *)str));
}

/*-------------------------------------------------------------------------------------------------------------
  Find string start position to center it on screen
-------------------------------------------------------------------------------------------------------------*/
uint8_t VT100_find_str_center(uint8_t *str)
{
  int16_t l = 0;
  while (*(str + l) != 0) l++;  // Find string length
  return (COLCOUNT - l) / 2;
}

/*-----------------------------------------------------------------------------------------------------
  Get string from input

  Parameters:
    lp - pointer to string buffer
    n  - maximum string length

  Return:
    Number of characters read
-----------------------------------------------------------------------------------------------------*/
int VT100_get_string(char *lp, int n)
{
  int  cnt = 0;
  char c;
  GET_MCBL;

  do
  {
    if (WAIT_CHAR((unsigned char *)&c, ms_to_ticks(1000)) == RES_OK)
    {
      switch (c)
      {
        case VT100_CNTLQ:  // ignore Control S/Q
        case VT100_CNTLS:
          break;

        case VT100_BCKSP:
        case VT100_DEL:
          if (cnt == 0)
          {
            break;
          }
          cnt--;  // decrement count
          lp--;   // and line VOID*
                  // echo backspace
          MPRINTF("\x008 \x008");
          break;
        case VT100_ESC:
          *lp = 0;  // ESC - stop editing line
          return (RES_ERROR);
        default:
          MPRINTF("*");
          *lp = c;  // echo and store character
          lp++;     // increment line VOID*
          cnt++;    // and count
          break;
      }
    }
  } while (cnt < n - 1 && c != 0x0d);  // check limit and line feed
  *lp = 0;  // mark end of string
  return (RES_OK);
}

/*-----------------------------------------------------------------------------------------------------
  String input

  Parameters:
    buf      - buffer for input characters
    buf_len  - buffer size including null byte. Buffer should also accommodate initial string
    row      - row where input will be performed
    instr    - string with initial value

  Return:
    RES_OK if input was successful
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_string_in_pos(char *buf, int buf_len, int row, char *instr)
{
  int     indx = 0;
  uint8_t b;
  int     res;
  uint8_t bs_seq[] = {VT100_BCKSP, ' ', VT100_BCKSP, 0};
  GET_MCBL;

  indx = 0;
  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);
  MPRINTF(">");

  if (instr != 0)
  {
    indx = strlen(instr);
    if (indx >= (buf_len - 1)) indx = buf_len - 1;
    SEND_BUF(instr, indx);
    for (uint32_t n = 0; n < indx; n++)
    {
      buf[n] = instr[n];
    }
  }

  do
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) != RES_OK)
    {
      res = RES_ERROR;
      goto exit_;
    };

    if (b == VT100_BCKSP)
    {
      if (indx > 0)
      {
        indx--;
        SEND_BUF(bs_seq, sizeof(bs_seq));
      }
    }
    else if (b == VT100_ESC)
    {
      res = RES_ERROR;
      goto exit_;
    }
    else if (b != VT100_CR && b != VT100_LF && b != 0)
    {
      SEND_BUF(&b, 1);
      buf[indx] = b; /* String[i] value set to alpha */
      indx++;
      if (indx >= buf_len)
      {
        res = RES_ERROR;
        goto exit_;
      };
    }
  } while ((b != VT100_CR) && (indx < COL));

  res       = RES_OK;
  buf[indx] = 0; /* End of string set to NUL */
exit_:

  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);

  return (res);
}

/*-----------------------------------------------------------------------------------------------------
  String editing

  Parameters:
    buf          - buffer for string editing
    buf_len      - buffer size for editing not including terminating 0
    instr        - initial string value

  Return:
    int32_t
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_string(char *buf, uint32_t buf_len, char *instr)
{
  int     indx = 0;
  uint8_t b;
  int     res;
  uint8_t bs_seq[] = {VT100_BCKSP, ' ', VT100_BCKSP, 0};
  GET_MCBL;

  indx = 0;
  MPRINTF(">");

  if (instr != 0)
  {
    indx = strlen(instr);
    if (indx >= (buf_len - 1)) indx = buf_len - 1;
    SEND_BUF(instr, indx);
    for (uint32_t n = 0; n < indx; n++)
    {
      buf[n] = instr[n];
    }
  }

  do
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) != RES_OK)
    {
      res = RES_ERROR;
      goto exit_;
    };

    if (b == VT100_BCKSP)
    {
      if (indx > 0)
      {
        indx--;
        SEND_BUF(bs_seq, sizeof(bs_seq));
      }
    }
    else if (b == VT100_ESC)
    {
      res = RES_ERROR;
      goto exit_;
    }
    else if (b != VT100_CR && b != VT100_LF && b != 0)
    {
      if (indx < (buf_len - 1))
      {
        SEND_BUF(&b, 1);
        buf[indx] = b;
        indx++;
      };
    }
  } while ((b != VT100_CR) && (indx < COL));

  res       = RES_OK;
  buf[indx] = 0;
exit_:

  return (res);
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_uinteger_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  sprintf(str, "%d", *value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%d", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_integer_val(uint32_t row, int32_t *value, int32_t minv, int32_t maxv)
{
  char    str[32];
  char    buf[32];
  int32_t tmpv;
  sprintf(str, "%d", *value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%d", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_float_val(uint32_t row, float *value, float minv, float maxv)
{
  char  str[32];
  char  buf[32];
  float tmpv;
  sprintf(str, "%f", (double)*value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%f", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Edit unsigned integer value in hexadecimal format

  Parameters:
    row   - row for input (input will be at this row, prompt should be printed separately)
    value - pointer to value
    minv  - minimum allowed value
    maxv  - maximum allowed value

  Return:
    int32_t - RES_OK if input successful, RES_ERROR if cancelled (ESC)
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_uinteger_hex_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  sprintf(str, "%08X", *value);  // Print as 8-digit hex
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%x", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
      return RES_OK;
    }
  }
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Edit unsigned integer value with selectable format (hex or decimal)

  Parameters:
    row      - row for input (input will be at this row, prompt should be printed separately)
    value    - pointer to value
    minv     - minimum allowed value
    maxv     - maximum allowed value
    hex_mode - if true, input is hex, else decimal

  Return:
    int32_t - RES_OK if input successful, RES_ERROR if cancelled (ESC)
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_uinteger_val_mode(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv, bool hex_mode)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  if (hex_mode)
  {
    sprintf(str, "%08X", *value);
  }
  else
  {
    sprintf(str, "%u", *value);
  }
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (hex_mode)
    {
      if (sscanf(buf, "%x", &tmpv) == 1)
      {
        if (tmpv > maxv) tmpv = maxv;
        if (tmpv < minv) tmpv = minv;
        *value = tmpv;
        return RES_OK;
      }
    }
    else
    {
      if (sscanf(buf, "%u", &tmpv) == 1)
      {
        if (tmpv > maxv) tmpv = maxv;
        if (tmpv < minv) tmpv = minv;
        *value = tmpv;
        return RES_OK;
      }
    }
  }
  return RES_ERROR;
}

/*------------------------------------------------------------------------------
  Memory dump output

  Parameters:
    addr       - displayed starting address of dump
    buf        - pointer to memory
    buf_len    - number of bytes
    sym_in_str - number of bytes displayed per dump line

  Return:
    int32_t
 ------------------------------------------------------------------------------*/
void VT100_print_dump(uint32_t addr, void *buf, uint32_t buf_len, uint8_t sym_in_str)
{
  uint32_t i;
  uint32_t scnt;
  uint8_t *pbuf;
  GET_MCBL;

  pbuf = (uint8_t *)buf;
  scnt = 0;
  for (i = 0; i < buf_len; i++)
  {
    if (scnt == 0)
    {
      MPRINTF("%08X: ", addr);
    }

    MPRINTF("%02X ", pbuf[i]);

    addr++;
    scnt++;
    if (scnt >= sym_in_str)
    {
      scnt = 0;
      MPRINTF("\r\n");
    }
  }

  if (scnt != 0)
  {
    MPRINTF("\r\n");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Input waiting function with VT100 special keys handling

  Processes ESC sequences for arrow keys and returns
  special codes defined in Monitor_utilites.h

  Parameters:
    key     - pointer to variable for storing received character
    timeout - wait time in ticks

  Return:
    RES_OK if character received, otherwise RES_ERROR
-----------------------------------------------------------------------------------------------------*/
int VT100_wait_special_key(uint8_t *key, int timeout)
{
  GET_MCBL;
  uint8_t b;
  int     res;
  res = WAIT_CHAR(&b, timeout);

  if (res != RES_OK)
  {
    return res;  // Timeout expired or error
  }

  // Check if character is the start of ESC sequence
  if (b == VT100_ESC)
  {
    // Wait for second character (should be '[', but skip check for speed)
    if (WAIT_CHAR(&b, 50) != RES_OK)
    {
      *key = VT100_ESC;  // If second character didn't arrive, return ESC
      return RES_OK;
    }

    if (b != '[')
    {
      *key = b;  // If second character is not '[', return it
      return RES_OK;
    }

    // Wait for third character (key code)
    if (WAIT_CHAR(&b, 50) != RES_OK)
    {
      *key = '[';  // If third character didn't arrive, return '['
      return RES_OK;
    }

    // Process arrow key codes
    switch (b)
    {
      case 'A':  // Up arrow
        *key = VT100_UP_ARROW;
        break;
      case 'B':  // Down arrow
        *key = VT100_DOWN_ARROW;
        break;
      case 'C':  // Right arrow
        *key = VT100_RIGHT_ARROW;
        break;
      case 'D':  // Left arrow
        *key = VT100_LEFT_ARROW;
        break;
      default:
        *key = b;  // If sequence not recognized, return last character
        break;
    }
  }
  else
  {
    *key = b;  // Regular character, just return it
  }

  return RES_OK;
}
