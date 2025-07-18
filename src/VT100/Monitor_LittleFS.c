#include "App.h"
#include "LittleFS/littlefs_demo.h"
#include "LittleFS/littlefs_adapter.h"

#define LFS_TEST_FILE_SIZE 256

static uint8_t g_lfs_test_buffer[LFS_TEST_FILE_SIZE];

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_init(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Initialization ===\n\r");

  // Initialize LittleFS adapter
  int result = Littlefs_demo_init();

  if (result == 0)
  {
    MPRINTF("LittleFS initialized successfully\n\r");
  }
  else
  {
    MPRINTF("LittleFS initialization failed: %d\n\r", result);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_format(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Comprehensive Test ===\n\r");

  MPRINTF("Running comprehensive filesystem test...\n\r");

  int result = Littlefs_demo_test();

  if (result == 0)
  {
    MPRINTF("All tests passed successfully!\n\r");
  }
  else
  {
    MPRINTF("Test failed with error: %d\n\r", result);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_info(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS File List ===\n\r");

  int result = Littlefs_demo_list_files();

  if (result != 0)
  {
    MPRINTF("Failed to list files: %d\n\r", result);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_test_file_ops(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== File Operations Test ===\n\r");

  // Fill test buffer with pattern
  for (uint32_t i = 0; i < LFS_TEST_FILE_SIZE; i++)
  {
    g_lfs_test_buffer[i] = (uint8_t)(i & 0xFF);
  }

  MPRINTF("Testing file write...\n\r");
  int result = Littlefs_demo_write_file("/test.bin", g_lfs_test_buffer, LFS_TEST_FILE_SIZE);

  if (result == 0)
  {
    MPRINTF("File write successful\n\r");

    // Clear buffer and read back
    memset(g_lfs_test_buffer, 0, LFS_TEST_FILE_SIZE);

    MPRINTF("Testing file read...\n\r");
    int bytes_read = Littlefs_demo_read_file("/test.bin", g_lfs_test_buffer, LFS_TEST_FILE_SIZE);

    if (bytes_read > 0)
    {
      MPRINTF("File read successful, %d bytes read\n\r", bytes_read);

      // Verify data integrity
      bool data_ok = true;
      for (int i = 0; i < bytes_read; i++)
      {
        if (g_lfs_test_buffer[i] != (uint8_t)(i & 0xFF))
        {
          data_ok = false;
          break;
        }
      }

      if (data_ok)
      {
        MPRINTF("Data integrity check: PASSED\n\r");
      }
      else
      {
        MPRINTF("Data integrity check: FAILED\n\r");
      }

      // Test file deletion
      MPRINTF("Testing file deletion...\n\r");
      result = Littlefs_demo_delete_file("/test.bin");

      if (result == 0)
      {
        MPRINTF("File deletion successful\n\r");
      }
      else
      {
        MPRINTF("File deletion failed: %d\n\r", result);
      }
    }
    else
    {
      MPRINTF("File read failed: %d\n\r", bytes_read);
    }
  }
  else
  {
    MPRINTF("File write failed: %d\n\r", result);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_dir_ops(uint8_t keycode)
{
  Do_LittleFS_info(keycode);  // Reuse the file listing function
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_benchmark(uint8_t keycode)
{
  Do_LittleFS_format(keycode);  // Reuse the comprehensive test function
}

/*-----------------------------------------------------------------------------------------------------

  \param keycode

  \return none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_mount_test(uint8_t keycode)
{
  Do_LittleFS_init(keycode);  // Reuse the initialization function
}

const T_VT100_Menu_item MENU_LittleFS_items[] = {
  { '1', Do_LittleFS_init,         NULL },
  { '2', Do_LittleFS_format,       NULL },
  { '3', Do_LittleFS_info,         NULL },
  { '4', Do_LittleFS_test_file_ops, NULL },
  { 'R', NULL,                     NULL },
  { 0 } // End of menu
};

const T_VT100_Menu MENU_LittleFS = {
  "LittleFS Testing",
  "\033[5C LittleFS file system testing menu\r\n"
  "\033[5C <1> - Initialize LittleFS\r\n"
  "\033[5C <2> - Run comprehensive test\r\n"
  "\033[5C <3> - List files\r\n"
  "\033[5C <4> - Test file operations\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_LittleFS_items
};
