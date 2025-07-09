#include "stdint.h"
/*-----------------------------------------------------------------------------------------------------
  Get build date string

  Parameters:
    None

  Return:
    const char* - compilation date string
-----------------------------------------------------------------------------------------------------*/
const char *Get_build_date(void)
{
  return __DATE__;
}

/*-----------------------------------------------------------------------------------------------------
  Get build time string

  Parameters:
    None

  Return:
    const char* - compilation time string
-----------------------------------------------------------------------------------------------------*/
const char *Get_build_time(void)
{
  return __TIME__;
}
