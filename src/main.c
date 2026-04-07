#include <stdio.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "gpio.h"
#include "analog.h"
#include "i2c_hw.h"
#include "sh1106.h"
#include "sh1106_fonts.h"
#include "rtos.h"
#include "fft.h"

#define MXC_GPIO_PORT_OUT MXC_GPIO0
#define MXC_GPIO_PIN_OUT MXC_GPIO_PIN_14

static sh1106_t display;

/**< Initializing task, will be suspended after completion */
static void MainTask(void *pParameters)
{
  (void) pParameters;
  char str[32];
  uint16_t counter = 0U;

  (void)sh1106_init(&display, I2C_HW_Send, SH1106_I2C_ADDR);
  fft_init();

  ANALOG_StartDMA();

  while (1)
  {
    sprintf(str, "%02d:%02d", (counter % 3600) / 60, counter % 60);
    sh1106_draw_new_string(&display, 0, 16, str, &font_24x40, SH1106_WHITE);
    sh1106_update(&display);
    MXC_GPIO_OutClr(MXC_GPIO_PORT_OUT, MXC_GPIO_PIN_OUT);
    vTaskDelay(pdMS_TO_TICKS(500U));
    MXC_GPIO_OutSet(MXC_GPIO_PORT_OUT, MXC_GPIO_PIN_OUT);
    vTaskDelay(pdMS_TO_TICKS(500U));
    counter++;
    if (ANALOG_IsReady() == true)
    {
      ANALOG_StartDMA();
    }
  }
}

static void GPIO_Init(void)
{
  mxc_gpio_cfg_t gpio_out;

  /* Setup output pin. */
  gpio_out.port = MXC_GPIO_PORT_OUT;
  gpio_out.mask = MXC_GPIO_PIN_OUT;
  gpio_out.pad = MXC_GPIO_PAD_NONE;
  gpio_out.func = MXC_GPIO_FUNC_OUT;
  gpio_out.vssel = MXC_GPIO_VSSEL_VDDIO;
  gpio_out.drvstr = MXC_GPIO_DRVSTR_0;
  MXC_GPIO_Config(&gpio_out);
}

/**< Initializing task, will be suspended after completion */
static void InitTask(void *pParameters)
{
  (void) pParameters;
  BaseType_t xRet;

  /**< Initialize and check EEPROM */
  //EEPROM_Configuration();

  GPIO_Init();

  #if defined(DEF_RELEASE)
  /**< Enable watchdog */
  //HW_StartWD();
  #endif

  xRet = xTaskCreate(MainTask, (const char *) "Main Task", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
  (void)xRet;

//  /**< Create ADC task with lowest priority */
//  xRet = xTaskCreate(MONITOR_Task, (const char *) "ADC Task", MONITOR_TASK_STACK_SIZE, NULL, MONITOR_TASK_PRIORITY, NULL);
//  (void)xRet;
//  /**< Create communication task with high priority */
//  xRet = xTaskCreate(COMM_Task, (const char *) "Comm Task", COMM_TASK_STACK_SIZE, NULL, COMM_TASK_PRIORITY, &xTaskComm);
//  (void)xRet;

  #if defined(DEF_CAN)
  /**< Create CAN communication task with high priority */
//  T_CommData comm_data = {EE_Bitrate,
//                          EE_CanId,
//                          EE_DevId,
//                          EE_CanMode,
//                          EE_NodeId,
//                          EE_ReplySkip};
//  xRet = xTaskCreate(COMM_CAN_Task, (const char *) "CAN Task", COMM_TASK_STACK_SIZE, (void*)&comm_data, COMM_TASK_PRIORITY, NULL);
//  (void)xRet;
  /**< Allocate the stack for CAN thread */
//  tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, COMM_TASK_STACK_SIZE, TX_NO_WAIT);
//  tx_thread_create(&comm_thread, "CAN Task", COMM_CAN_Task, 0, pointer, COMM_TASK_STACK_SIZE, COMM_TASK_PRIORITY, COMM_TASK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  #endif

  /**< Suspend current initialization task, is not deleted because of heap_1, dynamically memory configuration is not wished */
  vTaskSuspend(NULL);
}

int main(void)
{
  //HW_Init();

  I2C_HW_Init();
  ANALOG_Configuration();

  NVIC_SetPriorityGrouping(0);  // All bits = preempt priority, no subpriority

  /**< Create initializing task because of usage of RTOS functions during EEPROM initialization */
  xTaskCreate(InitTask, (const char *) "Init Task", INIT_TASK_STACK_SIZE, NULL, INIT_TASK_PRIORITY, NULL);

  /**< Start FreeRTOS Scheduler */
  vTaskStartScheduler();
}
