#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

#define BUF_SIZE 256
#define SERVER_INFO_PATH "/tmp/OffPeak-SMServer.info"

// Modified `my_data` struct to use `char` for the data field
typedef struct {
    struct _pulse hdr;  // Our real data comes after this header
    int ClientID;       // Our data (unique id from client)
    char data;          // Our data (changed to char)
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

    msg.ClientID = 500;  // Unique client ID
    char input;          // For user input

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

    // Read user input and send data packets
    while (1) {
        printf("Enter sensor input ('e' for East-West Car | 'n' for North-South Car | 'x' for North-South Ped | 'y' for East-West Ped): ");
        input = getchar();

        while (getchar() != '\n');  // Clear the input buffer

        if (input == 'e' || input == 'n' || input == 'x' || input == 'y') {
            // Set up the data packet with user input
            msg.data = input;
            // The data we are sending is in msg.data
            printf("Client (ID:%d), sending sensor input as data packet with the character value: %c\n", msg.ClientID, msg.data);
            fflush(stdout);
            int err = MsgSend(server_coid, &msg, sizeof(msg), &reply, sizeof(reply));
            if (err == -1) {
                printf("Error data '%c' NOT sent to server\n", msg.data);
                break;
            }
            else {
                // Now process the reply
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
