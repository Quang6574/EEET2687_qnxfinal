#include <stdint.h>

#include "msg_service.h"
#include "msg_parser.h"
#include "on_peak_sm.h"
#include "off_peak_sm.h"
#include "server.h"

int peakTypeFlag;

extern int Offpeak_SM_living;
extern int Onpeak_SM_living;

enum states currentState;
int Switch_asked = 0;

my_reply ParseMSG(Shortcode code, Src src_node, int32_t data)
{
	my_reply replymsg;

	switch (code)
	{
	case INIT:
		//data here is the Initial state send by the control node
		//data can be cast to the type State defined in msg_parser.h
		//No need to import your state union since it's already in msg_parser.h

		currentState = (States) data;
		// MSG = RSP/ACK/I2/OK
		sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, I2, OK));
		break;

	case STRT:
		//data here is the peak on which we start
		//data=0 -> OnPeak || data=1 -> OffPeak

		if (Offpeak_SM_living && data == 0) {
			Offpeak_SM_living = 0;

			OnPeakSMStart(currentState);

		} else if (Onpeak_SM_living && data == 1) {
			Onpeak_SM_living = 0;

			OffPeakSMStart(&currentState);

		} else if (data == 0) {
			OnPeakSMStart(currentState);
		} else if (data == 1) {
			OffPeakSMStart(&currentState);
		} else {
			sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, I1, ERR_ON_NODE));
			break;
		}

		sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, I1, OK));

		break;

	case LSRM:
		//TODO : Handle Light switch request. See if its possible to switch if yes then switch
		//if no, answer should be lel no

		// If signal from a train node, lock or unlock depending on state
		if (src_node == TR) {
			if (data == 0) {
				isLocked_ON = 0;
				isLocked_OFF = 0;
			} else if (data == 1) {
				isLocked_ON = 1;
				isLocked_OFF = 1;
			}
		} else if (src_node == I1 || src_node == CTRL) {
			if (data == EWY_NSR) {
				Switch_asked = 1;
			} else if (data == EWR_NSY) {
				Switch_asked = 2;
			}
		}
		sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, I2, OK));
		break;
	default:
		//Other Messages : PING or Unused is discarded
		//Craft an empty answer
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		//Return the answer
		return replymsg;
		break;
	}
	return replymsg;
}

my_reply ParseRESP()
{
	//TODO
	my_reply replymsg = {0};

	return replymsg;
}

my_reply ParseSIG()
{
	//TODO
	my_reply replymsg = {0};

	return replymsg;
}

my_reply ParseMsg(Msg* payload)
{
	my_reply replymsg;

	Type type = payload->type;

	switch(type)
	{
	case MSG:
		replymsg = ParseMSG(payload->shortcode, payload->src, payload->data);
		break;
	case RESP:
		//In this case the message received is from a response no need to answer an answer
		replymsg = ParseRESP();
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	case SIGNAL:
		//In this case the node needs to take immediate action !
		replymsg = ParseSIG();
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	default:
		//Node should not receive unused message, so answer an empty message
		printf("Ill-formated message received : DISCARDED\n");
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	}

	return replymsg;
}
