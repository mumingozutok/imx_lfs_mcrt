/*
 * mcrt_adaptor.c
 *
 *  Created on: Mar 4, 2022
 *      Author: mg
 */

#include "fsl_gpt.h"
#include "fsl_gpio.h"
#include "fsl_lpuart.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "mcrt_os_adaptor.h"
#include "mcrt_adaptor.h"

#define UART_COMM_GPT_IRQ_ID     GPT2_IRQn
#define UART_COMM_GPT            GPT2
#define UART_COMM_GPT_IRQHandler GPT2_IRQHandler
#define UART_COMM_GPT_CLK_FREQ CLOCK_GetFreq(kCLOCK_OscRc48MDiv2)

#define SYS_TICK_GPT_IRQ_ID     GPT1_IRQn
#define SYS_TICK_GPT            GPT1
#define SYS_TICK_GPT_IRQHandler GPT1_IRQHandler
#define SYS_TICK_GPT_CLK_FREQ CLOCK_GetFreq(kCLOCK_OscRc48MDiv2)

#define UART_COMM_LPUART            LPUART1
#define UART_COMM_LPUART_CLK_FREQ   BOARD_DebugConsoleSrcFreq()
#define UART_COMM_LPUART_IRQn       LPUART1_IRQn
#define UART_COMM_LPUART_IRQHandler LPUART1_IRQHandler

uint32_t sys_tick = 0;

static Digital_Channel inputChannel[ECU_INPUT_CH_COUNT];
static Digital_Channel outputChannel[ECU_OUTPUT_CH_COUNT];

volatile uint8_t f_ModBusComm_Rcv_Evt = 0;
uint8_t get_ModBusComm_Rcv_Evt() {return f_ModBusComm_Rcv_Evt;}
void clr_ModBusComm_Rcv_Evt(){f_ModBusComm_Rcv_Evt = 0;}

//Interrupt Handlers
//timer interrupt handler
void UART_COMM_GPT_IRQHandler(void)
{
	uint8_t ui8evtMsgBuffer[1]={0x01};
	/* Clear interrupt flag.*/
	GPT_ClearStatusFlags(UART_COMM_GPT, kGPT_OutputCompare1Flag);

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif

	stop_timer();

#ifdef MCRT_OS_ADAPTOR
	//f_ModBusComm_Rcv_Evt = 1;
	trigger_evt_fromISR(EVT_PRIO_HIGH, ui8evtMsgBuffer);

#else
	//process the datas in the runtime comm channel-0
	Runtime_CommDataService_Process_DataBuffer(0);
#endif
}

//uart interrupt handler
void UART_COMM_LPUART_IRQHandler(void)
{
	uint8_t uart_rx_data[1];

	/* If new data arrived. */
	if ((kLPUART_RxDataRegFullFlag)&LPUART_GetStatusFlags(UART_COMM_LPUART))
	{
		uart_rx_data[0] = LPUART_ReadByte(UART_COMM_LPUART);
		Runtime_CommDataService_NewData_Received(0, &uart_rx_data, 1);
		start_timer();
	}
	SDK_ISR_EXIT_BARRIER;
}

void initiate_uart_timer(){
	uint32_t gptFreq;
	gpt_config_t gptConfig;

	GPT_GetDefaultConfig(&gptConfig);

	/* Initialize GPT module */
	GPT_Init(UART_COMM_GPT, &gptConfig);

	/* Divide GPT clock source frequency by 3 inside GPT module */
	GPT_SetClockDivider(UART_COMM_GPT, 1);

	/* Get GPT clock frequency */
	gptFreq = UART_COMM_GPT_CLK_FREQ;

	/* GPT frequency is divided by 3 inside module */
	//gptFreq /= 3;

	/* Set both GPT modules to 100 usecond duration */
	GPT_SetOutputCompareValue(UART_COMM_GPT, kGPT_OutputCompare_Channel1, 1000);

	/* Enable GPT Output Compare1 interrupt */
	GPT_EnableInterrupts(UART_COMM_GPT, kGPT_OutputCompare1InterruptEnable);

	NVIC_SetPriority(UART_COMM_GPT_IRQ_ID, 2);

	/* Enable at the Interrupt */
	EnableIRQ(UART_COMM_GPT_IRQ_ID);
}

