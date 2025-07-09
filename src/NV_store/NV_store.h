#ifndef NV_STORE_H
#define NV_STORE_H

#ifdef ENABLE_SDRAM
  #define NV_MALLOC_PENDING SDRAM_malloc_pending
  #define NV_MEM_FREE       SDRAM_free
#else
  #define NV_MALLOC_PENDING App_malloc_pending
  #define NV_MEM_FREE       App_free
#endif

#define PARAMS_APP_INI_FILE_NAME         "\\PARAMS.INI"
#define PARAMS_APP_USED_INI_FILE_NAME    "\\PARAMS._NI"
#define PARAMS_APP_JSON_FILE_NAME        "\\Settings.json"
#define PARAMS_APP_COMPR_JSON_FILE_NAME  "\\Settings.dat"

#define CA_CERTIFICATE_FILE_NAME         "\\CA.der"

#define COMMAND_KEY                      "OpCode"           // Идентификатор JSON блока с командой устройству
#define MAIN_PARAMETERS_KEY              "Parameters"       //
#define DATETIME_SETTINGS_KEY            "DateTime"         //
#define DEVICE_HEADER_KEY                "Device"           //
#define PARAMETERS_TREE_KEY              "Parameters_tree"  //

#define RESTORED_DEFAULT_SETTINGS        0
#define RESTORED_SETTINGS_FROM_DATAFLASH 1
#define RESTORED_SETTINGS_FROM_JSON_FILE 2
#define RESTORED_SETTINGS_FROM_INI_FILE  3

#define SAVED_TO_DATAFLASH_NONE          0
#define SAVED_TO_DATAFLASH_OK            1
#define SAVED_TO_DATAFLASH_ERROR         2

#define DATAFLASH_PARAMS_AREA_SIZE       (0x800)
#define DATAFLASH_SUPLIMENT_AREA_SIZE    (4 + 4 + 4)  // Размер дополнительных данных размещаемых в DataFlash: размер юлока данных, номер записи и CRC

#define DATAFLASH_CA_CERT_AREA_SIZE      (0x800)      // Размер области корневого сертификата

#define DATAFLASH_BLUETOOTH_DATA_SIZE    (0x400)      // 1024 байта на структуру bt_nv размер кторой = 796 байт
#define DATAFLASH_NV_COUNTERS_AREA_SIZE  (0x1000)

#define APPLICATION_PARAMS               0
#define PARAMS_TYPES_NUM                 1

#define DATAFLASH_APP_PARAMS_1_ADDR      (DATA_FLASH_START)
#define DATAFLASH_APP_PARAMS_2_ADDR      (DATAFLASH_APP_PARAMS_1_ADDR + DATAFLASH_PARAMS_AREA_SIZE)
#define DATAFLASH_CA_CERT_ADDR           (DATAFLASH_APP_PARAMS_2_ADDR + DATAFLASH_PARAMS_AREA_SIZE)
#define DATAFLASH_BLUETOOTH_DATA_ADDR    (DATAFLASH_CA_CERT_ADDR + DATAFLASH_CA_CERT_AREA_SIZE)
#define DATAFLASH_COUNTERS_DATA_ADDR     (DATAFLASH_BLUETOOTH_DATA_ADDR + DATAFLASH_BLUETOOTH_DATA_SIZE)

#define MEDIA_TYPE_FILE                  1
#define MEDIA_TYPE_DATAFLASH             2

#define NV_COUNTERS_BLOCK_SZ             128  // Размер блока хранения энергонезависимых счетчиков. Размер должен быть кратен размеру стираемого блока DATAFLASH равному 64 байт

typedef struct
{
  uint32_t reboot_cnt;             // Количество перезагрузок системы
  uint32_t accumulated_work_time;  // Общее время работы в секундах

} T_sys_counters;

#define APP_NV_COUNTERS_SZ (NV_COUNTERS_BLOCK_SZ - 4 - sizeof(T_sys_counters))

typedef struct __packed__
{
  T_sys_counters sys;
  uint8_t        data[APP_NV_COUNTERS_SZ];  // Пространтсво хранения счетчиков приложения
  uint32_t       crc;

} T_nv_counters_block;


