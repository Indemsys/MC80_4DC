/* generated common source file - do not edit */
#include "common_data.h"

dmac_instance_ctrl_t g_sdmmc_transfer_ctrl;
transfer_info_t      g_sdmmc_transfer_info = {
       .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
       .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
       .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
       .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
       .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.size           = TRANSFER_SIZE_4_BYTE,
       .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
       .p_dest                                  = (void *)NULL,
       .p_src                                   = (void const *)NULL,
       .num_blocks                              = 0,
       .length                                  = 128,
};
const dmac_extended_cfg_t g_sdmmc_transfer_extend = {
  .offset          = 1,
  .src_buffer_size = 1,
#if defined(VECTOR_NUMBER_DMAC2_INT)
  .irq = VECTOR_NUMBER_DMAC2_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
  .ipl               = (3),
  .channel           = 2,
  .p_callback        = g_sdmmc_dmac_callback,
  .p_context         = &g_sdmmc_ctrl,
  .activation_source = ELC_EVENT_SDHIMMC1_DMA_REQ,
};
const transfer_cfg_t g_sdmmc_transfer_cfg = {
  .p_info   = &g_sdmmc_transfer_info,
  .p_extend = &g_sdmmc_transfer_extend,
};
/* Instance structure to use this module. */
const transfer_instance_t g_sdmmc_transfer = {
  .p_ctrl = &g_sdmmc_transfer_ctrl,
  .p_cfg  = &g_sdmmc_transfer_cfg,
  .p_api  = &g_transfer_on_dmac
};
#define RA_NOT_DEFINED (UINT32_MAX)
#if (RA_NOT_DEFINED) != (1)

  /* If the transfer module is DMAC, define a DMAC transfer callback. */
  #include "r_dmac.h"
extern void r_sdhi_transfer_callback(sdhi_instance_ctrl_t *p_ctrl);

void g_sdmmc_dmac_callback(dmac_callback_args_t *p_args)
{
  r_sdhi_transfer_callback((sdhi_instance_ctrl_t *)p_args->p_context);
}
#endif
#undef RA_NOT_DEFINED

sdhi_instance_ctrl_t g_sdmmc_ctrl;
sdmmc_cfg_t          g_sdmmc_cfg = {
           .bus_width            = SDMMC_BUS_WIDTH_4_BITS,
           .channel              = 1,
           .p_callback           = rm_block_media_sdmmc_callback,
           .p_context            = &g_rm_sdmmc_block_media_ctrl,
           .block_size           = 512,
           .card_detect          = SDMMC_CARD_DETECT_NONE,
           .write_protect        = SDMMC_WRITE_PROTECT_NONE,

           .p_extend             = NULL,
           .p_lower_lvl_transfer = &g_sdmmc_transfer,

           .access_ipl           = (12),
           .sdio_ipl             = BSP_IRQ_DISABLED,
           .card_ipl             = (BSP_IRQ_DISABLED),
           .dma_req_ipl          = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_SDHIMMC1_ACCS)
  .access_irq = VECTOR_NUMBER_SDHIMMC1_ACCS,
#else
  .access_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SDHIMMC1_CARD)
  .card_irq = VECTOR_NUMBER_SDHIMMC1_CARD,
#else
  .card_irq = FSP_INVALID_VECTOR,
#endif
  .sdio_irq = FSP_INVALID_VECTOR,
#if defined(VECTOR_NUMBER_SDHIMMC1_DMA_REQ)
  .dma_req_irq = VECTOR_NUMBER_SDHIMMC1_DMA_REQ,
#else
  .dma_req_irq = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const sdmmc_instance_t g_sdmmc = {
  .p_ctrl = &g_sdmmc_ctrl,
  .p_cfg  = &g_sdmmc_cfg,
  .p_api  = &g_sdmmc_on_sdhi
};
const rm_block_media_sdmmc_extended_cfg_t g_rm_sdmmc_block_media_cfg_extend = {
  .p_sdmmc = &g_sdmmc,
};
const rm_block_media_cfg_t g_rm_sdmmc_block_media_cfg = {
  .p_extend   = &g_rm_sdmmc_block_media_cfg_extend,
  .p_callback = rm_filex_block_media_memory_callback,
  .p_context  = &g_rm_filex_sdmmc_block_media_ctrl,
};
rm_block_media_sdmmc_instance_ctrl_t g_rm_sdmmc_block_media_ctrl;
const rm_block_media_instance_t      g_rm_sdmmc_block_media = {
       .p_api  = &g_rm_block_media_on_sdmmc,
       .p_ctrl = &g_rm_sdmmc_block_media_ctrl,
       .p_cfg  = &g_rm_sdmmc_block_media_cfg,
};
rm_filex_block_media_instance_ctrl_t g_rm_filex_sdmmc_block_media_ctrl;

