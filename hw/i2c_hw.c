#include "i2c_hw.h"
#include "i2c.h"

#define I2C_MASTER    MXC_I2C1
#define I2C_FREQ_HZ   400000UL

void I2C_HW_Init(void)
{
  int error;

  error = MXC_I2C_Init(I2C_MASTER, 1, 0);
  (void)error;
  MXC_I2C_SetFrequency(I2C_MASTER, I2C_FREQ_HZ);
}

void I2C_HW_Disable(void)
{
  MXC_I2C_Shutdown(I2C_MASTER);
}

int I2C_HW_Send(uint8_t addr, const uint8_t *data, size_t len)
{
  mxc_i2c_req_t reqMaster;

  reqMaster.i2c = I2C_MASTER;
  reqMaster.addr = addr;
  reqMaster.tx_buf = (unsigned char*)data;
  reqMaster.tx_len = len;
  reqMaster.rx_buf = NULL;
  reqMaster.rx_len = 0;
  reqMaster.restart = 0;
  reqMaster.callback = NULL;

  if ((MXC_I2C_MasterTransaction(&reqMaster)) == 0)
  {
  }

  return 0;
}
