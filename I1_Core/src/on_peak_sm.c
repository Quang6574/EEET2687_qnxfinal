#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/neutrino.h>

#include "msg_parser.h"
#include "server.h"
#include "msg_service.h"
#include "pedestrian.h"

extern int I1_id;

extern int CTL_server_coid;
extern int I2_server_coid;
extern int TR_server_coid;

int Onpeak_SM_living;
int isLocked_ON = 0;
extern enum states currentState;

void* OnPeakSM(void *arg) {
	printf("***Traffic Light On Peak Started***\n");
	currentState = *(enum states *) arg;

    Onpeak_SM_living = 1;
    int32_t pingmsg;

	while(Onpeak_SM_living) {

		if (isLocked_ON) {
			printf("	***INCOMING TRAIN***\n");
		}

		switch(currentState) {
		case EWG_NSR:
			printf("East-West Green, North-South Red\tPedestrian: East-West Green, North-South Red.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWG_NSR);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(8);

			if (isLocked_ON) {
				// Stuck in this state until not locked anymore
				printf("Waiting until train passes...\n");
				while(isLocked_ON);
				printf("...Train has left\n");
			}

			currentState = EWY_NSR;

			break;

		case EWY_NSR:
			printf("East-West Yellow, North-South Red\tPedestrian: East-West Flashing Red, North-South Red.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWY_NSR);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(2);
			currentState = EWR_NSR_1;

			break;

		case EWR_NSR_1:
			printf("East-West Red, North-South Red\t\tPedestrian: East-West Red, North-South Red.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWR_NSR_1);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(2);
			if (PedestrianTakePending(PED_NS, isLocked_ON)) {
				PedestrianService(PED_NS);
				currentState = EWG_NSR;
			} else {
				currentState = EWR_NSG;
			}

			break;

		case EWR_NSG:
			printf("East-West Red, North-South Green\tPedesrian: East-West Red, North-South Green.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWR_NSG);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(8);

			if (isLocked_ON) {
				// Change to safe state when train coming
				printf("Train detected - Changing States\n");
				currentState = EWR_NSY;
				break;
			}

			currentState = EWR_NSY;

			break;

		case EWR_NSY:
			printf("East-West Red, North-South Yellow\tPedestrian: East-West Red, North-South Flashing Red.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWR_NSY);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(2);
			currentState = EWR_NSR_2;

			break;

		case EWR_NSR_2:
			printf("East-West Red, North-South Red\t\tPedestrian: East-West Red, North-South Red.\n");

			// Ping CTRL Node
			pingmsg = Msg_craft(MSG, PING, I1, EWR_NSR_2);
			Msg_send((Msg*) &pingmsg, I1_id, CTL_server_coid);

			sleep(2);
			if (PedestrianTakePending(PED_EW, isLocked_ON)) {
				PedestrianService(PED_EW);
			}

			currentState = EWG_NSR;

			break;

		}
	}

	return 0;
}

int OnPeakSMStart(enum states currentState) {

    pthread_t SM_thread;

	// Execute state machine
	Onpeak_SM_living = 1;
	if (pthread_create(&SM_thread, NULL, OnPeakSM, &currentState) != 0) {
		perror("Failed to create server thread");
		Onpeak_SM_living = 0;
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}
