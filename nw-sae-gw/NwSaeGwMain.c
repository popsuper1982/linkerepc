/** 
 * @file NwSaeGwMain.c
 * @brief This main file demostrates usage of nw-gtpv2c library for a SAE gateway(SGW/PGW) 
 * application.
 */

#include "linker_pthread.h"
#include "linker_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NW_SAE_GW_MAX_GW_INSTANCE               (10)

unsigned int g_cpu_start = 0; 
unsigned int g_cpu_cur = 0;
unsigned int g_cpu_cnt = 2;
unsigned int g_pthread_udp_loc = 0;
unsigned int g_pthread_ipv4_loc = 0;
unsigned int g_pthread_gtpu_loc = 0;

unsigned int g_udp_queue_loc = 0;

pgwPthreadInfo pgw_pthreads_udp;
pgwPthreadInfo pgw_pthreads_ipv4;
pgwPthreadInfo pgw_pthreads_gtpu;

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

/* add 20160501 */
extern unsigned int g_queue_gtpv2_curr;
extern Type_queue_gtpv2 gtpv2_queue[NUM_QUEUE_GTPV2_NUMBER];
/* add 20160501 */

NwSaeGwT           saeGw;
NwSaeGwT* psaeGw = &saeGw;

NwRcT
nwSaeGwCmdLineHelp()
{
  printf("\nSupported command line arguments are:\n");
  printf("\n+----------------------+-------------+----------------------------------------+");
  printf("\n| ARGUMENT             | PRESENCE    | DESCRIPTION                            |");
  printf("\n+----------------------+-------------+----------------------------------------+");
  printf("\n| --sgw-s11-ip | -ss11 | MANDATORY   | S11 control IP address of the SGW.     |");
  printf("\n| --sgw-s5-ip  | -ss5  | MANDATORY   | S5 control IP address of the SGW.      |");
  printf("\n| --sgw-s4-ip  | -ss4  | OPTIONAL    | S4 control IP address of the SGW.      |");
  printf("\n| --pgw-s5-ip  | -ps5  | MANDATORY   | S5 control IP address of the PGW.      |");
  printf("\n| --gtpu-ip    | -gi   | MANDATORY   | IP address for the GTPU User Plane.    |");
  printf("\n| --apn        | -ap   | MANDATORY   | Access Point Name to be served.        |");
  printf("\n| --ippool-subnet| -is | MANDATORY   | IPv4 address pool for UEs.             |");
  printf("\n| --ippool-mask | -im  | MANDATORY   | IPv4 address pool for UEs.             |");
  printf("\n| --sgi-if     | -si   | OPTIONAL    | Network interface name for the SGi.    |");
  printf("\n| --max-ue     | -mu   | OPTIONAL    | Maximum number of UEs to support.      |");
  printf("\n| --combined-gw | -cgw | OPTIONAL    | Combine SGW and PGW funtions.          |");
  printf("\n+----------------------+-------------+----------------------------------------+");
  printf("\n\nExample Usage: \n$ nwLteSaeGw --sgw-s11-ip 10.124.25.153 --sgw-s5-ip 192.168.0.1 --pgw-s5-ip 192.168.139.5 --gtpu-ip 10.124.25.153 --sgi-if eth0 --ippool-subnet 10.66.10.0 --ippool-mask 255.255.255.0 -cgw");
  printf("\n");
  exit(0);
}

