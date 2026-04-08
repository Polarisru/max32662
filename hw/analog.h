#ifndef ANALOG_H
#define ANALOG_H

#include <stdint.h>
#include <stdbool.h>

#define ANALOG_BUFF_LEN         2048U

void ANALOG_Configuration(void);
void ANALOG_StartDMA(void);
bool ANALOG_IsReady(void);
uint8_t* ANALOG_GetFullBuff(void);
uint16_t ANALOG_GetRate(void);

#endif
