/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
extern "C"
{
#endif  /* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
  #define VECTOR_DATA_IRQ_COUNT (28)
#endif
  /* ISR prototypes */
  void spi_b_rxi_isr(void);
  void spi_b_txi_isr(void);
  void spi_b_tei_isr(void);
  void spi_b_eri_isr(void);
  void fcu_frdyi_isr(void);
  void fcu_fiferr_isr(void);
  void agt_int_isr(void);
  void rtc_alarm_periodic_isr(void);
  void rtc_carry_isr(void);
  void dmac_int_isr(void);
  void ether_eint_isr(void);
  void sdhimmc_accs_isr(void);
  void usbfs_interrupt_handler(void);
  void usbfs_resume_handler(void);
  void usbfs_d0fifo_handler(void);
  void usbfs_d1fifo_handler(void);
  void Adc_scan_end_isr(void);
  void canfd_error_isr(void);
  void canfd_channel_tx_isr(void);
  void canfd_common_fifo_rx_isr(void);
  void canfd_rx_fifo_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_SPI0_RXI      ((IRQn_Type)0)  /* SPI0 RXI (Receive buffer full) */
#define SPI0_RXI_IRQn               ((IRQn_Type)0)  /* SPI0 RXI (Receive buffer full) */
#define VECTOR_NUMBER_SPI0_TXI      ((IRQn_Type)1)  /* SPI0 TXI (Transmit buffer empty) */
#define SPI0_TXI_IRQn               ((IRQn_Type)1)  /* SPI0 TXI (Transmit buffer empty) */
#define VECTOR_NUMBER_SPI0_TEI      ((IRQn_Type)2)  /* SPI0 TEI (Transmission complete event) */
#define SPI0_TEI_IRQn               ((IRQn_Type)2)  /* SPI0 TEI (Transmission complete event) */
#define VECTOR_NUMBER_SPI0_ERI      ((IRQn_Type)3)  /* SPI0 ERI (Error) */
#define SPI0_ERI_IRQn               ((IRQn_Type)3)  /* SPI0 ERI (Error) */
#define VECTOR_NUMBER_FCU_FRDYI     ((IRQn_Type)4)  /* FCU FRDYI (Flash ready interrupt) */
#define FCU_FRDYI_IRQn              ((IRQn_Type)4)  /* FCU FRDYI (Flash ready interrupt) */
#define VECTOR_NUMBER_FCU_FIFERR    ((IRQn_Type)5)  /* FCU FIFERR (Flash access error interrupt) */
#define FCU_FIFERR_IRQn             ((IRQn_Type)5)  /* FCU FIFERR (Flash access error interrupt) */
#define VECTOR_NUMBER_AGT0_INT      ((IRQn_Type)6)  /* AGT0 INT (AGT interrupt) */
#define AGT0_INT_IRQn               ((IRQn_Type)6)  /* AGT0 INT (AGT interrupt) */
#define VECTOR_NUMBER_RTC_ALARM     ((IRQn_Type)7)  /* RTC ALARM (Alarm interrupt) */
#define RTC_ALARM_IRQn              ((IRQn_Type)7)  /* RTC ALARM (Alarm interrupt) */
#define VECTOR_NUMBER_RTC_PERIOD    ((IRQn_Type)8)  /* RTC PERIOD (Periodic interrupt) */
#define RTC_PERIOD_IRQn             ((IRQn_Type)8)  /* RTC PERIOD (Periodic interrupt) */
#define VECTOR_NUMBER_RTC_CARRY     ((IRQn_Type)9)  /* RTC CARRY (Carry interrupt) */
#define RTC_CARRY_IRQn              ((IRQn_Type)9)  /* RTC CARRY (Carry interrupt) */
#define VECTOR_NUMBER_DMAC3_INT     ((IRQn_Type)10) /* DMAC3 INT (DMAC3 transfer end) */
#define DMAC3_INT_IRQn              ((IRQn_Type)10) /* DMAC3 INT (DMAC3 transfer end) */
#define VECTOR_NUMBER_EDMAC0_EINT   ((IRQn_Type)11) /* EDMAC0 EINT (EDMAC 0 interrupt) */
#define EDMAC0_EINT_IRQn            ((IRQn_Type)11) /* EDMAC0 EINT (EDMAC 0 interrupt) */
#define VECTOR_NUMBER_SDHIMMC1_ACCS ((IRQn_Type)12) /* SDHIMMC1 ACCS (Card access) */
#define SDHIMMC1_ACCS_IRQn          ((IRQn_Type)12) /* SDHIMMC1 ACCS (Card access) */
#define VECTOR_NUMBER_DMAC2_INT     ((IRQn_Type)13) /* DMAC2 INT (DMAC2 transfer end) */
#define DMAC2_INT_IRQn              ((IRQn_Type)13) /* DMAC2 INT (DMAC2 transfer end) */
#define VECTOR_NUMBER_USBFS_INT     ((IRQn_Type)14) /* USBFS INT (USBFS interrupt) */
#define USBFS_INT_IRQn              ((IRQn_Type)14) /* USBFS INT (USBFS interrupt) */
#define VECTOR_NUMBER_USBFS_RESUME  ((IRQn_Type)15) /* USBFS RESUME (USBFS resume interrupt) */
#define USBFS_RESUME_IRQn           ((IRQn_Type)15) /* USBFS RESUME (USBFS resume interrupt) */
#define VECTOR_NUMBER_USBFS_FIFO_0  ((IRQn_Type)16) /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
#define USBFS_FIFO_0_IRQn           ((IRQn_Type)16) /* USBFS FIFO 0 (DMA/DTC transfer request 0) */
#define VECTOR_NUMBER_USBFS_FIFO_1  ((IRQn_Type)17) /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
#define USBFS_FIFO_1_IRQn           ((IRQn_Type)17) /* USBFS FIFO 1 (DMA/DTC transfer request 1) */
#define VECTOR_NUMBER_DMAC1_INT     ((IRQn_Type)18) /* DMAC1 INT (DMAC1 transfer end) */
#define DMAC1_INT_IRQn              ((IRQn_Type)18) /* DMAC1 INT (DMAC1 transfer end) */
#define VECTOR_NUMBER_DMAC0_INT     ((IRQn_Type)19) /* DMAC0 INT (DMAC0 transfer end) */
#define DMAC0_INT_IRQn              ((IRQn_Type)19) /* DMAC0 INT (DMAC0 transfer end) */
#define VECTOR_NUMBER_ADC0_SCAN_END ((IRQn_Type)20) /* ADC0 SCAN END (ADC0 scan end) */
#define ADC0_SCAN_END_IRQn          ((IRQn_Type)20) /* ADC0 SCAN END (ADC0 scan end) */
#define VECTOR_NUMBER_CAN0_CHERR    ((IRQn_Type)21) /* CAN0 CHERR (Channel error) */
#define CAN0_CHERR_IRQn             ((IRQn_Type)21) /* CAN0 CHERR (Channel error) */
#define VECTOR_NUMBER_CAN0_TX       ((IRQn_Type)22) /* CAN0 TX (Transmit interrupt) */
#define CAN0_TX_IRQn                ((IRQn_Type)22) /* CAN0 TX (Transmit interrupt) */
#define VECTOR_NUMBER_CAN0_COMFRX   ((IRQn_Type)23) /* CAN0 COMFRX (Common FIFO receive interrupt) */
#define CAN0_COMFRX_IRQn            ((IRQn_Type)23) /* CAN0 COMFRX (Common FIFO receive interrupt) */
#define VECTOR_NUMBER_CAN_GLERR     ((IRQn_Type)24) /* CAN GLERR (Global error) */
#define CAN_GLERR_IRQn              ((IRQn_Type)24) /* CAN GLERR (Global error) */
#define VECTOR_NUMBER_CAN_RXF       ((IRQn_Type)25) /* CAN RXF (Global receive FIFO interrupt) */
#define CAN_RXF_IRQn                ((IRQn_Type)25) /* CAN RXF (Global receive FIFO interrupt) */
#define VECTOR_NUMBER_DMAC4_INT     ((IRQn_Type)26) /* DMAC4 INT (DMAC4 transfer end) */
#define DMAC4_INT_IRQn              ((IRQn_Type)26) /* DMAC4 INT (DMAC4 transfer end) */
#define VECTOR_NUMBER_DMAC5_INT     ((IRQn_Type)27) /* DMAC5 INT (DMAC5 transfer end) */
#define DMAC5_INT_IRQn              ((IRQn_Type)27) /* DMAC5 INT (DMAC5 transfer end) */
#ifdef __cplusplus
}
#endif
#endif /* VECTOR_DATA_H */