#define SETT_OK          0
#define SETT_WRONG_SIZE  1
#define SETT_WRONG_CRC   2
#define SETT_WRONG_CHECK 3
#define SETT_IS_BLANK    4

typedef struct
{
  uint32_t area_sz[2];
  uint32_t area_wr_cnt[2];
  uint32_t area_state[2];
  uint32_t area_start_condition[2];
} T_settings_state;

// Error code macros for DataFlash operations
#define NV_ERR_INVALID_TYPE          1  // Invalid parameter type
#define NV_ERR_BUFFER_TOO_LARGE      2  // Buffer size exceeds maximum allowed
#define NV_ERR_BUFFER_TOO_SMALL      3  // Buffer size below minimum required
#define NV_ERR_MEMORY_ALLOCATION     4  // Failed to allocate memory
#define NV_ERR_ERASE_AREA1           5  // Failed to erase area 1
#define NV_ERR_WRITE_AREA1           6  // Failed to write to area 1
#define NV_ERR_ERASE_AREA2           7  // Failed to erase area 2
#define NV_ERR_WRITE_AREA2           8  // Failed to write to area 2

#define DF_RESTORE_ERR_NONE          0  // No error
#define DF_RESTORE_ERR_INVALID_TYPE  1  // Invalid parameter type
#define DF_RESTORE_ERR_NO_VALID_DATA 2  // No valid data found in DataFlash areas

/* Макросы кодов ошибок для области хранения настроек в DataFlash */
#define DF_AREA_ERR_NONE             0  // Нет ошибок
#define DF_AREA_ERR_WRONG_SIZE       1  // Неверный размер данных
#define DF_AREA_ERR_CRC_MISMATCH     2  // Ошибка контрольной суммы
#define DF_AREA_ERR_DECOMP_SIZE      3  // Слишком большой размер декомпрессированных данных
#define DF_AREA_ERR_MEM_ALLOC        4  // Ошибка выделения памяти
#define DF_AREA_ERR_DECOMPRESS       5  // Ошибка декомпрессии
#define DF_AREA_ERR_JSON_PARSE       6  // Ошибка десериализации JSON
#define DF_AREA_ERR_READ_DATA        7  // Ошибка чтения данных из DataFlash

extern T_nv_counters_block g_nv_cnts;
extern uint8_t             g_nv_ram_couners_valid;
extern uint8_t             g_dataflash_couners_valid;

const T_NV_parameters_instance *Get_settings_instance(uint8_t ptype);


int32_t Restore_settings(uint8_t ptype);
void    Return_def_params(uint8_t ptype);

uint32_t Restore_settings_from_INI_file(uint8_t ptype);
uint32_t Save_settings_to_INI_file(uint8_t ptype);
uint32_t Save_settings_to_file(char *file_name, uint8_t *buf, ULONG buf_sz, uint8_t ptype, uint8_t compressed);
uint32_t Save_settings_to_DataFlash(uint8_t *buf, uint32_t buf_sz, uint8_t ptype);

uint32_t Save_settings(uint8_t ptype, uint8_t media_type, char *file_name);
uint32_t Restore_settings_from_JSON_file(uint8_t ptype, char *file_name);

uint32_t Delete_app_settings_file(uint8_t ptype);

uint32_t Accept_certificates_from_file(void);
uint32_t Check_settings_in_DataFlash(uint8_t ptype, T_settings_state *sstate);
void     Reset_settings_wr_counters(void);

uint32_t Save_buf_to_DataFlash(uint32_t start_addr, uint8_t *buf, uint32_t buf_sz);
uint32_t Restore_buf_from_DataFlash(uint32_t start_addr, uint8_t *buf, uint32_t buf_sz);

uint32_t Restore_NV_counters_from_DataFlash(void);
uint32_t Save_NV_counters_to_DataFlash();
uint32_t Save_NV_counters_to_NVRAM(void);
uint32_t Load_NV_counters_from_NVRAM(T_nv_counters_block *block_ptr);

// Function to get error description
const char *Get_settings_save_error_str(uint32_t err_code);
const char *Get_settings_restore_error_str(uint32_t err_code);
const char *Get_settings_area_error_str(uint32_t err_code);

#endif
