#include <stddef.h>
#include "adc.h"
#include "adc_revb.h"
#include "dma.h"
#include "dma_reva.h"
#include "tmr.h"
#include "nvic_table.h"
#include "analog.h"

#define ADC_CHANNEL           MXC_ADC_CH_1

#define ANALOG_TACT_LEN_US    200U

#define CHUNK_SIZE            256  // 256 * 4 = 1KB — manageable RAM

volatile bool dma_done = false;
static mxc_adc_conversion_req_t adc_conv;
static uint32_t dma_pingpong[2][CHUNK_SIZE];
static uint8_t  active_buf = 0;
static uint8_t ANALOG_Buff[ANALOG_BUFF_LEN];  // final 16-bit output
static uint16_t fill_index = 0;
mxc_adc_slot_req_t single_slot = { ADC_CHANNEL };

void adc_dma_callback(int ch, int error)
{
  uint32_t *buf = dma_pingpong[active_buf];

  // Strip upper 16 bits, fill output buffer
  for (int i = 0; i < CHUNK_SIZE && fill_index < ANALOG_BUFF_LEN; i++)
      ANALOG_Buff[fill_index++] = (uint8_t)((uint16_t)(buf[i] & 0x0FFF) >> 4);

  // Swap and re-arm FIRST
  active_buf ^= 1;
  MXC_DMA->ch[adc_conv.dma_channel].dst = (uint32_t)dma_pingpong[active_buf];
  MXC_DMA->ch[adc_conv.dma_channel].cnt = CHUNK_SIZE * sizeof(uint32_t);
  MXC_DMA_Start(adc_conv.dma_channel);  // re-enable EN bit

  if (fill_index >= ANALOG_BUFF_LEN) {
      fill_index = 0;
      dma_done = true;
  }
}

void DMA_IRQHandler(void)
{
  MXC_DMA_Handler();
}

static void ANALOG_SetTimer(void)
{
  mxc_tmr_cfg_t tmr;
  const mxc_tmr_pres_t prescalerReg = TMR_PRES_1;
  const uint32_t prescalerDiv = 1U;
  uint32_t freqHz = 1000000U / ANALOG_TACT_LEN_US;
  unsigned int periodTicks = MXC_TMR_GetPeriod(MXC_TMR1, MXC_TMR_APB_CLK,
                                                prescalerDiv, freqHz);

  /**< Disable the timer */
  MXC_TMR_Shutdown(MXC_TMR1);
  /**< Set the pre-scale value */
  tmr.pres = prescalerReg;
  /**< Setup polarity, PWM parameters */
  tmr.mode = TMR_MODE_CONTINUOUS;
  tmr.bitMode = TMR_BIT_MODE_32;
  tmr.clock = MXC_TMR_APB_CLK;
  tmr.cmp_cnt = periodTicks;
  tmr.pol = 1;
  /**< Initialize timer */
  if (MXC_TMR_Init(MXC_TMR1, &tmr, false, MAP_A) != E_NO_ERROR) {}
  /**< Start timer */
  MXC_TMR_Start(MXC_TMR1);
}

void ANALOG_Configuration(void)
{
  mxc_adc_req_t adc_cfg;

  adc_cfg.clock     = MXC_ADC_HCLK;
  adc_cfg.clkdiv    = MXC_ADC_CLKDIV_16;
  adc_cfg.cal       = MXC_ADC_EN_CAL;
  adc_cfg.trackCount = 4;
  adc_cfg.idleCount  = 17;
  adc_cfg.ref       = MXC_ADC_REF_INT_2V048;

  if (MXC_ADC_Init(&adc_cfg) != E_NO_ERROR) {}

  // STEP 1: Init DMA controller first
  MXC_DMA_Init();

  // STEP 2: Acquire channel (now valid)
  int dma_channel = MXC_DMA_AcquireChannel();
  adc_conv.dma_channel = dma_channel;

  // STEP 3: Configure ADC conversion request (dma_channel is now set)
  adc_conv.mode           = MXC_ADC_CONTINUOUS_CONV;
  adc_conv.trig           = MXC_ADC_TRIG_HARDWARE;
  adc_conv.hwTrig         = MXC_ADC_TRIG_SEL_TMR1;
  adc_conv.avg_number     = MXC_ADC_AVG_1;
  adc_conv.fifo_format    = MXC_ADC_DATA;
  adc_conv.fifo_threshold = 0;
  adc_conv.lpmode_divder  = MXC_ADC_DIV_2_5K_50K_ENABLE;
  adc_conv.num_slots      = 0;

  // STEP 4: Apply ADC configuration after dma_channel is set
  MXC_ADC_Configuration(&adc_conv);
  MXC_ADC_SlotConfiguration(&single_slot, 0);

  // STEP 5: Register DMA IRQ
  MXC_NVIC_SetVector(MXC_DMA_CH_GET_IRQ(dma_channel), DMA_IRQHandler);
  NVIC_EnableIRQ(MXC_DMA_CH_GET_IRQ(dma_channel));

  // STEP 6: Start timer last
  ANALOG_SetTimer();
}

void ANALOG_StartDMA(void)
{
    dma_done   = false;
    fill_index = 0;
    active_buf = 0;

    MXC_TMR_Stop(MXC_TMR1);

    // Full ADC+DMA init — done once here, never in callback
    MXC_ADC->fifodmactrl |= MXC_F_ADC_REVB_FIFODMACTRL_FLUSH;
    while (MXC_ADC->fifodmactrl & MXC_F_ADC_REVB_FIFODMACTRL_FLUSH);

    MXC_ADC_StartConversionDMA(&adc_conv, (int*)dma_pingpong[0], adc_dma_callback);

    MXC_DMA->ch[adc_conv.dma_channel].cnt = CHUNK_SIZE * sizeof(uint32_t);
    MXC_DMA->ch[adc_conv.dma_channel].dst = (uint32_t)dma_pingpong[0];

    MXC_TMR_Start(MXC_TMR1);
}

bool ANALOG_IsReady(void)
{
  bool result = false;

  if (dma_done)
  {
    dma_done = false;  // clear flag here, not in StartDMA
    result =  true;
  }

  return result;
}

uint8_t* ANALOG_GetFullBuff(void)
{
  return ANALOG_Buff;
}

uint16_t ANALOG_GetRate(void)
{
  return (uint16_t)(1000000U / ANALOG_TACT_LEN_US);
}
