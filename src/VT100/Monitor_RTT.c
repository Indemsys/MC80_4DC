#include "App.h"

// External reference to RTT control block for diagnostics
extern SEGGER_RTT_CB _SEGGER_RTT;

/*-----------------------------------------------------------------------------------------------------
  Monitor for testing SEGGER RTT output functionality

  This module provides VT100 terminal interface for testing RTT output operations:
  - Basic text output testing
  - Performance testing with different message sizes
  - Buffer overflow testing
  - Multiple buffer testing
  - Stress testing with high-frequency output
-----------------------------------------------------------------------------------------------------*/

// Test data sizes for performance testing
#define RTT_TEST_SMALL_SIZE    32
#define RTT_TEST_MEDIUM_SIZE   256
#define RTT_TEST_LARGE_SIZE    1024
#define RTT_TEST_STRESS_COUNT  1000

// Function declarations
void Test_basic_rtt_output(uint8_t keycode);
void Test_rtt_performance(uint8_t keycode);
void Test_rtt_buffer_overflow(uint8_t keycode);
void Test_rtt_multiple_buffers(uint8_t keycode);
void Test_rtt_stress_test(uint8_t keycode);
void Show_rtt_buffer_status(uint8_t keycode);
void Reset_rtt_buffer_menu(uint8_t keycode);
void RTT_simple_test(void);
void RTT_reset_buffer(void);

const T_VT100_Menu_item MENU_RTT_ITEMS[] =
{
  { '1', Test_basic_rtt_output,       0 },
  { '2', Test_rtt_performance,        0 },
  { '3', Test_rtt_buffer_overflow,    0 },
  { '4', Test_rtt_multiple_buffers,   0 },
  { '5', Test_rtt_stress_test,        0 },
  { '6', Show_rtt_buffer_status,      0 },
  { '7', Show_rtt_addresses,          0 },
  { '8', Reset_rtt_buffer_menu,       0 },
  { 'R', 0,                           0 },
  { 0 }
};

const T_VT100_Menu MENU_RTT = {
  "RTT Testing",
  "\033[5C SEGGER RTT testing menu\r\n"
  "\033[5C <1> - Basic RTT output test\r\n"
  "\033[5C <2> - RTT performance test\r\n"
  "\033[5C <3> - RTT buffer overflow test\r\n"
  "\033[5C <4> - Multiple RTT buffers test\r\n"
  "\033[5C <5> - RTT stress test\r\n"
  "\033[5C <6> - Show RTT buffer status\r\n"
  "\033[5C <7> - Show RTT addresses\r\n"
  "\033[5C <8> - Reset RTT buffer\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_RTT_ITEMS,
};

