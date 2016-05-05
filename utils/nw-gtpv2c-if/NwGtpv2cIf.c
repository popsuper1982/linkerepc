
#if 0
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "NwEvt.h"
#include "NwUtils.h"
#include "NwLog.h"
#include "NwGtpv2cIf.h"
#include "NwGtpv2cIfLog.h"
#endif

#include "NwLog.h"
#include "NwGtpv2cIf.h"
#include "NwGtpv2cIfLog.h"
#include "linker_pthread.h"
#include "linker_queue.h"

extern pgwPthreadInfo pgw_pthreads_udp;

/* add 20160501 */
unsigned int g_queue_gtpv2_curr = 0;

Type_queue_gtpv2 gtpv2_queue[NUM_QUEUE_GTPV2_NUMBER];

/* add 20160501 */

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *                          U D P     E N T I T Y 
 *--------------------------------------------------------------------------*/


NwRcT nwGtpv2cIfInitialize(NwGtpv2cIfT* thiz, NwU32T ipAddr, NwGtpv2cStackHandleT hGtpcStack)
{
  int sd;
  struct sockaddr_in addr;

  sd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sd < 0)
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  addr.sin_family       = AF_INET;
  addr.sin_port         = htons(NW_GTPC_UDP_PORT);
  addr.sin_addr.s_addr  = htonl(ipAddr);
  memset(addr.sin_zero, '\0', sizeof (addr.sin_zero));

  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "Bind error for %x:%u - %s", ipAddr, NW_GTPC_UDP_PORT, strerror(errno));
    NW_ASSERT(0);
  }

  /* set socketopt 20160501 */
  int iRecvLen = 1000000*1024;
  int iRet = -1;
  int iRecvLen_get;
  int iOptlen = 4;

  iRet=getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &iRecvLen_get, (socklen_t*)&iOptlen);
  if(iRet != 0 )
  {
     NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO,"soct %d getsockopt err", sd);
     NW_ASSERT(0);
  }

  if(iRecvLen > iRecvLen_get)
  {
    iRet = setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &iRecvLen, 4);
    if(iRet != 0 )
    {
       NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO,"soct %d setsockopt err", sd);
       NW_ASSERT(0);
    }
  }
  /* set socketopt 20160501 */

  thiz->hSocket         = sd;
  thiz->hGtpcStack      = hGtpcStack;
  thiz->ipAddr          = ipAddr;

  return NW_OK;
}

NwRcT nwGtpv2cIfGetSelectionObject(NwGtpv2cIfT* thiz, NwU32T *pSelObj)
{
  *pSelObj = thiz->hSocket;
  return NW_OK;
}

void NW_EVT_CALLBACK(nwGtpv2cIfDataIndicationCallback)
{
#if 1
  /* add 20160501 */
  int                   bytesRead;
  struct sockaddr_in    peer;
  NwU32T                peerLen;
  NwGtpv2cIfT* thiz   = (NwGtpv2cIfT*) arg;
  peerLen             = sizeof(peer);
  unsigned int          q_tail_tmp;

  /* 
   * choose the queue num && queue num's queue_tail
   */

  //NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "GTPCv2 message coming...");

  while(1)
  {
    pthread_mutex_lock(&gtpv2_queue[g_queue_gtpv2_curr].queue_mutex);

    if ( gtpv2_queue[g_queue_gtpv2_curr].queue_length == NUM_QUEUE_GTPV2_NODE_MAX )
    {
      pthread_mutex_unlock(&gtpv2_queue[g_queue_gtpv2_curr].queue_mutex);
      g_queue_gtpv2_curr = (g_queue_gtpv2_curr+1) % NUM_QUEUE_GTPV2_NUMBER;
      usleep(1);
    }
    else
    {
      pthread_mutex_unlock(&gtpv2_queue[g_queue_gtpv2_curr].queue_mutex);
      break;
    }
  }

  if(gtpv2_queue[g_queue_gtpv2_curr].queue_length != 0)
  {
    q_tail_tmp = (gtpv2_queue[g_queue_gtpv2_curr].queue_tail + 1) % NUM_QUEUE_GTPV2_NODE_MAX;
  }
  else
  {
    q_tail_tmp = gtpv2_queue[g_queue_gtpv2_curr].queue_tail;
  }
      /* 
       * update the tail and the length
       */
#if 0
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d] get the chance! befor: length:[%d], head:[%d], tail[%d].", g_queue_gtpv2_curr, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_length, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_head, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_tail);
#endif

      /* 
       * fill in the Type_gtpv2_msg_queue_node
       */
      bytesRead = recvfrom(thiz->hSocket, \
                       gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].buf, \
                       MAX_GTPV2C_PAYLOAD_LEN, \
                       0, \
                       (struct sockaddr *) &peer, \
                       (socklen_t*) &peerLen);
      gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].typepoint = thiz->hGtpcStack;
      gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].bufsize = bytesRead;
      gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].peer = peer;

