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
#include "cli.h"
#include "msg_service.h"
#include "scenarios.h"

#define SERVER_INFO_PATH "/tmp/CLI.info"

// Modified `my_data` struct to use `char` for the data field
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // Our data (unique id from client)
    char data[256];          // Our data (changed to char)
} command_data;

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} command_reply;

typedef struct {
    int serverPID;
    int serverCHID;
} server_info;

void *server_cli(void *arg) {
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

    command_data msg;
    int rcvid = 0, msgnum = 0;  	// no message received yet
    int Stay_alive = 0;				// server stays running (ignores _PULSE_CODE_DISCONNECT request)
    int living = 1;

    command_reply replymsg; 				// replymsg structure for sending back to client
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
                    printf("\nCommunication with CLI down");
                    living = 0;  // Set living to 0 to stop the main loop
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
                printf("\n gns service is running....");
                continue;	// go back to top of while loop
            }

            // Some other I/O message was received; reject it
            if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) {
                MsgError(rcvid, ENOSYS);
                printf("\n Server received and IO message and rejected it....");
                continue;	// go back to top of while loop
            }

            // A message received
            char * Command = msg.data; // Set sensor input based on received message
            printf("Command received : %s\n", Command);

            if (strcmp(Command, "Switch_peak_1") == 0 || strcmp(Command, "Switch_peak_0") == 0 )
            {
            	//Send a Start Message to everyone
            	if (Command[strlen(Command) - 1] == '0')
            		SwitchPeak(0);
            	else if (Command[strlen(Command) - 1] == '1')
            		SwitchPeak(1);
            }

            // Put your message handling code here and assemble a reply message
            sprintf(replymsg.buf, "Signal %d received (%c)", msgnum, Command);

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
