#include "linker_pthread.h"
#include "linker_queue.h"
#include "NwGtpv2cIfLog.h"

extern unsigned int g_cpu_start; 
extern unsigned int g_cpu_cur;
extern unsigned int g_cpu_cnt;
extern unsigned int g_pthread_udp_loc;
extern unsigned int g_pthread_ipv4_loc;
extern unsigned int g_pthread_gtpu_loc;

extern pgwPthreadInfo pgw_pthreads_udp;
extern pgwPthreadInfo pgw_pthreads_ipv4;
extern pgwPthreadInfo pgw_pthreads_gtpu;

extern pthread_mutex_t ippool_mutex;
extern pthread_mutex_t teidMap_mutex;
extern pthread_mutex_t tunnelMap_mutex;
extern pthread_mutex_t ipv4AddrMap_mutex;
extern pthread_mutex_t uePgwSessionRbt_mutex;
extern pthread_mutex_t ueSgwSessionRbt_mutex;
extern pthread_mutex_t activeTimerList_mutex;
extern pthread_mutex_t outstandingRxSeqNumMap_mutex;
extern pthread_mutex_t outstandingTxSeqNumMap_mutex;
extern pthread_mutex_t gpGtpv2cTrxnPool_mutex;
extern pthread_mutex_t gpGtpv2cMsgPool_mutex;
extern pthread_mutex_t gpSdpFlowContextPool_mutex;
extern pthread_mutex_t gpTunnelEndPointPool_mutex;
extern pthread_mutex_t gpGtpv2cTimeoutInfoPool_mutex;
extern pthread_mutex_t nwGtpv2cTmrMinHeap_mutex;
extern pthread_mutex_t memPool_mutex;

extern NwSaeGwT saeGw;
extern NwSaeGwT* psaeGw;

/* add 20160501 */
extern Type_queue_gtpv2 gtpv2_queue[NUM_QUEUE_GTPV2_NUMBER];
/* add 20160501 */


void cpu_bind(int bind_type,int cpu_num)
{
    int cpu_max; 
    int ret;
    cpu_set_t mask;
    
    NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO,"##### start #####");
    
    cpu_max=sysconf(_SC_NPROCESSORS_CONF);
    NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG,"CPU max num:[%d]", cpu_max);
    
    if((cpu_num >= cpu_max) || (cpu_num < 0)) 
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO,"input cpu is error ,cpu:[%d]  cpu_max:[%d]",cpu_num,cpu_max);
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO,"##### end #####");
        return;
    }   
    NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG,"cpu bind type[%d] num:[%d]",bind_type,cpu_num);
    
    CPU_ZERO(&mask);
    CPU_SET(cpu_num, &mask);
    if(PGW_CPU_BIND_PROC == bind_type)
    {   
        ret=sched_setaffinity(0, sizeof(mask), &mask);
        NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG,"cpu sched_setaffinity ret:[%d]",ret);
        if( ret < 0)  
        {
            NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO,"cpu sched_setaffinity failed!!! ret:[%d]",ret);
            NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO,"##### end #####");
            return ;
        }   
    }   
    else if(PGW_CPU_BIND_THREAD == bind_type)
    {   
        ret=pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
        NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG,"cpu pthread_setaffinity_np ret:[%d]",ret);
        if( ret < 0)  
        {
            NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO,"cpu pthread_setaffinity_np failed!!! ret:[%d]",ret);
            NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO,"##### end #####");
            return ;
        }   
    }   
    else
    {   
        NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO," bind type error :[%d]",bind_type);
    }   
    NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO,"##### end #####");
    return;
}

void linkerPthreadpoolInit()
{
    memset(&pgw_pthreads_udp, 0, sizeof(pgw_pthreads_udp));
    memset(&pgw_pthreads_ipv4, 0, sizeof(pgw_pthreads_ipv4));
    memset(&pgw_pthreads_gtpu, 0, sizeof(pgw_pthreads_gtpu));
}

