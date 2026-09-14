#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/dispatch.h>

#define GLOBAL_ATTACH_POINT_I2  "/net/Intersection_2/dev/name/local/I2_attach"
#define GLOBAL_ATTACH_POINT_I1  "/net/Intersection_1/dev/name/local/I1_attach"
#define GLOBAL_ATTACH_POINT_TR  "/net/Train_Node/dev/name/local/TR_attach"


// prototypes
int client(char *sname);
