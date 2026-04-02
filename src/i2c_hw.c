#include "i2c_hw.h"
#include "i2c.h"

#define I2C_MASTER  MXC_I2C0
#define I2C_FREQ    100000

void I2C_HW_Init(void)
{
  int error;
  
  error = MXC_I2C_Init(I2C_MASTER, 1, 0);
  MXC_I2C_SetFrequency(I2C_MASTER, I2C_FREQ);
}

void I2C_HW_Disable(void)
{
  MXC_I2C_Shutdown(I2C_MASTER);  
}

void I2C_HW_Send(uint8_t addr, uint8_t *tx_data, uint16_t len)
{
  mxc_i2c_req_t reqMaster;

  reqMaster.i2c = I2C_MASTER;
  reqMaster.addr = addr;
  reqMaster.tx_buf = tx_data;
  reqMaster.tx_len = len;
  reqMaster.rx_buf = NULL;
  reqMaster.rx_len = 0;
  reqMaster.restart = 0;
  reqMaster.callback = NULL;

  if ((MXC_I2C_MasterTransaction(&reqMaster)) == 0) 
  {
  }
}