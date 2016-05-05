/*----------------------------------------------------------------------------*
 *                                                                            *
 * Copyright (c) 2010-2011 Amit Chawre                                        *
 * All rights reserved.                                                       *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions         *
 * are met:                                                                   *
 *                                                                            *
 * 1. Redistributions of source code must retain the above copyright          *
 *    notice, this list of conditions and the following disclaimer.           *
 * 2. Redistributions in binary form must reproduce the above copyright       *
 *    notice, this list of conditions and the following disclaimer in the     *
 *    documentation and/or other materials provided with the distribution.    *
 * 3. The name of the author may not be used to endorse or promote products   *
 *    derived from this software without specific prior written permission.   *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR       *
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.    *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,           *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT   *
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF   *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          *
 *----------------------------------------------------------------------------*/

/** 
 * @file NwIpv4If.c
 * @brief This files defines IP interface entity.
 */
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>

#include "NwEvt.h"
#include "NwUtils.h"
#include "NwLog.h"
#include "NwIpv4If.h"
#include "NwIpv4IfLog.h"

#include "linker_pthread.h"
extern pgwPthreadInfo pgw_pthreads_ipv4;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NwArpPacket
{
  NwU8T  dstMac[6];
  NwU8T  srcMac[6];
  NwU16T protocol;
  NwU16T hwType;
  NwU16T protoType;
  NwU8T  hwAddrLen;
  NwU8T  protoAddrLen;
  NwU16T opCode;
  NwU8T  senderMac[6];
  NwU8T  senderIpAddr[4];
  NwU8T  targetMac[6];
  NwU8T  targetIpAddr[4];
} NwArpPacketT;

/*---------------------------------------------------------------------------
 *                          I P     E N T I T Y 
 *--------------------------------------------------------------------------*/

NwRcT nwIpv4IfInitialize(NwIpv4IfT* thiz, NwU8T* device, NwSdpHandleT hSdp, NwU8T *pHwAddr)
{
  int sd, send_sd;
  struct sockaddr_ll sll;
  struct ifreq ifr;

  /*
   * Create Socket for listening IP packets
   */
  sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));

  if (sd < 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));

  /* First Get the Interface Index  */

  strncpy((char *)ifr.ifr_name, (const char*)device, IFNAMSIZ);

  if((ioctl(sd, SIOCGIFHWADDR, &ifr)) == -1)
  {     
    printf("Error getting Interface hw address!\n");
    exit(-1);
  }
  else
  {
#if 0
    printf("HW address of interface is: %02x:%02x:%02x:%02x:%02x:%02x\n", 
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[0],
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[1],
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[2],
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[3],
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[4],
        (unsigned char)ifr.ifr_ifru.ifru_hwaddr.sa_data[5]);
#endif
    memcpy(pHwAddr, ifr.ifr_hwaddr.sa_data, 6);
    memcpy(thiz->hwAddr, ifr.ifr_hwaddr.sa_data, 6);
  }

  if((ioctl(sd, SIOCGIFINDEX, &ifr)) == -1)
  {     
    printf("Error getting Interface index !\n");
    exit(-1);
  }

  thiz->ifindex = ifr.ifr_ifindex;

  /* Bind our raw socket to this interface */

  sll.sll_family        = PF_PACKET;
  sll.sll_ifindex       = ifr.ifr_ifindex;
  sll.sll_protocol      = htons(ETH_P_IP);

  if((bind(sd, (struct sockaddr *)&sll, sizeof(sll)))== -1)
  {
    printf("Error binding raw socket to interface\n");
    exit(-1);
  }

  thiz->hRecvSocketIpv4 = sd;
  thiz->hSdp            = hSdp;


  /*
   * Create Socket for listening ARP requests
   */
  sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));

  if (sd < 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));

  /* First Get the Interface Index  */

  strncpy((char *)ifr.ifr_name, (const char*)device, IFNAMSIZ);
  if((ioctl(sd, SIOCGIFINDEX, &ifr)) == -1)
  {     
    printf("Error getting Interface index !\n");
    exit(-1);
  }

  /* Bind our raw socket to this interface */

  sll.sll_family        = PF_PACKET;
  sll.sll_ifindex       = ifr.ifr_ifindex;
  sll.sll_protocol      = htons(ETH_P_ARP);

  if((bind(sd, (struct sockaddr *)&sll, sizeof(sll)))== -1)
  {
    printf("Error binding raw socket to interface\n");
    exit(-1);
  }

  thiz->hRecvSocketArp     = sd;

  /*
   * Create socket sending data to L2
   */
  if((send_sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))== -1)
  {
    printf("Error creating raw socket: send fd");
    exit(1);
  }

  int yes = 1;
  if(setsockopt(send_sd, IPPROTO_IP, IP_HDRINCL, &yes, sizeof(yes)) < 0 ) {
    perror("Error in Setting socket option:");
    exit(1);
  }

  thiz->hSendSocket     = send_sd;

  return NW_OK;
}

