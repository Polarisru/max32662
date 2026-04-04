#include "adc.h"
#include "dma.h"
#include "adc_hw.h"

#define ADC_CHANNEL      MXC_ADC_CH_1

volatile unsigned int dma_done = 0;
static mxc_adc_conversion_req_t adc_conv;
static uint8_t ANALOG_Buff[ANALOG_BUFF_LEN];

void StartHWTriggerTimer(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr; // to configure timer
    unsigned int periodTicks = MXC_TMR_GetPeriod(MXC_TMR1, MXC_TMR_APB_CLK, 32, 1);

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the pre-scale value
    3. Set polarity, PWM parameters
    4. Configure the timer for PWM mode
    5. Enable Timer
    */

    MXC_TMR_Shutdown(MXC_TMR1);

    tmr.pres = TMR_PRES_32;
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = MXC_TMR_APB_CLK;
    tmr.cmp_cnt = periodTicks;
    tmr.pol = 1;

    if (MXC_TMR_Init(MXC_TMR1, &tmr, false, MAP_A) != E_NO_ERROR) {
        Console_Init();
        printf("Failed PWM timer Initialization.\n");
        return;
    }

    MXC_TMR_Start(MXC_TMR1);

    // LED On indicates Timer started.
    printf("Timer started.\n\n");
}

void ANALOG_Configuration(void)
{
  mxc_adc_req_t adc_cfg;

  adc_cfg.clock = MXC_ADC_HCLK;
  adc_cfg.clkdiv = MXC_ADC_CLKDIV_16;
  adc_cfg.cal = MXC_ADC_EN_CAL;
  adc_cfg.trackCount = 4;
  adc_cfg.idleCount = 17;
  adc_cfg.ref = MXC_ADC_REF_INT_2V048;

  /* Initialize ADC */
  if (MXC_ADC_Init(&adc_cfg) != E_NO_ERROR)
  {

  }

  adc_conv.mode = MXC_ADC_ATOMIC_CONV;
  adc_conv.trig = MXC_ADC_TRIG_SOFTWARE;
  adc_conv.avg_number = MXC_ADC_AVG_16;
  adc_conv.fifo_format = MXC_ADC_DATA_STATUS;
  adc_conv.fifo_threshold = 0;
  adc_conv.lpmode_divder = MXC_ADC_DIV_2_5K_50K_ENABLE;
  adc_conv.num_slots = 0;

  MXC_ADC_Configuration(&adc_conv);

  MXC_ADC_SlotConfiguration(&single_slot, 0);

  MXC_DMA_Init();
}

void ANALOG_StartDMA(void)
{
  dma_done = 0;

  int dma_channel = MXC_DMA_AcquireChannel();
  adc_conv.dma_channel = dma_channel;

  MXC_NVIC_SetVector(MXC_DMA_CH_GET_IRQ(dma_channel), DMA_IRQHandler);
  NVIC_EnableIRQ(MXC_DMA_CH_GET_IRQ(dma_channel));
  MXC_ADC_StartConversionDMA(&adc_conv, adc_val, adc_dma_callback);
}

bool ANALOG_IsReady(void)
{
  if (dma_done == true)
  {
    MXC_DMA_ReleaseChannel(adc_conv.dma_channel);
    result = true;
  }

  return result;
}

uint8_t* ANALOG_GetFullBuff(void)
{
  return ANALOG_Buff;
}

uint16_t ANALOG_GetRate(void)
{
  return (uint16_t)(1000000U / ANALOG_TACT_LENGTH);
}
