#ifndef __MCRuntime_H__
#define __MCRuntime_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_UART_CALLBACK_FUNCTION_SLOT 0

//Core Functions
void mcCore_SM();

//report functions
uint32_t control_exec_list();

//Communication functions
void Runtime_CommDataService_NewData_Received(uint32_t channel, uint8_t* buf, uint32_t len);
uint8_t Runtime_CommDataService_Process_DataBuffer(uint32_t channel);

#ifdef __cplusplus
}
#endif

#endif
