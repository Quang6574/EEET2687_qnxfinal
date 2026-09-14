#include <stdint.h>

#include "server.h"
#include "msg_service.h"
#include "msg_parser.h"
#include "log.h"
#include "pretty_print.h"

extern int I1_init_resp;
extern int I2_init_resp;
extern int TR_init_resp;

int blockingValue = 0;
int stateValue = 0;

extern States currentState;

my_reply ParseMSG(Shortcode code, Src src_node, int32_t data)
{
	//Message received from Control Node should be either Response or PING messages
	my_reply replymsg = {0};
	switch(code)
	{
	case PING:

//		printf("Ping data is %d:\n", data);

		if(src_node != TR){
			currentState = data;
		}

		if (src_node == TR) {
			blockingValue = data;
		} else {
			stateValue = data;
		}

		PrettyPrinter(stateValue, blockingValue);

		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	default:
		break;
	}
	return replymsg;
}

my_reply ParseRESP(Src src_node, int32_t data)
{
	if (src_node == I1 && data == OK) {
		I1_init_resp = 1;
	}

	if (src_node == I2 && data == OK) {
		I2_init_resp = 1;
	}

	if (src_node == TR && data == OK) {
		TR_init_resp = 1;
	}

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
		replymsg= ParseRESP(payload->src, payload->data);
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	default:
		//Control Node should not receive signal or unused message, so add warning in log
		WARN_LOG("Ill-formated message received", "DISCARDED")
		sprintf(replymsg.buf, "%d", Msg_craft(0, 0, 0, 0));
		break;
	}

	return replymsg;
}