const rm_filex_block_media_cfg_t g_rm_filex_sdmmc_block_media_cfg = {
  .p_lower_lvl_block_media = (rm_block_media_instance_t *)&g_rm_sdmmc_block_media,
  .partition               = RM_FILEX_BLOCK_MEDIA_PARTITION0,
  .p_callback              = g_rm_filex_sdmmc_block_media_callback
};

const rm_filex_block_media_instance_t g_rm_filex_sdmmc_block_media_instance = {
  .p_ctrl = &g_rm_filex_sdmmc_block_media_ctrl,
  .p_cfg  = &g_rm_filex_sdmmc_block_media_cfg,
  .p_api  = &g_filex_on_block_media
};
ether_phy_instance_ctrl_t g_ether_phy0_ctrl;

const ether_phy_extended_cfg_t g_ether_phy0_extended_cfg = {
  .p_target_init                     = NULL,
  .p_target_link_partner_ability_get = NULL

};

const ether_phy_cfg_t g_ether_phy0_cfg = {

  .channel                  = 0,
  .phy_lsi_address          = 0,
  .phy_reset_wait_time      = 0x00020000,
  .mii_bit_access_wait_time = 8,
  .phy_lsi_type             = ETHER_PHY_LSI_TYPE_KIT_COMPONENT,
  .flow_control             = ETHER_PHY_FLOW_CONTROL_DISABLE,
  .mii_type                 = ETHER_PHY_MII_TYPE_RMII,
  .p_context                = NULL,
  .p_extend                 = &g_ether_phy0_extended_cfg,

};
/* Instance structure to use this module. */
const ether_phy_instance_t g_ether_phy0 = {
  .p_ctrl = &g_ether_phy0_ctrl,
  .p_cfg  = &g_ether_phy0_cfg,
  .p_api  = &g_ether_phy_on_ether_phy
};
ether_instance_ctrl_t g_ether0_ctrl;

uint8_t g_ether0_mac_address[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };

__attribute__((__aligned__(16))) ether_instance_descriptor_t g_ether0_tx_descriptors[4] ETHER_BUFFER_PLACE_IN_SECTION;
__attribute__((__aligned__(16))) ether_instance_descriptor_t g_ether0_rx_descriptors[4] ETHER_BUFFER_PLACE_IN_SECTION;

const ether_extended_cfg_t g_ether0_extended_cfg_t = {
  .p_rx_descriptors  = g_ether0_rx_descriptors,
  .p_tx_descriptors  = g_ether0_tx_descriptors,
  .eesr_event_filter = (ETHER_EESR_EVENT_MASK_RFOF | ETHER_EESR_EVENT_MASK_RDE | ETHER_EESR_EVENT_MASK_FR | ETHER_EESR_EVENT_MASK_TFUF | ETHER_EESR_EVENT_MASK_TDE | ETHER_EESR_EVENT_MASK_TC | 0U),
  .ecsr_event_filter = (0U),
};

const ether_cfg_t g_ether0_cfg = {
  .channel            = 0,
  .zerocopy           = ETHER_ZEROCOPY_ENABLE,
  .multicast          = ETHER_MULTICAST_ENABLE,
  .promiscuous        = ETHER_PROMISCUOUS_DISABLE,
  .flow_control       = ETHER_FLOW_CONTROL_DISABLE,
  .padding            = ETHER_PADDING_2BYTE,
  .padding_offset     = 14,
  .broadcast_filter   = 0,
  .p_mac_address      = g_ether0_mac_address,

  .num_tx_descriptors = 4,
  .num_rx_descriptors = 4,

  .pp_ether_buffers   = NULL,

  .ether_buffer_size  = 1536,

#if defined(VECTOR_NUMBER_EDMAC0_EINT)
  .irq = VECTOR_NUMBER_EDMAC0_EINT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_priority   = (12),

  .p_callback           = rm_netxduo_ether_callback,
  .p_ether_phy_instance = &g_ether_phy0,
  .p_context            = &g_netxduo_ether_0_instance,
  .p_extend             = &g_ether0_extended_cfg_t,
};

