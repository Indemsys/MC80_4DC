 #pragma optimize=none
 SECTION .itcm_data : CODE ; Помещаем секцию в ITCM

  EXPORT Delay_m85
  THUMB ; Указываем что код в режиме THUMB

         ;  Cortex-M85
         ;  Задержка без сброса кэша (ITCM)
         ;  Количество тактов: (R0 * 3) + 2

Delay_m85
         ; Проверка на 0
         CBZ      r0, Delay_m85_exit      ; 1 такт (если r0 == 0, переход)
         // Этот цикл выполняется за 3 такта процессора ARM Cortex-M85
Delay_m85_loop:
         SUBS     r0, r0, #1              ; 1 такт (уменьшаем счетчик на 1)
         CMP      r0, #0                  ; 1 такт (сравниваем r0 с 0)
         NOP
         NOP
         NOP
         NOP
         NOP
         NOP
         BNE      Delay_m85_loop          ; 2 такта (если r0 != 0, переход на Delay_m85_loop)
Delay_m85_exit:
         BX       lr                      ; 2 такта (возврат)

         END
