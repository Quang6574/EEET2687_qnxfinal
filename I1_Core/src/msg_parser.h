#pragma once

#include "msg_service.h"
#include "server.h"

typedef enum states {
	EWG_NSR,
	EWY_NSR,
	EWR_NSR_1,
	EWR_NSG,
	EWR_NSY,
	EWR_NSR_2
} States;

enum respcode {
	ERR_ON_NODE,
	OK,
} ResponseCode;

// Check which peak it's in (OffPeak = 0 | OnPeak = 1)
extern int peakTypeFlag;

my_reply ParseMsg(Msg* payload);
