#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <time.h>

#include "server.h"
#include "msg_service.h"
#include "msg_parser.h"
#include "pedestrian.h"

#define SERVER_INFO_PATH "/tmp/OffPeak-SMServer.info"

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // Our data (unique id from client)
    char data;          // Our data
} sensor_data;

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} sensor_reply;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronisation

extern int I1_id;

extern int CTL_server_coid;
extern int I2_server_coid;
extern int TR_server_coid;

int living = 1;
int isLocked_OFF = 0;
int Offpeak_SM_living;

int server_thread_created = 0; // Flag to check if the server thread is created
char sensorInput = '\0';

extern enum states currentState;
extern int Switch_asked;

/*** Server code ***/
void *server(void *arg) {
    int serverPID = 0, chid = 0; 	// Server PID and channel ID

    serverPID = getpid();			// Get server process ID

    // Create Channel
    chid = ChannelCreate(_NTO_CHF_DISCONNECT);
    if (chid == -1) {  // _NTO_CHF_DISCONNECT flag used to allow detach
        printf("\nFailed to create communication channel on server\n");
        pthread_exit((void*)EXIT_FAILURE);
    }

    // Open the file in write mode
    FILE *file;
    file = fopen(SERVER_INFO_PATH, "w");

    if (file == NULL) {
        perror("Failed to open file");
        pthread_exit((void*)EXIT_FAILURE);
    }

    // Write serverPID and chid to the file
    fprintf(file, "%d\n", serverPID);  // Write server PID to the first line
    fprintf(file, "%d\n", chid);       // Write Channel ID to the second line

    // Close the file
    fclose(file);

    sensor_data msg;
    int rcvid = 0, msgnum = 0;  	// no message received yet
    int Stay_alive = 0;				// server stays running (ignores _PULSE_CODE_DISCONNECT request)

    sensor_reply replymsg; 				// replymsg structure for sending back to client
    replymsg.hdr.type = 0x01;
    replymsg.hdr.subtype = 0x00;

    while (living) {
        // Do your MsgReceive's here now with the chid
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);

        if (rcvid == -1) {  // Error condition, exit
            printf("\nFailed to MsgReceive\n");
            break;
        }

        // did we receive a Pulse or message?
        // for Pulses:
        if (rcvid == 0) {  //  Pulse received, work out what type
            switch (msg.hdr.code) {
            // TODO: Need to address what happens if the sensors go down
            case _PULSE_CODE_DISCONNECT:
                // A client disconnected all its connections by running name_close() for each name_open() or terminated
                if (Stay_alive == 0) {
                    ConnectDetach(msg.hdr.scoid);
                    printf("\nCommunication with sensors down\n\n");
                    continue;

                } else {
                    printf("\nServer received Detach pulse from ClientID:%d but rejected it ...\n", msg.ClientID);
                }

                break;

            case _PULSE_CODE_UNBLOCK:
                // REPLY blocked client wants to unblock (was hit by a signal or timed out). It's up to you if you reply now or later.
                printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
                break;

            case _PULSE_CODE_COIDDEATH:  // from the kernel
                printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
                break;

            case _PULSE_CODE_THREADDEATH: // from the kernel
                printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
                break;

            default:
                // Some other pulse sent by one of your processes or the kernel
                printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
                break;
            }

            continue; // go back to top of while loop
        }

        // for messages:
        if (rcvid > 0) { // if true then A message was received
            msgnum++;

            // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
            if (msg.hdr.type == _IO_CONNECT) {
                MsgReply(rcvid, EOK, NULL, 0);
                printf("\n gns service is running....\n");
                continue;	// go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) {
                MsgError(rcvid, ENOSYS);
                printf("\n Server received and IO message and rejected it....\n");
                continue;	// go back to top of while loop
            }

            // A message received
            sensorInput = msg.data; // Set sensor input based on received message

			if (sensorInput == 'x') {
				PedestrianRequest(PED_NS, isLocked_OFF);
			} else if (sensorInput == 'y') {
				PedestrianRequest(PED_EW, isLocked_OFF);
			}

            // Put your message handling code here and assemble a reply message
            sprintf(replymsg.buf, "Signal %d received (%c)", msgnum, sensorInput);
            if (sensorInput == 'n') {
				printf("\n ---> Car sensed in North/South Signal\n\n");
            } else if (sensorInput == 'e') {
				printf("\n ---> Car sensed in East/West Signal\n\n");
            } else if (sensorInput == 'x') {
				printf("\n ---> Pedestrian pressed cross button for North/South\n\n");
            } else if (sensorInput == 'y') {
				printf("\n ---> Pedestrian pressed cross button for East/West\n\n");
            }
            fflush(stdout);

            MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));

        } else {
            printf("\nERROR: Server received something, but could not handle it correctly\n");
        }
    }

    printf("\nServer received Destroy command\n");
    // Destroyed channel before exiting
    ChannelDestroy(chid);

    pthread_exit(EXIT_SUCCESS);
}