/* Instance structure to use this module. */
const ether_instance_t g_ether0 = {
  .p_ctrl = &g_ether0_ctrl,
  .p_cfg  = &g_ether0_cfg,
  .p_api  = &g_ether_on_ether
};
static NX_PACKET *g_netxduo_ether_0_tx_packets[4];
static NX_PACKET *g_netxduo_ether_0_rx_packets[4];

static rm_netxduo_ether_ctrl_t g_netxduo_ether_0_ctrl;
static rm_netxduo_ether_cfg_t  g_netxduo_ether_0_cfg = {
   .p_ether_instance = &g_ether0,
   .mtu              = 1500,
   .p_tx_packets     = g_netxduo_ether_0_tx_packets,
   .p_rx_packets     = g_netxduo_ether_0_rx_packets
};

rm_netxduo_ether_instance_t g_netxduo_ether_0_instance = {
  .p_ctrl = &g_netxduo_ether_0_ctrl,
  .p_cfg  = &g_netxduo_ether_0_cfg
};

/*
 * NetX Duo Driver: g_netxduo_ether_0
 * Passes rm_netxduo_ether instance and driver request into the rm_netxduo_ether driver.
 */
void g_netxduo_ether_0(NX_IP_DRIVER *driver_req_ptr)
{
  rm_netxduo_ether(driver_req_ptr, &g_netxduo_ether_0_instance);
}
#ifndef NX_DISABLE_IPV6
NXD_ADDRESS g_ip0_ipv6_global_address = {
  .nxd_ip_version    = NX_IP_VERSION_V6,
  .nxd_ip_address.v6 = {
   ((0x2001 << 16) | 0x0),
   ((0x0 << 16) | 0x0),
   ((0x0 << 16) | 0x0),
   ((0x0 << 16) | 0x1) }
};

NXD_ADDRESS g_ip0_ipv6_link_local_address = {
  .nxd_ip_version    = NX_IP_VERSION_V6,
  .nxd_ip_address.v6 = {
   ((0x0 << 16) | 0x0),
   ((0x0 << 16) | 0x0),
   ((0x0 << 16) | 0x0),
   ((0x0 << 16) | 0x0) }
};

#endif

dmac_instance_ctrl_t g_transfer_OSPI_ctrl;
transfer_info_t      g_transfer_OSPI_info = {
       .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
       .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
       .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
       .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.size           = TRANSFER_SIZE_1_BYTE,
       .transfer_settings_word_b.mode           = TRANSFER_MODE_BLOCK,
       .p_dest                                  = (void *)NULL,
       .p_src                                   = (void const *)NULL,
       .num_blocks                              = 1,
       .length                                  = 64,
};
const dmac_extended_cfg_t g_transfer_OSPI_extend = {
  .offset          = 0,
  .src_buffer_size = 0,
#if defined(VECTOR_NUMBER_DMAC3_INT)
  .irq = VECTOR_NUMBER_DMAC3_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
  .ipl               = (10),
  .channel           = 3,
  .p_callback        = NULL,
  .p_context         = NULL,
  .activation_source = ELC_EVENT_NONE,
};
const transfer_cfg_t g_transfer_OSPI_cfg = {
  .p_info   = &g_transfer_OSPI_info,
  .p_extend = &g_transfer_OSPI_extend,
};
/* Instance structure to use this module. */
const transfer_instance_t g_transfer_OSPI = {
  .p_ctrl = &g_transfer_OSPI_ctrl,
  .p_cfg  = &g_transfer_OSPI_cfg,
  .p_api  = &g_transfer_on_dmac
};

elc_instance_ctrl_t g_elc_ctrl;

extern const elc_cfg_t g_elc_cfg;

const elc_instance_t g_elc = {
  .p_ctrl = &g_elc_ctrl,
  .p_api  = &g_elc_on_elc,
  .p_cfg  = &g_elc_cfg
};

ospi_b_instance_ctrl_t g_OSPI_ctrl;

