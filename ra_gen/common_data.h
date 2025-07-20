/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_sdhi.h"
#include "r_sdmmc_api.h"
#include "rm_block_media_sdmmc.h"
#include "rm_block_media_api.h"
#include "rm_filex_block_media.h"
#include "fx_api.h"
#include "nx_api.h"
#include "r_ether_phy.h"
#include "r_ether_phy_api.h"
#include "r_ether.h"
#include "r_ether_api.h"
#include "rm_netxduo_ether.h"
#include "nxd_bsd.h"
#include "r_elc.h"
#include "r_elc_api.h"
#include "r_rsip_key_injection.h"
#include "r_rsip_key_injection_api.h"
#include "hw_sce_ra_private.h"
#include "r_ospi_b.h"
#include "r_spi_flash_api.h"
#include "rm_levelx_nor_spi.h"
#include "lx_api.h"
#include "rm_filex_levelx_nor.h"
#include "fx_api.h"
#include "rm_netx_secure_crypto.h"
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_sdmmc_transfer;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_sdmmc_transfer_ctrl;
extern const transfer_cfg_t g_sdmmc_transfer_cfg;

#ifndef g_sdmmc_dmac_callback
void g_sdmmc_dmac_callback(transfer_callback_args_t* p_args);
#endif
/** SDMMC on SDMMC Instance. */
extern const sdmmc_instance_t g_sdmmc;

/** Access the SDMMC instance using these structures when calling API functions directly (::p_api is not used). */
extern sdhi_instance_ctrl_t g_sdmmc_ctrl;
extern sdmmc_cfg_t          g_sdmmc_cfg;

#ifndef rm_block_media_sdmmc_callback
void rm_block_media_sdmmc_callback(sdmmc_callback_args_t* p_args);
#endif
extern const rm_block_media_instance_t g_rm_sdmmc_block_media;

/** Access the SDMMC instance using these structures when calling API functions directly (::p_api is not used). */
extern rm_block_media_sdmmc_instance_ctrl_t g_rm_sdmmc_block_media_ctrl;
extern const rm_block_media_cfg_t           g_rm_sdmmc_block_media_cfg;

#ifndef rm_filex_block_media_memory_callback
void rm_filex_block_media_memory_callback(rm_block_media_callback_args_t* p_args);
#endif
extern const rm_filex_block_media_instance_t g_rm_filex_sdmmc_block_media_instance;

/** Access the FileX Block Media instance using these structures when calling API functions directly (::p_api is not used). */
extern rm_filex_block_media_instance_ctrl_t g_rm_filex_sdmmc_block_media_ctrl;
extern const rm_filex_block_media_cfg_t     g_rm_filex_sdmmc_block_media_cfg;

#ifndef g_rm_filex_sdmmc_block_media_callback
void g_rm_filex_sdmmc_block_media_callback(rm_filex_block_media_callback_args_t* p_args);
#endif
#define FAT_FS_MEDIA_MEDIA_MEMORY_SIZE    (512)
#define FAT_FS_MEDIA_VOLUME_NAME          ("Volume 1")
#define FAT_FS_MEDIA_NUMBER_OF_FATS       (1)
#define FAT_FS_MEDIA_DIRECTORY_ENTRIES    (256)
#define FAT_FS_MEDIA_HIDDEN_SECTORS       (0)
#define FAT_FS_MEDIA_TOTAL_SECTORS        (65536)
#define FAT_FS_MEDIA_BYTES_PER_SECTOR     (512)
#define FAT_FS_MEDIA_SECTORS_PER_CLUSTER  (1)
#define FAT_FS_MEDIA_VOLUME_SERIAL_NUMBER (12345)
#define FAT_FS_MEDIA_BOUNDARY_UNIT        (128)

/** ELC Instance */
extern const elc_instance_t g_elc;

/** Access the ELC instance using these structures when calling API functions directly (::p_api is not used). */
extern elc_instance_ctrl_t g_elc_ctrl;
extern const elc_cfg_t     g_elc_cfg;
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
  #include "r_dmac.h"
#endif
#if OSPI_CFG_DOTF_SUPPORT_ENABLE
  #include "r_sce_if.h"
#endif

#define IOPORT_CFG_NAME g_bsp_pin_cfg
#define IOPORT_CFG_OPEN R_IOPORT_Open
#define IOPORT_CFG_CTRL g_ioport_ctrl

/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
void                          g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