// Function to manage the traffic light state machine
void *TrafficLightSM(void* arg) {
	printf("***Traffic Light Off Peak Started***\n");
	currentState = *(enum states *) arg;

	int32_t pingCTRL;
	int32_t alert;

	while (Offpeak_SM_living) {

		pthread_mutex_lock(&mutex); // Lock the mutex to safely access shared variables

		if (isLocked_OFF) {
			printf("	***INCOMING TRAIN***\n");
		}

		// State transition logic
		switch (currentState) {
		case EWG_NSR:
			printf("EWG-NSR: East-West Green, North-South Red\n");
			// Send ping message to CTRL Node
			pingCTRL = Msg_craft(MSG, PING, I1, EWG_NSR);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);

			while (1) {
				if (sensorInput == 'n' && isLocked_OFF) {
					printf("Train detected: Cannot change state until train departs\n");
					sensorInput = '\0';
				} else if (sensorInput == 'x' && isLocked_OFF) {
					printf("Train detected: Cannot change state until train departs\n");
					sensorInput = '\0';
				} else if (sensorInput == 'n' && !isLocked_OFF) {
					printf("Car on North-South detected: request change to EWR-NSG\n");
					break;
				} else if (!isLocked_OFF &&
						(PedestrianPending(PED_NS) || PedestrianPending(PED_EW))) {
					printf("Pedestrian request detected: changing to a safe crossing phase\n");
					break;
				} else if (Switch_asked == 1) {
					Switch_asked = 0;
					break;
				} else if (Offpeak_SM_living == 0){
					break;
				}
			}

			pthread_mutex_lock(&mutex);
			// Reset sensor input after processing
			sensorInput = '\0';
			pthread_mutex_unlock(&mutex);

			alert = Msg_craft(MSG, LSRM, I1, EWY_NSR);
			Msg_send((Msg *) &alert, I1_id, I2_server_coid);
			Msg_send((Msg *) &alert, I1_id, TR_server_coid);

			currentState = EWY_NSR;
			break;

		case EWY_NSR:
			printf("EWY-NSR: East-West Yellow, North-South Red\n");
			// Send ping message to CTRL Node
			pingCTRL = Msg_craft(MSG, PING, I1, EWY_NSR);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);
			sleep(3);

			currentState = EWR_NSR_1;
			break;

		case EWR_NSR_1:
			printf("EWR-NSR: East-West Red, North-South Red\n");
			// Send ping message to CTRL Node
			pingCTRL = Msg_craft(MSG, PING, I1, EWR_NSR_1);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);
			sleep(1);

			if (PedestrianTakePending(PED_NS, isLocked_OFF)) {
				PedestrianService(PED_NS);
				currentState = EWG_NSR;
			} else {
				currentState = EWR_NSG;
			}
			break;

		case EWR_NSG:
			// Send ping message to CTRL Node
			printf("EWR-NSG: East-West Red, North-South Green\n");
			pingCTRL = Msg_craft(MSG, PING, I1, EWR_NSG);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);

			while(1) {
				if (isLocked_OFF) {
					printf("Train detected. Changing state\n");
					break;
				} else if (!isLocked_OFF &&
						(PedestrianPending(PED_NS) || PedestrianPending(PED_EW))) {
					printf("Pedestrian request detected: changing to a safe crossing phase\n");
					break;
				} else if (sensorInput == 'e') {
					printf("Car on East-West detected : request change to EWG-NSR\n");
					break;
				} else if (Switch_asked == 2) {
					Switch_asked = 0;
					break;
				} else if (Offpeak_SM_living == 0) {
					break;
				}
			}

			pthread_mutex_lock(&mutex);
			// Reset sensor input after processing
			sensorInput = '\0';
			pthread_mutex_unlock(&mutex);

			// Message both I2 and Train Node
			alert = Msg_craft(MSG, LSRM, I1, EWR_NSY);
			Msg_send((Msg *) &alert, I1_id, I2_server_coid);
			Msg_send((Msg *) &alert, I1_id, TR_server_coid);

			currentState = EWR_NSY;
			break;

		case EWR_NSY:
			printf("EWR-NSY: East-West Red, North-South Yellow\n");
			sleep(3);
			// Send ping message to CTRL Node
			pingCTRL = Msg_craft(MSG, PING, I1, EWR_NSY);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);

			currentState = EWR_NSR_2;
			break;

		case EWR_NSR_2:
			printf("EWR-NSR: East-West Red, North-South Red\n");
			sleep(1);
			// Send ping message to CTRL Node
			pingCTRL = Msg_craft(MSG, PING, I1, EWR_NSR_2);
			Msg_send((Msg *) &pingCTRL, I1_id, CTL_server_coid);

			if (PedestrianTakePending(PED_EW, isLocked_OFF)) {
				PedestrianService(PED_EW);
			}

			currentState = EWG_NSR;
			break;

		default:
			printf("Invalid State!\n");

			currentState = EWR_NSR_1;
			break;
		}
		pthread_mutex_unlock(&mutex); // Unlock the mutex after updating shared variables
	}
	return NULL;
}

int OffPeakSMStart(enum states *CurrentState) {

    pthread_t server_thread, SM_thread;

    // Check if the server thread already exists
	pthread_mutex_lock(&mutex);
	if (!server_thread_created) {
		// Create the server thread
		if (pthread_create(&server_thread, NULL, server, NULL) != 0) {
			perror("Failed to create server thread");
			pthread_mutex_unlock(&mutex);
			return EXIT_FAILURE;
		}
		server_thread_created = 1; // Mark the thread as created
	}
	pthread_mutex_unlock(&mutex);

	// Execute state machine
	Offpeak_SM_living = 1;
	if (pthread_create(&SM_thread, NULL, TrafficLightSM, CurrentState) != 0) {
		perror("Failed to create server thread");
		Offpeak_SM_living = 0;
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}
