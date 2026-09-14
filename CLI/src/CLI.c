#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

#define BUF_SIZE 256
#define SERVER_INFO_PATH "/tmp/CLI.info"

// Modified `my_data` struct to use `char` for the data field
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // Our data (unique id from client)
    char data[256];          // Our data (changed to char)
} sensor_data;

typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    char buf[BUF_SIZE]; // Message we send back to clients to tell them the messages was processed correctly.
} sensor_reply;

typedef struct {
    int serverPID;
    int serverCHID;
} server_info;

// Prototypes
void *client_thread(void *arg);

int main(int argc, char *argv[]) {
    printf("Client running\n");

    // Connection data
    int serverPID = 0;   // Placeholder for server PID
    int serverCHID = 0;  // Placeholder for server Channel ID

    FILE *file;
    // Open the file in read mode
    file = fopen(SERVER_INFO_PATH, "r");
    if (file == NULL) {
        perror("Failed to open server info file");
        return EXIT_FAILURE;
    }

    // Read serverPID and serverCHID from the file
    if (fscanf(file, "%d\n%d", &serverPID, &serverCHID) != 2) {
        perror("Failed to read server information from file");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Close the file
    fclose(file);

    printf("Read Server PID: %d, Channel ID: %d from file\n", serverPID, serverCHID);

    // Create server info struct to pass to the thread
    server_info *info = (server_info *)malloc(sizeof(server_info));
    info->serverPID = serverPID;
    info->serverCHID = serverCHID;

    // Create a client thread
    pthread_t client_tid;
    int ret = pthread_create(&client_tid, NULL, client_thread, (void *)info);
    if (ret != 0) {
        perror("Failed to create client thread");
        free(info);
        return EXIT_FAILURE;
    }

    // Wait for the client thread to finish
    pthread_join(client_tid, NULL);

    printf("Main (client) Terminated....\n");
    return EXIT_SUCCESS;
}

/*** Client code as a thread ***/
void *client_thread(void *arg) {
    server_info *info = (server_info *)arg;
    int serverPID = info->serverPID;
    int serverChID = info->serverCHID;

    sensor_data msg;
    sensor_reply reply;

    msg.ClientID = 600;  // Unique client ID

    int server_coid;     // Server connection ID

    printf("   --> Trying to connect (server) process which has a PID: %d\n", serverPID);
    printf("   --> on channel: %d\n\n", serverChID);

    // Set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, serverPID, serverChID, _NTO_SIDE_CHANNEL, 0);
    if (server_coid == -1) {
        printf("\n    ERROR, could not connect to server!\n\n");
        pthread_exit((void*) EXIT_FAILURE);
    }

    printf("Connection established to process with PID:%d, Ch:%d\n", serverPID, serverChID);

    // Initialise the message header
    msg.hdr.type = 0x00;
    msg.hdr.subtype = 0x00;

    char *input = calloc(256, 1);          // For user input

    // Read user input and send data packets
    while (1) {
        printf("Enter command: \n");
        printf("\n\tSwitch_peak_0: Change to Onpeak mode\n");
        printf("\tSwitch_peak_1: Change to Offpeak mode\n");
        printf("\n");
        scanf("%s", input);
        if (strcmp(input, "Switch_peak_0") == 0 || strcmp(input, "Switch_peak_1") == 0)
        {
            fflush(stdout);
            sprintf(msg.data, "%s", input);
        	int err = MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply));
        	if (err == -1) {
        	    printf("Failed to send command Switch_peak\n");
        	    break;
        	}
        	else
        	{
        		printf("Command Sent!\n");
                printf("   -->Reply is: '%s'\n\n", reply.buf);

        	}
        }
    }

    // Close the connection
    printf("\n Sending message to server to tell it to close the connection\n");
    ConnectDetach(server_coid);
    free(info);
    pthread_exit(EXIT_SUCCESS);
}
