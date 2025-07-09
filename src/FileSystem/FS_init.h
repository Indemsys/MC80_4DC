#ifndef FS_INIT_H
  #define FS_INIT_H

#define WINDOWS_DIR                "System Volume Information"

#define G_FX_MEDIA_MEDIA_MEMORY_SIZE (1024*10)

extern FX_MEDIA                    fat_fs_media;
extern uint8_t                     fs_memory[];


uint32_t                    Init_SD_card_file_system(void);
uint32_t                    Delete_SD_card_file_system(void);

#endif // FS_INIT_H
