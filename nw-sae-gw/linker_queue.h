#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include "linker_pthread.h"

/* define */
#define NUM_QUEUE_GTPV2_NUMBER     (PGW_PTHREAD_UDP_MAX)
#define NUM_QUEUE_GTPV2_NODE_MAX   (3/NUM_QUEUE_GTPV2_NUMBER)
#define QUEUE_RET_ERRO    (-1)
#define QUEUE_RET_SUCC    (0)

typedef struct{
        unsigned int* typepoint;
        char buf[1024 + 1];
        unsigned int bufsize;
        struct sockaddr_in peer;
}Type_gtpv2_msg_queue_node;

typedef struct{
        unsigned int queue_length;
        unsigned int queue_head;
        unsigned int queue_tail;
        pthread_mutex_t queue_mutex;
        Type_gtpv2_msg_queue_node queue_node[NUM_QUEUE_GTPV2_NODE_MAX];
}Type_queue_gtpv2;

void init_mutex_queue();
void destory_mutex_queue();
void init_queue(Type_queue_gtpv2* q);