/*-----------------------------------------------------------------------------------------------------
  Basic RTT output testing - sends simple messages to test basic functionality

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Test_basic_rtt_output(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Basic RTT Output Test =====\n\r");
  MPRINTF("Sending test messages to RTT channel 0...\n\r\n\r");

  // Test string output
  SEGGER_RTT_WriteString(0, "RTT Test: Basic string output\n");
  MPRINTF("[OK] String output sent\n\r");

  // Test character output
  for (int i = 0; i < 10; i++)
  {
    SEGGER_RTT_PutChar(0, '0' + i);
  }
  SEGGER_RTT_PutChar(0, '\n');
  MPRINTF("[OK] Character sequence sent (0-9)\n\r");

  // Test binary data output
  unsigned char test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFE, 0xFD};
  SEGGER_RTT_Write(0, test_data, sizeof(test_data));
  MPRINTF("[OK] Binary data sent (%d bytes)\n\r", sizeof(test_data));

  // Test formatted output via printf-like function
  char *formatted_msg = App_malloc(64);
  if (formatted_msg != NULL)
  {
    snprintf(formatted_msg, 64, "RTT Test: Counter value = %d, hex = 0x%08X\n", 12345, 0xDEADBEEF);
    SEGGER_RTT_WriteString(0, formatted_msg);
    MPRINTF("[OK] Formatted message sent\n\r");
    App_free(formatted_msg);
  }
  else
  {
    MPRINTF("[ERROR] Failed to allocate memory for formatted message\n\r");
  }

  MPRINTF("\n\rBasic RTT test completed. Check RTT Viewer for output.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Shows RTT address information for J-Link RTT Viewer configuration

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Show_rtt_addresses(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);

  volatile SEGGER_RTT_CB* pRTTCB = (volatile SEGGER_RTT_CB*)((uintptr_t)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);

  MPRINTF(" ========== RTT Address Information ==========\n\r");
  MPRINTF(" RTT Control Block address: 0x%08X\n\r", (uintptr_t)pRTTCB);
  MPRINTF(" _SEGGER_RTT base address:  0x%08X\n\r", (uintptr_t)&_SEGGER_RTT);
  MPRINTF(" SEGGER_RTT_UNCACHED_OFF:   0x%08X\n\r", SEGGER_RTT_UNCACHED_OFF);

  // Check alignment
  uint32_t rtt_base_addr = (uint32_t)&_SEGGER_RTT;
  uint32_t rtt_cb_addr = (uint32_t)pRTTCB;
  bool is_base_aligned = (rtt_base_addr & 0xFFF) == 0;  // Check 4KB alignment
  bool is_cb_aligned = (rtt_cb_addr & 0xFFF) == 0;      // Check 4KB alignment

  MPRINTF(" ==========================================\n\r");
  MPRINTF(" Alignment Analysis:\n\r");
  MPRINTF(" - Base addr alignment:     %s (0x%03X offset)\n\r",
          is_base_aligned ? "4KB aligned" : "NOT aligned",
          rtt_base_addr & 0xFFF);
  MPRINTF(" - CB addr alignment:       %s (0x%03X offset)\n\r",
          is_cb_aligned ? "4KB aligned" : "NOT aligned",
          rtt_cb_addr & 0xFFF);

  if (!is_cb_aligned)
  {
    uint32_t next_aligned = (rtt_cb_addr & ~0xFFF) + 0x1000;
    uint32_t prev_aligned = rtt_cb_addr & ~0xFFF;
    MPRINTF(" - Next 4KB boundary:       0x%08X (+%d bytes)\n\r",
            next_aligned, next_aligned - rtt_cb_addr);
    MPRINTF(" - Prev 4KB boundary:       0x%08X (-%d bytes)\n\r",
            prev_aligned, rtt_cb_addr - prev_aligned);
    MPRINTF(" WARNING: RTT CB not 4KB aligned!\n\r");
    MPRINTF(" J-Link RTT Viewer may have trouble finding it.\n\r");
  }
  else
  {
    MPRINTF(" OK: RTT CB is properly 4KB aligned for J-Link.\n\r");
  }

  MPRINTF(" Cache line size setting:   %d bytes\n\r", SEGGER_RTT_CPU_CACHE_LINE_SIZE);
  MPRINTF(" ==========================================\n\r\n\r");

  if (pRTTCB->MaxNumUpBuffers > 0)
  {
    MPRINTF(" Up buffer 0 address:       0x%08X\n\r", (uintptr_t)pRTTCB->aUp[0].pBuffer);
    MPRINTF(" Up buffer 0 size:          %u bytes\n\r", pRTTCB->aUp[0].SizeOfBuffer);
    MPRINTF(" Up buffer 0 name:          \"%s\"\n\r", pRTTCB->aUp[0].sName ? pRTTCB->aUp[0].sName : "NULL");
    MPRINTF("\n\r");
  }

  if (pRTTCB->MaxNumDownBuffers > 0)
  {
    MPRINTF(" Down buffer 0 address:     0x%08X\n\r", (uintptr_t)pRTTCB->aDown[0].pBuffer);
    MPRINTF(" Down buffer 0 size:        %u bytes\n\r", pRTTCB->aDown[0].SizeOfBuffer);
    MPRINTF(" Down buffer 0 name:        \"%s\"\n\r", pRTTCB->aDown[0].sName ? pRTTCB->aDown[0].sName : "NULL");
    MPRINTF("\n\r");
  }

  MPRINTF(" RTT CB ID string:          \"%.16s\"\n\r", pRTTCB->acID);
  MPRINTF(" Max up buffers:            %u\n\r", pRTTCB->MaxNumUpBuffers);
  MPRINTF(" Max down buffers:          %u\n\r", pRTTCB->MaxNumDownBuffers);
  MPRINTF("\n\r");
  MPRINTF(" ==========================================\n\r");
  MPRINTF(" Configure J-Link RTT Viewer:\n\r");
  MPRINTF(" - Search address: 0x%08X\n\r", (uintptr_t)pRTTCB);
  MPRINTF(" - Search size:    0x1000 (4096 bytes)\n\r");
  MPRINTF(" - Or use auto-detection with above address\n\r");
  MPRINTF(" ==========================================\n\r");

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  RTT performance testing - measures output speed with different data sizes

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Test_rtt_performance(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== RTT Performance Test =====\n\r");
  MPRINTF("Testing RTT output performance with different data sizes...\n\r\n\r");

  // Allocate test buffers dynamically
  char *small_buffer = App_malloc(RTT_TEST_SMALL_SIZE);
  char *medium_buffer = App_malloc(RTT_TEST_MEDIUM_SIZE);
  char *large_buffer = App_malloc(RTT_TEST_LARGE_SIZE);

  if (small_buffer == NULL || medium_buffer == NULL || large_buffer == NULL)
  {
    MPRINTF("[ERROR] Failed to allocate memory for test buffers\n\r");
    if (small_buffer) App_free(small_buffer);
    if (medium_buffer) App_free(medium_buffer);
    if (large_buffer) App_free(large_buffer);

    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Fill buffers with test data
  for (int i = 0; i < RTT_TEST_SMALL_SIZE; i++)
  {
    small_buffer[i] = 'A' + (i % 26);
  }
  for (int i = 0; i < RTT_TEST_MEDIUM_SIZE; i++)
  {
    medium_buffer[i] = 'A' + (i % 26);
  }
  for (int i = 0; i < RTT_TEST_LARGE_SIZE; i++)
  {
    large_buffer[i] = 'A' + (i % 26);
  }

  // Test small buffer performance
  MPRINTF("Testing %d byte transfers...\n\r", RTT_TEST_SMALL_SIZE);
  unsigned long start_time = tx_time_get();
  for (int i = 0; i < 100; i++)
  {
    SEGGER_RTT_Write(0, small_buffer, RTT_TEST_SMALL_SIZE);
  }
  unsigned long small_time = tx_time_get() - start_time;
  MPRINTF("[OK] Small buffer test: %lu ticks for 100 transfers\n\r", small_time);

  // Test medium buffer performance
  MPRINTF("Testing %d byte transfers...\n\r", RTT_TEST_MEDIUM_SIZE);
  start_time = tx_time_get();
  for (int i = 0; i < 50; i++)
  {
    SEGGER_RTT_Write(0, medium_buffer, RTT_TEST_MEDIUM_SIZE);
  }
  unsigned long medium_time = tx_time_get() - start_time;
  MPRINTF("[OK] Medium buffer test: %lu ticks for 50 transfers\n\r", medium_time);

  // Test large buffer performance
  MPRINTF("Testing %d byte transfers...\n\r", RTT_TEST_LARGE_SIZE);
  start_time = tx_time_get();
  for (int i = 0; i < 10; i++)
  {
    SEGGER_RTT_Write(0, large_buffer, RTT_TEST_LARGE_SIZE);
  }
  unsigned long large_time = tx_time_get() - start_time;
  MPRINTF("[OK] Large buffer test: %lu ticks for 10 transfers\n\r", large_time);

  // Free allocated memory
  App_free(small_buffer);
  App_free(medium_buffer);
  App_free(large_buffer);

  MPRINTF("\n\rPerformance test completed.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  RTT buffer overflow testing - tests behavior when buffer is full

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Test_rtt_buffer_overflow(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== RTT Buffer Overflow Test =====\n\r");
  MPRINTF("Testing RTT behavior when buffer is full...\n\r\n\r");

  // Allocate large test data dynamically to fill buffer quickly
  char *overflow_data = App_malloc(512);
  if (overflow_data == NULL)
  {
    MPRINTF("[ERROR] Failed to allocate memory for overflow test\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  for (int i = 0; i < 512; i++)
  {
    overflow_data[i] = 'X';
  }

  MPRINTF("Sending large amounts of data to fill RTT buffer...\n\r");

  // Send data rapidly to test overflow behavior
  for (int i = 0; i < 20; i++)
  {
    unsigned bytes_written = SEGGER_RTT_Write(0, overflow_data, 512);
    if (bytes_written < 512)
    {
      MPRINTF("Buffer overflow detected at iteration %d: %u/%u bytes written\n\r",
              i, bytes_written, 512);
      break;
    }

    // Brief delay to allow J-Link to read some data
    if (i % 5 == 0)
    {
      tx_thread_sleep(1);
    }
  }

  // Test skip mode behavior
  MPRINTF("\nTesting SKIP mode behavior...\n\r");
  unsigned char test_char = '!';
  for (int i = 0; i < 100; i++)
  {
    unsigned result = SEGGER_RTT_PutCharSkip(0, test_char);
    if (result == 0)
    {
      MPRINTF("Character skipped at iteration %d\n\r", i);
      break;
    }
  }

  // Free allocated memory
  App_free(overflow_data);

  MPRINTF("\n\rBuffer overflow test completed.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Multiple RTT buffers testing - tests usage of different RTT channels

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Test_rtt_multiple_buffers(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Multiple RTT Buffers Test =====\n\r");
  MPRINTF("Testing multiple RTT channels...\n\r\n\r");

  // Configure additional RTT buffers if not already done
  // Note: This requires buffers to be pre-configured in SEGGER_RTT_Conf.h

  // Test channel 0 (Terminal)
  SEGGER_RTT_WriteString(0, "Channel 0: Terminal output\n");
  MPRINTF("[OK] Data sent to channel 0 (Terminal)\n\r");

  // Check if RTT control block is accessible safely
  volatile SEGGER_RTT_CB* pRTTCB = (volatile SEGGER_RTT_CB*)((uintptr_t)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);

  // Show available buffers information
  MPRINTF("RTT Buffer Configuration:\n\r");
  MPRINTF("- Max up buffers: %u\n\r", pRTTCB->MaxNumUpBuffers);
  MPRINTF("- Max down buffers: %u\n\r", pRTTCB->MaxNumDownBuffers);

  // Try to configure channel 1 if it doesn't exist
  if (pRTTCB->MaxNumUpBuffers > 1 && pRTTCB->aUp[1].pBuffer == NULL)
  {
    MPRINTF("- Channel 1 not configured, trying to initialize...\n\r");

    // Allocate buffer for channel 1
    char *channel1_buffer = App_malloc(256);
    if (channel1_buffer != NULL)
    {
      // Configure channel 1 manually
      SEGGER_RTT_ConfigUpBuffer(1, "Debug", channel1_buffer, 256, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
      MPRINTF("- Channel 1 initialized with 256 byte buffer\n\r");
    }
    else
    {
      MPRINTF("- Failed to allocate buffer for channel 1\n\r");
    }
  }

  // Check maximum number of up buffers before trying to use channel 1
  if (pRTTCB->MaxNumUpBuffers > 1 && pRTTCB->aUp[1].pBuffer != NULL)
  {
    // Test channel 1 (if available)
    unsigned bytes_written = SEGGER_RTT_Write(1, "Channel 1: Debug output\n", 24);
    if (bytes_written > 0)
    {
      MPRINTF("[OK] Data sent to channel 1 (Debug) - %u bytes\n\r", bytes_written);
    }
    else
    {
      MPRINTF("[WARN] Channel 1 write failed (buffer full or not configured)\n\r");
    }
  }
  else
  {
    MPRINTF("[INFO] Channel 1 not available (MaxNumUpBuffers = %u)\n\r", pRTTCB->MaxNumUpBuffers);
  }

  // Allocate memory for channel messages
  char *channel0_msg = App_malloc(32);
  char *channel1_msg = App_malloc(32);

  if (channel0_msg != NULL && channel1_msg != NULL)
  {
    // Use safe string copying with known lengths
    const char* msg0 = "CH0: Application messages\n";
    const char* msg1 = "CH1: Debug information\n";

    // Copy messages safely
    strncpy(channel0_msg, msg0, 31);
    channel0_msg[31] = '\0';  // Ensure null termination
    strncpy(channel1_msg, msg1, 31);
    channel1_msg[31] = '\0';  // Ensure null termination

    for (int i = 0; i < 5; i++)
    {
      // Send to channel 0 (always available)
      SEGGER_RTT_Write(0, channel0_msg, 26);  // Known length

      // Send to channel 1 only if it exists and is configured
      if (pRTTCB->MaxNumUpBuffers > 1 && pRTTCB->aUp[1].pBuffer != NULL)
      {
        SEGGER_RTT_Write(1, channel1_msg, 23);  // Known length
      }

      tx_thread_sleep(1); // Small delay
    }

    MPRINTF("[OK] Multiple messages sent to available channels\n\r");
  }
  else
  {
    MPRINTF("[ERROR] Failed to allocate memory for channel messages\n\r");
  }

  // Free allocated memory
  if (channel0_msg) App_free(channel0_msg);
  if (channel1_msg) App_free(channel1_msg);

  // NOTE: We don't free the channel 1 buffer here because RTT may still be using it
  // In a real application, you would manage RTT channel buffers more carefully

  MPRINTF("\n\rMultiple buffers test completed.\n\r");
  MPRINTF("IMPORTANT: Channel 1 buffer remains allocated for RTT use.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  RTT stress testing - high-frequency output to test system stability

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Test_rtt_stress_test(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== RTT Stress Test =====\n\r");
  MPRINTF("Running high-frequency RTT output test...\n\r");
  MPRINTF("WARNING: This will generate a lot of RTT traffic!\n\r\n\r");

  MPRINTF("Press 'Y' to continue or any other key to cancel: ");
  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Stress test cancelled.\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("\n\rStarting stress test...\n\r");

  // Allocate buffer for stress messages
  char *stress_msg = App_malloc(64);
  if (stress_msg == NULL)
  {
    MPRINTF("[ERROR] Failed to allocate memory for stress test\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  unsigned long start_time = tx_time_get();

  for (int i = 0; i < RTT_TEST_STRESS_COUNT; i++)
  {
    snprintf(stress_msg, 64, "Stress test message #%04d - %08X\n", i, i * 0x12345678);
    SEGGER_RTT_WriteString(0, stress_msg);

    // Show progress every 100 messages
    if (i % 100 == 0)
    {
      MPRINTF("Progress: %d/%d messages sent\n\r", i, RTT_TEST_STRESS_COUNT);
    }
  }

  unsigned long total_time = tx_time_get() - start_time;
  MPRINTF("\n\rStress test completed!\n\r");
  MPRINTF("Sent %d messages in %lu ticks\n\r", RTT_TEST_STRESS_COUNT, total_time);
  if (total_time > 0)
  {
    MPRINTF("Average: %lu messages per second\n\r",
            (RTT_TEST_STRESS_COUNT * TX_TIMER_TICKS_PER_SECOND) / total_time);
  }

  // Free allocated memory
  App_free(stress_msg);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Show RTT buffer status and statistics

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Show_rtt_buffer_status(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== RTT Buffer Status =====\n\r");
  MPRINTF("RTT buffer information:\n\r\n\r");

  // Note: SEGGER RTT doesn't provide direct access to buffer status
  // This is a basic implementation showing what we can determine

  MPRINTF("RTT Configuration:\n\r");
  MPRINTF("- Buffer 0 (Terminal): Available\n\r");
  MPRINTF("- Maximum buffers: %d up, %d down\n\r",
          SEGGER_RTT_MAX_NUM_UP_BUFFERS, SEGGER_RTT_MAX_NUM_DOWN_BUFFERS);
  MPRINTF("- Buffer sizes: %d up, %d down\n\r", BUFFER_SIZE_UP, BUFFER_SIZE_DOWN);

  // Test if we can write to see buffer availability
  unsigned char test_byte = 0x55;
  unsigned result = SEGGER_RTT_PutCharSkip(0, test_byte);
  MPRINTF("- Buffer 0 write test: %s\n\r", result ? "PASS" : "FULL/FAIL");

  // Test reading capabilities - allocate buffer dynamically
  char *read_buffer = App_malloc(16);
  if (read_buffer != NULL)
  {
    unsigned read_bytes = SEGGER_RTT_Read(0, read_buffer, 16);
    MPRINTF("- Buffer 0 read test: %u bytes available\n\r", read_bytes);
    App_free(read_buffer);
  }
  else
  {
    MPRINTF("- Buffer 0 read test: [ERROR] Failed to allocate memory\n\r");
  }

  MPRINTF("\n\rRTT Status Information:\n\r");
  MPRINTF("- RTT Version: 8.12c (from source)\n\r");
  MPRINTF("- Buffer mode: Based on SEGGER_RTT_MODE_DEFAULT\n\r");
  MPRINTF("- Thread safety: Depends on SEGGER_RTT_LOCK configuration\n\r");

  // Test basic RTT output right now
  MPRINTF("\n\rTesting RTT output now...\n\r");
  SEGGER_RTT_WriteString(0, "RTT Status Check: This message should appear in RTT Viewer\n");
  MPRINTF("[OK] RTT test message sent\n\r");

  // Show RTT addresses for J-Link configuration
  volatile SEGGER_RTT_CB* pRTTCB = (volatile SEGGER_RTT_CB*)((uintptr_t)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);
  MPRINTF("\n\rRTT Address Information:\n\r");
  MPRINTF("- RTT Control Block address: 0x%08X\n\r", (uintptr_t)pRTTCB);
  MPRINTF("- _SEGGER_RTT base address: 0x%08X\n\r", (uintptr_t)&_SEGGER_RTT);
  MPRINTF("- SEGGER_RTT_UNCACHED_OFF: 0x%08X\n\r", SEGGER_RTT_UNCACHED_OFF);
  MPRINTF("- Up buffer 0 address: 0x%08X\n\r", (uintptr_t)pRTTCB->aUp[0].pBuffer);
  MPRINTF("- Down buffer 0 address: 0x%08X\n\r", (uintptr_t)pRTTCB->aDown[0].pBuffer);
  MPRINTF("- RTT CB ID: %.12s\n\r", pRTTCB->acID);

  MPRINTF("\n\rFor detailed buffer statistics, use J-Link RTT Viewer\n\r");
  MPRINTF("or J-Link Commander with RTT commands.\n\r");
  MPRINTF("Configure J-Link RTT Viewer to search at address: 0x%08X\n\r", (uintptr_t)pRTTCB);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Simple RTT test function - can be called from any part of the code for debugging

  Parameters:
    None

  Return:
-----------------------------------------------------------------------------------------------------*/
void RTT_simple_test(void)
{
  static uint32_t test_counter = 0;
  volatile SEGGER_RTT_CB* pRTTCB;
  volatile SEGGER_RTT_BUFFER_UP* pRing;
  unsigned avail_space;

  test_counter++;

  // Get RTT control block and buffer 0 pointer
  pRTTCB = (volatile SEGGER_RTT_CB*)((uintptr_t)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);
  pRing = &pRTTCB->aUp[0];

  // Calculate available space manually
  unsigned RdOff = pRing->RdOff;
  unsigned WrOff = pRing->WrOff;
  if (RdOff <= WrOff)
  {
    avail_space = pRing->SizeOfBuffer - 1u - WrOff + RdOff;
  }
  else
  {
    avail_space = RdOff - WrOff - 1u;
  }

  // Test basic output
  SEGGER_RTT_WriteString(0, "=== RTT Simple Test ===\n");
  SEGGER_RTT_printf(0, "Test counter: %lu\n", test_counter);
  SEGGER_RTT_printf(0, "System tick: %lu\n", tx_time_get());
  SEGGER_RTT_printf(0, "RTT CB address: 0x%08X\n", (uintptr_t)pRTTCB);
  SEGGER_RTT_printf(0, "_SEGGER_RTT address: 0x%08X\n", (uintptr_t)&_SEGGER_RTT);
  SEGGER_RTT_printf(0, "UNCACHED_OFF: 0x%08X\n", SEGGER_RTT_UNCACHED_OFF);
  SEGGER_RTT_printf(0, "Buffer state: WrOff=%u, RdOff=%u, Size=%u\n", WrOff, RdOff, pRing->SizeOfBuffer);
  SEGGER_RTT_printf(0, "Available space: %u bytes\n", avail_space);
  SEGGER_RTT_printf(0, "Buffer flags: 0x%X\n", pRing->Flags);
  SEGGER_RTT_printf(0, "RTT CB ID: %.12s\n", pRTTCB->acID);
  SEGGER_RTT_printf(0, "Up buffer 0 pointer: 0x%08X\n", (uintptr_t)pRing->pBuffer);
  SEGGER_RTT_WriteString(0, "RTT diagnostic complete!\n\n");
}/*-----------------------------------------------------------------------------------------------------
  Reset RTT buffer to clear stuck state

  Parameters:
    None

  Return:
-----------------------------------------------------------------------------------------------------*/
void RTT_reset_buffer(void)
{
  volatile SEGGER_RTT_CB* pRTTCB;
  volatile SEGGER_RTT_BUFFER_UP* pRing;

  // Get RTT control block and buffer 0 pointer
  pRTTCB = (volatile SEGGER_RTT_CB*)((uintptr_t)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);
  pRing = &pRTTCB->aUp[0];

  // Reset buffer pointers to clear any stuck state
  SEGGER_RTT_LOCK();
  pRing->WrOff = 0;
  pRing->RdOff = 0;
  SEGGER_RTT_UNLOCK();

  SEGGER_RTT_WriteString(0, "RTT buffer reset complete\n");
}

/*-----------------------------------------------------------------------------------------------------
  Reset RTT buffer menu function

  Parameters:
    keycode: Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void Reset_rtt_buffer_menu(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Reset RTT Buffer =====\n\r");
  MPRINTF("This will reset RTT buffer pointers to clear stuck state.\n\r\n\r");

  // Reset the buffer
  RTT_reset_buffer();

  MPRINTF("[OK] RTT buffer has been reset\n\r");
  MPRINTF("Buffer pointers (WrOff, RdOff) set to 0\n\r");

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}