static ospi_b_timing_setting_t g_OSPI_timing_settings = {

  .command_to_command_interval = OSPI_B_COMMAND_INTERVAL_CLOCKS_2,
  .cs_pullup_lag               = OSPI_B_COMMAND_CS_PULLUP_CLOCKS_NO_EXTENSION,
  .cs_pulldown_lead            = OSPI_B_COMMAND_CS_PULLDOWN_CLOCKS_NO_EXTENSION,
  .sdr_drive_timing            = OSPI_B_SDR_DRIVE_TIMING_BEFORE_CK,
  .sdr_sampling_edge           = OSPI_B_CK_EDGE_FALLING,
  .sdr_sampling_delay          = OSPI_B_SDR_SAMPLING_DELAY_NONE,
  .ddr_sampling_extension      = OSPI_B_DDR_SAMPLING_EXTENSION_NONE,
};

static const spi_flash_erase_command_t g_OSPI_command_set_initial_erase_commands[] = {
  { .command = 0x20, .size = 4096 },
  { .command = 0xD8, .size = 65536 },
  { .command = 0x60, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE },
};
static const ospi_b_table_t g_OSPI_command_set_initial_erase_table = {
  .p_table = (void *)g_OSPI_command_set_initial_erase_commands,
  .length  = sizeof(g_OSPI_command_set_initial_erase_commands) / sizeof(g_OSPI_command_set_initial_erase_commands[0]),
};
static const spi_flash_erase_command_t g_OSPI_command_set_high_speed_erase_commands[] = {};
static const ospi_b_table_t            g_OSPI_command_set_high_speed_erase_table      = {
                  .p_table = (void *)g_OSPI_command_set_high_speed_erase_commands,
                  .length  = sizeof(g_OSPI_command_set_high_speed_erase_commands) / sizeof(g_OSPI_command_set_high_speed_erase_commands[0]),
};

static const ospi_b_xspi_command_set_t g_OSPI_command_set_table[] = {
  {
   .protocol               = SPI_FLASH_PROTOCOL_1S_1S_1S,
   .frame_format           = OSPI_B_FRAME_FORMAT_STANDARD,
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,
   .command_bytes          = OSPI_B_COMMAND_BYTES_1,
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,
   .address_msb_mask       = 0xF0,
   .status_needs_address   = false,
   .status_address         = 0U,
   .status_address_bytes   = (spi_flash_address_bytes_t)0U,
   .p_erase_commands       = &g_OSPI_command_set_initial_erase_table,
   .read_command           = 0x0C,  // FAST READ4B fast read data
   .read_dummy_cycles      = 8,     // 8 dummy cycles для Fast Read в 1S-1S-1S
   .program_command        = 0x12,  // PP4B page program
   .program_dummy_cycles   = 0,
   .row_load_command       = 0x00,
   .row_load_dummy_cycles  = 0,
   .row_store_command      = 0x00,
   .row_store_dummy_cycles = 0,
   .write_enable_command   = 0x06,
   .status_command         = 0x05,  // RDSR read status register
   .status_dummy_cycles    = 0,
  },
  {
   .protocol               = SPI_FLASH_PROTOCOL_8D_8D_8D,
   .frame_format           = OSPI_B_FRAME_FORMAT_XSPI_PROFILE_1,
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,
   .command_bytes          = OSPI_B_COMMAND_BYTES_2,
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,
   .address_msb_mask       = 0xF0,
   .status_needs_address   = true,
   .status_address         = 0x00,
   .status_address_bytes   = SPI_FLASH_ADDRESS_BYTES_4,
   .p_erase_commands       = &g_OSPI_command_set_high_speed_erase_table,
   .read_command           = 0xEE13,  // 8READ  8 I/O read
   .read_dummy_cycles      = 20,
   .program_command        = 0x12ED,  // PP4B (Page Program)
   .program_dummy_cycles   = 0,
   .row_load_command       = 0x00,
   .row_load_dummy_cycles  = 0,
   .row_store_command      = 0x00,
   .row_store_dummy_cycles = 0,
   .write_enable_command   = 0x06F9,
   .status_command         = 0x05FA,
   .status_dummy_cycles    = 4,
  }
};

static const ospi_b_table_t g_OSPI_command_set = {
  .p_table = (void *)g_OSPI_command_set_table,
  .length  = 2

};

#if OSPI_B_CFG_DOTF_SUPPORT_ENABLE
extern uint8_t g_ospi_dotf_iv[];
extern uint8_t g_ospi_dotf_key[];

