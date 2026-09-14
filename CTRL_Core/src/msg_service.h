#pragma once

#include <stdint.h>
#include <pthread.h>

typedef enum type {
    UNKNOWN = 0,
    MSG = 1,
    RESP = 2,
    SIGNAL = 3,
} Type;

typedef enum shortcode {
    UNUSED = 0,
    INIT = 1,
    STRT = 2,
    PING = 3,
    LSRM = 4,
    ACKR = 5,
    LSRR = 6,
    DCTL = 7,
    ELSS = 8,
    LSS = 9,
} Shortcode;

typedef enum src {
    CTRL = 0,
    I1 = 1,
    I2 = 2,
    P1 = 3,
    P2 = 4,
    TR = 5,
} Src;

typedef struct {
    uint32_t data: 22;
    uint32_t src : 4;
    uint32_t shortcode : 4;
    uint32_t type : 2;
}__attribute__((packed)) Msg;



uint32_t Msg_craft(int32_t type, int32_t shortcode, int32_t src, int32_t data);
void Msg_send(Msg *msg, int ClientId, int target_coid);
void *Msg_receiver(void *args);
int Init_MsgServices(pthread_t *msgr_tid);

