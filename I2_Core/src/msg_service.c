#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "msg_service.h"
#include "msg_parser.h"
#include "server.h"

void print_msg(Msg *msg)
{
    printf("Message Received !\n");
    printf("Type : ");
    switch (msg->type)
    {
    case 0:
        printf("NAN\n");
        break;
    case 1:
        printf("Message\n");
        break;
    case 2:
        printf("Response\n");
        break;
    default:
        printf("Signal\n");
        break;
    }

    printf("Shortcode : ");
    switch (msg->shortcode)
    {
    case 1:
        printf("INIT\n");
        break;
    case 2:
        printf("STRT\n");
        break;
    case 3:
        printf("PING\n");
        break;
    case 4:
        printf("LSRM\n");
        break;
    case 5:
        printf("ACKR\n");
        break;
    case 6:
        printf("LSRR\n");
        break;
    case 7:
        printf("DCTL\n");
        break;
    case 8:
        printf("ELSS\n");
        break;
    case 9:
        printf("LSS\n");
        break;
    default:
        printf("UNUSED\n");
        break;
    }
    printf("src : ");
    switch (msg->src)
    {
    case 0:
        printf("Control Node\n");
        break;
    case 1:
        printf("I1 Inter Node\n");
        break;
    case 2:
        printf("I2 Inter Node\n");
        break;
    case 3:
        printf("P1 Cross Node\n");
        break;
    case 4:
        printf("P2 Cross Node\n");
        break;
    case 5:
        printf("Train Inter Node\n");
        break;
    default:
        break;
    }
    printf("data : %ld\n", (int64_t) msg->data);
}

uint32_t Msg_craft(int32_t type, int32_t shortcode, int32_t src, int32_t data)
{
    uint32_t resp = 0;
    resp = type;
    resp = ((resp << 4) | shortcode);
    resp = ((resp << 4) | src);
    resp = ((resp << 22) | data);
    return resp;
}

void *Msg_receiver(void* args)
{
	my_data msg;
	my_reply replymsg;
	Th_args* arg = (Th_args*) args;

	int rcvid=0, msgnum=0;  		// no message received yet
	int Stay_alive=0, living=0;	// server stays running (ignores _PULSE_CODE_DISCONNECT request)
	living =1;
	while (living)
	{
		// Do your MsgReceive's here now with the chid
	    rcvid = MsgReceive(arg->attach->chid, &msg, sizeof(msg), NULL);

	    if (rcvid == -1)  // Error condition, exit
	    {
	        printf("\nFailed to MsgReceive\n");
	        break;
	    }

	    // did we receive a Pulse or message?
	    // for Pulses:
	    if (rcvid == 0)  //  Pulse received, work out what type
	    {
			printf("\nServer received a pulse from ClientID:%d ...\n", msg.ClientID);
			printf("Pulse received:%d \n", msg.hdr.code);

	        switch (msg.hdr.code)
	        {
	        case _PULSE_CODE_DISCONNECT:
	        	printf("Pulse case:    %d \n", _PULSE_CODE_DISCONNECT);
						// A client disconnected all its connections by running
						// name_close() for each name_open()  or terminated
	        	if( Stay_alive == 0)
	        	{
	        		ConnectDetach(msg.hdr.scoid);
	        		printf("\nServer was told to Detach from ClientID:%d ...\n", msg.ClientID);
	        		living = 0; // kill while loop
					continue;
	        	}
				else
				{
					printf("\nServer received Detach pulse from ClientID:%d but rejected it ...\n", msg.ClientID);
				}
				break;

			case _PULSE_CODE_UNBLOCK:
				// REPLY blocked client wants to unblock (was hit by a signal
				// or timed out).  It's up to you if you reply now or later.
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
	        continue;// go back to top of while loop
	     }

	     // for messages:
	     if(rcvid > 0) // if true then A message was received
	     {
	    	 msgnum++;

			 // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
	    	 if (msg.hdr.type == _IO_CONNECT )
			 {
				 MsgReply( rcvid, EOK, NULL, 0 );
				 printf("\nClient messaged indicating that GNS service is running....");
				 printf("\n    -----> replying with: EOK\n");
				 msgnum--;
				 continue;	// go back to top of while loop
			 }

			 // Some other I/O message was received; reject it
			 if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
			 {
				  MsgError( rcvid, ENOSYS );
				  printf("\n Server received and IO message and rejected it....");
				  continue;	// go back to top of while loop
			 }

			// A message (presumably ours) received
			//TODO IMPLEMENT HANDLING MESSAGE LOGIC FOR I2
			Msg *payload = (Msg *) &(msg.data);
			replymsg = ParseMsg(payload);
			MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
	     }
	     else
	     {
	    	 printf("\nERROR: Server received something, but could not handle it correctly\n");
	     }

	}

	// Remove the attach point name from the file system (i.e. /dev/name/global/<myname>)
	name_detach(arg->attach, 0);
	return (void*) EXIT_SUCCESS;
}


//Msg_sender will change depending on number of channel to send
void Msg_send(Msg *msg, int ClientId, int target_coid)
{

	my_data message;
	my_reply reply; // replymsg structure for sending back to client

	message.ClientID = ClientId;

	uint32_t* payload = (uint32_t *) msg;
	message.data = (uint32_t) *payload;

	if (MsgSend(target_coid, &message, sizeof(message), &reply, sizeof(reply)) == -1)
	{
	     printf(" Error data '%d' NOT sent to server\n", message.data);
	}
	else
	{ // now process the reply

	     // printf("   -->Reply is: '%s'\n", reply.buf);

	     //TODO HERE : IMPLEM LOGIC FOR I2 NODE ON REPLY
	}
	return;
}
