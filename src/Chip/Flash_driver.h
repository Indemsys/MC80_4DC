#ifndef FLASHER_DRIVER_H
#define FLASHER_DRIVER_H

// Flash driver state tracking
typedef enum
{
  FLASH_DRIVER_NONE   = 0,  // No driver initialized
  FLASH_DRIVER_NO_BGO = 1,  // Non-BGO driver initialized (g_flash_cbl)
  FLASH_DRIVER_BGO    = 2   // BGO driver initialized (g_flash_bgo_cbl)
} T_flash_driver_state;

#define BIT_FLASH_EVENT_ERASE_COMPLETE BIT(0)
#define BIT_FLASH_EVENT_WRITE_COMPLETE BIT(1)
#define BIT_FLASH_EVENT_BLANK          BIT(2)
#define BIT_FLASH_EVENT_NOT_BLANK      BIT(3)
#define BIT_FLASH_EVENT_ERR_DF_ACCESS  BIT(4)
#define BIT_FLASH_EVENT_ERR_CF_ACCESS  BIT(5)
#define BIT_FLASH_EVENT_ERR_CMD_LOCKED BIT(6)
#define BIT_FLASH_EVENT_ERR_FAILURE    BIT(7)
#define BIT_FLASH_EVENT_ERR_ONE_BIT    BIT(8)

fsp_err_t            Flash_driver_init(void);
fsp_err_t            Flash_driver_deinit(void);
uint32_t             Flash_driver_bgo_init(void);
uint32_t             Flash_driver_bgo_deinit(void);
T_flash_driver_state Get_flash_driver_state(void);
uint32_t             Get_bgo_status(void);
void                 Set_bgo_status(uint32_t stat);
uint32_t             Wait_bgo_end(ULONG wait_option);
uint32_t             DataFlash_bgo_EraseArea(uint32_t start_addr, uint32_t area_size);
uint32_t             DataFlash_bgo_WriteArea(uint32_t start_addr, uint8_t *buf, uint32_t buf_size);
uint32_t             DataFlash_bgo_ReadArea(uint32_t start_addr, uint8_t *buf, uint32_t buf_size);
uint32_t             DataFlash_bgo_BlankCheck(uint32_t start_addr, uint32_t num_bytes);
fsp_err_t            Set_Flash_protection(uint8_t const *const p_id_bytes, flash_id_code_mode_t mode);
uint32_t             Switch_Flash_driver_to_no_bgo(void);
uint32_t             Switch_Flash_driver_to_bgo(void);
fsp_err_t            Flash_blank_check(uint32_t const address, uint32_t const num_bytes, flash_result_t *const p_blank_check_result);
fsp_err_t            Flash_erase_block(uint32_t const address, uint32_t const num_blocks);
fsp_err_t            Flash_write_block(uint32_t const src_address, uint32_t const flash_address, uint32_t const num_bytes);

#endif
