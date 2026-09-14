#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <unistd.h>  // For getpid()
#include <fcntl.h>   // For open(), write(), and close()
#include <string.h>  // For strlen()
#include <pthread.h> // For threading

#include "server.h"
#include "msg_service.h"
#include "msg_parser.h"

#define SERVER_INFO_PATH "/tmp/Train-SMServer.info"

// Define states for the state machine
enum trainStates {
	IntersectionUnlocked,
	IntersectionLocked,
} TrainState;

// Structure for client message data
typedef struct {
	struct _pulse hdr;  // Header
	int ClientID;       // Unique client ID
	int data;           // Data
} sensor_data;

// Structure for server reply message
typedef struct {
	struct _pulse hdr;  // Header
	char buf[BUF_SIZE]; // Reply message to client
} sensor_reply;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronisation

extern int TR_id;

extern int CTL_server_coid;
extern int I1_server_coid;
extern int I2_server_coid;

enum trainStates CurrentState;

char sensorInput = '\0';
int server_thread_created = 0; // Flag to check if the server thread is created

int Train_SM_living = 0;
int living = 1;
char trainPresent;

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
                    printf("\nCommunication with train sensors down\n\n");
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

            // Put your message handling code here and assemble a reply message
            sprintf(replymsg.buf, "Signal %d received (%c)", msgnum, sensorInput);
            if (sensorInput == 't') {
				printf("\n ---> Train sent a signal\n\n");
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

// Function for traffic light state machine
void *Railway(void* arg) {

	CurrentState = *(enum states *) arg;

	int32_t pingmsg;
	int32_t i1msg;
	int32_t i2msg;

	printf("Signal from Control Node received and Railway started successfully !\n");

	while (Train_SM_living) {
		// Train comes when key pressed
		// Two states
		// Variable locked
		// read input from thread

		switch (CurrentState) {
		case IntersectionUnlocked:
			printf("No train incoming, barrier is up\n");
			pingmsg = Msg_craft(MSG, PING, TR, IntersectionUnlocked);
			Msg_send((Msg *) &pingmsg, TR_id, CTL_server_coid);

			while (1) {
				if (sensorInput == 't') {
					CurrentState = IntersectionLocked;
					sensorInput = '\0';
					break;
				}
			}

			i1msg = Msg_craft(MSG, LSRM, TR, IntersectionLocked);
			i2msg = Msg_craft(MSG, LSRM, TR, IntersectionLocked);
			Msg_send((Msg *) &i1msg, TR_id, I1_server_coid);
			Msg_send((Msg *) &i2msg, TR_id, I2_server_coid);

			break;

		case IntersectionLocked:
			printf("Train incoming, barrier is moving down\n");
			pingmsg = Msg_craft(MSG, PING, TR, IntersectionLocked);
			Msg_send((Msg *) &pingmsg, TR_id, CTL_server_coid);

			sleep(6);
			printf("Train incoming, barrier down\n");

			while (1) {
				if (sensorInput == 't') {
					CurrentState = IntersectionUnlocked;
					sensorInput = '\0';
					break;
				}
			}

			i1msg = Msg_craft(MSG, LSRM, TR, IntersectionUnlocked);
			i2msg = Msg_craft(MSG, LSRM, TR, IntersectionUnlocked);
			Msg_send((Msg *) &i1msg, TR_id, I1_server_coid);
			Msg_send((Msg *) &i2msg, TR_id, I2_server_coid);

			break;

		}
	}
	return NULL;
}

int TrainSMStart() {

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

	CurrentState = IntersectionUnlocked;

	// Execute state machine
	Train_SM_living = 1;
	if (pthread_create(&SM_thread, NULL, Railway, &CurrentState) != 0) {
		perror("Failed to create server thread");
		Train_SM_living = 0;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

