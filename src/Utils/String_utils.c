// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2023-02-02
// 15:08:29
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

const char *monts[12]      = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char  pass_symbols[] = "@*#$&0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*-----------------------------------------------------------------------------------------------------
  Validate password symbols

  Parameters:
    password - password string to validate
    pass_len - password length

  Return:
    RES_OK if valid, RES_ERROR if invalid
-----------------------------------------------------------------------------------------------------*/
uint32_t Validate_password_symbols(char *password, uint32_t pass_len)
{
  for (uint32_t i = 0; i < pass_len; i++)
  {
    if (strchr(pass_symbols, password[i]) == NULL) return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Convert BCD2 value to byte value

  Parameters:
    val - BCD2 value to convert

  Return:
    Converted byte value
-----------------------------------------------------------------------------------------------------*/
uint8_t BCD2ToBYTE(uint8_t val)
{
  uint32_t tmp;
  tmp = ((val & 0xF0) >> 4) * 10;
  return (uint8_t)(tmp + (val & 0x0F));
}

/*-----------------------------------------------------------------------------------------------------
  Convert BCD value to byte

  Parameters:
    val - BCD value to convert

  Return:
    Converted byte value
-----------------------------------------------------------------------------------------------------*/
uint8_t BYTEToBCD2(uint8_t val)
{
  uint8_t b1 = val / 10;
  uint8_t b2 = val % 10;

  return (b1 << 4) + b2;
}

/*-----------------------------------------------------------------------------------------------------
  Convert ASCII character to hexadecimal value

  Parameters:
    c - ASCII character to convert

  Return:
    Hexadecimal value
-----------------------------------------------------------------------------------------------------*/
uint8_t ascii_to_hex(uint8_t c)
{
  if (c >= '0' && c <= '9')
    return (c - '0') & 0x0f;
  else if (c >= 'a' && c <= 'f')
    return (c - 'a' + 10) & 0x0f;
  else
    return (c - 'A' + 10) & 0x0f;
}

/*-----------------------------------------------------------------------------------------------------
  Convert hexadecimal value to ASCII character

  Parameters:
    c - Hexadecimal value to convert

  Return:
    ASCII character
-----------------------------------------------------------------------------------------------------*/
uint8_t hex_to_ascii(uint8_t c)
{
  c = c & 0xf;
  if (c <= 9) return (c + 0x30);
  return (c + 'A' - 10);
}

/*-------------------------------------------------------------------------------------------------------------
   Trim spaces from front and back and remove quotes if present
-------------------------------------------------------------------------------------------------------------*/
char *Trim_and_dequote_str(char *str)
{
  int i;

  while (*str == ' ')
  {
    str++;
    if (*str == 0) return str;
  }

  for (i = (strlen(str) - 1); i > 0; i--)
  {
    if (str[i] != ' ') break;
    str[i] = 0;
  }

  if ((str[0] == '"') && (str[strlen(str) - 1] == '"'))
  {
    str[strlen(str) - 1] = 0;
    str++;
  }
  return str;
}

/*-------------------------------------------------------------------------------------------------------------
  Read string data from buffer buf into string str until null or combination "\r\n" or "\n\r" is found,
  but not longer than len characters.
  Advance the buffer pointer buf
  Characters \r and \n are removed from the string

  Returns -1 if byte 0 is found at the beginning of the file
-------------------------------------------------------------------------------------------------------------*/
int Read_cstring_from_buf(char **buf, char *str, uint32_t len)
{
  int32_t pos;
  char    c;
  char   *ptr;

  ptr = *buf;
  pos = 0;
  do
  {
    c = *ptr;  // Read character from current position
    if (c == 0)
    {
      if (pos == 0)
      {
        *buf = ptr;
        return (-1);  // Return -1 if first character in first position = 0
      }
      // Remove '\r' and '\n' characters from previous string position so they don't remain in the string
      if ((str[pos - 1] == '\r') || (str[pos - 1] == '\n'))
      {
        str[pos - 1] = 0;
      }
      *buf = ptr;    // Return pointer to the beginning of next substring
      return (len);
    }
    str[pos++] = c;  // Write character to string
    ptr++;           // Advance current position in string
    str[pos] = 0;    // Add 0 so string always ends with null
    if (pos > 1)     // Check for pattern end of string
    {
      if (strcmp(&str[pos - 2], "\r\n") == 0)
      {
        str[pos - 2] = 0;
        *buf         = ptr;
        return (pos - 2);
      }
      if (strcmp(&str[pos - 2], "\n\r") == 0)
      {
        str[pos - 2] = 0;
        ptr--;
        *buf = ptr;
        return (pos - 2);
      }
    }
    if (pos >= len)
    {
      *buf = ptr;
      return (len);
    }
  } while (1);
}

/*-----------------------------------------------------------------------------------------------------
  Search for a string in buffer ending with 0 or one of the characters "\r" and "\n"
  Characters "\r" and "\n" in buffer are replaced with 0

  Parameters:
    **buf      - Pointer to buffer. After function call shifts to beginning of next string
    *buf_len   - Pointer to buffer length. If string found pointer returns number of remaining unprocessed data

  Return:
    Pointer to beginning of null-terminated string

  Example:
  Processing such buffer - {0, '1' , '2' , '3' , '\r', '\n' , '4' , '5' , '6', '\r', '7' , '8' , '\n', 0 , 'q', 'w' , 0 , 0 , 0 , 'e', 'r'};
  Will result in output of following strings: "123" "456" "78" "qw" "e"
-----------------------------------------------------------------------------------------------------*/
uint8_t *Isolate_string_in_buf(uint8_t **buf, uint32_t *buf_len)
{
  uint8_t *ptr;
  int32_t  pos;
  uint8_t  ch;
  int      cnt;
  int      len;

  len = *buf_len;
  if (len < 2) return 0;
  cnt = 0;
  ptr = *buf;
  pos = 0;
  do
  {
    ch = ptr[pos];  // Read character from current position
    if (ch == 0)
    {
      if (cnt == 0)
      {
        // Skip first zeros going to end of buffer
        if (pos < (len - 2))
        {
          pos++;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        *buf     = &ptr[pos];
        *buf_len = len - pos;
        return &ptr[pos - cnt];
      }
    }
    else if ((ch == '\r') || (ch == '\n'))
    {
      ptr[pos] = 0;
    }
    else
    {
      if (pos < (len - 1))
      {
        cnt++;
        pos++;
      }
      else
      {
        ptr[pos] = 0;
        *buf     = &ptr[pos];
        *buf_len = len - pos;
        return &ptr[pos - cnt];
      }
    }
  } while (1);
}

/*-----------------------------------------------------------------------------------------------------
  Extract month, day and year numbers from date string

  Parameters:
    date_str - Date string to parse
    pmonts   - Pointer to store month number
    pday     - Pointer to store day number
    pyear    - Pointer to store year number

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_nums_from_date(const char *date_str, uint32_t *pmonts, uint32_t *pday, uint32_t *pyear)
{
  char month_str[8];

  sscanf(date_str, "%s %d %d", month_str, pday, pyear);
  for (uint32_t i = 0; i < 12; i++)
  {
    if (strncmp(monts[i], month_str, 3) == 0)
    {
      *pmonts = i + 1;
      break;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Extract hour, minute and second numbers from time string

  Parameters:
    time_str - Time string to parse
    hours    - Pointer to store hour number
    mins     - Pointer to store minute number
    secs     - Pointer to store second number

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_nums_from_time(const char *time_str, uint32_t *hours, uint32_t *mins, uint32_t *secs)
{
  sscanf(time_str, "%d:%d:%d", hours, mins, secs);
}

/*-----------------------------------------------------------------------------------------------------
  Generate build date and time version string

  Parameters:
    ver_str     - Output string buffer to store version information
    buffer_size - Size of the output buffer

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_build_date_time(char *ver_str, uint32_t buffer_size)
{
  uint32_t monts;
  uint32_t day;
  uint32_t year;
  uint32_t hours;
  uint32_t mins;
  uint32_t secs;

  if (ver_str == NULL || buffer_size == 0)
  {
    return;
  }

  Get_nums_from_date(Get_build_date(), &monts, &day, &year);
  Get_nums_from_time(Get_build_time(), &hours, &mins, &secs);
  snprintf(ver_str, buffer_size, "VER: %04d_%02d_%02d_%02d%02d", year, monts, day, hours, mins);
}

/*-----------------------------------------------------------------------------------------------------
  Search for a number in HTML text stream enclosed between markers specified in control structure

  Parameters:
    block      - input data block
    block_size - size of input data block in bytes
    fnd        - control structure for state machine

  Return:
    1 if marked number found, 0 otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Find_marked_number(uint8_t *block, uint32_t block_size, T_marked_str_finder *fnd)
{
  uint8_t b;

  if (fnd->step == 0)
  {
    fnd->cnt         = 0;
    fnd->fragment_sz = 0;
    fnd->step        = 1;
  }
  else if (fnd->step < 0)
  {
    return 0;
  }
  else if (fnd->step > 2)
  {
    return 1;
  }

  while (block_size > 0)
  {
    b = *block;
    switch (fnd->step)
    {
      case 1:
        // Search for match with left text marker
        if (b == fnd->left_mark[fnd->cnt])
        {
          fnd->cnt++;
          if (fnd->cnt == fnd->left_mark_sz)
          {
            fnd->step++;
            fnd->cnt = 0;
          }
        }
        else
        {
          fnd->cnt = 0;
        }
        break;

      case 2:
        // Search for match with right text marker
        if (b == fnd->right_mark[fnd->cnt])
        {
          fnd->cnt++;
          if (fnd->cnt == fnd->right_mark_sz)
          {
            fnd->step = 0;
            if ((fnd->fragment_sz - fnd->cnt) > 0)
            {
              fnd->fragment_sz                = fnd->fragment_sz - fnd->cnt + 1;
              fnd->fragment[fnd->fragment_sz] = 0;
              fnd->cnt                        = 0;
              fnd->step                       = 3;
              fnd->next_sym_ptr               = block + 1;
              return 1;
            }
            fnd->cnt = 0;
          }
        }
        else
        {
          fnd->cnt = 0;
        }
        // Accumulate string that may contain the searched fragment
        fnd->fragment[fnd->fragment_sz] = b;
        fnd->fragment_sz++;
        if ((fnd->fragment_sz - fnd->cnt) >= MAX_MARKED_STR_SIZE)
        {
          // If string reaches maximum size, cancel it and restart the entire search
          fnd->step = 0;
        }
        break;
    }
    block++;
    block_size--;
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Convert 8-bit value to binary string representation

  Parameters:
    str - Output string buffer for binary representation
    b   - 8-bit value to convert

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_8bit_str(char *str, uint8_t b)
{
  uint8_t i;
  uint8_t mask = 0x80;
  uint8_t n    = 0;
  for (i = 0; i < 4; i++)
  {
    if (b & mask)
    {
      str[n++] = '1';
    }
    else
    {
      str[n++] = '0';
    }
    mask >>= 1;
  }
  str[n++] = ' ';
  for (i = 0; i < 4; i++)
  {
    if (b & mask)
    {
      str[n++] = '1';
    }
    else
    {
      str[n++] = '0';
    }
    mask >>= 1;
  }
  str[n] = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Convert 16-bit value to binary string representation

  Parameters:
    str - Output string buffer for binary representation
    b   - 16-bit value to convert

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_16bit_str(char *str, uint16_t b)
{
  uint16_t mask = 0x8000;
  uint8_t  n    = 0;
  for (uint8_t j = 0; j < 4; j++)
  {
    for (uint8_t i = 0; i < 4; i++)
    {
      if (b & mask)
      {
        str[n++] = '1';
      }
      else
      {
        str[n++] = '0';
      }
      mask >>= 1;
    }
    str[n++] = ' ';
  }
  str[n] = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Convert byte array to hexadecimal string representation

  Parameters:
    str - Output string buffer for hexadecimal representation
    len - Length of array to convert
    arr - Byte array to convert

  Return:
    Pointer to the output string
-----------------------------------------------------------------------------------------------------*/
char *Buf_to_hex_str(char *str, uint32_t len, uint8_t *arr)
{
  char *res = str;
  for (uint32_t i = 0; i < len; i++)
  {
    *str++ = hex_to_ascii(arr[i] >> 4);
    *str++ = hex_to_ascii(arr[i]);
    *str++ = ' ';
  }
  *str = 0;
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Check if all elements in byte array are zeros

  Parameters:
    arr  - Byte array to check
    size - Size of array in bytes

  Return:
    0 if all elements are zero, 1 otherwise
-----------------------------------------------------------------------------------------------------*/
uint8_t Is_All_Zeros(uint8_t *arr, uint32_t size)
{
  for (uint32_t i = 0; i < size; i++)
  {
    if (arr[i] != 0)
    {
      return 1;
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Print formatted string to buffer or calculate required buffer size

  Parameters:
    buffer  - Pointer to buffer where printing is performed, or NULL to calculate size only
    offset  - Offset in buffer to start printing at
    fmt_ptr - Format string

  Return:
    Number of bytes that would be printed
-----------------------------------------------------------------------------------------------------*/
uint32_t Print_to(char *buffer, uint32_t offset, const char *fmt_ptr, ...)
{
  uint32_t n;
  va_list  ap;
  va_start(ap, fmt_ptr);
  if (buffer != NULL)
  {
    n = vsprintf(buffer + offset, (const char *)fmt_ptr, ap);
  }
  else
  {
    n = vsnprintf(NULL, 0, (const char *)fmt_ptr, ap);
  }
  va_end(ap);
  return n;
}

/*-----------------------------------------------------------------------------------------------------
  Remove leading and trailing whitespace from string and copy content from src to dst

  Parameters:
    src      - Source string to trim
    dst      - Destination buffer for trimmed string
    dst_size - Size of destination buffer

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Trim_str(const char *src, char *dst, uint32_t dst_size)
{
  if (src == NULL || dst == NULL) return;

  // Skip whitespace at the beginning of string
  while (isspace((unsigned char)*src)) src++;

  // Copy string after removing leading whitespace
  // Leave space for null terminator
  size_t to_copy = strlen(src);
  if (to_copy >= dst_size) to_copy = dst_size - 1;

  strncpy(dst, src, to_copy);
  dst[to_copy] = '\0';  // Ensure string is properly terminated

  // Remove whitespace at the end of string
  char *end    = dst + strlen(dst) - 1;
  while (end > dst && isspace((unsigned char)*end))
  {
    *end = '\0';
    end--;
  }
}
