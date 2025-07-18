/*-----------------------------------------------------------------------------------------------------
  Description: LittleFS demo functions for testing file operations

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "littlefs_adapter.h"
#include "RTT_utils.h"

// Global LittleFS context
extern T_littlefs_context g_littlefs_context;

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize and mount LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_init(void)
{
  int err;

  // Initialize LittleFS configuration
  err = Littlefs_initialize();
  if (err != 0)
  {
    RTT_err_printf(0, "LittleFS initialization failed: %d\n\r", err);
    return err;
  }

  // Try to mount the filesystem
  err = Littlefs_mount();
  if (err != 0)
  {
    RTT_printf(0, "Mount failed, trying to format...\n\r");

    // Format the filesystem if mount fails
    err = Littlefs_format();
    if (err != 0)
    {
      RTT_err_printf(0, "LittleFS format failed: %d\n\r", err);
      return err;
    }

    // Try to mount again after format
    err = Littlefs_mount();
    if (err != 0)
    {
      RTT_err_printf(0, "LittleFS mount failed after format: %d\n\r", err);
      return err;
    }
  }

  RTT_printf(0, "LittleFS initialized and mounted successfully\n\r");
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Write test data to a file

  Parameters: filename - name of the file to write
              data - data to write
              size - size of data to write

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_write_file(const char *filename, const void *data, size_t size)
{
  lfs_file_t file;
  int err;

  // Open file for writing
  err = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to open file %s for writing: %d\n\r", filename, err);
    return err;
  }

  // Write data to file
  lfs_ssize_t written = lfs_file_write(&g_littlefs_context.lfs, &file, data, size);
  if (written < 0)
  {
    RTT_err_printf(0, "Failed to write data to file %s: %d\n\r", filename, (int)written);
    lfs_file_close(&g_littlefs_context.lfs, &file);
    return (int)written;
  }

  if ((size_t)written != size)
  {
    RTT_err_printf(0, "Partial write to file %s: %d of %u bytes\n\r", filename, (int)written, size);
  }

  // Close file
  err = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to close file %s: %d\n\r", filename, err);
    return err;
  }

  RTT_printf(0, "Written %d bytes to file %s\n\r", (int)written, filename);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read test data from a file

  Parameters: filename - name of the file to read
              buffer - buffer to store read data
              buffer_size - size of the buffer

  Return: number of bytes read on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_read_file(const char *filename, void *buffer, size_t buffer_size)
{
  lfs_file_t file;
  int err;

  // Open file for reading
  err = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_RDONLY);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to open file %s for reading: %d\n\r", filename, err);
    return err;
  }

  // Read data from file
  lfs_ssize_t read_bytes = lfs_file_read(&g_littlefs_context.lfs, &file, buffer, buffer_size);
  if (read_bytes < 0)
  {
    RTT_err_printf(0, "Failed to read data from file %s: %d\n\r", filename, (int)read_bytes);
    lfs_file_close(&g_littlefs_context.lfs, &file);
    return (int)read_bytes;
  }

  // Close file
  err = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to close file %s: %d\n\r", filename, err);
    return err;
  }

  RTT_printf(0, "Read %d bytes from file %s\n\r", (int)read_bytes, filename);
  return (int)read_bytes;
}

/*-----------------------------------------------------------------------------------------------------
  Description: List files in the root directory

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_list_files(void)
{
  lfs_dir_t dir;
  struct lfs_info info;
  int err;
  int file_count = 0;

  // Open root directory
  err = lfs_dir_open(&g_littlefs_context.lfs, &dir, "/");
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to open root directory: %d\n\r", err);
    return err;
  }

  RTT_printf(0, "Files in root directory:\n\r");

  // Read directory entries
  while (true)
  {
    err = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
    if (err < 0)
    {
      RTT_err_printf(0, "Failed to read directory: %d\n\r", err);
      lfs_dir_close(&g_littlefs_context.lfs, &dir);
      return err;
    }

    // End of directory
    if (err == 0)
    {
      break;
    }

    // Skip "." and ".." entries
    if (info.name[0] == '.')
    {
      continue;
    }

    if (info.type == LFS_TYPE_REG)
    {
      RTT_printf(0, "  File: %s (size: %u bytes)\n\r", info.name, info.size);
    }
    else if (info.type == LFS_TYPE_DIR)
    {
      RTT_printf(0, "  Dir:  %s\n\r", info.name);
    }

    file_count++;
  }

  // Close directory
  err = lfs_dir_close(&g_littlefs_context.lfs, &dir);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to close directory: %d\n\r", err);
    return err;
  }

  RTT_printf(0, "Total entries: %d\n\r", file_count);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Delete a file

  Parameters: filename - name of the file to delete

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_delete_file(const char *filename)
{
  int err;

  err = lfs_remove(&g_littlefs_context.lfs, filename);
  if (err < 0)
  {
    RTT_err_printf(0, "Failed to delete file %s: %d\n\r", filename, err);
    return err;
  }

  RTT_printf(0, "File %s deleted successfully\n\r", filename);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Run comprehensive LittleFS test

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_test(void)
{
  int err;
  const char *test_filename = "test_file.txt";
  const char *test_data = "Hello LittleFS! This is a test file.";
  char read_buffer[100];

  RTT_printf(0, "Starting LittleFS test...\n\r");

  // Initialize filesystem
  err = Littlefs_demo_init();
  if (err != 0)
  {
    return err;
  }

  // List files before test
  RTT_printf(0, "Files before test:\n\r");
  Littlefs_demo_list_files();

  // Write test file
  err = Littlefs_demo_write_file(test_filename, test_data, strlen(test_data));
  if (err != 0)
  {
    return err;
  }

  // Read test file
  memset(read_buffer, 0, sizeof(read_buffer));
  err = Littlefs_demo_read_file(test_filename, read_buffer, sizeof(read_buffer) - 1);
  if (err < 0)
  {
    return err;
  }

  // Verify data
  if (strcmp(test_data, read_buffer) == 0)
  {
    RTT_printf(0, "Data verification SUCCESS!\n\r");
  }
  else
  {
    RTT_err_printf(0, "Data verification FAILED!\n\r");
    RTT_err_printf(0, "Expected: %s\n\r", test_data);
    RTT_err_printf(0, "Read:     %s\n\r", read_buffer);
    return -1;
  }

  // List files after test
  RTT_printf(0, "Files after test:\n\r");
  Littlefs_demo_list_files();

  // Delete test file
  err = Littlefs_demo_delete_file(test_filename);
  if (err != 0)
  {
    return err;
  }

  // List files after delete
  RTT_printf(0, "Files after delete:\n\r");
  Littlefs_demo_list_files();

  RTT_printf(0, "LittleFS test completed successfully!\n\r");
  return 0;
}
