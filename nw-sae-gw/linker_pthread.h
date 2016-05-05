#ifndef __LINKER_PTHREAD_H__
#define __LINKER_PTHREAD_H__

#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "NwEvt.h"
#include "NwLog.h"
#include "NwMem.h"
#include "NwUtils.h"
#include "NwSaeGwLog.h"
#include "NwLogMgr.h"
#include "NwGtpv2c.h"
#include "NwSaeGwUe.h"
#include "NwSaeGwUlp.h"
#include "NwGtpv2cIf.h"
#include "NwSaeGwDpe.h"

#define PGW_PTHREAD_UDP_MAX        2
#define PGW_START_THREADS_UDP_NUM  (PGW_PTHREAD_UDP_MAX)

#define PGW_PTHREAD_IPV4_MAX        5
#define PGW_START_THREADS_IPV4_NUM  2

#define PGW_PTHREAD_GTPU_MAX        5
#define PGW_START_THREADS_GTPU_NUM  2

#define PGW_RET_OK                 1  
#define PGW_RET_NG                 0  
#define PGW_RET_ERR               -1 
#define PGW_RET_NODATA            -2

typedef struct
{
  NwU8T                         isCombinedGw;
  NwU8T                         apn[1024];
  NwU32T                        ippoolSubnet;
  NwU32T                        ippoolMask;
  NwU32T                        numOfUe;
  NwGtpv2cIfT                   udp;

  struct {
    NwU32T                      s11cIpv4Addr;
    NwU32T                      s5cIpv4Addr;
    NwU32T                      s4cIpv4Addr;
    NwU32T                      s1uIpv4Addr;
    NwSaeGwUlpT                 *pGw;
  } sgwUlp;

  struct {
    NwU32T                      s5cIpv4Addr;
    NwU32T                      s5uIpv4Addr;
    NwSaeGwUlpT                 *pGw;
  } pgwUlp;

  struct {
    NwSaeGwDpeT                 *pDpe;           /*< Data Plane Entity   */
    NwU32T                      gtpuIpv4Addr;
    NwU8T                       sgiNwIfName[128];
  } dataPlane;
} NwSaeGwT;


typedef struct{
    unsigned int thd_sts;
    pthread_t child_id;
    int CPU;
}pthreadInfoT;

typedef struct{
    int childNum;                                       /* have created threads numbers */
    pthreadInfoT client[PGW_PTHREAD_UDP_MAX];
}pgwPthreadInfo;


enum pgwThreadStatusType{
    PGW_THD_STS_TYPE_NULL=0,                           /* haven't created */
    PGW_THD_STS_TYPE_END,                              /* have killed by parents */
    PGW_THD_STS_TYPE_FREE,                             /* not work */
    PGW_THD_STS_TYPE_BUSY                              /* working */
};

enum pgw_cpu_bind_type{
    PGW_CPU_BIND_PROC=0,
    PGW_CPU_BIND_THREAD
};


typedef struct {
    int locate;
}pgw_pthread_udp_arg;

typedef struct {
    int locate;
    int socketfd;
}pgw_pthread_ipv4_arg;

typedef struct {
    int locate;
    int socketfd;
}pgw_pthread_gtpu_arg;

void cpu_bind(int bind_type,int cpu_num);
void linkerPthreadpoolInit();
int pgw_ptheads_udp_create();
int pgw_pthreads_udp_choose();
void *pgw_thread_udp_hander(void* arg);
int pgw_ptheads_ipv4_create();
int pgw_pthreads_ipv4_choose();
void *pgw_thread_ipv4_hander(void* arg);
int pgw_ptheads_gtpu_create();
int pgw_pthreads_gtpu_choose();
void *pgw_thread_gtpu_hander(void* arg);
void pgw_pthread_destory();
void pgw_thread_mutex_create();
void pgw_thread_mutex_destory();


#endif
