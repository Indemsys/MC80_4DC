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
void g_sdmmc_dmac_callback(transfer_callback_args_t * p_args);
#endif
/** SDMMC on SDMMC Instance. */
extern const sdmmc_instance_t g_sdmmc;


/** Access the SDMMC instance using these structures when calling API functions directly (::p_api is not used). */
extern sdhi_instance_ctrl_t g_sdmmc_ctrl;
extern sdmmc_cfg_t g_sdmmc_cfg;

#ifndef rm_block_media_sdmmc_callback
void rm_block_media_sdmmc_callback(sdmmc_callback_args_t * p_args);
#endif
extern const rm_block_media_instance_t g_rm_sdmmc_block_media;

/** Access the SDMMC instance using these structures when calling API functions directly (::p_api is not used). */
extern rm_block_media_sdmmc_instance_ctrl_t g_rm_sdmmc_block_media_ctrl;
extern const rm_block_media_cfg_t g_rm_sdmmc_block_media_cfg;

#ifndef rm_filex_block_media_memory_callback
void rm_filex_block_media_memory_callback(rm_block_media_callback_args_t * p_args);
#endif
extern const rm_filex_block_media_instance_t g_rm_filex_sdmmc_block_media_instance;

/** Access the FileX Block Media instance using these structures when calling API functions directly (::p_api is not used). */
extern rm_filex_block_media_instance_ctrl_t g_rm_filex_sdmmc_block_media_ctrl;
extern const rm_filex_block_media_cfg_t g_rm_filex_sdmmc_block_media_cfg;

#ifndef g_rm_filex_sdmmc_block_media_callback
void g_rm_filex_sdmmc_block_media_callback(rm_filex_block_media_callback_args_t * p_args);
#endif
#define FAT_FS_MEDIA_MEDIA_MEMORY_SIZE (512)
#define FAT_FS_MEDIA_VOLUME_NAME ("Volume 1")
#define FAT_FS_MEDIA_NUMBER_OF_FATS (1)
#define FAT_FS_MEDIA_DIRECTORY_ENTRIES (256)
#define FAT_FS_MEDIA_HIDDEN_SECTORS (0)
#define FAT_FS_MEDIA_TOTAL_SECTORS (65536)
#define FAT_FS_MEDIA_BYTES_PER_SECTOR (512)
#define FAT_FS_MEDIA_SECTORS_PER_CLUSTER (1)
#define FAT_FS_MEDIA_VOLUME_SERIAL_NUMBER (12345)
#define FAT_FS_MEDIA_BOUNDARY_UNIT (128)
#ifndef ETHER_PHY_LSI_TYPE_KIT_COMPONENT
  #define ETHER_PHY_LSI_TYPE_KIT_COMPONENT ETHER_PHY_LSI_TYPE_DEFAULT
#endif

#ifndef NULL
void NULL(ether_phy_instance_ctrl_t * p_instance_ctrl);
#endif

#ifndef NULL
bool NULL(ether_phy_instance_ctrl_t * p_instance_ctrl, uint32_t line_speed_duplex);
#endif

/** ether_phy on ether_phy Instance. */
extern const ether_phy_instance_t g_ether_phy0;

/** Access the Ethernet PHY instance using these structures when calling API functions directly (::p_api is not used). */
extern ether_phy_instance_ctrl_t g_ether_phy0_ctrl;
extern const ether_phy_cfg_t g_ether_phy0_cfg;
extern const ether_phy_extended_cfg_t g_ether_phy0_extended_cfg;
#if (BSP_FEATURE_TZ_HAS_TRUSTZONE == 1) && (BSP_TZ_SECURE_BUILD != 1) && (BSP_TZ_NONSECURE_BUILD != 1) && (BSP_FEATURE_ETHER_SUPPORTS_TZ_SECURE == 0)
#define ETHER_BUFFER_PLACE_IN_SECTION BSP_PLACE_IN_SECTION(".ns_buffer.eth")
#else
#define ETHER_BUFFER_PLACE_IN_SECTION
#endif

/** ether on ether Instance. */
extern const ether_instance_t g_ether0;

/** Access the Ethernet instance using these structures when calling API functions directly (::p_api is not used). */
extern ether_instance_ctrl_t g_ether0_ctrl;
extern const ether_cfg_t g_ether0_cfg;

#ifndef rm_netxduo_ether_callback
void rm_netxduo_ether_callback(ether_callback_args_t * p_args);
#endif
/* NetX Duo Ethernet Driver */
void g_netxduo_ether_0(NX_IP_DRIVER * driver_req_ptr);

