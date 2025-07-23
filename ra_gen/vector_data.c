/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0

// Interrupt vector table - maps vector numbers to interrupt service routines (ISRs)
// Each entry contains a function pointer to the ISR that handles specific interrupt
// Vector numbers DO NOT determine interrupt priority levels - priorities are set separately via NVIC->IPR registers
// Placed in special memory section for direct hardware access during interrupt handling
BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_MAX_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
{
 [0]  = spi_b_rxi_isr,           /* SPI0 RXI (Receive buffer full) */
 [1]  = spi_b_txi_isr,           /* SPI0 TXI (Transmit buffer empty) */
 [2]  = spi_b_tei_isr,           /* SPI0 TEI (Transmission complete event) */
 [3]  = spi_b_eri_isr,           /* SPI0 ERI (Error) */
 [4]  = fcu_frdyi_isr,           /* FCU FRDYI (Flash ready interrupt) */
 [5]  = fcu_fiferr_isr,          /* FCU FIFERR (Flash access error interrupt) */
 [6]  = agt_int_isr,             /* AGT0 INT (AGT interrupt) */
 [7]  = rtc_alarm_periodic_isr,  /* RTC ALARM (Alarm interrupt) */
 [8]  = rtc_alarm_periodic_isr,  /* RTC PERIOD (Periodic interrupt) */
 [9]  = rtc_carry_isr,           /* RTC CARRY (Carry interrupt) */
 [10] = dmac_int_isr,            /* DMAC3 INT (DMAC3 transfer end) */
 [11] = ether_eint_isr,          /* EDMAC0 EINT (EDMAC 0 interrupt) */
 [12] = sdhimmc_accs_isr,        /* SDHIMMC1 ACCS (Card access) */
 [13] = dmac_int_isr,            /* DMAC2 INT (DMAC2 transfer end) */
 [14] = usbfs_interrupt_handler, /* USBFS INT (USBFS interrupt) */
 [15] = usbfs_resume_handler,    /* USBFS RESUME (USBFS resume interrupt) */
 [16] = usbfs_d0fifo_handler,    /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
 [17] = usbfs_d1fifo_handler,    /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
 [18] = dmac_int_isr,            /* DMAC1 INT (DMAC1 transfer end) */
 [19] = dmac_int_isr,            /* DMAC0 INT (DMAC0 transfer end) */
 [20] = Adc_scan_end_isr,        /* ADC0 SCAN END (ADC scan end interrupt) */
 [21] = canfd_error_isr,         /* CAN0 CHERR (Channel error) */
 [22] = canfd_channel_tx_isr,    /* CAN0 TX (Transmit interrupt) */
 [23] = canfd_common_fifo_rx_isr,/* CAN0 COMFRX (Common FIFO receive interrupt) */
 [24] = canfd_error_isr,         /* CAN GLERR (Global error) */
 [25] = canfd_rx_fifo_isr,       /* CAN RXF (Global receive FIFO interrupt) */
 [26] = dmac_int_isr,            /* DMAC4 INT (DMAC4 transfer end) */
 [27] = dmac_int_isr,            /* DMAC5 INT (DMAC5 transfer end) */
 [28] = ospi_cmdcmp_isr,         /* OSPI CMDCMP (Command completion) */

};
  #if BSP_FEATURE_ICU_HAS_IELSR

// Interrupt Event Link Select Register (IELSR) configuration table
// IELSR registers (32-bit) map vector numbers to specific peripheral interrupt events
// Each IELSR[n] register corresponds to vector number n and contains the ELC event code
// Hardware uses this mapping to route peripheral events to correct interrupt vectors
// When peripheral generates event, ICU checks IELSR[n] to determine which vector to activate
// This table is loaded into IELSR registers during system initialization
// Each entry contains elc_event_t enum value that identifies specific peripheral event source
const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_MAX_ENTRIES] =
{
 [0]  = ELC_EVENT_SPI0_RXI,      /* SPI0 RXI (Receive buffer full) */
 [1]  = ELC_EVENT_SPI0_TXI,      /* SPI0 TXI (Transmit buffer empty) */
 [2]  = ELC_EVENT_SPI0_TEI,      /* SPI0 TEI (Transmission complete event) */
 [3]  = ELC_EVENT_SPI0_ERI,      /* SPI0 ERI (Error) */
 [4]  = ELC_EVENT_FCU_FRDYI,     /* FCU FRDYI (Flash ready interrupt) */
 [5]  = ELC_EVENT_FCU_FIFERR,    /* FCU FIFERR (Flash access error interrupt) */
 [6]  = ELC_EVENT_AGT0_INT,      /* AGT0 INT (AGT interrupt) */
 [7]  = ELC_EVENT_RTC_ALARM,     /* RTC ALARM (Alarm interrupt) */
 [8]  = ELC_EVENT_RTC_PERIOD,    /* RTC PERIOD (Periodic interrupt) */
 [9]  = ELC_EVENT_RTC_CARRY,     /* RTC CARRY (Carry interrupt) */
 [10] = ELC_EVENT_DMAC3_INT,     /* DMAC3 INT (DMAC3 transfer end) */
 [11] = ELC_EVENT_EDMAC0_EINT,   /* EDMAC0 EINT (EDMAC 0 interrupt) */
 [12] = ELC_EVENT_SDHIMMC1_ACCS, /* SDHIMMC1 ACCS (Card access) */
 [13] = ELC_EVENT_DMAC2_INT,     /* DMAC2 INT (DMAC2 transfer end) */
 [14] = ELC_EVENT_USBFS_INT,     /* USBFS INT (USBFS interrupt) */
 [15] = ELC_EVENT_USBFS_RESUME,  /* USBFS RESUME (USBFS resume interrupt) */
 [16] = ELC_EVENT_USBFS_FIFO_0,  /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
 [17] = ELC_EVENT_USBFS_FIFO_1,  /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
 [18] = ELC_EVENT_DMAC1_INT,     /* DMAC1 INT (DMAC1 transfer end) */
 [19] = ELC_EVENT_DMAC0_INT,     /* DMAC0 INT (DMAC0 transfer end) */
 [20] = ELC_EVENT_ADC0_SCAN_END, /* ADC0 SCAN END (ADC scan end interrupt) */
 [21] = ELC_EVENT_CAN0_CHERR,    /* CAN0 CHERR (Channel error) */
 [22] = ELC_EVENT_CAN0_TX,       /* CAN0 TX (Transmit interrupt) */
 [23] = ELC_EVENT_CAN0_COMFRX,   /* CAN0 COMFRX (Common FIFO receive interrupt) */
 [24] = ELC_EVENT_CAN_GLERR,     /* CAN GLERR (Global error) */
 [25] = ELC_EVENT_CAN_RXF,       /* CAN RXF (Global receive FIFO interrupt) */
 [26] = ELC_EVENT_DMAC4_INT,     /* DMAC4 INT (DMAC4 transfer end) */
 [27] = ELC_EVENT_DMAC5_INT,     /* DMAC5 INT (DMAC5 transfer end) */
 [28] = ELC_EVENT_XSPI_CMP,      /* OSPI CMDCMP (Command completion) */
};
  #endif
#endif