int pgw_ptheads_udp_create()
{
    int loop_create;
    int ret;
    int cnt;
    int creat_cnt;
    pgw_pthread_udp_arg *pgw_udp_pthread_arg_tmp;
    int pgw_pthread_udp_loc = PGW_RET_NODATA;

    cnt = (PGW_PTHREAD_UDP_MAX - pgw_pthreads_udp.childNum) / PGW_START_THREADS_UDP_NUM;
    if(cnt == 0)              /* have pockets threads */
    {
        cnt = (PGW_PTHREAD_UDP_MAX - pgw_pthreads_udp.childNum) % PGW_START_THREADS_UDP_NUM;
    }else                     /* have couple threads */
    {
        cnt = PGW_START_THREADS_UDP_NUM;
    }

    if(cnt == 0)              /* no threads */
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "childNum[%d], PGW_PTHREAD_UDP_MAX[%d], can't create", \
                     pgw_pthreads_udp.childNum, PGW_PTHREAD_UDP_MAX);
        return PGW_RET_NODATA;
    }else
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "pthread_create_client cnt [%d], creating...", cnt);
    
    creat_cnt = 0;

    /* create pthreads for client, needs check success */
    for(loop_create=g_pthread_udp_loc; loop_create<PGW_PTHREAD_UDP_MAX; )
    {
        if(pgw_pthreads_udp.client[g_pthread_udp_loc].thd_sts == PGW_THD_STS_TYPE_NULL) 
        {
            pgw_udp_pthread_arg_tmp = malloc(sizeof(pgw_pthread_udp_arg));
            pgw_udp_pthread_arg_tmp->locate = g_pthread_udp_loc;
            
            /* set the thd_sts and CPU for the current struct */
            pgw_pthreads_udp.client[g_pthread_udp_loc].thd_sts = PGW_THD_STS_TYPE_BUSY;
            if(g_cpu_cnt != 0)
            {
                pgw_pthreads_udp.client[g_pthread_udp_loc].CPU = g_cpu_start + (g_cpu_cur++%g_cpu_cnt);
            }else
            {
                pgw_pthreads_udp.client[g_pthread_udp_loc].CPU = -1;
            } 
            
            ret = pthread_create(&(pgw_pthreads_udp.client[g_pthread_udp_loc].child_id), \
                                 NULL, pgw_thread_udp_hander, (void *)pgw_udp_pthread_arg_tmp);
            if(ret == 0)
            {
                /* update globle variable */
                pgw_pthreads_udp.childNum++;
                pgw_pthread_udp_loc = g_pthread_udp_loc; 
                NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "client create ok! client_loc is [%d]", g_pthread_udp_loc);
                loop_create++;
                g_pthread_udp_loc++;
                creat_cnt++;
                usleep(20);
            }else
            {
                free(pgw_udp_pthread_arg_tmp);
                NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "client[%d] create error! release memary!", g_pthread_udp_loc);
            }
        }
        else
        {
            g_pthread_udp_loc++;
            loop_create++;
        }
        if(g_pthread_udp_loc == PGW_PTHREAD_UDP_MAX)
        {
            g_pthread_udp_loc --;
            break;
        }
        if(creat_cnt >= cnt)
        {
            break;
        }
    }
    NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "leave client create, return value is (location) [%d]", pgw_pthread_udp_loc);

    return pgw_pthread_udp_loc;
}

int pgw_pthreads_udp_choose()
{
    int ret = PGW_RET_NODATA;
    int loop;

    for(loop=0; loop<PGW_PTHREAD_UDP_MAX; loop++) {
        if(pgw_pthreads_udp.client[loop].thd_sts == PGW_THD_STS_TYPE_FREE) {
            ret = loop;
            break;
        }
    }

    if(loop == PGW_PTHREAD_UDP_MAX)
    {
        //NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "no enough threads, enter to create...");
        ret = pgw_ptheads_udp_create();
        if(ret == PGW_RET_NODATA)
        {
            NW_SAE_GW_LOG(NW_LOG_LEVEL_WARN, "no threads were choose");
        }
    }
    return ret;
}

