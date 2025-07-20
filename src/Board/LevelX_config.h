#ifndef LEVELX_CONFIG_H
#define LEVELX_CONFIG_H

#define G_FX_MEDIA_OSPI_NOR_MEDIA_MEMORY_SIZE (512)
#define G_FX_MEDIA_OSPI_NOR_VOLUME_NAME ("Volume 1")
#define G_FX_MEDIA_OSPI_NOR_NUMBER_OF_FATS (1)
#define G_FX_MEDIA_OSPI_NOR_DIRECTORY_ENTRIES (256)
#define G_FX_MEDIA_OSPI_NOR_HIDDEN_SECTORS (0)
#define G_FX_MEDIA_OSPI_NOR_TOTAL_SECTORS (57337)
#define G_FX_MEDIA_OSPI_NOR_BYTES_PER_SECTOR (512)
#define G_FX_MEDIA_OSPI_NOR_SECTORS_PER_CLUSTER (1)
#define G_FX_MEDIA_OSPI_NOR_VOLUME_SERIAL_NUMBER (12345)
#define G_FX_MEDIA_OSPI_NOR_BOUNDARY_UNIT (128)

// Forward declarations for callback functions
void rm_filex_levelx_nor_spi_callback(rm_levelx_nor_spi_callback_args_t * p_args);
void g_rm_filex_levelx_NOR_callback(rm_filex_levelx_nor_callback_args_t * p_args);

// External declarations for LevelX NOR OSPI configuration structures

// LevelX NOR SPI instance control structure
extern rm_levelx_nor_spi_instance_ctrl_t g_rm_levelx_nor_OSPI_ctrl;

// LevelX NOR SPI configuration
extern rm_levelx_nor_spi_cfg_t g_rm_levelx_nor_OSPI_cfg;

// LevelX NOR flash instance
extern LX_NOR_FLASH g_lx_NOR;

// FileX LevelX NOR instance control structure
extern rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;

// FileX LevelX NOR configuration
extern const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg;

// FileX LevelX NOR instance
extern const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance;

// LevelX NOR read buffer (if LX_DIRECT_READ is disabled)
#ifndef LX_DIRECT_READ
#define FSP_LX_READ_BUFFER_SIZE_WORDS (128U)
extern ULONG g_rm_levelx_nor_OSPI_read_buffer[FSP_LX_READ_BUFFER_SIZE_WORDS];
#endif

// LevelX NOR driver functions
UINT g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash);
fsp_err_t g_rm_levelx_nor_OSPI_close(void);
UINT g_rm_levelx_nor_OSPI_system_error(UINT error_code);

#endif // LEVELX_CONFIG_H
