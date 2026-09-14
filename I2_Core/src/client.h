#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/dispatch.h>

#define GLOBAL_ATTACH_POINT_I1  "/net/Intersection_1/dev/name/local/I1_attach"
#define GLOBAL_ATTACH_POINT_CTL  "/net/Control_Node/dev/name/local/CTL_attach"
#define GLOBAL_ATTACH_POINT_TR  "/net/Train_Node/dev/name/local/TR_attach"


// prototypes
int client(char *sname);