void *pgw_thread_udp_hander(void* arg)
{

    pthread_detach(pthread_self());

    pgw_pthread_udp_arg udp_pthread_arg_tmp;

    pgw_pthread_udp_arg* argtmp = (pgw_pthread_udp_arg*)arg;

    udp_pthread_arg_tmp.locate = argtmp->locate;

    free(arg);
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "notes:[%d]", udp_pthread_arg_tmp.locate);


    Type_gtpv2_msg_queue_node queue_node_head_tmp;

    /* CPU bind */
    if(g_cpu_cnt > 0)
    {
        cpu_bind(PGW_CPU_BIND_THREAD, pgw_pthreads_udp.client[udp_pthread_arg_tmp.locate].CPU);
        NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_INFO, "CPU_SET ok!!");
    }

    while(pgw_pthreads_udp.client[udp_pthread_arg_tmp.locate].thd_sts == PGW_THD_STS_TYPE_BUSY)
    {
        pthread_mutex_lock(&gtpv2_queue[udp_pthread_arg_tmp.locate].queue_mutex);

        if(gtpv2_queue[udp_pthread_arg_tmp.locate].queue_length != 0)
        {
#if 0
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue[%d] has msg, befor queue_length[%d], queue_head:[%d], queue_tail[%d]", \
                         udp_pthread_arg_tmp.locate, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_length, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_tail);
#endif

            memcpy( (char*)&queue_node_head_tmp, \
		    (char*)&gtpv2_queue[udp_pthread_arg_tmp.locate].queue_node[gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head], \
                    sizeof(Type_gtpv2_msg_queue_node) );

	    memset((char*)&gtpv2_queue[udp_pthread_arg_tmp.locate].queue_node[gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head],
		    0x00, sizeof(Type_gtpv2_msg_queue_node));

#if 0
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue_node_head[%d].typepoint:[%p]", gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, queue_node_head_tmp.typepoint);
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue_node_head[%d].buf:[%s]", gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, queue_node_head_tmp.buf);
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue_node_head[%d].bufsize:[%d]", gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, queue_node_head_tmp.bufsize);
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue_node_head[%d].peer:[%d]", gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, queue_node_head_tmp.peer);
#endif

            gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head = (gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head + 1) % NUM_QUEUE_GTPV2_NODE_MAX;

            if(gtpv2_queue[udp_pthread_arg_tmp.locate].queue_length == 1)
            {
	        gtpv2_queue[udp_pthread_arg_tmp.locate].queue_tail = gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head;
            }

            gtpv2_queue[udp_pthread_arg_tmp.locate].queue_length--;

#if 0
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---2---queue[%d] has msg, after queue_length[%d], queue_head:[%d], queue_tail[%d]", \
                         udp_pthread_arg_tmp.locate, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_length, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_head, \
                         gtpv2_queue[udp_pthread_arg_tmp.locate].queue_tail);
#endif

            pthread_mutex_unlock(&gtpv2_queue[udp_pthread_arg_tmp.locate].queue_mutex);

            nwGtpv2cProcessUdpReq((NwGtpv2cStackHandleT)queue_node_head_tmp.typepoint, \
                                        queue_node_head_tmp.buf, \
                                        queue_node_head_tmp.bufsize, \
                                        ntohs(queue_node_head_tmp.peer.sin_port), \
                                        (queue_node_head_tmp.peer.sin_addr.s_addr));
            memset((char*)&queue_node_head_tmp, 0x00,sizeof(queue_node_head_tmp));
            //usleep(2);
        }
	else
        {
           pthread_mutex_unlock(&gtpv2_queue[udp_pthread_arg_tmp.locate].queue_mutex);
           usleep(1);
        }
    }
    pgw_pthreads_udp.client[udp_pthread_arg_tmp.locate].thd_sts = PGW_THD_STS_TYPE_END;
    return NULL;
}



