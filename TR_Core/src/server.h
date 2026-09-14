#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <sys/dispatch.h>

// Define where the "local" attach point is located. It will be located at <hostname>/dev/name/global/myname"
#define ATTACH_POINT_TR "TR_attach"  // This must be the same name that is used for the client.

#define BUF_SIZE 256

typedef union {  // This replaced the standard:  union sigval
	union{
		_Uint32t sival_int;
		void *sival_ptr;// This has a different size in 32-bit and 64-bit systems
	};
	_Uint32t dummy[4]; // Hence, we need this dummy variable to create space
}_mysigval;


typedef struct _Mypulse {   // This replaced the standard:  typedef struct _pulse msg_header_t;
	_Uint16t type;
	_Uint16t subtype;
	_Int8t code;
	_Uint8t zero[3];         // Same padding that is used in standard _pulse struct
	_mysigval value;
	_Uint8t zero2[2];// Extra padding to ensure alignment access.
	_Int32t scoid;
} msg_header_t;

typedef struct {
	msg_header_t hdr;  // Custom header
	int ClientID;      // our data (unique id from client)
	int data;          // our data <-- This is what we are here for
} my_data;

typedef struct {
	msg_header_t hdr;   // Custom header
	char buf[BUF_SIZE]; // Message to send back to send back to other thread
} my_reply;

typedef struct args {
	name_attach_t *attach;
} Th_args;