void initiate_system_tick_timer(){
	uint32_t gptFreq;
	gpt_config_t gptConfig;

	GPT_GetDefaultConfig(&gptConfig);

	/* Initialize GPT module */
	GPT_Init(SYS_TICK_GPT, &gptConfig);

	/* Divide GPT clock source frequency by 3 inside GPT module */
	GPT_SetClockDivider(SYS_TICK_GPT, 1000);

	/* Get GPT clock frequency */
	gptFreq = SYS_TICK_GPT_CLK_FREQ;

	GPT_StartTimer(SYS_TICK_GPT);
}

void initiate_uart(){
	lpuart_config_t config;

	LPUART_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx     = true;
	config.enableRx     = true;

	LPUART_Init(UART_COMM_LPUART, &config, UART_COMM_LPUART_CLK_FREQ);

	/* Enable RX interrupt. */
	LPUART_EnableInterrupts(UART_COMM_LPUART, kLPUART_RxDataRegFullInterruptEnable);
	EnableIRQ(UART_COMM_LPUART_IRQn);
}

void start_timer(){
	GPT_StopTimer(UART_COMM_GPT);
	GPT_StartTimer(UART_COMM_GPT);
}

void stop_timer(){
	GPT_StopTimer(UART_COMM_GPT);
}

uint32_t hal_get_tick() {
	return GPT_GetCurrentTimerCount(SYS_TICK_GPT);
}

void hal_modbus_uart_tx(uint8_t *pData, uint16_t Size) {
	/*uint16_t i;

	for(i=0;i<Size;i++){
		//MAP_UART_transmitData(EUSCI_A0_BASE, pData[i]);
		LPUART_WriteByte(UART_COMM_LPUART, pData[i]);
	}
	*/
	LPUART_WriteBlocking(UART_COMM_LPUART, pData, Size);
}

void initiate_input_channels() {
	uint8_t i;
	gpio_pin_config_t led_config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};

	//s1
	inputChannel[0].port = BOARD_USER_BUTTON_GPIO;
	inputChannel[0].pin = BOARD_USER_BUTTON_GPIO_PIN;

	for(i=0;i<ECU_INPUT_CH_COUNT;i++){
		GPIO_PinInit(inputChannel[0].port, inputChannel[0].pin, &led_config);
	}
}

void initiate_output_channels() {
	uint8_t i = 0;
	gpio_pin_config_t led_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
	outputChannel[0].port = BOARD_USER_LED_GPIO;
	outputChannel[0].pin = BOARD_USER_LED_GPIO_PIN;

	for(i=0;i<ECU_OUTPUT_CH_COUNT;i++){
		GPIO_PinWrite(outputChannel[i].port, outputChannel[i].pin, 0U);
		GPIO_PinInit(outputChannel[i].port, outputChannel[i].pin , &led_config);
	}
}

void hal_gpio_write_pin(uint16_t chNum, uint8_t value) {
	if(chNum >= ECU_OUTPUT_CH_COUNT) return;

	if(value == 1){
		GPIO_PinWrite(outputChannel[chNum].port, outputChannel[chNum].pin, 1U);
	}

	else{
		GPIO_PinWrite(outputChannel[chNum].port, outputChannel[chNum].pin, 0U);
	}
}

uint8_t hal_gpio_read_pin(uint32_t chNum) {
	if(chNum >= ECU_INPUT_CH_COUNT) return;

	return GPIO_PinRead(inputChannel[chNum].port, inputChannel[chNum].pin);
	//return GPIO_PinRead(BOARD_USER_BUTTON_GPIO, BOARD_USER_BUTTON_GPIO_PIN);
}

void get_uniqueid(uint8_t *id, uint16_t len) {
	uint32_t *buf = id;
	buf[0] = 0xAABBCCDD;
	buf[1] = 0xEEFF0011;
	buf[2] = 2233445566;
}

void initiate_ECU(){
	initiate_input_channels();
	initiate_output_channels();

	initiate_uart_timer();
	initiate_uart();
	initiate_system_tick_timer();
}


//testing for the adaptation layer
//output testing function
void test1(){
	static bool g_pinSet = false;

	if (g_pinSet)
	{
		hal_gpio_write_pin(0, 1U);
		g_pinSet = false;
	}
	else
	{
		hal_gpio_write_pin(0, 0U);
		g_pinSet = true;
	}
}

//input testing function
void test2(){
	if(hal_gpio_read_pin(0) == 1){
		hal_gpio_write_pin(0, 1);
	}
	else{
		hal_gpio_write_pin(0, 0);
	}
}

//system tick testing function
void test3(){
	PRINTF("HAL Tick: %d\r\n", hal_get_tick());
}
