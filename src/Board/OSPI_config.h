#ifndef OSPI_CONFIG_H
#define OSPI_CONFIG_H

#include "App.h"

// External declarations for OSPI configuration structures
extern const spi_flash_instance_t  g_OSPI;
extern const spi_flash_cfg_t       g_OSPI_cfg;
extern const ospi_b_extended_cfg_t g_OSPI_extended_cfg;
extern ospi_b_instance_ctrl_t      g_OSPI_ctrl;

// OSPI command set tables
extern const ospi_b_table_t            g_OSPI_command_set;
extern const ospi_b_xspi_command_set_t g_OSPI_command_set_table[];

// Erase command tables
extern const ospi_b_table_t g_OSPI_command_set_initial_erase_table;
extern const ospi_b_table_t g_OSPI_command_set_high_speed_erase_table;

// Transfer instance (if DMA is enabled)
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
extern dmac_instance_ctrl_t g_transfer_OSPI_ctrl;
extern transfer_info_t g_transfer_OSPI_info;
extern const dmac_extended_cfg_t g_transfer_OSPI_extend;
extern const transfer_cfg_t g_transfer_OSPI_cfg;
extern const transfer_instance_t g_transfer_OSPI;
#endif

// RSIP instance (if DOTF protected mode is enabled)
#if defined OSPI_B_CFG_DOTF_PROTECTED_MODE_SUPPORT_ENABLE
extern rsip_instance_t const *const gp_rsip_instance;
#endif

#endif  // OSPI_CONFIG_H