int pgw_ptheads_ipv4_create()
{
    int loop_create;
    int ret;
    int cnt;
    int creat_cnt;
    pgw_pthread_ipv4_arg *pgw_ipv4_pthread_arg_tmp;
    int pgw_pthread_ipv4_loc = PGW_RET_NODATA;

    cnt = (PGW_PTHREAD_IPV4_MAX - pgw_pthreads_ipv4.childNum) / PGW_START_THREADS_IPV4_NUM;
    if(cnt == 0)              /* have pockets threads */
    {
        cnt = (PGW_PTHREAD_IPV4_MAX - pgw_pthreads_ipv4.childNum) % PGW_START_THREADS_IPV4_NUM;
    }else                     /* have couple threads */
    {
        cnt = PGW_START_THREADS_IPV4_NUM;
    }

    if(cnt == 0)              /* no threads */
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "childNum[%d], PGW_PTHREAD_IPV4_MAX[%d], can't create", \
                      pgw_pthreads_ipv4.childNum, PGW_PTHREAD_IPV4_MAX);
        return PGW_RET_NODATA;
    }else
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "pthread_create_client cnt [%d], creating...", cnt);
    
    creat_cnt = 0;

    /* create pthreads for client, needs check success */
    for(loop_create=g_pthread_ipv4_loc; loop_create<PGW_PTHREAD_IPV4_MAX; )
    {
        if(pgw_pthreads_ipv4.client[g_pthread_ipv4_loc].thd_sts == PGW_THD_STS_TYPE_NULL) 
        {
            pgw_ipv4_pthread_arg_tmp = malloc(sizeof(pgw_pthread_ipv4_arg));
            pgw_ipv4_pthread_arg_tmp->locate = g_pthread_ipv4_loc;
            pgw_ipv4_pthread_arg_tmp->socketfd = psaeGw->dataPlane.pDpe->gtpuIf.hSocket;
            
            /* set the thd_sts and CPU for the current struct */
            pgw_pthreads_ipv4.client[g_pthread_ipv4_loc].thd_sts = PGW_THD_STS_TYPE_FREE;
            if(g_cpu_cnt != 0)
            {
                pgw_pthreads_ipv4.client[g_pthread_ipv4_loc].CPU = g_cpu_start + (g_cpu_cur++%g_cpu_cnt);
            }else
            {
                pgw_pthreads_ipv4.client[g_pthread_ipv4_loc].CPU = -1;
            } 
            
            ret = pthread_create(&(pgw_pthreads_ipv4.client[g_pthread_ipv4_loc].child_id), \
                                 NULL, pgw_thread_ipv4_hander, (void *)pgw_ipv4_pthread_arg_tmp);
            if(ret == 0)
            {
                /* update globle variable */
                pgw_pthreads_ipv4.childNum++;
                pgw_pthread_ipv4_loc = g_pthread_ipv4_loc; 
                NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "client create ok! client_loc is [%d]", g_pthread_ipv4_loc);
                loop_create++;
                g_pthread_ipv4_loc++;
                creat_cnt++;
                usleep(20);
            }else
            {
                free(pgw_ipv4_pthread_arg_tmp);
                NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "client[%d] create error! release memary!", g_pthread_ipv4_loc);
            }
        }
        else
        {
            g_pthread_ipv4_loc++;
            loop_create++;
        }
        if(g_pthread_ipv4_loc == PGW_PTHREAD_IPV4_MAX)
        {
            g_pthread_ipv4_loc --;
            break;
        }
        if(creat_cnt >= cnt)
        {
            break;
        }
    }
    NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "leave client create, return value is (location) [%d]", pgw_pthread_ipv4_loc);

    return pgw_pthread_ipv4_loc;
}

int pgw_pthreads_ipv4_choose()
{
    int ret = PGW_RET_NODATA;
    int loop;

    for(loop=0; loop<PGW_PTHREAD_IPV4_MAX; loop++) {
        if(pgw_pthreads_ipv4.client[loop].thd_sts == PGW_THD_STS_TYPE_FREE) {
            ret = loop;
            break;
        }
    }

    if(loop == PGW_PTHREAD_IPV4_MAX)
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "no enough threads, enter to create...");
        ret = pgw_ptheads_ipv4_create();
        if(ret == PGW_RET_NODATA)
        {
            NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "no threads were choose");
        }
    }
    return ret;
}