/* Instance for storing state information for the Ethernet Driver. */
extern rm_netxduo_ether_instance_t g_netxduo_ether_0_instance;
#define G_PACKET_POOL0_PACKET_SIZE (1568)
            #define G_PACKET_POOL0_PACKET_NUM  (16)
            extern NX_PACKET_POOL g_packet_pool0;
            extern uint8_t g_packet_pool0_pool_memory[(G_PACKET_POOL0_PACKET_NUM * (G_PACKET_POOL0_PACKET_SIZE + sizeof(NX_PACKET)))];

            /* Quick Setup for g_packet_pool0 instance. */
            void g_packet_pool0_quick_setup();
#define G_IP0_ADDRESS      (IP_ADDRESS(192,168,0,2))
#define G_IP0_SUBNET_MASK     (IP_ADDRESS(255,255,255,0))
#define G_IP0_GATEWAY_ADDRESS (IP_ADDRESS(0,0,0,0))
#define G_IP0_TASK_STACK_SIZE (2048)
#define G_IP0_TASK_PRIORITY   (3)
#define G_IP0_ARP_CACHE_SIZE (520 * 1)

#define RA_NOT_DEFINED 0xFFFFFFFF
#if (RA_NOT_DEFINED != g_netxduo_ether_0)
#define G_IP0_NETWORK_DRIVER g_netxduo_ether_0
#elif (RA_NOT_DEFINED != RA_NOT_DEFINED)
#define G_IP0_NETWORK_DRIVER rm_netxduo_wifi
#else
#define G_IP0_NETWORK_DRIVER nx_driver_ewf_adapter
#endif
#undef RA_NOT_DEFINED

void g_ip0_quick_setup();
void g_ip0_error_handler(UINT status);

#ifndef NX_DISABLE_IPV6
extern NXD_ADDRESS g_ip0_ipv6_global_address;
extern NXD_ADDRESS g_ip0_ipv6_link_local_address;
#endif
#define BSD_SUPPORT_TASK_STACK_SIZE (2048)
#define BSD_SUPPORT_TASK_PRIORITY   (3)

void bsd_quick_setup();
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer_OSPI;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_transfer_OSPI_ctrl;
extern const transfer_cfg_t g_transfer_OSPI_cfg;

#ifndef NULL
void NULL(transfer_callback_args_t * p_args);
#endif
/** ELC Instance */
extern const elc_instance_t g_elc;

/** Access the ELC instance using these structures when calling API functions directly (::p_api is not used). */
extern elc_instance_ctrl_t g_elc_ctrl;
extern const elc_cfg_t g_elc_cfg;
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
              #include "r_dmac.h"
            #endif
            #if OSPI_CFG_DOTF_SUPPORT_ENABLE
              #include "r_sce_if.h"
            #endif
            extern const spi_flash_instance_t g_OSPI;
            extern ospi_b_instance_ctrl_t g_OSPI_ctrl;
            extern const spi_flash_cfg_t g_OSPI_cfg;
/** Access the LevelX NOR SPI instance using these structures when calling functions directly (::p_api is not used). */
extern rm_levelx_nor_spi_instance_ctrl_t g_rm_levelx_nor_OSPI_ctrl;
extern rm_levelx_nor_spi_cfg_t g_rm_levelx_nor_OSPI_cfg;

#ifndef rm_filex_levelx_nor_spi_callback
void rm_filex_levelx_nor_spi_callback(rm_levelx_nor_spi_callback_args_t * p_args);
#endif

UINT g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash);
fsp_err_t g_rm_levelx_nor_OSPI_close();

#define RA_NOT_DEFINED 0xFFFFFFFF
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
#define G_RM_LEVELX_NOR_OSPI_SECTOR_SIZE (RA_NOT_DEFINED)
#elif (RA_NOT_DEFINED != RA_NOT_DEFINED)
#define G_RM_LEVELX_NOR_OSPI_SECTOR_SIZE (RA_NOT_DEFINED)
#else
#define G_RM_LEVELX_NOR_OSPI_SECTOR_SIZE (4096)
#endif
#undef RA_NOT_DEFINED
extern LX_NOR_FLASH g_lx_NOR;
extern const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance;

/** Access the FileX LevelX NOR instance using these structures when calling API functions directly (::p_api is not used). */
extern rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;
extern const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg;

#ifndef g_rm_filex_levelx_NOR_callback
void g_rm_filex_levelx_NOR_callback(rm_filex_levelx_nor_callback_args_t * p_args);
#endif
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
#define IOPORT_CFG_NAME g_bsp_pin_cfg
#define IOPORT_CFG_OPEN R_IOPORT_Open
#define IOPORT_CFG_CTRL g_ioport_ctrl

/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
