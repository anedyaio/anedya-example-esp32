
// This file declares all the parameters required by all tasks during runtime.

#ifndef _RTPARAMETER_H_
#define _RTPARAMETER_H_


#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

//============================================================================
// Runtime EventGroups
//============================================================================
extern EventGroupHandle_t ConnectionEvents; // Declaration

#define WIFI_FAIL BIT0
#define CONNECTION_FAIL BIT1
#define CONNECTION_PUSH_FAIL BIT3

#endif // !_RTPARAMETER_H_