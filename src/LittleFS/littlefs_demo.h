#ifndef LITTLEFS_DEMO_H
#define LITTLEFS_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize and mount LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_init(void);

/*-----------------------------------------------------------------------------------------------------
  Description: Write test data to a file

  Parameters: filename - name of the file to write
              data - data to write
              size - size of data to write

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_write_file(const char *filename, const void *data, size_t size);

/*-----------------------------------------------------------------------------------------------------
  Description: Read test data from a file

  Parameters: filename - name of the file to read
              buffer - buffer to store read data
              buffer_size - size of the buffer

  Return: number of bytes read on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_read_file(const char *filename, void *buffer, size_t buffer_size);

/*-----------------------------------------------------------------------------------------------------
  Description: List files in the root directory

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_list_files(void);

/*-----------------------------------------------------------------------------------------------------
  Description: Delete a file

  Parameters: filename - name of the file to delete

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_delete_file(const char *filename);

/*-----------------------------------------------------------------------------------------------------
  Description: Run comprehensive LittleFS test

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_test(void);

#ifdef __cplusplus
}
#endif

#endif // LITTLEFS_DEMO_H
