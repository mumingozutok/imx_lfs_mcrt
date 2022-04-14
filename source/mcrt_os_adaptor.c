/*
 * mcrt_os_adaptor.c
 *
 *  Created on: Mar 15, 2022
 *      Author: mg
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

//mcrt includes
#include "MCRuntime_V1.h"

#include "mcrt_os_adaptor.h"




/*******************************************************************************
 * Globals
 ******************************************************************************/

/* Priority queue handles */
static QueueHandle_t objPrioQ[MAX_PRIORITY] = { NULL, NULL, NULL };

/* Semaphore for event-message service */
static SemaphoreHandle_t xMsgServiceSem = NULL;

/* Task priorities. */
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* Application API */
BaseType_t trigger_evt(enum EVT_PRIO enumEvtPrio, char *pEvt);

/* Event worker tasks */
static void mcrt_system_task  (void *pvParameters);
static void low_prio_evt_task  (void *pvParameters);
static void mid_prio_evt_task  (void *pvParameters);
static void high_prio_evt_task (void *pvParameters);

void enter_critical_section_adaptor(){
	taskENTER_CRITICAL();
}

void exit_critical_section_adaptor(){
	taskEXIT_CRITICAL();
}

/*!
 * @brief log_init function
 */
void init_mcrt_os(void)
{
	xMsgServiceSem = xSemaphoreCreateBinary();

    if (xMsgServiceSem == NULL)
    {
        while (1)
            ;
    }

    xSemaphoreGive(xMsgServiceSem);

	objPrioQ[EVT_PRIO_LOW] = xQueueCreate(MAX_ITEM_SIZE, MAX_ITEM_LENGTH);

    /* Enable queue view in MCUX IDE FreeRTOS TAD plugin. */
    if (objPrioQ[EVT_PRIO_LOW] != NULL)
    {
        vQueueAddToRegistry(objPrioQ[EVT_PRIO_LOW], "lowPrioQ");
    }

    if (xTaskCreate(low_prio_evt_task, "low_prio_task", 1000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        while (1)
            ;
    }

	objPrioQ[EVT_PRIO_MID] = xQueueCreate(MAX_ITEM_SIZE, MAX_ITEM_LENGTH);

    // Enable queue view in MCUX IDE FreeRTOS TAD plugin.
    if (objPrioQ[EVT_PRIO_MID] != NULL)
    {
        vQueueAddToRegistry(objPrioQ[EVT_PRIO_MID], "midPrioQ");
    }

    if (xTaskCreate(mid_prio_evt_task, "mid_prio_task", 2000, NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        while (1)
            ;
    }

	objPrioQ[EVT_PRIO_HIGH] = xQueueCreate(MAX_ITEM_SIZE, MAX_ITEM_LENGTH);

    /* Enable queue view in MCUX IDE FreeRTOS TAD plugin. */
    if (objPrioQ[EVT_PRIO_HIGH] != NULL)
    {
        vQueueAddToRegistry(objPrioQ[EVT_PRIO_HIGH], "highPrioQ");
    }

    if (xTaskCreate(high_prio_evt_task, "high_prio_task", 1000, NULL, tskIDLE_PRIORITY + 3, NULL) != pdPASS)
    {
        while (1)
            ;
    }

    if (xTaskCreate(mcrt_system_task, "mcrt_system_task", configMINIMAL_STACK_SIZE + 166, NULL, tskIDLE_PRIORITY + 4, NULL) != pdPASS)
    {
        while (1)
            ;
    }
}

/*!
 * @brief trigger_evt function
 */
long trigger_evt(enum EVT_PRIO enumEvtPrio, char *pEvt)
{
	/*
	 * Generate a message event based on the assigned priority
	 * and allow submitting the event content.
	 */
	if ((enumEvtPrio <= EVT_PRIO_HIGH) && (pEvt != NULL))
	{
		if (xQueueSend(objPrioQ[enumEvtPrio], pEvt, 0) != pdPASS)
		{
			return pdFAIL;
		}
	}

	return pdPASS;
}

long trigger_evt_fromISR(enum EVT_PRIO enumEvtPrio, char *pEvt)
{
	/*
	 * Generate a message event based on the assigned priority
	 * and allow submitting the event content.
	 */
	BaseType_t xHigherPriorityTaskWoken;

	/* We have not woken a task at the start of the ISR. */
	xHigherPriorityTaskWoken = pdFALSE;

	if ((enumEvtPrio <= EVT_PRIO_HIGH) && (pEvt != NULL))
	{
		if (xQueueSendFromISR(objPrioQ[enumEvtPrio], pEvt, &xHigherPriorityTaskWoken) != pdTRUE )
		{
			return pdFAIL;
		}

		//Now the buffer is empty we can switch context if necessary.
		if( xHigherPriorityTaskWoken )
		{
			taskYIELD ();
		}
	}

	return pdPASS;
}
/*******************************************************************************
 * Event-interface functions
 ******************************************************************************/

/*!
 * @brief evt_simulation_task function
 */
static void mcrt_system_task(void *pvParameters)
{
	uint8_t counter = 0;
	uint8_t ui8evtMsgBuffer[MAX_ITEM_LENGTH]={0x00};
	uint16_t mcrt_cyclic_loop_trig_period = 1;

	TickType_t xLastWakeTime;

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	//system task will trig mid priority cyclic loop in every 1 ms
	//low priority loop will be triggered at 10 ms period
	while (1)
	{
		//first control execution list of the cyclic mcrt tasks
		//if the last function run then trig again
		if(control_exec_list() == 1){
			trigger_evt(EVT_PRIO_MID, ui8evtMsgBuffer);
		}

		else{
			mcrt_cyclic_loop_trig_period += 1;
		}

    	vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(mcrt_cyclic_loop_trig_period));
	}

    vTaskSuspend(NULL);
}

/*!
 * @brief low_prio_evt_task function
 */
static void low_prio_evt_task(void *pvParameters)
{
	uint32_t ui8evtMsgBuffer[2];


	/*
	 * Service all low-priority events!
	 */
	while (1)
	{
		if (xQueueReceive(objPrioQ[EVT_PRIO_LOW], ui8evtMsgBuffer, portMAX_DELAY) == pdPASS)
		{
			//print_evt_service(ui8evtMsgBuffer);
			void (*callback)(param_index_t) = (void*) (ui8evtMsgBuffer[0]);
			(callback)(ui8evtMsgBuffer[1]);

		}
		else
		{
			//PRINTF("low priority event Task message failure! \r\n");
		}

	}

	vTaskSuspend(NULL);
}

/*!
 * @brief mid_prio_evt_task function
 */
static void mid_prio_evt_task(void *pvParameters)
{
	uint8_t ui8evtMsgBuffer[MAX_ITEM_LENGTH];
	/*
	 * Service all mid-priority events!
	 */

	TickType_t xLastWakeTime;

	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		if (xQueueReceive(objPrioQ[EVT_PRIO_MID], ui8evtMsgBuffer, portMAX_DELAY) == pdPASS)
		{
			mcCore_SM();
		}
		else
		{
		}
	}

	vTaskSuspend(NULL);
}

/*!
 * @brief high_prio_evt_task function
 */
static void high_prio_evt_task(void *pvParameters)
{
	uint8_t ui8evtMsgBuffer[MAX_ITEM_LENGTH];

	/*
	 * Service all high-priority events!
	 */
	while (1)
	{
		if (xQueueReceive(objPrioQ[EVT_PRIO_HIGH], ui8evtMsgBuffer, portMAX_DELAY) == pdPASS)
		{
			if(ui8evtMsgBuffer[0] == 1){
				Runtime_CommDataService_Process_DataBuffer(0);
			}
		}
		else
		{
		}
	}

	vTaskSuspend(NULL);
}


