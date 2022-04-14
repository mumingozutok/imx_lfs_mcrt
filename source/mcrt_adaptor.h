/*
 * mcrt_adaptor.h
 *
 *  Created on: Mar 4, 2022
 *      Author: mg
 */

#ifndef MCRT_ADAPTOR_H_
#define MCRT_ADAPTOR_H_

#define ECU_OUTPUT_CH_COUNT 1
#define ECU_INPUT_CH_COUNT 1

typedef struct S_Digital_Channel{
    uint32_t port;
    uint32_t pin;
} Digital_Channel;

void initiate_ECU();
void comm_timer_handler();
void comm_data_recv_handle();
void start_timer();
void stop_timer();
uint32_t hal_get_tick();
void initiate_runtime();
uint8_t get_ModBusComm_Rcv_Evt();
void clr_ModBusComm_Rcv_Evt();

void MCRT_LPUART_IRQHandler(void);

#endif /* MCRT_ADAPTOR_H_ */
