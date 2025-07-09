#include "bsp_api.h"
#include "tx_api.h"

#ifdef TX_USER_TRACE_BUFFER_DECLARE
TX_USER_TRACE_BUFFER_DECLARE;
#endif

extern void Main_thread_create(void* first_unused_memory);

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void tx_application_define(void* first_unused_memory)
{
  Main_thread_create(first_unused_memory);

#ifdef TX_USER_ENABLE_TRACE
  TX_USER_ENABLE_TRACE;
#endif
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
int main(void)
{
  tx_kernel_enter();

  return 0;
}