NwRcT
nwSaeGwParseCmdLineOpts(NwSaeGwT*  thiz, int argc, char* argv[])
{
  NwRcT rc = NW_OK;
  int i = 0;

  if(argc < 2)
    return NW_FAILURE;
  
  /* Set default values */
  thiz->isCombinedGw    = NW_FALSE;
  thiz->numOfUe         = 99999999;
  thiz->ippoolSubnet    = ntohl(inet_addr("192.168.128.0"));
  thiz->ippoolMask      = ntohl(inet_addr("255.255.255.0"));
  strcpy((char*)thiz->dataPlane.sgiNwIfName, "");

  i++;
  while( i < argc )
  {
    NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "Processing cmdline arg %s", argv[i]);
    if((strcmp("--sgw-s11-ip", argv[i]) == 0)
        || (strcmp(argv[i], "-ss11") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      thiz->sgwUlp.s11cIpv4Addr = ntohl(inet_addr(argv[i])); 
    }
    else if((strcmp("--sgw-s5-ip", argv[i]) == 0)
        || (strcmp(argv[i], "-ss5") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      thiz->sgwUlp.s5cIpv4Addr  = ntohl(inet_addr(argv[i])); 
    }
    else if((strcmp("--sgw-s4-ip", argv[i]) == 0)
        || (strcmp(argv[i], "-ss4") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      thiz->sgwUlp.s4cIpv4Addr  = ntohl(inet_addr(argv[i])); 
    }
    else if((strcmp("--pgw-s5-ip", argv[i]) == 0)
        || (strcmp(argv[i], "-ps5") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      thiz->pgwUlp.s5cIpv4Addr  = ntohl(inet_addr(argv[i])); 
    }
    else if((strcmp("--apn", argv[i]) == 0)
        || (strcmp(argv[i], "-ap") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;
      strncpy((char*)thiz->apn, argv[i], 1023);
    }
    else if((strcmp("--ippool-subnet", argv[i]) == 0)
        || (strcmp(argv[i], "-is") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;
      thiz->ippoolSubnet = ntohl(inet_addr(argv[i]));
      NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "IP pool subnet address %s", argv[i]);
    }
    else if((strcmp("--ippool-mask", argv[i]) == 0)
        || (strcmp(argv[i], "-im") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;
      thiz->ippoolMask = ntohl(inet_addr(argv[i]));
      NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "Ip pool mask %s", argv[i]);
    }
    else if((strcmp("--gtpu-ip", argv[i]) == 0)
        || (strcmp(argv[i], "-gi") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "User Plane IP address %s", argv[i]);
      thiz->dataPlane.gtpuIpv4Addr = ntohl(inet_addr(argv[i]));
    }
    else if((strcmp("--sgi-if", argv[i]) == 0)
        || (strcmp(argv[i], "-si") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "SGi network inteface name %s", argv[i]);
      strcpy((char*)thiz->dataPlane.sgiNwIfName, (argv[i]));
    }
    else if((strcmp("--num-of-ue", argv[i]) == 0)
        || (strcmp(argv[i], "-nu") == 0))
    {
      i++;
      if(i >= argc)
        return NW_FAILURE;

      NW_SAE_GW_LOG(NW_LOG_LEVEL_DEBG, "number of UE %s", argv[i]);
      thiz->numOfUe = atoi(argv[i]);
    }
    else if((strcmp("--combined-gw", argv[i]) == 0)
        || (strcmp(argv[i], "-cgw") == 0))
    {
      thiz->isCombinedGw = NW_TRUE;
    }
    else if((strcmp("--help", argv[i]) == 0)
        || (strcmp(argv[i], "-h") == 0))
    {
      nwSaeGwCmdLineHelp();
    }
    else if((strcmp("--cpuc", argv[i]) == 0) 
        || (strcmp(argv[i], "-cc") == 0) )
    {
      i++;
      g_cpu_cnt = atoi(argv[i]);
    }
    else if((strcmp("--cpus", argv[i]) == 0) 
        || (strcmp(argv[i], "-cs") == 0) )
    {
      i++;
      g_cpu_start = atoi(argv[i]);
    }
    else
    {
      rc = NW_FAILURE;
    }
    i++;
  }

  return rc;
}

NwRcT
nwSaeGwInitialize(NwSaeGwT* thiz)
{
  NwRcT rc = NW_OK;
  NwSaeGwUlpT* pGw;
  NwSaeGwUlpConfigT cfg;

  /* Create Data Plane instance. */

  thiz->dataPlane.pDpe    = nwSaeGwDpeInitialize();

  /* Create PGW ULP instances. */

  if(thiz->pgwUlp.s5cIpv4Addr)
  {
    NW_SAE_GW_LOG(NW_LOG_LEVEL_NOTI, "Creating PGW instance with S5 Ipv4 address "NW_IPV4_ADDR, NW_IPV4_ADDR_FORMAT(htonl(thiz->pgwUlp.s5cIpv4Addr)));

    cfg.maxUeSessions   = thiz->numOfUe;
    cfg.ippoolSubnet    = thiz->ippoolSubnet;
    cfg.ippoolMask      = thiz->ippoolMask;
    cfg.s5cIpv4AddrPgw  = thiz->pgwUlp.s5cIpv4Addr;
    cfg.pDpe            = thiz->dataPlane.pDpe;

    strncpy((char*)cfg.apn, (const char*)thiz->apn, 1023);

    pGw = nwSaeGwUlpNew(); 
    rc = nwSaeGwUlpInitialize(pGw, NW_SAE_GW_TYPE_PGW, &cfg);
    NW_ASSERT( NW_OK == rc );
    thiz->pgwUlp.pGw = pGw;
  }


  if(thiz->dataPlane.gtpuIpv4Addr)
  {
    rc = nwSaeGwDpeCreateGtpuService(thiz->dataPlane.pDpe, thiz->dataPlane.gtpuIpv4Addr);
  }

  if(strlen((const char*)(thiz->dataPlane.sgiNwIfName)) != 0)
  {
    rc = nwSaeGwDpeCreateIpv4Service(thiz->dataPlane.pDpe, thiz->dataPlane.sgiNwIfName);
  }
 
  return rc;
}

NwRcT
nwSaeGwFinalize(NwSaeGwT*  thiz)
{
  return NW_OK;
}

/*---------------------------------------------------------------------------
 *                T H E      M A I N      F U N C T I O N 
 *--------------------------------------------------------------------------*/

/*
    create the pgw_udp_pthread_pool; by wenxinlee 20150410
 */




int main(int argc, char* argv[])
{
  NwRcT rc; 

  memset(&saeGw, 0, sizeof(NwSaeGwT));

  /*---------------------------------------------------------------------------
   *  Parse Commandline Arguments 
   *--------------------------------------------------------------------------*/

  rc = nwSaeGwParseCmdLineOpts(&saeGw, argc, argv);
  if(rc != NW_OK)
  {
    printf("\nUsage error. Please refer help.\n\n");
    exit(0);
  }

  /*---------------------------------------------------------------------------
   *  Initialize event library
   *--------------------------------------------------------------------------*/

  NW_EVT_INIT();

  /*---------------------------------------------------------------------------
   *  Initialize Memory Manager 
   *--------------------------------------------------------------------------*/

  rc = nwMemInitialize();
  NW_ASSERT(NW_OK == rc);

  /*---------------------------------------------------------------------------
   *  Initialize LogMgr
   *--------------------------------------------------------------------------*/

  rc = nwLogMgrInit(nwLogMgrGetInstance(), (NwU8T*)"NW-SAEGW", getpid());
  NW_ASSERT(NW_OK == rc);

  /*---------------------------------------------------------------------------
   * Initialize SAE GW 
   *--------------------------------------------------------------------------*/

  rc =  nwSaeGwInitialize(&saeGw);
  NW_ASSERT(NW_OK == rc);
  
  /*---------------------------------------------------------------------------
   *  Initialize pthread_pool/mutex; by wenxinlee 20150410
   *--------------------------------------------------------------------------*/

  cpu_bind(PGW_CPU_BIND_PROC, 0);
  
/* 20160501 */
  init_queue(&gtpv2_queue);
  init_mutex_queue();
/* 20160501 */

  pgw_thread_mutex_create();
  linkerPthreadpoolInit();
  pgw_ptheads_udp_create();
  //pgw_ptheads_ipv4_create();
  //pgw_ptheads_gtpu_create();

  /*---------------------------------------------------------------------------
   * Event Loop 
   *--------------------------------------------------------------------------*/

  NW_EVT_LOOP();

  NW_SAE_GW_LOG(NW_LOG_LEVEL_ERRO, "Exit from eventloop, no events to process!");

  /*---------------------------------------------------------------------------
   * Finalize SAE GW 
   *--------------------------------------------------------------------------*/

  rc =  nwSaeGwFinalize(&saeGw);
  NW_ASSERT(NW_OK == rc);

  rc =  nwMemFinalize();
  NW_ASSERT(NW_OK == rc);
  
  
  /*---------------------------------------------------------------------------
  *  Finalize pthread_pool/mutex; by wenxinlee 20150410
  *--------------------------------------------------------------------------*/
  pgw_thread_mutex_destory();
  
  pgw_pthread_destory();

  destory_mutex_queue();

  return 0;
}

#ifdef __cplusplus
}
#endif
