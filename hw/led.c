#include "drivers.h"
#include "led.h"

#if defined(DEF_DA26)
  #define LED_PORT      GPIO_PORTA
  #define LED_PIN       27
#else
  #define LED_PORT      GPIO_PORTA
  #define LED_PIN       28
#endif

/** \brief Configure blinking LED
 *
 * \return void
 *
 */
void LED_Configuration(void)
{
  /**< Configure LED pin */
  GPIO_SetupPin(LED_PORT, LED_PIN, GPIO_PIN_FUNC_OFF, GPIO_DIRECTION_OUT, GPIO_LEVEL_LOW);
}

/** \brief Toggle LED
 *
 * \return void
 *
 */
void LED_Toggle(void)
{
  GPIO_TogglePin(LED_PORT, LED_PIN);
}