NwRcT nwIpv4IfGetSelectionObjectIpv4(NwIpv4IfT* thiz, NwU32T *pSelObj)
{
  *pSelObj = thiz->hRecvSocketIpv4;
  return NW_OK;
}

NwRcT nwIpv4IfGetSelectionObjectArp(NwIpv4IfT* thiz, NwU32T *pSelObj)
{
  *pSelObj = thiz->hRecvSocketArp;
  return NW_OK;
}

void NW_EVT_CALLBACK(nwIpv4IfArpDataIndicationCallback)
{
#if 0
  NwRcT         rc;
  NwS32T        bytesRead;
  NwArpPacketT  arpReq;
  NwIpv4IfT* thiz = (NwIpv4IfT*) arg;

  bytesRead = recvfrom(thiz->hRecvSocketArp, &arpReq, sizeof(NwArpPacketT), 0, NULL, NULL);

  if(bytesRead > 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_DEBG, "Received ARP message of length %u", bytesRead);
    if(arpReq.opCode == htons(0x0001) &&
      (*((NwU32T*)arpReq.targetIpAddr) != *((NwU32T*)arpReq.senderIpAddr)))
    {
      nwLogHexDump((NwU8T*)&arpReq, bytesRead);
      rc = nwSdpProcessIpv4DataInd(thiz->hSdp, 0, (NwU8T*)(&arpReq), (bytesRead));
    }
  }
  else
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
#endif
  int ret = PGW_RET_NODATA;
  ret = pgw_pthreads_ipv4_choose();
  if(ret == PGW_RET_NODATA)
  {   
      NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "no thread for choose, wait for the next loop, ret==[%d]", ret);
  }   
  else
  {   
      pgw_pthreads_ipv4.client[ret].thd_sts = PGW_THD_STS_TYPE_BUSY;    
      NW_SAE_GW_LOG(NW_LOG_LEVEL_LINKER, "choose thread[%d], change the status", ret);  
      usleep(30);/* wait for child thread receive msg */
  }
  
  return;
}

void NW_EVT_CALLBACK(nwIpv4IfDataIndicationCallback)
{
#if 0
  NwRcT         rc;
  NwU8T         ipDataBuf[MAX_IP_PAYLOAD_LEN];
  NwS32T        bytesRead;
  NwIpv4IfT* thiz = (NwIpv4IfT*) arg;

  bytesRead = recvfrom(thiz->hRecvSocketIpv4, ipDataBuf, MAX_IP_PAYLOAD_LEN , 0, NULL, NULL);

  if(bytesRead > 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_DEBG, "Received IP message of length %u", bytesRead);
    nwLogHexDump((NwU8T*)ipDataBuf, bytesRead);
    rc = nwSdpProcessIpv4DataInd(thiz->hSdp, 0, (ipDataBuf), (bytesRead));
  }
  else
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
#endif
  
  int ret = PGW_RET_NODATA;
  ret = pgw_pthreads_ipv4_choose();
  if(ret == PGW_RET_NODATA)
  {   
      NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "no thread for choose, wait for the next loop, ret==[%d]", ret);
  }   
  else
  {   
      pgw_pthreads_ipv4.client[ret].thd_sts = PGW_THD_STS_TYPE_BUSY;    
      NW_SAE_GW_LOG(NW_LOG_LEVEL_LINKER, "choose thread[%d], change the status", ret);  
      usleep(30);/* wait for child thread receive msg */
  }
}

#if 1
typedef struct {
    NwSdpHandleT hThiz;
    char dataBuf[1024];
    NwU32T dataSize;
}nwIpv4IfIpv4Data;

