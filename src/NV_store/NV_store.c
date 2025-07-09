// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2018.09.03
// 23:06:41
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"
#include <stdarg.h>

#define INI_STR_SIZE 512
#define TMP_BUF_SZ   512

extern const T_NV_parameters_instance *Get_app_params_instance(void);

const uint32_t df_params_addr[PARAMS_TYPES_NUM][2] =
{
 {DATAFLASH_APP_PARAMS_1_ADDR, DATAFLASH_APP_PARAMS_2_ADDR}};

char *ini_fname[PARAMS_TYPES_NUM]        = {PARAMS_APP_INI_FILE_NAME};
char *ini_used_fname[PARAMS_TYPES_NUM]   = {PARAMS_APP_USED_INI_FILE_NAME};
char *json_fname[PARAMS_TYPES_NUM]       = {PARAMS_APP_JSON_FILE_NAME};
char *json_compr_fname[PARAMS_TYPES_NUM] = {PARAMS_APP_COMPR_JSON_FILE_NAME};

uint32_t g_setting_wr_counters[PARAMS_TYPES_NUM][2];        // Счетчики количества записей в каждую область
uint32_t g_settings_area_error_codes[PARAMS_TYPES_NUM][2];  // Регистры ошибок каждой области

static uint32_t Restore_settings_from_DataFlash(uint8_t ptype);

uint32_t            g_nv_counters_curr_addr;
T_nv_counters_block g_nv_cntc;
uint8_t             g_nv_ram_couners_valid;
uint8_t             g_dataflash_couners_valid;
/*-----------------------------------------------------------------------------------------------------


  \param ptype

  \return T_NV_parameters_instance*
-----------------------------------------------------------------------------------------------------*/
const T_NV_parameters_instance *Get_settings_instance(uint8_t ptype)
{
  switch (ptype)
  {
    case APPLICATION_PARAMS:
      return &wvar_inst;
      break;
    default:
      return NULL;
  }
}

