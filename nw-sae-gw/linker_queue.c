#include "linker_queue.h"

extern unsigned int g_queue_gtpv2_curr;
extern Type_queue_gtpv2 gtpv2_queue[NUM_QUEUE_GTPV2_NUMBER];

void init_mutex_queue()
{
	int rc;
	int loop;
	for(loop=0; loop<NUM_QUEUE_GTPV2_NUMBER; loop++)
	{
	    printf("gtpv2_queue[%d].queue_mutex[%d]\n", loop, gtpv2_queue[loop].queue_mutex);
	    rc = pthread_mutex_init(&(gtpv2_queue[loop].queue_mutex), NULL);
	    if(rc != 0)
	    {
		printf("mutex init err:[%s]\n", strerror(errno));
	    }
	    printf("gtpv2_queue[%d].queue_mutex[%d]\n", loop, gtpv2_queue[loop].queue_mutex);
        }
}

void destory_mutex_queue()
{
	int loop;
	for(loop=0; loop<NUM_QUEUE_GTPV2_NUMBER; loop++)
	{
	    pthread_mutex_destroy(&gtpv2_queue[loop].queue_mutex);
        }
}

void init_queue(Type_queue_gtpv2* q)
{
    memset((char *)q, 0, sizeof(Type_queue_gtpv2) * NUM_QUEUE_GTPV2_NUMBER);
    return;
}