void *pgw_thread_ipv4_hander(void* arg)
{
    pthread_detach(pthread_self());
    char buff[65535];
    int len;
    int ll;
    int rc = -2;
    struct sockaddr_in socket_client_addr;
    socklen_t socket_client_addr_len = sizeof(socket_client_addr);

    pgw_pthread_ipv4_arg ipv4_pthread_arg_tmp;

    pgw_pthread_ipv4_arg* argtmp = (pgw_pthread_ipv4_arg*)arg;

    ipv4_pthread_arg_tmp.locate = argtmp->locate;
    ipv4_pthread_arg_tmp.socketfd = argtmp->socketfd;

    free(arg);
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_INFO, "notes:[%d]", ipv4_pthread_arg_tmp.locate);

    /* CPU bind */
    if(g_cpu_cnt > 0)
    {
        cpu_bind(PGW_CPU_BIND_THREAD, pgw_pthreads_ipv4.client[ipv4_pthread_arg_tmp.locate].CPU);
        NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_INFO, "CPU_SET ok!!");
    }

    while(pgw_pthreads_ipv4.client[ipv4_pthread_arg_tmp.locate].thd_sts >= PGW_THD_STS_TYPE_FREE) {
        if(pgw_pthreads_ipv4.client[ipv4_pthread_arg_tmp.locate].thd_sts == PGW_THD_STS_TYPE_BUSY)
        {
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Received ipv4 message begin");
            len = recvfrom(ipv4_pthread_arg_tmp.socketfd, buff, MAX_GTPV2C_PAYLOAD_LEN , 0, (struct sockaddr *) &socket_client_addr,(socklen_t*) &socket_client_addr_len);
            if(len)
            {
                NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "Received ipv4 message of length %u from %s:%u", len, inet_ntoa(socket_client_addr.sin_addr), ntohs(socket_client_addr.sin_port));
                nwLogHexDump(buff, len);

                rc = nwSdpProcessIpv4DataInd(saeGw.dataPlane.pDpe->gtpuIf.hSdp, 0, buff, len);
            }
            else
            {
                NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
            }
            pgw_pthreads_ipv4.client[ipv4_pthread_arg_tmp.locate].thd_sts = PGW_THD_STS_TYPE_FREE;
            memset(buff,0x00,sizeof(buff));
        }
        usleep(1);
    }
    pgw_pthreads_ipv4.client[ipv4_pthread_arg_tmp.locate].thd_sts = PGW_THD_STS_TYPE_END;
    return NULL;
}




int pgw_ptheads_gtpu_create()
{
    int loop_create;
    int ret;
    int cnt;
    int creat_cnt;
    pgw_pthread_gtpu_arg *pgw_gtpu_pthread_arg_tmp;
    int pgw_pthread_gtpu_loc = PGW_RET_NODATA;

    cnt = (PGW_PTHREAD_GTPU_MAX - pgw_pthreads_gtpu.childNum) / PGW_START_THREADS_GTPU_NUM;
    if(cnt == 0)              /* have pockets threads */
    {
        cnt = (PGW_PTHREAD_GTPU_MAX - pgw_pthreads_gtpu.childNum) % PGW_START_THREADS_GTPU_NUM;
    }else                     /* have couple threads */
    {
        cnt = PGW_START_THREADS_GTPU_NUM;
    }

    if(cnt == 0)              /* no threads */
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "childNum[%d], PGW_PTHREAD_GTPU_MAX[%d], can't create", \
                      pgw_pthreads_gtpu.childNum, PGW_PTHREAD_GTPU_MAX);
        return PGW_RET_NODATA;
    }else
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "pthread_create_client cnt [%d], creating...", cnt);
    
    creat_cnt = 0;

    /* create pthreads for client, needs check success */
    for(loop_create=g_pthread_gtpu_loc; loop_create<PGW_PTHREAD_GTPU_MAX; )
    {
        if(pgw_pthreads_gtpu.client[g_pthread_gtpu_loc].thd_sts == PGW_THD_STS_TYPE_NULL) 
        {
            pgw_gtpu_pthread_arg_tmp = malloc(sizeof(pgw_pthread_gtpu_arg));
            pgw_gtpu_pthread_arg_tmp->locate = g_pthread_gtpu_loc;
            pgw_gtpu_pthread_arg_tmp->socketfd = psaeGw->dataPlane.pDpe->gtpuIf.hSocket;
            
            /* set the thd_sts and CPU for the current struct */
            pgw_pthreads_gtpu.client[g_pthread_gtpu_loc].thd_sts = PGW_THD_STS_TYPE_FREE;
            if(g_cpu_cnt != 0)
            {
                pgw_pthreads_gtpu.client[g_pthread_gtpu_loc].CPU = g_cpu_start + (g_cpu_cur++%g_cpu_cnt);
            }else
            {
                pgw_pthreads_gtpu.client[g_pthread_gtpu_loc].CPU = -1;
            } 
            
            ret = pthread_create(&(pgw_pthreads_gtpu.client[g_pthread_gtpu_loc].child_id), \
                                 NULL, pgw_thread_gtpu_hander, (void *)pgw_gtpu_pthread_arg_tmp);
            if(ret == 0)
            {
                /* update globle variable */
                pgw_pthreads_gtpu.childNum++;
                pgw_pthread_gtpu_loc = g_pthread_gtpu_loc; 
                NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "client create ok! client_loc is [%d]", g_pthread_gtpu_loc);
                loop_create++;
                g_pthread_gtpu_loc++;
                creat_cnt++;
                usleep(20);
            }else
            {
                free(pgw_gtpu_pthread_arg_tmp);
                NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "client[%d] create error! release memary!", g_pthread_gtpu_loc);
            }
        }
        else
        {
            g_pthread_gtpu_loc++;
            loop_create++;
        }
        if(g_pthread_gtpu_loc == PGW_PTHREAD_GTPU_MAX)
        {
            g_pthread_gtpu_loc --;
            break;
        }
        if(creat_cnt >= cnt)
        {
            break;
        }
    }
    NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "leave client create, return value is (location) [%d]", pgw_pthread_gtpu_loc);

    return pgw_pthread_gtpu_loc;
}

