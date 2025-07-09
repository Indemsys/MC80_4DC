/* generated configuration header file - do not edit */
#ifndef FX_USER_H_
#define FX_USER_H_
#ifdef __cplusplus
extern "C"
{
#endif

#if (256 + 0)
  #define FX_MAX_LONG_NAME_LEN (256)
#endif
#if (256 + 0)
  #define FX_MAX_LAST_NAME_LEN (256)
#endif
#if (2048 + 0)
  #define FX_MAX_SECTOR_CACHE (2048)
#endif
#if (2048 + 0)
  #define FX_FAT_MAP_SIZE (2048)
#endif
#if (32 + 0)
  #define FX_MAX_FAT_CACHE (32)
#endif
#if (10 + 0)
  #define FX_UPDATE_RATE_IN_SECONDS (10)
  #define FX_UPDATE_RATE_IN_TICKS   (FX_UPDATE_RATE_IN_SECONDS * TX_TIMER_TICKS_PER_SECOND)
#endif
#if (+0)
  #define FX_FAULT_TOLERANT_BOOT_INDEX ()
#endif

#define FX_FAULT_TOLERANT

#define FX_ENABLE_FAULT_TOLERANT

#define FX_ENABLE_EXFAT

#ifdef __cplusplus
}
#endif
#endif /* FX_USER_H_ */
