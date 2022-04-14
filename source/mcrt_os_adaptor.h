/*
 * mcrt_os_adaptor.h
 *
 *  Created on: Mar 15, 2022
 *      Author: mg
 */

#ifndef MCRT_OS_ADAPTOR_H_
#define MCRT_OS_ADAPTOR_H_

#define MCRT_OS_ADAPTOR

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_ITEM_LENGTH 											(8)
#define MAX_PRIORITY												(3)
#define MAX_ITEM_SIZE												(10)

/*******************************************************************************
 * Types
 ******************************************************************************/

enum EVT_PRIO
{
	EVT_PRIO_LOW = 0,
	EVT_PRIO_MID,
	EVT_PRIO_HIGH
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

long trigger_evt_fromISR(enum EVT_PRIO enumEvtPrio, char *pEvt);
long trigger_evt(enum EVT_PRIO enumEvtPrio, char *pEvt);
void init_mcrt_os (void);

void enter_critical_section_adaptor();
void exit_critical_section_adaptor();


#endif /* MCRT_OS_ADAPTOR_H_ */