#if 0
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d].[%d], gtpv2_queue.typepoint:[%p]", g_queue_gtpv2_curr, q_tail_tmp, gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].typepoint);
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d].[%d], gtpv2_queue.buf:[%s]", g_queue_gtpv2_curr, q_tail_tmp, gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].buf);
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d].[%d], gtpv2_queue.bufsize:[%d]", g_queue_gtpv2_curr, q_tail_tmp, gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].bufsize);
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d].[%d], gtpv2_queue.peer:[%d]", g_queue_gtpv2_curr, q_tail_tmp, gtpv2_queue[g_queue_gtpv2_curr].queue_node[q_tail_tmp].peer);
#endif

      pthread_mutex_lock(&gtpv2_queue[g_queue_gtpv2_curr].queue_mutex);
   
      if( gtpv2_queue[g_queue_gtpv2_curr].queue_length != 0 )
      {
        gtpv2_queue[g_queue_gtpv2_curr].queue_tail = (gtpv2_queue[g_queue_gtpv2_curr].queue_tail + 1) % NUM_QUEUE_GTPV2_NODE_MAX;
      }

      gtpv2_queue[g_queue_gtpv2_curr].queue_length++;

#if 0
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "---1---queue[%d] get the chance! after: length:[%d], head:[%d], tail[%d].", g_queue_gtpv2_curr, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_length, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_head, \
			gtpv2_queue[g_queue_gtpv2_curr].queue_tail);
#endif

      pthread_mutex_unlock(&gtpv2_queue[g_queue_gtpv2_curr].queue_mutex);
   /* add 20160501 */
#endif

#if 0
  NwRcT         rc;
  NwU8T         udpBuf[MAX_GTPV2C_PAYLOAD_LEN];
  NwU32T        bytesRead;
  NwU32T        peerLen;
  struct sockaddr_in peer;
  NwGtpv2cIfT* thiz = (NwGtpv2cIfT*) arg;

  peerLen = sizeof(peer);

  bytesRead = recvfrom(thiz->hSocket, udpBuf, MAX_GTPV2C_PAYLOAD_LEN , 0, (struct sockaddr *) &peer,(socklen_t*) &peerLen);
#if 0
  if(bytesRead)
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Received GTPCv2 message of length %u from %X:%u", bytesRead, ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port));
    nwLogHexDump(udpBuf, bytesRead);

    rc = nwGtpv2cProcessUdpReq(thiz->hGtpcStack, udpBuf, bytesRead, ntohs(peer.sin_port), (peer.sin_addr.s_addr));
  }
  else
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
#endif
#endif
  
  
#if 0
  int ret = PGW_RET_NODATA;
  ret = pgw_pthreads_udp_choose();
  if(ret == PGW_RET_NODATA)
  {   
      NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_WARN, "no thread for choose, wait for the next loop, ret==[%d]", ret);
      //usleep(5);
  }   
  else
  {   
      pgw_pthreads_udp.client[ret].thd_sts = PGW_THD_STS_TYPE_BUSY;    
      //NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_LINKER, "choose thread[%d], change the status", ret);  
      //usleep(1);/* wait for child thread receive msg */
  }
