/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
osThreadId  defaultTaskHandle01;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
 static uint32_t blink_counter = 10;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osMutexId ledMutexHandle;
osStaticMutexDef_t ledMutexControlBlock;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void BlinkTaskRun(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */


  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of ledMutex */
  osMutexStaticDef(ledMutex, &ledMutexControlBlock);
  ledMutexHandle = osMutexCreate(osMutex(ledMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(defaultTask01, BlinkTaskRun, osPriorityNormal, 0, 128);
  defaultTaskHandle01 = osThreadCreate(osThread(defaultTask01), (void *)&blink_counter);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
#define BLINK_GREEN_LED() do { asm("NOP"); } while (0);

void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	uint8_t i = 0;
  /* Infinite loop */
  for(;;)
  {
    osDelay(300);
    osThreadSuspend(defaultTaskHandle01);
    if (i++ >= 3)
    {
    	osThreadResume(defaultTaskHandle01);
    	i = 0;
    }
	osMutexWait(ledMutexHandle, osWaitForever);
	BLINK_GREEN_LED();
	osMutexRelease(ledMutexHandle);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
#define   BLINK_GREEN_LED()  do { asm("NOP"); } while(0);

void BlinkTaskRun(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  uint32_t i = 0;
   uint32_t count = *((uint32_t*)argument);
  /* Infinite loop */
  for(;;)
  {
  	for (i = 0; i < count; i++)
  	{
  		//osMutexWait(ledMutexHandle, osWaitForever);
  		if ( osOK != osMutexWait(ledMutexHandle, 100) )
  		{
  			break;
  		}
  		BLINK_GREEN_LED();
  		osMutexRelease(ledMutexHandle);

  		osDelay(200);
  		if (count >= 2)
  		{
  			osThreadYield(); // run next thread
  		}
  	}

  	RED_GREEN_LED();

  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