int pgw_pthreads_gtpu_choose()
{
    int ret = PGW_RET_NODATA;
    int loop;

    for(loop=0; loop<PGW_PTHREAD_GTPU_MAX; loop++) {
        if(pgw_pthreads_gtpu.client[loop].thd_sts == PGW_THD_STS_TYPE_FREE) {
            ret = loop;
            break;
        }
    }

    if(loop == PGW_PTHREAD_GTPU_MAX)
    {
        NW_SAE_GW_LOG(NW_LOG_LEVEL_INFO, "no enough threads, enter to create...");
        ret = pgw_ptheads_gtpu_create();
        if(ret == PGW_RET_NODATA)
        {
            NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "no threads were choose");
        }
    }
    return ret;
}

void *pgw_thread_gtpu_hander(void* arg)
{
    pthread_detach(pthread_self());
    char buff[65535];
    int len;
    int ll;
    int rc = -2;
    struct sockaddr_in socket_client_addr;
    socklen_t socket_client_addr_len = sizeof(socket_client_addr);

    pgw_pthread_gtpu_arg gtpu_pthread_arg_tmp;

    pgw_pthread_gtpu_arg* argtmp = (pgw_pthread_gtpu_arg*)arg;

    gtpu_pthread_arg_tmp.locate = argtmp->locate;
    gtpu_pthread_arg_tmp.socketfd = argtmp->socketfd;

    free(arg);
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "notes:[%d]", gtpu_pthread_arg_tmp.locate);

    /* CPU bind */
    if(g_cpu_cnt > 0)
    {
        cpu_bind(PGW_CPU_BIND_THREAD, pgw_pthreads_gtpu.client[gtpu_pthread_arg_tmp.locate].CPU);
        NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_INFO, "CPU_SET ok!!");
    }

    while(pgw_pthreads_gtpu.client[gtpu_pthread_arg_tmp.locate].thd_sts >= PGW_THD_STS_TYPE_FREE) {
        if(pgw_pthreads_gtpu.client[gtpu_pthread_arg_tmp.locate].thd_sts == PGW_THD_STS_TYPE_BUSY)
        {
            NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Received gtpu message begin");
            len = recvfrom(gtpu_pthread_arg_tmp.socketfd, buff, MAX_GTPV2C_PAYLOAD_LEN , 0, (struct sockaddr *) &socket_client_addr,(socklen_t*) &socket_client_addr_len);
            if(len)
            {
                NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "Received gtpu message of length %u from %s:%u", len, inet_ntoa(socket_client_addr.sin_addr), ntohs(socket_client_addr.sin_port));
                nwLogHexDump(buff, len);

                rc = nwSdpProcessGtpuDataInd(saeGw.dataPlane.pDpe->gtpuIf.hSdp, buff, len, ntohs(socket_client_addr.sin_port), (socket_client_addr.sin_addr.s_addr));
            }
            else
            {
                NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
            }
            pgw_pthreads_gtpu.client[gtpu_pthread_arg_tmp.locate].thd_sts = PGW_THD_STS_TYPE_FREE;
            memset(buff,0x00,sizeof(buff));
        }
        usleep(1);
    }
    pgw_pthreads_gtpu.client[gtpu_pthread_arg_tmp.locate].thd_sts = PGW_THD_STS_TYPE_END;
    return NULL;
}