void linker_pthread_nwIpv4IfIpv4DataReq(void* arg)
{
	pthread_detach(pthread_self());

	NwS32T bytesSent;
	struct sockaddr_in peerAddr;
	nwIpv4IfIpv4Data linker_sent_data;

	memset(&linker_sent_data, 0, sizeof(linker_sent_data));
	memcpy(&linker_sent_data, (char*)arg, sizeof(linker_sent_data));

	free(arg);

	NwIpv4IfT* thiz = (NwIpv4IfT*) linker_sent_data.hThiz;
	peerAddr.sin_family       = AF_INET;
	memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));
	peerAddr.sin_addr.s_addr = *((NwU32T*)(linker_sent_data.dataBuf + 16));

	nwLogHexDump((NwU8T*)linker_sent_data.dataBuf, linker_sent_data.dataSize);

	bytesSent = sendto (thiz->hSendSocket, linker_sent_data.dataBuf, linker_sent_data.dataSize, 0, (struct sockaddr *) &peerAddr, sizeof(struct sockaddr));

	if(bytesSent < 0)
	{
		NW_IP_LOG(NW_LOG_LEVEL_ERRO, "IP PDU send error - %s", strerror(errno));
	}

	NW_IP_LOG(NW_LOG_LEVEL_DEBG, "Success! Sent buf of size %u for on handle %x ", bytesSent, linker_sent_data.hThiz);

  return;
}
#endif

NwRcT nwIpv4IfIpv4DataReq(NwSdpHandleT hThiz,
    NwU8T* dataBuf,
    NwU32T dataSize)
{
#if 0
  struct sockaddr_in peerAddr;
  NwS32T bytesSent;
  NwIpv4IfT* thiz = (NwIpv4IfT*) hThiz;

  peerAddr.sin_family       = AF_INET;
  memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));
  peerAddr.sin_addr.s_addr = *((NwU32T*)(dataBuf + 16));

  nwLogHexDump((NwU8T*)dataBuf, dataSize);

  bytesSent = sendto (thiz->hSendSocket, dataBuf, dataSize, 0, (struct sockaddr *) &peerAddr, sizeof(struct sockaddr));

  if(bytesSent < 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "IP PDU send error - %s", strerror(errno));
  }
#endif

#if 1
  NwRcT rc; 
  pthread_t tid;
  
  nwIpv4IfIpv4Data* Data_tmp = (nwIpv4IfIpv4Data*)malloc(sizeof(nwIpv4IfIpv4Data));
  memset(Data_tmp, 0, sizeof(nwIpv4IfIpv4Data));
  Data_tmp->hThiz= hThiz;
  Data_tmp->dataSize = dataSize;
  memcpy(Data_tmp->dataBuf, dataBuf, Data_tmp->dataSize);
  
  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto_nwIpv4IfIpv4DataReq begin");
  if( 0 != pthread_create(&tid, NULL, linker_pthread_nwIpv4IfIpv4DataReq, (void *)Data_tmp) )
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    free(Data_tmp);
  }
  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto_nwIpv4IfIpv4DataReq end");
#endif


  return NW_OK;
}




#if 1
typedef struct {
    NwSdpHandleT hThiz;
    NwU16T opCode;
    NwU8T              *pTargetMac;
    NwU8T              *pTargetIpAddr;
    NwU8T              *pSenderIpAddr;
}nwIpv4IfArpData;

void linker_pthread_nwIpv4IfArpDataReq(void* arg)
{
        pthread_detach(pthread_self());

        NwS32T bytesSent;
        struct sockaddr_in peerAddr;
        nwIpv4IfArpData linker_sent_data;

        memset(&linker_sent_data, 0, sizeof(linker_sent_data));
        memcpy(&linker_sent_data, (char*)arg, sizeof(linker_sent_data));

        free(arg);

  struct sockaddr_ll    sa; 
  NwArpPacketT          arpRsp;
  NwIpv4IfT* thiz = (NwIpv4IfT*) linker_sent_data.hThiz;

  sa.sll_family         = AF_PACKET;
  sa.sll_ifindex        = thiz->ifindex;
  sa.sll_protocol       = htons(ETH_P_ARP);
  sa.sll_hatype         = ARPHRD_ETHER;
  sa.sll_halen          = ETH_ALEN;
  sa.sll_pkttype        = PACKET_BROADCAST;

  memcpy(arpRsp.dstMac, linker_sent_data.pTargetMac, ETH_ALEN);
  memcpy(arpRsp.srcMac, thiz->hwAddr, ETH_ALEN);
  arpRsp.protocol       = htons(ETH_P_ARP);

  arpRsp.hwType         = htons(ARPHRD_ETHER);
  arpRsp.protoType      = htons(ETH_P_IP);

  arpRsp.hwAddrLen      = ETH_ALEN;
  arpRsp.protoAddrLen   = 0x04;
  arpRsp.opCode         = htons(linker_sent_data.opCode);

  memcpy(arpRsp.senderMac, thiz->hwAddr, 6); 
  memcpy(arpRsp.senderIpAddr, linker_sent_data.pSenderIpAddr, 4); 

  if(linker_sent_data.opCode == ARPOP_REQUEST)
    memset(arpRsp.targetMac, 0x00, 6); 
  else
    memcpy(arpRsp.targetMac, linker_sent_data.pTargetMac, 6); 

  memcpy(arpRsp.targetIpAddr, linker_sent_data.pTargetIpAddr, 4); 

  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "ARP PDU send begin, linker_pthread_nwIpv4IfArpDataReq");

  nwLogHexDump((NwU8T*)&arpRsp, sizeof(arpRsp));
  bytesSent = sendto (thiz->hRecvSocketArp, (void*)&arpRsp, sizeof(NwArpPacketT), 0, (struct sockaddr *) &sa, sizeof(sa));
  if(bytesSent < 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "ARP PDU send error - %s", strerror(errno));
  }
  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "ARP PDU send ok, linker_pthread_nwIpv4IfArpDataReq");

  return;
}
#endif




