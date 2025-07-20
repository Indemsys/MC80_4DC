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

elc_instance_ctrl_t g_elc_ctrl;

extern const elc_cfg_t g_elc_cfg;

const elc_instance_t g_elc = {
  .p_ctrl = &g_elc_ctrl,
  .p_api  = &g_elc_on_elc,
  .p_cfg  = &g_elc_cfg
};

ioport_instance_ctrl_t g_ioport_ctrl;

const ioport_instance_t g_ioport = {
  .p_api  = &g_ioport_on_ioport,
  .p_ctrl = &g_ioport_ctrl,
  .p_cfg  = &g_bsp_pin_cfg,
};