#endif
}

typedef struct {
    NwGtpv2cUdpHandleT udpHandle;
    char dataBuf[1024];
    NwU32T dataSize;
    NwU32T peerIpAddr;
    NwU32T peerPort;
}nwGtpv2cIfDataReqData;

void linker_pthread_nwGtpv2cIfDataReqData(void* arg)
{
    pthread_detach(pthread_self());
    
    NwS32T bytesSent;
    struct sockaddr_in peerAddr;
    nwGtpv2cIfDataReqData linker_sent_data;

    memset(&linker_sent_data, 0, sizeof(linker_sent_data));
    memcpy(&linker_sent_data, (char*)arg, sizeof(linker_sent_data));
    
    free(arg);
    
    NwGtpv2cIfT* thiz = (NwGtpv2cIfT*) linker_sent_data.udpHandle;

    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Sending buf of size %u for on handle %x to peer "NW_IPV4_ADDR, linker_sent_data.dataSize, linker_sent_data.udpHandle,
                     NW_IPV4_ADDR_FORMAT(linker_sent_data.peerIpAddr));

    peerAddr.sin_family       = AF_INET;
    peerAddr.sin_port         = htons(linker_sent_data.peerPort);
    peerAddr.sin_addr.s_addr  = (linker_sent_data.peerIpAddr);

    memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));
    
    nwLogHexDump(linker_sent_data.dataBuf, linker_sent_data.dataSize);

    bytesSent = sendto (thiz->hSocket, linker_sent_data.dataBuf, linker_sent_data.dataSize, 0, (struct sockaddr *) &peerAddr, sizeof(peerAddr));

    if(bytesSent < 0)
    {
        NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
        NW_ASSERT(0);
    }

    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Success! Sent buf of size %u for on handle %x to peer "NW_IPV4_ADDR, bytesSent, linker_sent_data.udpHandle, NW_IPV4_ADDR_FORMAT(linker_sent_data.peerIpAddr));

  return;
}


#ifdef __cplusplus
}
#endif



NwRcT nwGtpv2cIfDataReq(NwGtpv2cUdpHandleT udpHandle,
    NwU8T* dataBuf,
    NwU32T dataSize,
    NwU32T peerIpAddr,
    NwU32T peerPort)
{
#if 0
  struct sockaddr_in peerAddr;
  NwS32T bytesSent;
  NwGtpv2cIfT* thiz = (NwGtpv2cIfT*) udpHandle;

  NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "Sending buf of size %u for on handle %x to peer "NW_IPV4_ADDR, dataSize, udpHandle,
      NW_IPV4_ADDR_FORMAT(peerIpAddr));

  peerAddr.sin_family       = AF_INET;
  peerAddr.sin_port         = htons(peerPort);
  peerAddr.sin_addr.s_addr  = (peerIpAddr);
  memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));
  
  nwLogHexDump(dataBuf, dataSize);

  bytesSent = sendto (thiz->hSocket, dataBuf, dataSize, 0, (struct sockaddr *) &peerAddr, sizeof(peerAddr));

  if(bytesSent < 0)
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }
#endif
  
#if 1
  //NwRcT rc;
  pthread_t tid;
  
  nwGtpv2cIfDataReqData* Data_tmp = (nwGtpv2cIfDataReqData*)malloc(sizeof(nwGtpv2cIfDataReqData));
  memset(Data_tmp, 0, sizeof(nwGtpv2cIfDataReqData));
  Data_tmp->udpHandle = udpHandle;
  Data_tmp->dataSize = dataSize;
  Data_tmp->peerIpAddr = peerIpAddr;
  Data_tmp->peerPort = peerPort;
  memcpy(Data_tmp->dataBuf, dataBuf, Data_tmp->dataSize);
  
  NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto begin");
  if( 0 != pthread_create(&tid, NULL, linker_pthread_nwGtpv2cIfDataReqData, (void *)Data_tmp) )
  {
    NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    free(Data_tmp);
  }
  NW_GTPV2C_IF_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto end");
#endif  
  return NW_OK;
}