void pgw_pthread_destory()
{
    int loop_thread;
    for(loop_thread=0; loop_thread<PGW_PTHREAD_UDP_MAX; loop_thread++)
    {
        pthread_join(pgw_pthreads_udp.client[loop_thread].child_id, NULL);
    }
    
    for(loop_thread=0; loop_thread<PGW_PTHREAD_IPV4_MAX; loop_thread++)
    {
        pthread_join(pgw_pthreads_ipv4.client[loop_thread].child_id, NULL);
    }
    
    for(loop_thread=0; loop_thread<PGW_PTHREAD_GTPU_MAX; loop_thread++)
    {
        pthread_join(pgw_pthreads_gtpu.client[loop_thread].child_id, NULL);
    }
}

void pgw_thread_mutex_create()
{
    pthread_mutex_init(&ippool_mutex, NULL);
    pthread_mutex_init(&teidMap_mutex, NULL);
    pthread_mutex_init(&tunnelMap_mutex, NULL);
    pthread_mutex_init(&ipv4AddrMap_mutex, NULL);
    pthread_mutex_init(&uePgwSessionRbt_mutex, NULL);
    pthread_mutex_init(&ueSgwSessionRbt_mutex, NULL);
    pthread_mutex_init(&activeTimerList_mutex, NULL);
    pthread_mutex_init(&outstandingRxSeqNumMap_mutex, NULL);
    pthread_mutex_init(&outstandingTxSeqNumMap_mutex, NULL);
    
    pthread_mutex_init(&gpGtpv2cTrxnPool_mutex, NULL);
    pthread_mutex_init(&gpGtpv2cMsgPool_mutex, NULL);
    pthread_mutex_init(&gpSdpFlowContextPool_mutex, NULL);
    pthread_mutex_init(&gpTunnelEndPointPool_mutex, NULL);
    pthread_mutex_init(&gpGtpv2cTimeoutInfoPool_mutex, NULL);
    pthread_mutex_init(&nwGtpv2cTmrMinHeap_mutex, NULL);
    pthread_mutex_init(&memPool_mutex, NULL);
    
}


void pgw_thread_mutex_destory()
{
    pthread_mutex_destroy(&ippool_mutex);
    pthread_mutex_destroy(&teidMap_mutex);
    pthread_mutex_destroy(&tunnelMap_mutex);
    pthread_mutex_destroy(&ipv4AddrMap_mutex);
    pthread_mutex_destroy(&uePgwSessionRbt_mutex);
    pthread_mutex_destroy(&ueSgwSessionRbt_mutex);
    pthread_mutex_destroy(&activeTimerList_mutex);
    pthread_mutex_destroy(&outstandingRxSeqNumMap_mutex);
    pthread_mutex_destroy(&outstandingTxSeqNumMap_mutex);
    
    pthread_mutex_destroy(&gpGtpv2cTrxnPool_mutex);
    pthread_mutex_destroy(&gpGtpv2cMsgPool_mutex);
    pthread_mutex_destroy(&gpSdpFlowContextPool_mutex);
    pthread_mutex_destroy(&gpTunnelEndPointPool_mutex);
    
    pthread_mutex_destroy(&gpGtpv2cTimeoutInfoPool_mutex);
    pthread_mutex_destroy(&nwGtpv2cTmrMinHeap_mutex);
    pthread_mutex_destroy(&memPool_mutex);
}
