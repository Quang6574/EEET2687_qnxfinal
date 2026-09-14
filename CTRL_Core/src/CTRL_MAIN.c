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
#include "cli.h"

name_attach_t *attach_ctl;

int I1_server_coid;
int I2_server_coid;
int TR_server_coid;

int I1_init_resp = 0;
int I2_init_resp = 0;
int TR_init_resp = 0;

int restart_try = 5;
int Ctrl_id = 100;

States InitState = EWR_NSR_1;

States currentState = 0;

int Server_Init()
{
	// Create a global name (/dev/name/global/...)
	if ((attach_ctl = name_attach(NULL, ATTACH_POINT_CTL, 0)) == NULL)
	{
		printf("\nFailed to name_attach on ATTACH_POINT: %s \n", ATTACH_POINT_CTL);
	    printf("\n Possibly another server with the same name is already running or you need to start the gns service!\n");
	    return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int Client_Init()
{
	 printf("---> Trying to connect to server named: %s\n", GLOBAL_ATTACH_POINT_I2);
	 if ((I2_server_coid = name_open(GLOBAL_ATTACH_POINT_I2, 0)) == -1)
	 {
	     printf("\nERROR, could not connect to server!\n\n");
	     return EXIT_FAILURE;
	 }
	 printf("---> Trying to connect to server named: %s\n", GLOBAL_ATTACH_POINT_I1);
	 if ((I1_server_coid = name_open(GLOBAL_ATTACH_POINT_I1, 0)) == -1)
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

	 printf("Connection established to: %s, %s and %s\n", GLOBAL_ATTACH_POINT_I2, GLOBAL_ATTACH_POINT_I1, GLOBAL_ATTACH_POINT_TR);
	 return EXIT_SUCCESS;
}

void InitNodes(int32_t Initial_state) {
	int32_t Initmsg = Msg_craft(1, 1, 0, Initial_state);

	printf("Sending Init Message to I1, Targeted COID (%d)\n", I1_server_coid);
	Msg_send((Msg *) &Initmsg, Ctrl_id, I1_server_coid);

	printf("Sending Init Message to I2, targeted COID (%d)\n", I2_server_coid);
	Msg_send((Msg *) &Initmsg, Ctrl_id, I2_server_coid);

	printf("Sending Init Message to TR, targeted COID: (%d)\n", TR_server_coid);
	Msg_send((Msg *) &Initmsg, Ctrl_id, TR_server_coid);

}

void StartNodes() {
	int32_t Startmsg = Msg_craft(1, 2, 0, 0);

	printf("Sending Start Message to I1, Targeted COID (%d)\n", I1_server_coid);
	Msg_send((Msg *) &Startmsg, Ctrl_id, I1_server_coid);

	printf("Sending Init Message to I2, targeted COID (%d)\n", I2_server_coid);
	Msg_send((Msg *) &Startmsg, Ctrl_id, I2_server_coid);

	printf("Sending Init Message to TR, targeted COID: (%d)\n", TR_server_coid);
	Msg_send((Msg *) &Startmsg, Ctrl_id, TR_server_coid);
}

int main(void) {

	//Init the server and attach points
	int servcode = Server_Init();

	pthread_t cli_thread;
	//In case of previous failure try to restart the server
	while(servcode && (restart_try > 0))
	{
		printf("Retrying connection....\n");
		servcode = Server_Init(attach_ctl);
		restart_try--;
	}
	//serv definitely fail so exit.
	if (servcode) {
		printf("Error while starting the server\n");
		return EXIT_FAILURE;
	}

	//Init Command Line Interface server
	// Create the CLI thread
	if (pthread_create(&cli_thread, NULL, server_cli, NULL) != 0) {
		perror("Failed to create server thread");
		return EXIT_FAILURE;
	}
	printf("CLI server running!\n");

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

	printf("Local Node fully connected, listening entering connection on : %d\n", attach_ctl->chid);

	Th_args receiver_arg = {0};
	receiver_arg.attach = attach_ctl;

	pthread_t msg_recv_tid = 0;

	//START THREADS FOR EACH COMMUNICATION CHANNELS AND START PARSING MESSAGE LOGIC
	pthread_create(&msg_recv_tid, NULL, Msg_receiver, (void*) &receiver_arg);

	//Starting phase Send Init messages and if every response is positive start node
	InitNodes(INITIAL_STATE);
	if (I1_init_resp && I2_init_resp && TR_init_resp)
		StartNodes();
	else
		printf("Init/Start phase failed !");

	pthread_join(msg_recv_tid, NULL);

	return EXIT_SUCCESS;
}
