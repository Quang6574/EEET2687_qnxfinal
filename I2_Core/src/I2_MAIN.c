#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>

#include "server.h"
#include "client.h"
#include "msg_service.h"
#include "msg_parser.h"

name_attach_t *attach_i2;

int CTL_server_coid;
int I1_server_coid;
int TR_server_coid;

int I2_id = 300;

int restart_try = 5;

int Server_Init()
{
	// Create a global name (/dev/name/global/...)
	if ((attach_i2 = name_attach(NULL, ATTACH_POINT_I2, 0)) == NULL)
	{
		printf("\nFailed to name_attach on ATTACH_POINT: %s \n", ATTACH_POINT_I2);
	    printf("\nPossibly another server with the same name is already running or you need to start the gns service!\n");
	    return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int Client_Init()
{
	 printf("---> Trying to connect to server named: %s\n", GLOBAL_ATTACH_POINT_I1);
	 if ((I1_server_coid = name_open(GLOBAL_ATTACH_POINT_I1, 0)) == -1)
	 {
	     printf("\nERROR, could not connect to server!\n\n");
	     return EXIT_FAILURE;
	 }
	 printf("---> Trying to connect to server named: %s\n", GLOBAL_ATTACH_POINT_CTL);
	 if ((CTL_server_coid = name_open(GLOBAL_ATTACH_POINT_CTL, 0)) == -1)
	 {
	     printf("\nERROR, could not connect to server!\n\n");
	     return EXIT_FAILURE;
	 }
	 printf("---> Trying to connect to server named: %s\n", GLOBAL_ATTACH_POINT_TR);
	 if ((TR_server_coid = name_open(GLOBAL_ATTACH_POINT_TR, 0)) == -1)
	 {
	     printf("\nERROR, could not connect to server!\n\n");
	     return EXIT_FAILURE;
	 }

	 printf("Connection established to: %s, %s and %s\n", GLOBAL_ATTACH_POINT_I1, GLOBAL_ATTACH_POINT_CTL, GLOBAL_ATTACH_POINT_TR);
	 return EXIT_SUCCESS;
}

int main(void) {

	//Init the server and attach points
	int servcode = Server_Init();
	//In case of previous failure try to restart the server
	while(servcode && (restart_try > 0))
	{
		printf("Retrying connection....\n");
		servcode = Server_Init(attach_i2);
		restart_try--;
	}
	//serv definitely fail so exit.
	if (servcode) {
		printf("Error while starting the server\n");
		return EXIT_FAILURE;
	}

	printf("Local Server Initialized !\n");

	sleep(5);
	restart_try = 5;

	//Init the client connect to attach points
	int clientcode = Client_Init();
	//In case of previous failure try to restart the server
	while(clientcode && (restart_try > 0)) {
		printf("Retrying connection....\n");
		clientcode = Client_Init();
		restart_try--;
	}
	//Server definitely fail so exit.
	if (clientcode) {
		printf("Error while starting the server\n");
		return EXIT_FAILURE;
	}

	printf("Local Node fully connected !\n");

	Th_args receiver_arg = {0};
	receiver_arg.attach = attach_i2;

	pthread_t msg_recv_tid = 0;

	//START THREADS FOR EACH COMMUNICATION CHANNELS AND START PARSING MESSAGE LOGIC
	pthread_create(&msg_recv_tid, NULL, Msg_receiver, (void*) &receiver_arg);
	pthread_join(msg_recv_tid, NULL);


	return EXIT_SUCCESS;
}
