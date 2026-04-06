#ifndef RTOS_H
#define RTOS_H

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**< Configure task sizes */
#define INIT_TASK_STACK_SIZE        200
#define MAIN_TASK_STACK_SIZE        200
#define COMM_TASK_STACK_SIZE        400

#define INIT_TASK_PRIORITY          (tskIDLE_PRIORITY + 5)
#define MAIN_TASK_PRIORITY          (tskIDLE_PRIORITY + 2)
#define COMM_TASK_PRIORITY          (tskIDLE_PRIORITY + 3)

/**< CAN queue defined in SW module */
extern QueueHandle_t xQueueCAN;
/**< RS485 task handle defined  */
extern TaskHandle_t xTaskRS485;
/**< Communication sensors task handle defined  */
extern TaskHandle_t xTaskComm;

#endif