/*-------------------------------------------------------------------------------------------
  Восстановление параметров по умолчанию, после сбоев системы или смены версии
---------------------------------------------------------------------------------------------*/
void Return_def_params(uint8_t ptype)
{
  uint16_t               i;
  const T_NV_parameters *pp;

  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (p_pars == 0) return;

  // Загрузить параметры значениями по умолчанию
  for (i = 0; i < p_pars->items_num; i++)
  {
    pp = &p_pars->items_array[i];

    if ((pp->attr & VAL_NOINIT) == 0)
    {
      if (pp->val == NULL)
      {
        APPLOG("Return_def_params: NULL val pointer for param %u", i);
        continue;
      }
      switch (pp->vartype)
      {
        case tint8u:
          *(uint8_t *)pp->val = (uint8_t)pp->defval;
          break;
        case tint16u:
          *(uint16_t *)pp->val = (uint16_t)pp->defval;
          break;
        case tint32u:
          *(uint32_t *)pp->val = (uint32_t)pp->defval;
          break;
        case tint32s:
          *(int32_t *)pp->val = (int32_t)pp->defval;
          break;
        case tfloat:
          *(float *)pp->val = (float)pp->defval;
          break;
        case tstring:
        {
          if (pp->pdefval == NULL || pp->varlen == 0)
          {
            APPLOG("Return_def_params: NULL pdefval or zero varlen for tstring param %u", i);
            break;
          }
          uint8_t *st;
          strncpy((char *)pp->val, (const char *)pp->pdefval, pp->varlen - 1);
          st                 = (uint8_t *)pp->val;
          st[pp->varlen - 1] = 0;
        }
        break;
        case tarrofbyte:
          if (pp->pdefval == NULL || pp->varlen == 0)
          {
            APPLOG("Return_def_params: NULL pdefval or zero varlen for tarrofbyte param %u", i);
            break;
          }
          memcpy(pp->val, pp->pdefval, pp->varlen);
          break;
        case tarrofdouble:
          // Не реализовано. Добавьте обработку при необходимости.
          APPLOG("Return_def_params: tarrofdouble not supported for param %u", i);
          break;
        default:
          APPLOG("Return_def_params: Unknown vartype %d for param %u", pp->vartype, i);
          break;
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Восстанавливает параметры приложения из доступных источников в порядке приоритета:
  1. INI-файл - используется для первичной настройки или обновления параметров
  2. JSON-файл - для восстановления параметров из резервной копии
  3. DataFlash - энергонезависимая память для хранения текущих настроек
  4. Значения по умолчанию - если все остальные источники недоступны или повреждены

  После успешного восстановления из файлов (INI или JSON) параметры сохраняются в DataFlash
  для повышения надежности и дальнейшего использования. Исходные файлы при этом либо
  переименовываются (INI), либо удаляются (JSON).

  \param ptype  Тип параметров (APPLICATION_PARAMS и др.)

  \return int32_t  RES_OK при успешном восстановлении параметров любым способом
-----------------------------------------------------------------------------------------------------*/
int32_t Restore_settings(uint8_t ptype)
{
  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (!p_pars) return RES_ERROR;

  // Загрузка параметров по умолчанию как база
  Return_def_params(ptype);

  // Пробуем восстановить из INI файла (высший приоритет)
  uint32_t res = Restore_settings_from_INI_file(ptype);
  if (res == RES_OK)
  {
    APPLOG("Settings restored from INI file for type=%d", ptype);

    // Сохраняем восстановленные параметры в DataFlash
    res = Save_settings(ptype, MEDIA_TYPE_DATAFLASH, 0);
    if (res == RES_OK)
    {
      APPLOG("Settings successfully saved to DataFlash");
    }
    else
    {
      APPLOG("Failed to save settings to DataFlash: %s", Get_settings_save_error_str(res));
    }

    // Переименовываем INI файл после применения
    fx_file_rename(&fat_fs_media, ini_fname[ptype], ini_used_fname[ptype]);
    fx_media_flush(&fat_fs_media);
    return RES_OK;
  }

  // Пробуем восстановить из JSON файла (второй приоритет)
  res = Restore_settings_from_JSON_file(ptype, 0);
  if (res == RES_OK)
  {
    APPLOG("Settings restored from JSON file for type=%d", ptype);

    // Сохраняем восстановленные параметры в DataFlash
    res = Save_settings(ptype, MEDIA_TYPE_DATAFLASH, 0);
    if (res == RES_OK)
    {
      APPLOG("Settings successfully saved to DataFlash");
    }
    else
    {
      APPLOG("Failed to save settings to DataFlash: %s", Get_settings_save_error_str(res));
    }

    // Удаляем JSON файл после применения
    Delete_app_settings_file(ptype);
    return RES_OK;
  }

  // Пробуем восстановить из DataFlash (третий приоритет)
  if (Restore_settings_from_DataFlash(ptype) == RES_OK)
  {
    APPLOG("Settings restored from DataFlash for type=%d", ptype);
    return RES_OK;
  }

  // Если все способы не сработали, используем параметры по умолчанию
  APPLOG("Failed to restore settings from any source, using default settings for type=%d", ptype);
  Return_def_params(ptype);

  // Сохраняем параметры по умолчанию в DataFlash
  res = Save_settings(ptype, MEDIA_TYPE_DATAFLASH, 0);
  if (res == RES_OK)
  {
    APPLOG("Default settings successfully saved to DataFlash");
  }
  else
  {
    APPLOG("Failed to save default settings to DataFlash: %s", Get_settings_save_error_str(res));
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Сохраняет настройки устройства в указанный носитель (файл или DataFlash) в формате JSON или сжатого JSON.
  Формат и способ хранения зависят от параметра media_type и настроек системы.

  Алгоритм работы:
  1. Сериализует настройки в строку JSON.
  2. Опционально сжимает данные, если это требуется.
  3. Сохраняет данные в файл или DataFlash в зависимости от media_type.

  \param ptype      Тип параметров (например, APPLICATION_PARAMS).
  \param media_type Тип носителя: MEDIA_TYPE_FILE (файл) или MEDIA_TYPE_DATAFLASH (энергонезависимая память).
  \param file_name  Имя файла для сохранения (NULL — использовать имя по умолчанию).

  \return RES_OK при успехе, RES_ERROR при ошибке.
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_settings(uint8_t ptype, uint8_t media_type, char *file_name)
{
  uint32_t res;
  char    *json_str = NULL;
  uint32_t json_str_sz;
  uint8_t *compessed_data_ptr = 0;
  uint32_t compessed_data_sz;
  uint8_t *buf;
  uint32_t buf_sz;
  uint32_t flags;

  // Извлекаем все необходимые значения из wvar один раз в начале функции
  uint8_t en_formated_settings           = wvar.en_formated_settings;
  uint8_t en_compress_settins            = wvar.en_compress_settins;

  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (p_pars == 0) return RES_ERROR;

  if (media_type == MEDIA_TYPE_DATAFLASH)
  {
    flags = JSON_COMPACT;
  }
  else if (en_formated_settings)
  {
    flags = JSON_INDENT(1) | JSON_ENSURE_ASCII;
  }
  else
  {
    flags = JSON_COMPACT;
  }

  if (Serialze_settings_to_mem(p_pars, &json_str, &json_str_sz, flags) != RES_OK) goto EXIT_ON_ERROR;

  if (en_compress_settins || (media_type == MEDIA_TYPE_DATAFLASH))
  {
    // Выделить память для сжатого файла
    compessed_data_ptr = NV_MALLOC_PENDING(json_str_sz, 10);
    if (compessed_data_ptr == NULL) goto EXIT_ON_ERROR;

    compessed_data_sz = json_str_sz;
    res               = Compress_mem_to_mem(COMPR_ALG_SIXPACK, json_str, json_str_sz, compessed_data_ptr, &compessed_data_sz);
    if (res != RES_OK) goto EXIT_ON_ERROR;
    // Добавляем контрольную сумму
    uint16_t crc    = Get_CRC16_of_block(compessed_data_ptr, compessed_data_sz, 0xFFFF);
    buf             = compessed_data_ptr;
    buf_sz          = compessed_data_sz;
    buf[buf_sz]     = crc & 0xFF;
    buf[buf_sz + 1] = (crc >> 8) & 0xFF;
    buf_sz += 2;
  }
  else
  {
    buf    = (uint8_t *)json_str;
    buf_sz = json_str_sz;
  }

  res = RES_OK;
  switch (media_type)
  {
    case MEDIA_TYPE_FILE:
      res = Save_settings_to_file(file_name, buf, buf_sz, ptype, en_compress_settins);
      break;
    case MEDIA_TYPE_DATAFLASH:
      res = Save_settings_to_DataFlash(buf, buf_sz, ptype);
      break;
  }

  NV_MEM_FREE(compessed_data_ptr);
  if (json_str != 0) NV_MEM_FREE(json_str);
  return res;
EXIT_ON_ERROR:
  NV_MEM_FREE(compessed_data_ptr);
  if (json_str != 0) NV_MEM_FREE(json_str);
  return RES_ERROR;
}

/*-------------------------------------------------------------------------------------------
  Восстановить серийный номер, MAC адрес и другие постоянные величины из файла
---------------------------------------------------------------------------------------------*/
uint32_t Restore_settings_from_INI_file(uint8_t ptype)
{
  if (g_file_system_ready == 0) return RES_ERROR;
  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (!p_pars) return RES_ERROR;

  char *str   = NV_MALLOC_PENDING(INI_STR_SIZE + 1, 10);
  char *inbuf = NV_MALLOC_PENDING(TMP_BUF_SZ + 1, 10);
  if (!str || !inbuf)
  {
    NV_MEM_FREE(str);
    NV_MEM_FREE(inbuf);
    return RES_ERROR;
  }

  FX_FILE f;
  if (fx_file_open(&fat_fs_media, &f, ini_fname[ptype], FX_OPEN_FOR_READ) != FX_SUCCESS)
  {
    NV_MEM_FREE(str);
    NV_MEM_FREE(inbuf);
    return RES_ERROR;
  }

  uint32_t pcnt = 0;
  while (1)
  {
    int32_t scan_res;
    if (Scanf_from_file(&f, &scan_res, inbuf, TMP_BUF_SZ, "%s\n", str) == RES_ERROR) break;
    if (scan_res != 1) continue;
    if (str[0] == 0 || str[0] == ';') continue;

    char *val = strchr(str, '=');
    if (!val) continue;
    *val = 0;
    val++;

    char   *var_alias = Trim_and_dequote_str(str);
    int32_t n         = Find_param_by_alias(p_pars, var_alias);
    if (n < 0) continue;

    val = Trim_and_dequote_str(val);
    Convert_str_to_parameter(p_pars, (uint8_t *)val, n);
    pcnt++;
  }

  fx_file_close(&f);
  NV_MEM_FREE(str);
  NV_MEM_FREE(inbuf);
  return RES_OK;
}

/*-------------------------------------------------------------------------------------------
   Процедура сохранения в ini-файл параметров
---------------------------------------------------------------------------------------------*/
uint32_t Save_settings_to_INI_file(uint8_t ptype)
{
  if (g_file_system_ready == 0) return RES_ERROR;
  const T_NV_parameters_instance *p_pars = Get_settings_instance(ptype);
  if (!p_pars) return RES_ERROR;

  char *str   = NV_MALLOC_PENDING(INI_STR_SIZE + 1, 10);
  char *inbuf = NV_MALLOC_PENDING(TMP_BUF_SZ + 1, 10);
  if (!str || !inbuf)
  {
    NV_MEM_FREE(str);
    NV_MEM_FREE(inbuf);
    return RES_ERROR;
  }

  FX_FILE f;
  if (Recreate_file_for_write(&f, ini_fname[ptype]) != RES_OK)
  {
    NV_MEM_FREE(str);
    NV_MEM_FREE(inbuf);
    return RES_ERROR;
  }

  char    *prev_name = NULL;
  uint32_t offs      = 0;
  uint32_t n         = 0;

  for (uint32_t i = 0; i < p_pars->items_num; i++)
  {
    const T_NV_parameters *pp = &p_pars->items_array[i];
    if ((pp->attr & 1) != 0) continue;  // сохраняем только если параметр для записи

    offs       = 0;
    char *name = (char *)Get_mn_name(p_pars, pp->parmnlev);
    if (name != prev_name)
    {
      sprintf(inbuf + offs, ";--------------------------------------------------------\r\n");
      offs += strlen(inbuf + offs);
      sprintf(inbuf + offs, "; %s\r\n", name);
      offs += strlen(inbuf + offs);
      sprintf(inbuf + offs, ";--------------------------------------------------------\r\n\r\n");
      offs += strlen(inbuf + offs);
      prev_name = name;
    }
    sprintf(inbuf + offs, "; N=%03d %s\r\n%s=", n++, pp->var_description, pp->var_alias);
    offs += strlen(inbuf + offs);

    char *tmp_str;
    if (pp->vartype == tstring)
    {
      Convert_parameter_to_str(p_pars, (uint8_t *)(str + 1), MAX_PARAMETER_STRING_LEN, i);
      *str     = '"';
      tmp_str  = str + strlen(str);
      *tmp_str = '"';
      tmp_str++;
    }
    else
    {
      Convert_parameter_to_str(p_pars, (uint8_t *)str, MAX_PARAMETER_STRING_LEN, i);
      tmp_str = str + strlen(str);
    }
    *tmp_str++ = '\r';
    *tmp_str++ = '\n';
    *tmp_str++ = '\r';
    *tmp_str++ = '\n';
    *tmp_str   = '\0';
    sprintf(inbuf + offs, "%s", str);
    offs += strlen(inbuf + offs);

    if (fx_file_write(&f, inbuf, offs) != FX_SUCCESS)
    {
      fx_file_close(&f);
      NV_MEM_FREE(str);
      NV_MEM_FREE(inbuf);
      return RES_ERROR;
    }
  }

  fx_file_close(&f);
  fx_media_flush(&fat_fs_media);
  NV_MEM_FREE(str);
  NV_MEM_FREE(inbuf);
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Сохраняет буфер с настройками в файл (JSON или сжатый JSON).
  Если имя файла не задано, используется имя по умолчанию для типа параметров и формата.


  \param file_name  Имя файла для сохранения (или NULL для имени по умолчанию)
  \param buf        Указатель на буфер с данными
  \param buf_sz     Размер буфера данных
  \param ptype      Тип параметров (APPLICATION_PARAMS и др.)
  \param compressed Флаг: 1 - сохранять как сжатый JSON, 0 - обычный JSON

  \return RES_OK при успехе, RES_ERROR при ошибке
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_settings_to_file(char *file_name, uint8_t *buf, ULONG buf_sz, uint8_t ptype, uint8_t compressed)
{
  FX_FILE f;
  char   *fname;

  if (g_file_system_ready == 0) return RES_ERROR;
  if (ptype >= PARAMS_TYPES_NUM) return RES_ERROR;
  if (buf == NULL) return RES_ERROR;

  f.fx_file_id = 0;

  // Явный выбор имени файла
  if (file_name != NULL)
  {
    fname = file_name;
  }
  else
  {
    fname = compressed ? json_compr_fname[ptype] : json_fname[ptype];
  }

  if (Recreate_file_for_write(&f, fname) != RES_OK) goto error;
  if (fx_file_write(&f, buf, buf_sz) != FX_SUCCESS) goto error;

  fx_file_close(&f);
  fx_media_flush(&fat_fs_media);
  return RES_OK;

error:
  if (f.fx_file_id == FX_FILE_ID) fx_file_close(&f);
  fx_media_flush(&fat_fs_media);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Сохраняет настройки в две идентичные области памяти DataFlash для обеспечения надежности хранения.

  Структура данных в DataFlash:
  +-------------+--------------------------------------------------------------+
  | Смещение    | Описание                                                     |
  +-------------+--------------------------------------------------------------+
  | 0x00-0x03   | Размер сжатых данных JSON (N байт)                           |
  | 0x04-0x07   | Счетчик модификаций данных (g_setting_wr_counters[ptype][i]) |
  | 0x08-0x07+N | Сжатые данные JSON                                           |
  | 0x08+N-0x0B+N | Контрольная сумма CRC16 (дублируется в двух 16-битных словах)|
  +-------------+--------------------------------------------------------------+

  \param buf    - Указатель на буфер с сжатыми данными JSON
  \param buf_sz - Размер сжатых данных в байтах
  \param ptype  - Тип параметров (APPLICATION_PARAMS)

  \return RES_OK при успешном выполнении, или код ошибки (см. коды NV_ERR_*)
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_settings_to_DataFlash(uint8_t *buf, uint32_t buf_sz, uint8_t ptype)
{
  uint8_t *tmp_buf = 0;
  uint32_t tmp_buf_sz;
  uint16_t crc;
  uint32_t csz;
  uint32_t i;
  uint32_t err_code = 0;

  // Проверка входных параметров
  if (ptype >= PARAMS_TYPES_NUM) return NV_ERR_INVALID_TYPE;
  if (buf_sz > (DATAFLASH_PARAMS_AREA_SIZE - DATAFLASH_SUPLIMENT_AREA_SIZE)) return NV_ERR_BUFFER_TOO_LARGE;
  if (buf_sz < 8) return NV_ERR_BUFFER_TOO_SMALL;

  // Выравнивание размера буфера по границе 4 байт
  csz = buf_sz;
  if ((csz & 0x3) != 0) csz = (csz & 0xFFFFFFFC) + 4;

  // Размер буфера с учетом заголовка и CRC
  tmp_buf_sz = csz + DATAFLASH_SUPLIMENT_AREA_SIZE;

  // Выделение временного буфера
  tmp_buf    = NV_MALLOC_PENDING(tmp_buf_sz, 10);
  if (tmp_buf == NULL)
  {
    err_code = NV_ERR_MEMORY_ALLOCATION;
    goto EXIT_WITH_LOG;
  }

  // Инициализация буфера нулями (для неиспользуемой области)
  memset(tmp_buf, 0, tmp_buf_sz);

  // Записываем размер данных
  memcpy(&tmp_buf[0], &csz, 4);

  // Копируем данные
  memcpy(&tmp_buf[8], buf, buf_sz);

  // Записываем в обе области памяти
  for (i = 0; i < 2; i++)
  {
    // Стираем область памяти
    if (DataFlash_bgo_EraseArea(df_params_addr[ptype][i], DATAFLASH_PARAMS_AREA_SIZE) != RES_OK)
    {
      err_code = (i == 0) ? NV_ERR_ERASE_AREA1 : NV_ERR_ERASE_AREA2;
      goto EXIT_WITH_LOG;
    }

    // Обновляем счетчик записей и записываем его в буфер
    g_setting_wr_counters[ptype][i]++;
    memcpy(&tmp_buf[4], &g_setting_wr_counters[ptype][i], 4);

    // Вычисляем CRC16 для буфера (исключая поле CRC)
    crc = Get_CRC16_of_block(tmp_buf, tmp_buf_sz - 4, 0xFFFF);

    // Сохраняем CRC в конце буфера (дублируем для надежности)
    memcpy(&tmp_buf[8 + csz], &crc, 2);
    memcpy(&tmp_buf[10 + csz], &crc, 2);

    // Записываем буфер в DataFlash
    if (DataFlash_bgo_WriteArea(df_params_addr[ptype][i], tmp_buf, tmp_buf_sz) != RES_OK)
    {
      err_code = (i == 0) ? NV_ERR_WRITE_AREA1 : NV_ERR_WRITE_AREA2;
      goto EXIT_WITH_LOG;
    }
  }

  NV_MEM_FREE(tmp_buf);
  APPLOG("Settings saved to DataFlash: size=%d bytes, write counters=[%d, %d]",
         buf_sz, g_setting_wr_counters[ptype][0], g_setting_wr_counters[ptype][1]);
  return RES_OK;

EXIT_WITH_LOG:
  NV_MEM_FREE(tmp_buf);
  APPLOG("Failed to save settings to DataFlash: %s (code: %d)", Get_settings_save_error_str(err_code), err_code);
  return err_code;
}

/*-----------------------------------------------------------------------------------------------------
  Восстановление установок из JSON файла, опционально с декомпрессией.
  Если имя файла не задано, то приеняется имена по умолчанию


  \param p_pars
  \param file_name
  \param ptype        - тип параметров

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Restore_settings_from_JSON_file(uint8_t ptype, char *file_name)
{
  uint8_t *data_buf = NULL;
  uint32_t data_buf_sz;
  uint8_t *decompessed_data_ptr = NULL;
  uint32_t decompessed_data_sz;
  FX_FILE *pf = NULL;

  uint32_t res;
  uint32_t actual_sz;
  uint8_t  compressed = 0;

  if (g_file_system_ready == 0) return RES_ERROR;
  if (ptype >= PARAMS_TYPES_NUM) return RES_ERROR;

  pf = NV_MALLOC_PENDING(sizeof(FX_FILE), 10);
  if (pf == NULL) goto EXIT_ON_ERROR;

  // Открываем файл
  if (file_name == 0)
  {
    file_name = json_compr_fname[ptype];
    res       = fx_file_open(&fat_fs_media, pf, file_name, FX_OPEN_FOR_READ);
    if (res != FX_SUCCESS)
    {
      file_name = json_fname[ptype];
      res       = fx_file_open(&fat_fs_media, pf, file_name, FX_OPEN_FOR_READ);
      if (res != FX_SUCCESS) goto EXIT_ON_ERROR;
    }
    else
      compressed = 1;
  }
  else
  {
    res = fx_file_open(&fat_fs_media, pf, file_name, FX_OPEN_FOR_READ);
    if (res != FX_SUCCESS) goto EXIT_ON_ERROR;
  }

  data_buf_sz = pf->fx_file_current_file_size;
  data_buf    = NV_MALLOC_PENDING(data_buf_sz + 1, 10);
  if (data_buf == NULL) goto EXIT_ON_ERROR;

  actual_sz = 0;
  res       = fx_file_read(pf, data_buf, data_buf_sz, (ULONG *)&actual_sz);
  if ((res != FX_SUCCESS) || (actual_sz != data_buf_sz)) goto EXIT_ON_ERROR;
  fx_file_close(pf);

  if (compressed)
  {
    // Проверка контрольной суммы
    uint16_t crc  = Get_CRC16_of_block(data_buf, data_buf_sz - 2, 0xFFFF);
    uint16_t ecrc = data_buf[data_buf_sz - 2] + (data_buf[data_buf_sz - 1] << 8);
    if (crc != ecrc) goto EXIT_ON_ERROR;                  // Выход если не совпала контрольная сумма

    decompessed_data_sz = data_buf[0] + (data_buf[1] << 8) + (data_buf[2] << 16) + (data_buf[3] << 24);
    if (decompessed_data_sz > 65536) goto EXIT_ON_ERROR;  // Выход если после декомпрессии объем данных слишком большой
    decompessed_data_ptr = NV_MALLOC_PENDING(decompessed_data_sz + 1, 10);
    if (decompessed_data_ptr == NULL) goto EXIT_ON_ERROR;
    // Декомпрессия
    if (Decompress_mem_to_mem(COMPR_ALG_SIXPACK, data_buf, data_buf_sz - 2, decompessed_data_ptr, decompessed_data_sz) != decompessed_data_sz) goto EXIT_ON_ERROR;

    NV_MEM_FREE(data_buf);
    data_buf             = 0;
    data_buf             = decompessed_data_ptr;
    data_buf_sz          = decompessed_data_sz;
    decompessed_data_ptr = 0;
  }

  data_buf[data_buf_sz] = 0;  // Дополняем строку завершающим нулем

  // Парсим JSON формат данных
  res                   = JSON_Deser_settings(ptype, (char *)data_buf);
  if (res == RES_OK)
  {
    fx_file_delete(&fat_fs_media, file_name);
  }
  NV_MEM_FREE(decompessed_data_ptr);
  NV_MEM_FREE(data_buf);
  NV_MEM_FREE(pf);
  return res;

EXIT_ON_ERROR:
  if (pf->fx_file_id == FX_FILE_ID) fx_file_close(pf);
  NV_MEM_FREE(decompessed_data_ptr);
  NV_MEM_FREE(data_buf);
  NV_MEM_FREE(pf);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Восстановление параметров из энергонезависимой памяти DataFlash.

  Функция анализирует две зарезервированные области DataFlash для обеспечения надежности хранения
  настроек. Данные хранятся в каждой области в сжатом формате JSON с контрольной суммой.

  Алгоритм работы:
  1. Проверка валидности входного параметра (ptype)
  2. Последовательное сканирование двух областей DataFlash:
     - Чтение размера данных из заголовка
     - Проверка корректности размера
     - Чтение блока данных в буфер
     - Проверка контрольной суммы CRC16
     - Декомпрессия данных из формата SIXPACK
     - Десериализация JSON-данных в структуры настроек
     - При первой успешной десериализации процесс завершается
  3. Если обе области содержат корректные данные, используется первая область
  4. Для каждой области ведется журнал ошибок в массиве g_settings_area_error_codes:
     - 0: Нет ошибок
     - 1: Неверный размер данных
     - 2: Ошибка контрольной суммы
     - 3: Слишком большой размер декомпрессированных данных
     - 4: Ошибка выделения памяти
     - 5: Ошибка декомпрессии
     - 6: Ошибка десериализации JSON
     - 7: Ошибка чтения данных из DataFlash

  Структура данных в DataFlash:
  +-------------+--------------------------------------------------------------+
  | Смещение    | Описание                                                     |
  +-------------+--------------------------------------------------------------+
  | 0x00-0x03   | Размер сжатых данных (N байт)                                |
  | 0x04-0x07   | Счетчик модификаций данных (g_setting_wr_counters[ptype][i]) |
  | 0x08-0x07+N | Сжатые данные в формате JSON                                 |
  | 0x08+N-0x0B+N | Контрольная сумма CRC16 (дублируется в двух 16-битных словах)|
  +-------------+--------------------------------------------------------------+

  Механизм защиты от сбоев:
  - Данные хранятся в двух идентичных областях для обеспечения надежности
  - Каждая область имеет свой счетчик модификаций для отслеживания актуальности
  - Используется контрольная сумма CRC16 для проверки целостности данных
  - При обнаружении ошибок в одной области используется резервная

  Коды возврата:
  - RES_OK: Успешное восстановление настроек из DataFlash
  - RES_ERROR: Ошибка восстановления (детали в журнале ошибок g_settings_area_error_codes)

  Коды ошибок (см. константы DF_RESTORE_ERR_*):
  - DF_RESTORE_ERR_NONE (0): Нет ошибки
  - DF_RESTORE_ERR_INVALID_TYPE (1): Неверный тип параметров
  - DF_RESTORE_ERR_NO_VALID_DATA (2): Отсутствуют валидные данные в DataFlash

  \param ptype  - Тип параметров (APPLICATION_PARAMS)

  \return RES_OK при успешном восстановлении из DataFlash, или RES_ERROR при ошибке
-----------------------------------------------------------------------------------------------------*/
static uint32_t Restore_settings_from_DataFlash(uint8_t ptype)
{
  uint32_t done = 0;
  uint32_t sz, flash_addr, buf_sz;
  uint16_t crc1, crc2;
  uint8_t *buf               = NULL;
  uint8_t *decompressed_data = NULL;
  uint32_t decompressed_size;
  uint32_t err_code = DF_RESTORE_ERR_NONE;

  // Проверка входного параметра
  if (ptype >= PARAMS_TYPES_NUM)
  {
    err_code = DF_RESTORE_ERR_INVALID_TYPE;
    goto EXIT_WITH_LOG;
  }

  // Проходим по двум областям DataFlash в поисках валидных данных
  for (uint32_t i = 0; i < 2; i++)
  {
    g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_NONE;
    flash_addr                            = df_params_addr[ptype][i];

    // Читаем размер данных из заголовка области
    if (DataFlash_bgo_ReadArea(flash_addr, (uint8_t *)&sz, 4) != RES_OK)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_READ_DATA;
      continue;
    }

    // Проверка размера данных
    if ((sz > (DATAFLASH_PARAMS_AREA_SIZE - DATAFLASH_SUPLIMENT_AREA_SIZE)) || (sz < 8))
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_WRONG_SIZE;
      continue;
    }

    // Выделяем память для чтения сжатых данных с заголовком (размер + счетчик записей)
    buf_sz = sz + 8;
    buf    = NV_MALLOC_PENDING(buf_sz, 10);
    if (buf == NULL)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_MEM_ALLOC;
      continue;
    }

    // Читаем данные из DataFlash
    if (DataFlash_bgo_ReadArea(flash_addr, buf, buf_sz) != RES_OK)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_READ_DATA;
      NV_MEM_FREE(buf);
      continue;
    }

    // Читаем и проверяем контрольную сумму
    if (DataFlash_bgo_ReadArea(flash_addr + 8 + sz, (uint8_t *)&crc1, 4) != RES_OK)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_READ_DATA;
      NV_MEM_FREE(buf);
      continue;
    }

    crc2 = Get_CRC16_of_block(buf, buf_sz, 0xFFFF);
    if (crc1 != crc2)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_CRC_MISMATCH;
      NV_MEM_FREE(buf);
      continue;
    }

    // Получаем размер декомпрессированных данных
    memcpy(&decompressed_size, &buf[8], 4);
    if (decompressed_size >= 65536)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_DECOMP_SIZE;
      NV_MEM_FREE(buf);
      continue;
    }

    // Выделяем память для декомпрессированных данных
    decompressed_data = NV_MALLOC_PENDING(decompressed_size + 1, 10);
    if (decompressed_data == NULL)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_MEM_ALLOC;
      NV_MEM_FREE(buf);
      continue;
    }

    // Выполняем декомпрессию
    if (Decompress_mem_to_mem(COMPR_ALG_SIXPACK, &buf[8], sz, decompressed_data, decompressed_size) != decompressed_size)
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_DECOMPRESS;
      NV_MEM_FREE(decompressed_data);
      NV_MEM_FREE(buf);
      continue;
    }

    // Сохраняем счетчик записей для этой области
    memcpy(&g_setting_wr_counters[ptype][i], &buf[4], 4);

    // Освобождаем буфер сжатых данных, который больше не нужен
    NV_MEM_FREE(buf);
    buf                                  = NULL;

    // Добавляем завершающий ноль для строки JSON
    decompressed_data[decompressed_size] = 0;

    // Десериализуем JSON в структуры настроек
    if (JSON_Deser_settings(ptype, (char *)decompressed_data) == RES_OK)
    {
      done = 1;  // Успешно восстановили настройки
      NV_MEM_FREE(decompressed_data);
      break;
    }
    else
    {
      g_settings_area_error_codes[ptype][i] = DF_AREA_ERR_JSON_PARSE;
      NV_MEM_FREE(decompressed_data);
    }
  }

  // Проверяем, удалось ли восстановить настройки
  if (done == 0)
  {
    err_code = DF_RESTORE_ERR_NO_VALID_DATA;
    goto EXIT_WITH_LOG;
  }

  APPLOG("Successfully restored settings from DataFlash for type=%d, counters=[%d, %d]",
         ptype, g_setting_wr_counters[ptype][0], g_setting_wr_counters[ptype][1]);
  return RES_OK;

