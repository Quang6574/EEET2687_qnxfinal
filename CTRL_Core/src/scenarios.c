#include <stdint.h>
#include <stdio.h>

#include "scenarios.h"
#include "msg_parser.h"

extern int Ctrl_id;

extern int I1_server_coid;
extern int I2_server_coid;

void SwitchPeak(int input)
{
	//Send a start Msg with the last saved state to I1 and I2
	uint32_t switchmsg = Msg_craft(MSG, STRT, CTRL, input);
	Msg_send((Msg*) &switchmsg, Ctrl_id, I1_server_coid);
	Msg_send((Msg*) &switchmsg, Ctrl_id, I2_server_coid);
	printf("Switch Peak Message Sent\n\n");

}
