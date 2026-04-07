#ifndef I2C_HW_H
#define I2C_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void I2C_HW_Init(void);
void I2C_HW_Disable(void);
int I2C_HW_Send(uint8_t addr, const uint8_t *data, size_t len);

#endif