static ospi_b_dotf_cfg_t g_ospi_dotf_cfg = {
  .key_type     = OSPI_B_DOTF_AES_KEY_TYPE_128,
  .format       = OSPI_B_DOTF_KEY_FORMAT_PLAINTEXT,
  .p_start_addr = (uint32_t *)0x90000000,
  .p_end_addr   = (uint32_t *)0x90001FFF,
  .p_key        = (uint32_t *)g_ospi_dotf_key,
  .p_iv         = (uint32_t *)g_ospi_dotf_iv,
};
#endif

static const ospi_b_extended_cfg_t g_OSPI_extended_cfg = {
  .ospi_b_unit                             = 0,
  .channel                                 = (ospi_b_device_number_t)0,

  .p_timing_settings                       = &g_OSPI_timing_settings,
  .p_xspi_command_set                      = &g_OSPI_command_set,
  .data_latch_delay_clocks                 = OSPI_B_DS_TIMING_DELAY_NONE,
  .p_autocalibration_preamble_pattern_addr = (uint8_t *)0x00,
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
  .p_lower_lvl_transfer = &g_transfer_OSPI,
#endif
#if OSPI_B_CFG_DOTF_SUPPORT_ENABLE
  .p_dotf_cfg = &g_ospi_dotf_cfg,
#endif
#if OSPI_B_CFG_ROW_ADDRESSING_SUPPORT_ENABLE
  .row_index_bytes = 0xFF
#endif
};
const spi_flash_cfg_t g_OSPI_cfg = {
  .spi_protocol               = SPI_FLASH_PROTOCOL_1S_1S_1S,
  .read_mode                  = SPI_FLASH_READ_MODE_STANDARD,   /* Unused by OSPI_B */
  .address_bytes              = SPI_FLASH_ADDRESS_BYTES_4,
  .dummy_clocks               = SPI_FLASH_DUMMY_CLOCKS_DEFAULT, /* Unused by OSPI_B */
  .page_program_address_lines = (spi_flash_data_lines_t)0U,     /* Unused by OSPI_B */
  .page_size_bytes            = 256,
  .write_status_bit           = 0,
  .write_enable_bit           = 1,
  .page_program_command       = 0, /* OSPI_B uses command sets. See g_OSPI_command_set. */
  .write_enable_command       = 0, /* OSPI_B uses command sets. See g_OSPI_command_set. */
  .status_command             = 0, /* OSPI_B uses command sets. See g_OSPI_command_set. */
  .read_command               = 0, /* OSPI_B uses command sets. See g_OSPI_command_set. */
#if OSPI_B_CFG_XIP_SUPPORT_ENABLE
  .xip_enter_command = 0,
  .xip_exit_command  = 0,
#else
  .xip_enter_command = 0U,
  .xip_exit_command  = 0U,
#endif
  .erase_command_list_length = 0U,   /* OSPI_B uses command sets. See g_OSPI_command_set. */
  .p_erase_command_list      = NULL, /* OSPI_B uses command sets. See g_OSPI_command_set. */
  .p_extend                  = &g_OSPI_extended_cfg,
};

/** This structure encompasses everything that is needed to use an instance of this interface. */
const spi_flash_instance_t g_OSPI = {
  .p_ctrl = &g_OSPI_ctrl,
  .p_cfg  = &g_OSPI_cfg,
  .p_api  = &g_ospi_b_on_spi_flash,
};

#if defined OSPI_B_CFG_DOTF_PROTECTED_MODE_SUPPORT_ENABLE
rsip_instance_t const *const gp_rsip_instance = &RA_NOT_DEFINED;
#endif
rm_levelx_nor_spi_instance_ctrl_t g_rm_levelx_nor_OSPI_ctrl;

#define RA_NOT_DEFINED 0xFFFFFFFF
rm_levelx_nor_spi_cfg_t g_rm_levelx_nor_OSPI_cfg = {
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
  .p_lower_lvl  = &RA_NOT_DEFINED,
  .base_address = BSP_FEATURE_QSPI_DEVICE_START_ADDRESS,
#elif (RA_NOT_DEFINED != RA_NOT_DEFINED)
  .p_lower_lvl  = &RA_NOT_DEFINED,
  .base_address = BSP_FEATURE_OSPI_DEVICE_RA_NOT_DEFINED_START_ADDRESS,
#else
  .p_lower_lvl  = &g_OSPI,
  .base_address = BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS,
#endif
  .address_offset    = 0,
  .size              = 33554432,
  .poll_status_count = 0xFFFFFFFF,
  .p_context         = &g_rm_filex_levelx_NOR_ctrl,
  .p_callback        = rm_filex_levelx_nor_spi_callback
};
#undef RA_NOT_DEFINED

