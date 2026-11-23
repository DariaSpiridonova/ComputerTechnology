#ifndef CHAT_H
#define CHAT_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PATH_TO_SERVER_NAME "server.c"

#define NAME_LEN             64
#define MESSAGE_LEN          256
#define MAX_CLIENTS          64

const int SERVER_TYPE  = 1;     

const int ACT_REGISTER = 1;     
const int ACT_GET_ID   = 2;     
const int ACT_SEND_MSG = 3;     
const int ACT_PING     = 4;     
const int ACT_EXIT     = 5;     

struct msg_buf
{
    long mtype;                  
    int action; 
    long client_id; 
    char client_name[NAME_LEN];
    char rcv_name[NAME_LEN];
    char message[MESSAGE_LEN];
};

const size_t PACKET_SIZE = sizeof(struct msg_buf) - sizeof(long);

#endif