EXIT_WITH_LOG:
  APPLOG("Failed to restore settings from DataFlash: %s (code: %d)",
         Get_settings_restore_error_str(err_code), err_code);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Проверяет состояние сохраненных настроек в DataFlash памяти.

  Функция анализирует обе области памяти DataFlash для указанного типа параметров (ptype),
  и заполняет структуру T_settings_state информацией о состоянии этих областей:
  - Проверяет, является ли область пустой (стертой)
  - Проверяет размер сохраненных данных
  - Проверяет контрольную сумму данных
  - Считывает счетчик записей для каждой области
  - Определяет общее состояние каждой области (пусто, ошибка CRC, некорректный размер и т.д.)

  Используется для диагностики состояния сохраненных настроек.

  \param ptype  - Тип параметров (APPLICATION_PARAMS)
  \param sstate - Указатель на структуру T_settings_state для сохранения результатов проверки

  \return RES_OK при успешном выполнении или RES_ERROR при ошибке
-----------------------------------------------------------------------------------------------------*/
uint32_t Check_settings_in_DataFlash(uint8_t ptype, T_settings_state *sstate)
{
  // Проверка входных параметров
  if (ptype >= PARAMS_TYPES_NUM) return RES_ERROR;
  if (sstate == NULL) return RES_ERROR;

  for (uint32_t i = 0; i < 2; i++)
  {
    sstate->area_start_condition[i] = g_settings_area_error_codes[ptype][i];

    uint32_t flash_addr             = df_params_addr[ptype][i];

    if (DataFlash_bgo_BlankCheck(flash_addr, 8) == RES_OK)
    {
      sstate->area_sz[i]     = 0;
      sstate->area_state[i]  = SETT_IS_BLANK;
      sstate->area_wr_cnt[i] = 0;
    }
    else
    {
      // Читаем из DataFlash размер блока данных
      uint32_t sz;
      DataFlash_bgo_ReadArea(flash_addr, (uint8_t *)&sz, 4);
      DataFlash_bgo_ReadArea(flash_addr + 4, (uint8_t *)&sstate->area_wr_cnt[i], 4);

      if ((sz > (DATAFLASH_PARAMS_AREA_SIZE - DATAFLASH_SUPLIMENT_AREA_SIZE)) || (sz < 8))
      {
        sstate->area_sz[i]    = sz;
        sstate->area_state[i] = SETT_WRONG_SIZE;  // Ошибка - неправильный размер данных
      }
      else
      {
        uint32_t buf_sz    = sz + 8;
        sstate->area_sz[i] = buf_sz;
        uint8_t *buf       = NV_MALLOC_PENDING(buf_sz, 10);
        if (buf != NULL)
        {
          // Читаем данные
          DataFlash_bgo_ReadArea(flash_addr, (uint8_t *)buf, buf_sz);
          // Читаем записанную контрольную сумму
          uint16_t crc1, crc2;
          DataFlash_bgo_ReadArea(flash_addr + 8 + sz, (uint8_t *)&crc1, 4);
          // Расчитываем фактическую контрольную сумму
          crc2 = Get_CRC16_of_block(buf, buf_sz, 0xFFFF);
          if (crc1 != crc2)
          {
            sstate->area_state[i] = SETT_WRONG_CRC;
          }
          else
          {
            sstate->area_state[i] = SETT_OK;
          }
          NV_MEM_FREE(buf);
        }
        else
        {
          sstate->area_state[i] = SETT_WRONG_CHECK;  // Ошибка выделения памяти
        }
      }
    }
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------*/
void Reset_settings_wr_counters(void)
{
  for (uint32_t i = 0; i < PARAMS_TYPES_NUM; i++)
  {
    for (uint32_t j = 0; j < 2; j++)
    {
      g_setting_wr_counters[i][j] = 0;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Принять сертификат из файла

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Accept_certificates_from_file(void)
{
  uint8_t *data_buf = NULL;
  uint32_t file_size;
  FX_FILE  f;
  uint32_t res;
  uint32_t actual_sz;
  uint8_t *ptr_sz;

  res = fx_file_open(&fat_fs_media, &f, CA_CERTIFICATE_FILE_NAME, FX_OPEN_FOR_READ);
  if (res != FX_SUCCESS)
  {
    goto EXIT_ON_ERROR;
  }

  file_size = f.fx_file_current_file_size;
  if (file_size > (DATAFLASH_CA_CERT_AREA_SIZE - 4)) goto EXIT_ON_ERROR;
  data_buf = NV_MALLOC_PENDING(DATAFLASH_CA_CERT_AREA_SIZE, 10);
  if (data_buf == NULL) goto EXIT_ON_ERROR;

  res = fx_file_read(&f, data_buf, file_size, (ULONG *)&actual_sz);
  if ((res != FX_SUCCESS) || (actual_sz != file_size)) goto EXIT_ON_ERROR;
  fx_file_close(&f);

  ptr_sz = (uint8_t *)(&data_buf[DATAFLASH_CA_CERT_AREA_SIZE - 4]);
  memcpy(ptr_sz, &file_size, 4);
  if (DataFlash_bgo_EraseArea(DATAFLASH_CA_CERT_ADDR, DATAFLASH_CA_CERT_AREA_SIZE) != RES_OK) goto EXIT_ON_ERROR;
  if (DataFlash_bgo_WriteArea(DATAFLASH_CA_CERT_ADDR, data_buf, DATAFLASH_CA_CERT_AREA_SIZE) != RES_OK) goto EXIT_ON_ERROR;

  NV_MEM_FREE(data_buf);
  fx_file_delete(&fat_fs_media, CA_CERTIFICATE_FILE_NAME);
  return res;

EXIT_ON_ERROR:
  if (f.fx_file_id == FX_FILE_ID) fx_file_close(&f);
  NV_MEM_FREE(data_buf);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
uint32_t Delete_app_settings_file(uint8_t ptype)
{
  if (ptype >= PARAMS_TYPES_NUM) return RES_ERROR;

  fx_file_delete(&fat_fs_media, json_fname[ptype]);
  fx_file_delete(&fat_fs_media, json_compr_fname[ptype]);

  fx_media_flush(&fat_fs_media);
  Wait_ms(10);
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------


  \param buf
  \param buf_sz
  \param ptype

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_buf_to_DataFlash(uint32_t start_addr, uint8_t *buf, uint32_t buf_sz)
{
  uint8_t *tmp_buf = 0;
  uint32_t norm_sz;

  // Выравниваем размер буфера данных по 64, поскольку минимальный стрираемый блок имеет размер 64 байта
  norm_sz = buf_sz;
  if ((norm_sz & 0x3F) != 0) norm_sz = (norm_sz & 0xFFFFFFC0) + 64;

  // Стираем область памяти куда будем программировать данные
  if (DataFlash_bgo_EraseArea(start_addr, norm_sz) != RES_OK) goto EXIT_ON_ERROR;

  // Выравниваем размер буфера данных по 4, поскольку минимальный программируемый блок имеет размер 4 байта
  norm_sz = buf_sz;
  if ((norm_sz & 0x3) != 0) norm_sz = (norm_sz & 0xFFFFFFFC) + 4;

  // Выделяем буфер во внутренней RAM куда перепишем целевые данные и откуда будем программировать
  tmp_buf = NV_MALLOC_PENDING(norm_sz, 10);
  if (tmp_buf == NULL) goto EXIT_ON_ERROR;
  memset(tmp_buf + buf_sz, 0xFF, norm_sz - buf_sz);  // Избыточное пространство заполняем 0xFF
  memcpy(tmp_buf, buf, buf_sz);

  if (DataFlash_bgo_WriteArea(start_addr, tmp_buf, norm_sz) != RES_OK) goto EXIT_ON_ERROR;

  NV_MEM_FREE(tmp_buf);
  return RES_OK;
EXIT_ON_ERROR:
  NV_MEM_FREE(tmp_buf);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------


  \param start_addr
  \param buf
  \param buf_sz

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Restore_buf_from_DataFlash(uint32_t start_addr, uint8_t *buf, uint32_t buf_sz)
{
  return DataFlash_bgo_ReadArea(start_addr, buf, buf_sz);
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
static uint32_t Move_NV_couters_addr_forward_circular(void)
{
  g_nv_counters_curr_addr += NV_COUNTERS_BLOCK_SZ;
  if (g_nv_counters_curr_addr >= DATAFLASH_COUNTERS_DATA_ADDR + DATAFLASH_NV_COUNTERS_AREA_SIZE)
  {
    g_nv_counters_curr_addr = DATAFLASH_COUNTERS_DATA_ADDR;
    return 1;
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
static uint32_t Get_NV_couters_prev_addr(void)
{
  uint32_t addr;
  addr = g_nv_counters_curr_addr - NV_COUNTERS_BLOCK_SZ;
  if (addr < DATAFLASH_COUNTERS_DATA_ADDR)
  {
    addr = DATAFLASH_COUNTERS_DATA_ADDR + DATAFLASH_NV_COUNTERS_AREA_SIZE - NV_COUNTERS_BLOCK_SZ;
  }
  return addr;
}

/*-----------------------------------------------------------------------------------------------------


  \param data
  \param max_sz

  \return __weak void
-----------------------------------------------------------------------------------------------------*/
__weak void Get_App_persistent_counters(uint8_t *data, uint32_t max_sz)
{
}

/*-----------------------------------------------------------------------------------------------------


  \param data
  \param max_sz

  \return __weak void
-----------------------------------------------------------------------------------------------------*/
__weak void Set_App_persistent_counters(uint8_t *data, uint32_t max_sz)
{
}

/*-----------------------------------------------------------------------------------------------------
  Восстановление блока энергонезависмых счетчиков
  Блок записывается по кольцу в заданную область DataFlash микроконтроллера чтобы увеличить ресурс записей

  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Restore_NV_counters_from_DataFlash(void)
{
  uint32_t            crc;
  T_nv_counters_block nv_ram_block;  // Используем локальную переменную для чтения из NVRAM

  g_nv_ram_couners_valid    = 0;
  g_dataflash_couners_valid = 0;

  // Сначала проверяем нет ли актуальной записи в NV RAM (Secure Standby SRAM)
  if (Load_NV_counters_from_NVRAM(&nv_ram_block) == RES_OK)
  {
    g_nv_ram_couners_valid = 1;
  }

  // Двигаемся к актуальному блоку в цепочке в DataFlash
  g_nv_counters_curr_addr = DATAFLASH_COUNTERS_DATA_ADDR;
  do
  {
    if (DataFlash_bgo_BlankCheck(g_nv_counters_curr_addr, NV_COUNTERS_BLOCK_SZ) != RES_OK)
    {
      DataFlash_bgo_ReadArea(g_nv_counters_curr_addr, (uint8_t *)&g_nv_cntc, NV_COUNTERS_BLOCK_SZ);
      crc = Get_CRC16_of_block(&g_nv_cntc, NV_COUNTERS_BLOCK_SZ - 4, 0xFFFF);
      if (crc != g_nv_cntc.crc)
      {
        // Если область содержит неверный CRC, то сбрасываем счетчики и начинаем сначала
        goto err;
      }
      g_dataflash_couners_valid = 1;
      break;
    }

    // Находим адрес следующего  блока счетчиков
    if (Move_NV_couters_addr_forward_circular() == 1)
    {
      // Прошли всю область предназначенную для записи счетчиков и не нашли ни одной с записанными данными
      goto err;
    }

  } while (1);

  // Используем данные из NV RAM если они валидные и новее чем в DataFlash или если DataFlash не валиден
  if (g_nv_ram_couners_valid && (!g_dataflash_couners_valid || (nv_ram_block.sys.accumulated_work_time >= g_nv_cntc.sys.accumulated_work_time)))
  {
    memcpy(&g_nv_cntc, &nv_ram_block, sizeof(g_nv_cntc));
  }

  g_nv_cntc.sys.reboot_cnt++;  // Увеличиваем счетчик сбросов

  Move_NV_couters_addr_forward_circular();

  Set_App_persistent_counters(g_nv_cntc.data, APP_NV_COUNTERS_SZ);

  Save_NV_counters_to_DataFlash();

  return RES_OK;

err:
  // В цепочке обнаружена ошибка либо цепочка отсутствует
  // Инициализируем первую запись
  memset(&g_nv_cntc, 0, sizeof(g_nv_cntc));
  g_nv_counters_curr_addr             = DATAFLASH_COUNTERS_DATA_ADDR;
  g_nv_cntc.sys.reboot_cnt            = 1;
  g_nv_cntc.sys.accumulated_work_time = 0;
  // Очищаем текущий блок счетчиков
  Save_NV_counters_to_DataFlash();

  Set_App_persistent_counters(g_nv_cntc.data, APP_NV_COUNTERS_SZ);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Процедура сохранения NV данных в DataFlash микроконтроллера
  Вызывается приблизительно каждый час, чтобы не зависеть от способности внешних часов реального времени удерживать данные

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_NV_counters_to_DataFlash(void)
{
  uint32_t crc;

  // Вызываем функцию основного приложения чтобы забрать его энергонезависимые счетчики в переменную  g_nv_cntc
  Get_App_persistent_counters(g_nv_cntc.data, APP_NV_COUNTERS_SZ);

  // Пересчитываем контрольную сумму блока
  crc           = Get_CRC16_of_block(&g_nv_cntc, NV_COUNTERS_BLOCK_SZ - 4, 0xFFFF);
  g_nv_cntc.crc = crc;

  // Предварительно стираем область в DataFlash, если она не стерта
  if (DataFlash_bgo_BlankCheck(g_nv_counters_curr_addr, NV_COUNTERS_BLOCK_SZ) != RES_OK)
  {
    DataFlash_bgo_EraseArea(g_nv_counters_curr_addr, NV_COUNTERS_BLOCK_SZ);
  }
  // Сохраняем данные в DataFlash
  if (DataFlash_bgo_WriteArea(g_nv_counters_curr_addr, (uint8_t *)&g_nv_cntc, NV_COUNTERS_BLOCK_SZ) != RES_OK) return RES_ERROR;

  // Стираем предыдущий блок
  DataFlash_bgo_EraseArea(Get_NV_couters_prev_addr(), NV_COUNTERS_BLOCK_SZ);
  Move_NV_couters_addr_forward_circular();
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Процедура сохранения NV данных в Secure Standby SRAM микроконтроллера R7FA8M1AH
  Вызывается приблизительно каждую секунду, поскольку используется для учета времени наработки

  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Save_NV_counters_to_NVRAM(void)
{
  uint32_t crc;

  // Вызываем функцию основного приложения чтобы забрать его энергонезависимые счетчики в переменную g_nv_cntc
  Get_App_persistent_counters(g_nv_cntc.data, APP_NV_COUNTERS_SZ);

  // Пересчитываем контрольную сумму блока
  crc           = Get_CRC16_of_block(&g_nv_cntc, NV_COUNTERS_BLOCK_SZ - 4, 0xFFFF);
  g_nv_cntc.crc = crc;

  // Запись данных в Secure Standby SRAM микроконтроллера R7FA8M1AH
  memcpy((void *)STANDBY_SECURE_SRAM_START, (uint8_t *)&g_nv_cntc, NV_COUNTERS_BLOCK_SZ);

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Загрузка блока энергонезависмых счетчиков из NVRAM (Secure Standby SRAM)

  \param block_ptr Указатель на структуру для загрузки данных

  \return uint32_t RES_OK если CRC совпадает, иначе RES_ERROR
-----------------------------------------------------------------------------------------------------*/
uint32_t Load_NV_counters_from_NVRAM(T_nv_counters_block *block_ptr)
{
  uint32_t crc;

  if (block_ptr == NULL) return RES_ERROR;

  // Читаем данные из Secure Standby SRAM
  memcpy((uint8_t *)block_ptr, (void *)STANDBY_SECURE_SRAM_START, NV_COUNTERS_BLOCK_SZ);

  // Проверяем CRC
  crc = Get_CRC16_of_block(block_ptr, NV_COUNTERS_BLOCK_SZ - 4, 0xFFFF);
  if (crc == block_ptr->crc)
  {
    return RES_OK;
  }
  return RES_ERROR;
}

// clang-format off
/*-----------------------------------------------------------------------------------------------------
  Возвращает строку с описанием ошибки сохранения настроек в DataFlash

  \param err_code - код ошибки

  \return const char* - строковое описание ошибки
-----------------------------------------------------------------------------------------------------*/
const char *Get_settings_save_error_str(uint32_t err_code)
{
  switch (err_code)
  {
    case RES_OK:                    return "No error";
    case NV_ERR_INVALID_TYPE:       return "Invalid parameter type";
    case NV_ERR_BUFFER_TOO_LARGE:   return "Buffer too large";
    case NV_ERR_BUFFER_TOO_SMALL:   return "Buffer too small";
    case NV_ERR_MEMORY_ALLOCATION:  return "Memory allocation failed";
    case NV_ERR_ERASE_AREA1:        return "Failed to erase area 1";
    case NV_ERR_WRITE_AREA1:        return "Failed to write to area 1";
    case NV_ERR_ERASE_AREA2:        return "Failed to erase area 2";
    case NV_ERR_WRITE_AREA2:        return "Failed to write to area 2";
    default:                        return "Unknown error";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Возвращает строку с описанием ошибки восстановления настроек из DataFlash

  \param err_code - код ошибки

  \return const char* - строковое описание ошибки
-----------------------------------------------------------------------------------------------------*/
const char *Get_settings_restore_error_str(uint32_t err_code)
{
  switch (err_code)
  {
    case DF_RESTORE_ERR_NONE:            return "No error";
    case DF_RESTORE_ERR_INVALID_TYPE:    return "Invalid parameter type";
    case DF_RESTORE_ERR_NO_VALID_DATA:   return "No valid data found in DataFlash areas";
    default:                             return "Unknown error";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Возвращает строку с описанием кода ошибки области настроек в DataFlash

  \param err_code - код ошибки

  \return const char* - строковое описание ошибки
-----------------------------------------------------------------------------------------------------*/
const char *Get_settings_area_error_str(uint32_t err_code)
{
  switch (err_code)
  {
    case DF_AREA_ERR_NONE:         return "No error";
    case DF_AREA_ERR_WRONG_SIZE:   return "Wrong data size";
    case DF_AREA_ERR_CRC_MISMATCH: return "CRC error";
    case DF_AREA_ERR_DECOMP_SIZE:  return "Decompressed data too large";
    case DF_AREA_ERR_MEM_ALLOC:    return "Memory allocation failed";
    case DF_AREA_ERR_DECOMPRESS:   return "Decompression error";
    case DF_AREA_ERR_JSON_PARSE:   return "JSON parsing error";
    case DF_AREA_ERR_READ_DATA:    return "DataFlash read error";
    default:                       return "Unknown error";
  }
}
// clang-format on