#ifndef LX_DIRECT_READ
  #define FSP_LX_READ_BUFFER_SIZE_WORDS (128U)
ULONG g_rm_levelx_nor_OSPI_read_buffer[FSP_LX_READ_BUFFER_SIZE_WORDS] = { 0 };
#endif

/** WEAK system error call back */
#if defined(__ICCARM__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE
  #pragma weak g_rm_levelx_nor_OSPI_system_error = g_rm_levelx_nor_OSPI_system_error_internal
#elif defined(__GNUC__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE \
    __attribute__((weak, alias("g_rm_levelx_nor_OSPI_system_error_internal")))
#endif

UINT g_rm_levelx_nor_OSPI_system_error(UINT error_code) g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE;

/*****************************************************************************************************************
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_rm_levelx_nor_OSPI_system_error(UINT error_code)
 *
 * @param[in]  error_code represents the error that occurred.
 *****************************************************************************************************************/

UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code);
UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code)
{
  FSP_PARAMETER_NOT_USED(error_code);

  /** An error has occurred. Please check function arguments for more information. */
  BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);

  return LX_ERROR;
}

/* LevelX NOR instance "Read Sector" service */
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words);
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_Read(&g_rm_levelx_nor_OSPI_ctrl, flash_address, destination, words);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/* LevelX NOR instance "Write Sector" service */
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words);
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_Write(&g_rm_levelx_nor_OSPI_ctrl, flash_address, source, words);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/* LevelX NOR instance "Block Erase" service */
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count);
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_BlockErase(&g_rm_levelx_nor_OSPI_ctrl, block, block_erase_count);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/* LevelX NOR instance "Block Erased Verify" service */
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block);
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_BlockErasedVerify(&g_rm_levelx_nor_OSPI_ctrl, block);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/* LevelX NOR instance "Driver Initialization" service */
UINT g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash)
{
  fsp_err_t err;

  g_rm_levelx_nor_OSPI_cfg.p_lx_nor_flash = p_nor_flash;

  /* Open the rm_levelx_nor_spi driver */
  err                                     = RM_LEVELX_NOR_SPI_Open(&g_rm_levelx_nor_OSPI_ctrl, &g_rm_levelx_nor_OSPI_cfg);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

#ifndef LX_DIRECT_READ
  /** lx_nor_flash_sector_buffer is used only when LX_DIRECT_READ disabled */
  p_nor_flash->lx_nor_flash_sector_buffer = g_rm_levelx_nor_OSPI_ReadBuffer;
#endif

  p_nor_flash->lx_nor_flash_driver_read                = g_rm_levelx_nor_OSPI_read;
  p_nor_flash->lx_nor_flash_driver_write               = g_rm_levelx_nor_OSPI_write;
  p_nor_flash->lx_nor_flash_driver_block_erase         = g_rm_levelx_nor_OSPI_block_erase;
  p_nor_flash->lx_nor_flash_driver_block_erased_verify = g_rm_levelx_nor_OSPI_block_erased_verify;
  p_nor_flash->lx_nor_flash_driver_system_error        = g_rm_levelx_nor_OSPI_system_error;

  return LX_SUCCESS;
}

/* LevelX NOR instance "Driver Close" service */
fsp_err_t g_rm_levelx_nor_OSPI_close()
{
  return RM_LEVELX_NOR_SPI_Close(&g_rm_levelx_nor_OSPI_ctrl);
}
LX_NOR_FLASH                        g_lx_NOR;
rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;

const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg = {
  .close                 = g_rm_levelx_nor_OSPI_close,
  .nor_driver_initialize = g_rm_levelx_nor_OSPI_initialize,
  .p_nor_flash           = &g_lx_NOR,
  .p_nor_flash_name      = "g_rm_filex_levelx_NOR",
  .p_callback            = g_rm_filex_levelx_NOR_callback,
  .p_context             = NULL
};

const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance = {
  .p_ctrl = &g_rm_filex_levelx_NOR_ctrl,
  .p_cfg  = &g_rm_filex_levelx_NOR_cfg
};

ioport_instance_ctrl_t g_ioport_ctrl;

const ioport_instance_t g_ioport = {
  .p_api  = &g_ioport_on_ioport,
  .p_ctrl = &g_ioport_ctrl,
  .p_cfg  = &g_bsp_pin_cfg,
};
