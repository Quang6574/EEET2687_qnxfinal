#include <stdint.h>

#include "msg_service.h"
#include "msg_parser.h"
#include "train_sm.h"
#include "server.h"

my_reply ParseMSG(Shortcode code, Src src_node, int32_t data)
{
	my_reply replymsg;

	switch (code)
	{
	case INIT:
		//data here is the Initial state send by the control node
		//data can be cast to the type State defined in msg_parser.h
		//No need to import your state union since it's already in msg_parser.h

//		currentState = (States) data;

		// MSG = RSP/ACK/TR/OK
		sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, TR, OK));
		break;

	case STRT:
		//data here is the peak on which we start
		//TODO : Start the proper state machine

		if (data == 0 || data == 1) {

			TrainSMStart();

			// MSG = RSP/ACK/TR/OK
			sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, TR, OK));
		} else {
			// Below is when the data they send on start doesn't match to a SM (OnPeak || OffPeak)
			// MSG = RSP/ACK/TR/ERR_ON_NODE
			sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, TR, ERR_ON_NODE));
		}

		break;
	case LSRM:
		//TODO : Handle Light switch request. See if its possible to switch if yes then switch
		//if no, answer should be lel no

		if (src_node == I1 || src_node == I2){
			// Ack all LSRM's from I1 and I2
			sprintf(replymsg.buf, "%d", Msg_craft(RESP, ACKR, TR, OK));

		} else {
			// If LSRM from CTRL maybe do something else

		}
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