NwRcT nwIpv4IfArpDataReq(NwSdpHandleT       hThiz,
                     NwU16T             opCode,
                     NwU8T              *pTargetMac,
                     NwU8T              *pTargetIpAddr,
                     NwU8T              *pSenderIpAddr)
{
#if 0
  NwU32T                bytesSent;
  struct sockaddr_ll    sa;
  NwArpPacketT          arpRsp;
  NwIpv4IfT* thiz = (NwIpv4IfT*) hThiz;

  sa.sll_family         = AF_PACKET;
  sa.sll_ifindex        = thiz->ifindex;
  sa.sll_protocol       = htons(ETH_P_ARP);
  sa.sll_hatype         = ARPHRD_ETHER;
  sa.sll_halen          = ETH_ALEN;
  sa.sll_pkttype        = PACKET_BROADCAST;

  memcpy(arpRsp.dstMac, pTargetMac, ETH_ALEN);
  memcpy(arpRsp.srcMac, thiz->hwAddr, ETH_ALEN);
  arpRsp.protocol       = htons(ETH_P_ARP);

  arpRsp.hwType         = htons(ARPHRD_ETHER);
  arpRsp.protoType      = htons(ETH_P_IP);

  arpRsp.hwAddrLen      = ETH_ALEN;
  arpRsp.protoAddrLen   = 0x04;
  arpRsp.opCode         = htons(opCode);

  memcpy(arpRsp.senderMac, thiz->hwAddr, 6);
  memcpy(arpRsp.senderIpAddr, pSenderIpAddr, 4);

  if(opCode == ARPOP_REQUEST)
    memset(arpRsp.targetMac, 0x00, 6);
  else
    memcpy(arpRsp.targetMac, pTargetMac, 6);

  memcpy(arpRsp.targetIpAddr, pTargetIpAddr, 4);

  nwLogHexDump((NwU8T*)&arpRsp, sizeof(arpRsp));
  bytesSent = sendto (thiz->hRecvSocketArp, (void*)&arpRsp, sizeof(NwArpPacketT), 0, (struct sockaddr *) &sa, sizeof(sa));
  if(bytesSent < 0)
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "ARP PDU send error - %s", strerror(errno));
  }
#endif

#if 1
  NwRcT rc;
  pthread_t tid;

  nwIpv4IfArpData* Data_tmp = (nwIpv4IfArpData*)malloc(sizeof(nwIpv4IfArpData));
  memset(Data_tmp, 0, sizeof(nwIpv4IfArpData));
  Data_tmp->hThiz= hThiz;
  Data_tmp->opCode = opCode;
  Data_tmp->pTargetMac = pTargetMac;
  Data_tmp->pTargetIpAddr = pTargetIpAddr;
  Data_tmp->pSenderIpAddr = pSenderIpAddr;

  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto_nwIpv4IfArpDataReq begin");
  if( 0 != pthread_create(&tid, NULL, linker_pthread_nwIpv4IfArpDataReq, (void *)Data_tmp) )
  {
    NW_IP_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    free(Data_tmp);
  }
  NW_IP_LOG(NW_LOG_LEVEL_DEBG, "create linker_pthread_sendto_nwIpv4IfArpDataReq end");

#endif
  return NW_OK;
}

#ifdef __cplusplus
}
#endif


