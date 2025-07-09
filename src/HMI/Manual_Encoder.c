// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2022-09-12
// 11:00:52
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

T_enc_cbl enc_cbl;

#define ENCODER_SAMPLING_FREQ     (1000000ul / AGT_PERIOD_US)
#define TIMEOUT_MAX_LONG_PRESSING (10 * ENCODER_SAMPLING_FREQ)  // 10 сек  - прололжительность максимально длинного нажати
#define TIMEOUT_LONG_PRESSING     (2 * ENCODER_SAMPLING_FREQ)   // 2  сек  - продолжительность длинного нажати

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void AGT0_callback(timer_callback_args_t *p_args)
{
  Manual_encoder_processing();
}

/*-----------------------------------------------------------------------------------------------------
  Функция для инициализации и запуска таймера AGT0
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Manual_encode_init(void)
{
  fsp_err_t err = FSP_SUCCESS;

  // Инициализация и запуск AGT0
  err           = R_AGT_Open(&g_agt0_ctrl, &g_agt0_cfg);
  err           = R_AGT_Start(&g_agt0_ctrl);

  return err;
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
uint8_t Get_smpl_enc_a(void)
{
  return MANUAL_ENCODER_A_INPUT;
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
uint8_t Get_smpl_enc_b(void)
{
  return MANUAL_ENCODER_B_INPUT;
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
uint8_t Get_smpl_enc_sw(void)
{
  return MANUAL_ENCODER_SWITCH;
}

/*-----------------------------------------------------------------------------------------------------
  Процедура обработки сигналов ручного энкодера
  Вызвается с частотой не меньше 10 кГц


  Самый короткий импульс у энкодера зафиксирован длительностью 300 мкс

  \param void
-----------------------------------------------------------------------------------------------------*/
void Manual_encoder_processing(void)
{
  enc_cbl.curr_enc_a_smpl  = Get_smpl_enc_a();
  enc_cbl.curr_enc_b_smpl  = Get_smpl_enc_b();
  enc_cbl.curr_enc_sw_smpl = Get_smpl_enc_sw() ^ 1;

  // Ведем счетчик длительности состояния линии A
  if (enc_cbl.curr_enc_a_smpl == 0)
  {
    if (enc_cbl.enc_a_smpl_prev != 0)
      enc_cbl.A_cnt = 0;
    else if (enc_cbl.A_cnt < 0xFFFFFFFF)

      enc_cbl.A_cnt++;
  }
  else
  {
    if (enc_cbl.enc_a_smpl_prev == 0)
      enc_cbl.A_cnt = 0;
    else if (enc_cbl.A_cnt < 0xFFFFFFFF)
      enc_cbl.A_cnt++;
  }

  // Ведем счетчик длительности состояния линии B
  if (enc_cbl.curr_enc_b_smpl == 0)
  {
    if (enc_cbl.enc_b_smpl_prev != 0)
      enc_cbl.B_cnt = 0;
    else if (enc_cbl.B_cnt < 0xFFFFFFFF)
      enc_cbl.B_cnt++;
  }
  else
  {
    if (enc_cbl.enc_b_smpl_prev == 0)
      enc_cbl.B_cnt = 0;
    else if (enc_cbl.B_cnt < 0xFFFFFFFF)
      enc_cbl.B_cnt++;
  }

  // Ведем счетчик длительности состояния линии switch
  if (enc_cbl.curr_enc_sw_smpl == 0)
  {
    if (enc_cbl.enc_sw_smpl_prev != 0)
    {
      enc_cbl.prev_sw_cnt = enc_cbl.sw_cnt;
      enc_cbl.sw_cnt      = 0;
    }
    else if (enc_cbl.sw_cnt < 0xFFFFFFFF)
      enc_cbl.sw_cnt++;
  }
  else
  {
    if (enc_cbl.enc_sw_smpl_prev == 0)
    {
      enc_cbl.prev_sw_cnt = enc_cbl.sw_cnt;
      enc_cbl.sw_cnt      = 0;
    }
    else if (enc_cbl.sw_cnt < 0xFFFFFFFF)
      enc_cbl.sw_cnt++;
  }

  // Фильтруем дребезг сигнала B
  if (enc_cbl.B_cnt > 5)
  {
    enc_cbl.B_state = enc_cbl.curr_enc_b_smpl;
  }

  // Фильтруем дребезг сигнала A
  if (enc_cbl.A_cnt > 5)
  {
    enc_cbl.A_state = enc_cbl.curr_enc_a_smpl;
  }

  // Фильтруем дребезг сигнала switch
  if (enc_cbl.sw_cnt > 5)
  {
    enc_cbl.sw_state = enc_cbl.curr_enc_sw_smpl;
  }

  // Фиксируем фронт на линии A
  if ((enc_cbl.A_state == 1) && (enc_cbl.prev_A_state == 0))
  {
    enc_cbl.A_rising_edge = 1;  // Выставляем флаг фронта
    // Если фронт случился на высоком уровне B, то это отрицательный импульс
    if (enc_cbl.B_state && enc_cbl.B_rising_edge)
    {
      enc_cbl.encoder_counter--;
      enc_cbl.B_rising_edge = 0;  // Сбрасываем флаг фронта чтобы не было повторной реакции на колебания сигнала A при устойчивом сигнале B
    }
  }

  // Фиксируем фронт на линии B
  if ((enc_cbl.B_state == 1) && (enc_cbl.prev_B_state == 0))
  {
    enc_cbl.B_rising_edge = 1;  // Выставляем флаг фронта
    // Если фронт случился на высоком уровне A, то это положительный импульс
    if (enc_cbl.A_state && enc_cbl.A_rising_edge)
    {
      enc_cbl.encoder_counter++;
      enc_cbl.A_rising_edge = 0;  // Сбрасываем флаг фронта чтобы не было повторной реакции на колебания сигнала B при устойчивом сигнале A
    }
  }

  // Фиксируем отпускание switch
  if (enc_cbl.sw_state == 1)
  {
    if (enc_cbl.sw_cnt == TIMEOUT_MAX_LONG_PRESSING)
    {
      enc_cbl.sw_release_sig = ENC_SW_MAX_LONG_PRESSED;
    }
    else if (enc_cbl.sw_cnt == TIMEOUT_LONG_PRESSING)
    {
      enc_cbl.sw_release_sig = ENC_SW_LONG_PRESSED;
    }
  }

  if ((enc_cbl.sw_state == 0) && (enc_cbl.prev_sw_state == 1) && (enc_cbl.prev_sw_cnt < TIMEOUT_LONG_PRESSING))
  {
    enc_cbl.sw_release_sig = ENC_SW_PRESSED;
  }

  enc_cbl.enc_a_smpl_prev  = enc_cbl.curr_enc_a_smpl;
  enc_cbl.enc_b_smpl_prev  = enc_cbl.curr_enc_b_smpl;
  enc_cbl.enc_sw_smpl_prev = enc_cbl.curr_enc_sw_smpl;

  enc_cbl.prev_A_state     = enc_cbl.A_state;
  enc_cbl.prev_B_state     = enc_cbl.B_state;
  enc_cbl.prev_sw_state    = enc_cbl.sw_state;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_switch_press_signal(void)
{
  uint32_t s = enc_cbl.sw_release_sig;
  if (s == ENC_SW_PRESSED)
  {
    enc_cbl.sw_release_sig = 0;
    return 1;
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------



  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_switch_long_press_signal(void)
{
  uint32_t s = enc_cbl.sw_release_sig;
  if (s == ENC_SW_LONG_PRESSED)
  {
    enc_cbl.sw_release_sig = 0;
    return 1;
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return int32_t
-----------------------------------------------------------------------------------------------------*/
int32_t Get_encoder_counter(void)
{
  return enc_cbl.encoder_counter;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
int32_t Get_encoder_counter_delta(int32_t *p_now_cnt)
{
  int32_t now_cnt = *p_now_cnt;
  int32_t cnt;
  int32_t delta = 0;

  cnt           = Get_encoder_counter();
  if (now_cnt != cnt)
  {
    delta   = cnt - now_cnt;  //
    now_cnt = cnt;
  }
  *p_now_cnt = now_cnt;
  return delta;
}
