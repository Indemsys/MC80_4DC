// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.08.29
// 18:42:07
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

uint8_t  fs_memory[G_FX_MEDIA_MEDIA_MEMORY_SIZE];
FX_MEDIA fat_fs_media;
uint8_t  g_file_system_ready;
/*-----------------------------------------------------------------------------------------------------
  RM FileX SDMMC block media callback

  Parameters:
    p_args - callback arguments

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void g_rm_filex_sdmmc_block_media_callback(rm_filex_block_media_callback_args_t *p_args)
{
}
/*-----------------------------------------------------------------------------------------------------
  Prints SD card information to debug buffer

  Parameters:
    None

  Return:
    uint32_t - Returns RES_OK on success, RES_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
uint32_t Print_SD_Card_Info(void)
{
  FX_MEDIA *p_media = &fat_fs_media;
  ULONG64   available_bytes;
  ULONG64   total_bytes;
  ULONG64   used_bytes;
  ULONG64   total_mb;
  ULONG64   used_mb;
  ULONG64   available_mb;

  if (p_media->fx_media_id != FX_MEDIA_ID)
  {
    APPLOG("FS: Media is not open or invalid.");
    return RES_ERROR;
  }

  // Calculate total size of the SD card
  total_bytes    = (ULONG64)p_media->fx_media_total_sectors * p_media->fx_media_bytes_per_sector;

  // Get available space using extended function for large volumes
  UINT space_res = fx_media_extended_space_available(p_media, &available_bytes);
  if (space_res != FX_SUCCESS)
  {
    APPLOG("FS: Error getting available space: %d", space_res);
    return RES_ERROR;
  }
  used_bytes   = total_bytes - available_bytes;

  // Convert to megabytes
  total_mb     = total_bytes / (1024 * 1024);
  used_mb      = used_bytes / (1024 * 1024);
  available_mb = available_bytes / (1024 * 1024);

  APPLOG("FS: SD Card Information:");
  APPLOG("FS:   Total Card Size: %llu MB", total_mb);
  APPLOG("FS:   Used space: %llu MB", used_mb);
  APPLOG("FS:   Available space: %llu MB", available_mb);
  APPLOG("FS:   Bytes per sector: %lu", p_media->fx_media_bytes_per_sector);
  APPLOG("FS:   Sectors per cluster: %lu", p_media->fx_media_sectors_per_cluster);
  APPLOG("FS:   Total sectors: %llu", p_media->fx_media_total_sectors);
  APPLOG("FS:   Total clusters: %lu", p_media->fx_media_total_clusters);
  APPLOG("FS:   Hidden sectors: %lu", p_media->fx_media_hidden_sectors);
  APPLOG("FS:   Root dir. entries: %lu", p_media->fx_media_root_directory_entries);
  APPLOG("FS:   Sectors per FAT: %lu", p_media->fx_media_sectors_per_FAT);
  APPLOG("FS:   Number of FATs: %lu", p_media->fx_media_number_of_FATs);
  APPLOG("FS:   Available clusters: %lu", p_media->fx_media_available_clusters);
  if (p_media->fx_media_12_bit_FAT)
  {
    APPLOG("FS:   FAT type: FAT12");
  }
  else if (p_media->fx_media_32_bit_FAT)
  {
    APPLOG("FS:   FAT type: FAT32");
  }
  else if (p_media->fx_media_FAT_type == FX_exFAT)
  {
    APPLOG("FS:   FAT type: exFAT");
  }
  else
  {
    APPLOG("FS:   FAT type: FAT16");
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Removes Windows system directory "System Volume Information" from SD card.
  If the directory exists, it switches to it, deletes all files and subdirectories,
  then returns to root directory and deletes the directory itself.

  Parameters:
    None

  Return:
    uint32_t - Returns RES_OK on success, RES_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
uint32_t Delete_Windows_System_Directory(void)
{
  UINT res;
  // Delete Windows system directory
  if (fx_directory_name_test(&fat_fs_media, WINDOWS_DIR) == FX_SUCCESS)
  {
    res = fx_directory_default_set(&fat_fs_media, WINDOWS_DIR);
    if (res != FX_SUCCESS)
    {
      APPLOG("FS: Set def dir '%s' err: %d", WINDOWS_DIR, res);
      return RES_ERROR;
    }
    APPLOG("FS: Set def dir '%s' OK", WINDOWS_DIR);

    if (Delete_all_files_in_current_dir() != RES_OK)
    {
      APPLOG("FS: Del files in '%s' err", WINDOWS_DIR);
      return RES_ERROR;
    }
    APPLOG("FS: Del files in '%s' OK", WINDOWS_DIR);

    res = fx_directory_default_set(&fat_fs_media, "/");
    if (res != FX_SUCCESS)
    {
      APPLOG("FS: Set def dir '/' err: %d", res);
      return RES_ERROR;
    }
    APPLOG("FS: Set def dir '/' OK");

    res = fx_directory_delete(&fat_fs_media, WINDOWS_DIR);
    if (res != FX_SUCCESS)
    {
      APPLOG("FS: Del dir '%s' err: %d", WINDOWS_DIR, res);
      return RES_ERROR;
    }
    APPLOG("FS: Del dir '%s' OK", WINDOWS_DIR);
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize SD card file system

  Parameters:
    None

  Return:
    uint32_t - Returns RES_OK on success, RES_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
uint32_t Init_SD_card_file_system(void)
{
  rtc_time_t rt_time = {0};
  UINT       fx_res;

  g_file_system_ready   = 0;

  fsp_err_t rm_open_res = RM_FILEX_BLOCK_MEDIA_Open(&g_rm_filex_sdmmc_block_media_ctrl, &g_rm_filex_sdmmc_block_media_cfg);
  if (rm_open_res != FSP_SUCCESS)
  {
    APPLOG("FS: Open block media err: %d", rm_open_res);
    return RES_ERROR;
  }
  APPLOG("FS: Open block media OK");
  fx_system_initialize();

  fsp_err_t rtc_get_res = RTC_get_system_DateTime(&rt_time);
  if (rtc_get_res != FSP_SUCCESS)
  {
    APPLOG("FS: Get RTC time err: %d. Using compilation date.", rtc_get_res);
    Parse_compile_date_time(&rt_time);
    rt_time.tm_year = rt_time.tm_year + 1900;
    APPLOG("FS: Set compilation date: %04d-%02d-%02d %02d:%02d:%02d", rt_time.tm_year, rt_time.tm_mon + 1, rt_time.tm_mday, rt_time.tm_hour, rt_time.tm_min, rt_time.tm_sec);
  }

  UINT fx_date_res = fx_system_date_set(rt_time.tm_year, rt_time.tm_mon, rt_time.tm_mday);
  if (fx_date_res != FX_SUCCESS)
  {
    APPLOG("FS: Set sys date err: %d", fx_date_res);
    return RES_ERROR;
  }

  UINT fx_time_res = fx_system_time_set(rt_time.tm_hour, rt_time.tm_min, rt_time.tm_sec);
  if (fx_time_res != FX_SUCCESS)
  {
    APPLOG("FS: Set sys time err: %d", fx_time_res);
    return RES_ERROR;
  }
  APPLOG("FS: Set sys date-time OK");

  fx_res = fx_media_open(&fat_fs_media, (CHAR *)"C:", RM_FILEX_BLOCK_MEDIA_BlockDriver, (void *)&g_rm_filex_sdmmc_block_media_instance, fs_memory, G_FX_MEDIA_MEDIA_MEMORY_SIZE);
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Open media err: %d", fx_res);
    return RES_ERROR;
  }
  APPLOG("FS: Open media OK");
  // Set default directory path
  fx_res = fx_directory_default_set(&fat_fs_media, "/");
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Set def dir '/' err: %d", fx_res);
    return RES_ERROR;
  }
  APPLOG("FS: Set def dir '/' OK");

  // Remove Windows system directory
  if (Delete_Windows_System_Directory() == RES_OK)
  {
    APPLOG("FS: Del Win dir OK");
  }

  fx_res = fx_directory_default_set(&fat_fs_media, "/");
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Set def dir '/' err: %d", fx_res);
    return RES_ERROR;
  }

  fx_res = fx_media_flush(&fat_fs_media);
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Flush media err: %d", fx_res);
    return RES_ERROR;
  }

  // Print SD card information
  Print_SD_Card_Info();

  g_file_system_ready = 1;

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Delete SD card file system

  Parameters:
    None

  Return:
    uint32_t - Returns RES_OK
-----------------------------------------------------------------------------------------------------*/
uint32_t Delete_SD_card_file_system(void)
{
  APPLOG("FS: Start Del SD card");
  UINT fx_res = fx_media_flush(&fat_fs_media);
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Flush media err: %d", fx_res);
  }
  else
  {
    APPLOG("FS: Flush media OK");
  }

  fx_res = fx_media_close(&fat_fs_media);
  if (fx_res != FX_SUCCESS)
  {
    APPLOG("FS: Close media err: %d", fx_res);
  }
  else
  {
    APPLOG("FS: Close media OK");
  }
  APPLOG("FS: End Del SD card");
  return RES_OK;
}
