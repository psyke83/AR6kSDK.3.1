#include "os_wince.h"
#include "mpx.h"
#include "wfa_sta.h"

int cookie		= 0;
int resetsnd    = 0;
int counter     = 0;
int reset_recd  = 0;
int resetrcv    = 0;
int num_hello   = 0;
int proc_recd   = 0;
int num_retry   = 0;
int All_over    = 0;

int sta_test;                     // the test the STA was commanded to perform
int sta_id;                       // the test the STA was commanded to perform
unsigned int rmsg[512] = {0};           // rx msg buffer
unsigned int txmsg[512] = {0};          // tx msg buffer
unsigned int rrmsg[512] = {0};          // rx msg buffer
unsigned int rtxmsg[512] = {0};         // tx msg buffer
int msgsize = 256;                  // tx msg size

// txmsg[0]  msgno              send_txmsg
// txmsg[1]  dscp               send_txmsg
// txmsg[2]  my_group_cookie    send_txmsg
// txmsg[3]  my_cookie          send_txmsg
// txmsg[4]  my_sta_id          send_txmsg
// txmsg[5]
// txmsg[6]  param0  nexchang   create_apts_msg
// txmsg[7]  param1  nup        create_apts_msg
// txmsg[8]  param2  ndown      create_apts_msg
// txmsg[9]  param3             create_apts_msg
// txmsg[10] cmd                send_txmsg
// txmsg[11] cmd-text
// txmsg[12] cmd-text
// txmsg[13] msgno ascii        send_txmsg
// txmsg[14]

// rmsg[0] nrecv (downlink)
// rmsg[1]
// rmsg[2]
// rmsg[3]    sta->cookie      do_apts          nsent B_H  do_apts
// rmsg[4]
// rmsg[5]
// rmsg[6]
// rmsg[7]
// rmsg[8]
// rmsg[9]    id   (confirm)   do_apts
// rmsg[10]
// rmsg[11]
// rmsg[12]
// rmsg[13]
// rmsg[14]  nsent%10   do_apts

struct sockaddr dst, rcv;           // sock declarations
struct sockaddr_in from;
struct sockaddr_in local;

unsigned int fromlen = sizeof(from);        // sizeof socket struct
char     *targetname;                       // dst system name or ip address
int      sd,rd;                             // socket descriptor

unsigned long wfa_wmm_thread (void *thr_param);
void     WfaStaResetAll();
void     IAmDead();
struct   itimerval waitval_codec;
struct   itimerval waitval_state;

int codec_sec  = 0;                 // default values for codec period
int codec_usec = 10000;             // 10 ms
int state_sec  = 10;                // default values for codec period
int state_usec = 0;                 // 10 ms

int sockflags = MSG_DONTWAIT;       // no sockflags if compiled for cygwin

tgWMM_t wmm_thr[WFA_THREADS_NUM];

unsigned char dscp, ldscp;          // new/last dscp output values

#define PORT    12345               // port for sending/receiving
#define LII     2000000

int port = PORT;

struct apts_msg apts_msgs[] ={
    {0, -1},
    {"B.D", B_D},
    {"B.H", B_H},
    {"B.B", B_B},
    {"B.M", B_M},
    {"M.D", M_D},
    {"B.Z", B_Z},
    {"M.Y", M_Y},
    {"L.1", L_1},
    {"A.Y", A_Y},
    {"B.W", B_W},
    {"A.J", A_J},
    {"M.V", M_V},
    {"M.U", M_U},
    {"A.U", A_U},
    {"M.L", M_L},
    {"B.K", B_K},
    {"M.B", M_B},
    {"M.K", M_K},
    {"M.W", M_W},
    {"APTS TX         ", APTS_DEFAULT },
    {"APTS Hello      ", APTS_HELLO },
    {"APTS Broadcast  ", APTS_BCST },
    {"APTS Confirm    ", APTS_CONFIRM},
    {"APTS STOP       ", APTS_STOP},
    {"APTS CK BE      ", APTS_CK_BE },
    {"APTS CK BK      ", APTS_CK_BK },
    {"APTS CK VI      ", APTS_CK_VI },
    {"APTS CK VO      ", APTS_CK_VO },
    {"APTS RESET      ", APTS_RESET },
    {"APTS RESET RESP      ", APTS_RESET_RESP },
    {"APTS RESET STOP      ", APTS_RESET_STOP },
    {0, 0 }     // APTS_LAST
};

char traceflag;             // enable debug packet tracing
StationProcStatetbl_t stationProcStatetbl[LAST_TEST+1][11] = {
/* Dummy*/{0,0,0,0,0,0,0,0,0,0,0},
/* B.D*/  {{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.H*/  {{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.B*/  {{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBK,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* B.M*/  {{WfaStaSndVI,P_ON,30000000},{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.D*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}     ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* B.Z*/  {{WfaStaSndVO,P_ON,LII / 2 }  ,{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.Y*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBE,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0}},

/* L.1*/  {{WfaStaSndVOCyclic,P_ON,20000},{WfaStaWaitStop,P_ON,LII / 2 }},

/* A.Y*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBE,P_OFF,LII / 2}    ,{WfaStaSndBE,P_ON,LII / 2}   ,{WfaStaWaitStop,P_ON,LII / 2},{0,0,0},{0,0,0}},

/* B.W*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* A.J*/  {{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_OFF,LII / 2},{WfaStaWaitStop    ,P_ON,LII / 2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.V*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop  ,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.U*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSnd2VO,P_ON,LII / 2}   ,{WfaStaWaitStop  ,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* A.U*/  {{WfaStaSndVI,P_ON,LII / 2}  ,{WfaStaSndBE,P_OFF,LII / 2}      ,{WfaStaSndBE,P_ON,LII / 2}    ,{WfaStaSndBE,P_OFF,LII / 2}   ,{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVO,P_OFF,LII / 2} ,{WfaStaWaitStop ,P_ON,LII / 2},{0,0,0}},

/* M.L*/  {{WfaStaSndBE,P_ON,LII / 2}   ,{WfaStaWaitStop,P_ON,LII / 2}     ,{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* B.K*/  {{WfaStaSndVI,P_ON,LII / 2}   ,{WfaStaSndBE,P_ON,LII / 2}        ,{WfaStaSndVI,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.B*/  {{WfaStaSndVO,P_ON,LII / 2}   ,{WfaStaSndVI,P_ON,LII / 2}        ,{WfaStaSndBE,P_ON,LII / 2}     ,{WfaStaSndBK,P_ON,LII / 2}    ,{WfaStaWaitStop,P_ON,LII / 2} ,{0,0,0},{0,0,0},{0,0,0}},

/* M.K*/  {{WfaStaSndVI,P_ON,LII / 8}   ,{WfaStaSndBE,P_ON,LII / 8}        ,{WfaStaSndVI,P_ON,LII / 8}    ,{WfaStaWaitStop,P_ON,LII / 8}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}},

/* M.W*/  {{WfaStaSndVI,P_ON,LII / 8}   ,{WfaStaSndBE,P_ON,LII / 8}        ,{WfaStaSndVI,P_ON,LII / 8}    ,{WfaStaWaitStop,P_ON,LII / 8}  ,{0,0,0},{0,0,0},{0,0,0},{0,0,0}}

};

StationRecvProcStatetbl_t stationRecvProcStatetbl[LAST_TEST+1][5] = {
  {WfaRcvStop,0,0,0,0},
 /*B.D*/ {WfaRcvVO,WfaRcvStop,0,0,0},
 /*B.H*/ {WfaRcvVO,WfaRcvVO,WfaRcvStop,0,0},
 /*B.B*/ {WfaRcvStop,0,0,0,0},
 /*B.M*/ {WfaRcvStop,0,0,0,0},
 /*M.D*/ {WfaRcvBE,WfaRcvBK,WfaRcvVI,WfaRcvVO,WfaRcvStop},
 /*B.Z*/ {WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.Y*/ {WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*L.1*/ {WfaRcvVOCyclic,0,0,0,0},
 /*A.Y*/ {WfaRcvVI,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
 /*B.W*/ {WfaRcvBE,WfaRcvVI,WfaRcvBE,WfaRcvVI,WfaRcvStop},
 /*A.J*/ {WfaRcvVO,WfaRcvVI,WfaRcvBE,WfaRcvBK,WfaRcvStop},
 ///*M.V*/ {WfaRcvVI,WfaRcvBE,WfaRcvVI,WfaRcvStop,0},
 /*M.V*/ {WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.U*/ {WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvVO,WfaRcvStop},
 /*A.U*/ {WfaRcvVI,WfaRcvBE,WfaRcvVO,WfaRcvStop,0},
 /*M.L*/ {WfaRcvBE,WfaRcvStop,0,0,0},
 /*B.K*/ {WfaRcvVI,WfaRcvBE,WfaRcvStop,0,0},
 /*M.B*/ {WfaRcvStop,0,0,0,0},
 /*M.K*/ {WfaRcvBE,WfaRcvVI,WfaRcvStop,0,0},
 /*M.W*/ {WfaRcvBE,WfaRcvBE,WfaRcvBE,WfaRcvVI,WfaRcvStop},
 ///*M.W*/ {WfaRcvBE,WfaRcvBE,WfaRcvBE,WfaRcvStop,0},
};


void WfaStaResetAll ()
{
    int r = 0;
    pthread_mutex_lock (&wmm_thr[0].thr_flag_mutex);
    alarm(0);
    num_retry++;
    if(num_retry > MAXRETRY)
    {
        create_apts_msg (APTS_RESET_STOP, rtxmsg,sta_id);
        set_dscp (TOS_BE);

        r = sendto (sd, (char *)&rtxmsg[0], msgsize, sockflags, &dst, sizeof(dst));
        mpx ("STA msg",rtxmsg,64);
        printf ("Too many retries\n");
        exit (-8);
    }

    if(!reset_recd)
    {
        int resp_recd=0;
        signal (SIGALRM, &IAmDead);  // enable alarm signal

        if ( setitimer (ITIMER_REAL, &waitval_state, NULL) < 0 )
        {
            printf ("\n cant set timer");
            exit(-7);
        }

        create_apts_msg (APTS_RESET, rtxmsg, sta_id);

        set_dscp(TOS_BE);

        rtxmsg[1] = TOS_BE;

        //r = sendto (sd, rtxmsg, msgsize, sockflags, &dst, sizeof(dst));
        mpx ("STA msg",rtxmsg,64);

        while (!resp_recd)
        {

            if ((r = recvfrom (sd, (char *)&rrmsg[0], sizeof(rrmsg), sockflags, (struct sockaddr *)&from, &fromlen)) < 0)
            {
                continue;
            }

            if (rrmsg[10] != APTS_RESET_RESP)
            {
                printf("\r\n Got %d\n",rmsg[10]);
                continue;
            }

            alarm(0);
            resp_recd = 1;
        }
    }
    else
    {
        create_apts_msg (APTS_RESET_RESP, rtxmsg,sta_id);
        set_dscp (TOS_BE);
        r = sendto (sd, (char *)&rtxmsg[0], msgsize, sockflags, &dst, sizeof(dst));
        mpx ("STA msg",rtxmsg,64);
        reset_recd = 0;
    }
    resetsnd = 1;
    resetrcv = 1;

    pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);
}

void IAmDead ()
{
    printf ("\n Enough is Enough..bye\n");
    exit (-9);
}

void timeout ()
{
    int r = 0;
    waitval_codec.it_value.tv_sec = 1;      // 1 sec codec sample period
    waitval_codec.it_value.tv_usec = 0;
    if(num_hello >= MAXHELLO)
    {
        printf ("\n Too many hellos sent, no response! Quitting ...");
        exit(0);
    }

    if (traceflag)
    {
        printf ("STAUT timeout sec %d usec %d \n", waitval_codec.it_value.tv_sec, waitval_codec.it_value.tv_usec);
    }

    if ( (!proc_recd) && (setitimer (ITIMER_REAL, &waitval_codec, NULL) < 0))
    {
        perror ("setitimer: WAIT_STUAT_00: " );
        exit (-1);
    }

    num_hello++;
    set_dscp (TOS_BE);

    create_apts_msg (APTS_HELLO, txmsg, 0);       // send HELLO

    r = sendto (sd, (char *)&txmsg[0], msgsize, sockflags, &dst, sizeof(dst));
 }

wmain(int argc, WCHAR **argv1)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    char argv[16][32];

    int i = 0, r = 0, err = 0;

    tgThrData_t tdata;
    tgWMM_t *my_wmm = &wmm_thr[0];

    int ncnt = 0, cntThr = 0, rcv_state = 0, timer_start = 0;
    StationRecvProcStatetbl_t  *rcvstatarray = NULL;
    StationRecvProcStatetbl_t  func = {0};
    pthread_attr_t ptAttr;
    int ptPolicy = 0;

	printf("entry\n");
    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup (wVersionRequested, &wsaData);

    if ( err != 0 )
    {
        perror("Could not find a usable WinSock DLL");
        return 0;
    }

    for (i = 0; i < argc; i++)
    {
        wcstombs_s (NULL,argv[i],32, argv1[i], 32);
    }

	printf("wcstombs_s\n");
    parse_cmdline (argc, argv);

    codec_sec  = 1;
    codec_usec = 0;

	printf("targetname %s",targetname);
    setup_socket (targetname);
    pthread_attr_init (&ptAttr);

    tdata.tid               = cntThr;
    tdata.state_num         = 0;
    wmm_thr[0].stop_flag    = 0;
    wmm_thr[0].thr_flag     = 0;
	printf("mutesinit start\n");
    pthread_mutex_init (&wmm_thr[0].thr_flag_mutex, NULL);
    pthread_mutex_init (&wmm_thr[0].thr_stop_mutex, NULL);
    pthread_cond_init  (&wmm_thr[0].thr_flag_cond, NULL);
    pthread_cond_init  (&wmm_thr[0].thr_stop_cond, NULL);
	
	printf("mutesinit end\n");
    wmm_thr[0].thr_id = pthread_create (&wmm_thr[0].thr,
                                        &ptAttr, wfa_wmm_thread, &tdata);

    pthread_mutex_lock (&my_wmm->thr_flag_mutex);

    while (!my_wmm->thr_flag)
    {
        pthread_cond_wait (&my_wmm->thr_flag_cond, &my_wmm->thr_flag_mutex);
    }

    pthread_mutex_unlock (&my_wmm->thr_flag_mutex);

    my_wmm->thr_flag = 0;

    while (!All_over)
    {
        if (resetrcv)
        {
            while (!my_wmm->thr_flag)
            {
                pthread_cond_wait (&my_wmm->thr_flag_cond, &my_wmm->thr_flag_mutex);
            }

            my_wmm->thr_flag = 0;
            rcv_state        = 0;
            resetrcv         = 0;
            pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);
        }

        pthread_mutex_lock (&wmm_thr[0].thr_flag_mutex);
        r = recvfrom (sd, (char *)&rmsg[0], sizeof(rmsg), sockflags, (struct sockaddr *)&from, &fromlen);
        cookie = rmsg[0];

        pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);

        if (r < 0)
        {
            continue;
        }

        alarm (0);

        timer_start = 0;

        if (traceflag)
        {
            printf ("APTS Received #    length:%d\n",  r);
            mpx ("APTS RX", rmsg, 64);
        }

        if (rmsg[10] == APTS_RESET)
        {
            reset_recd = 1;
            WfaStaResetAll ();
            continue;
        }

        rcvstatarray = stationRecvProcStatetbl[sta_test];
        func = rcvstatarray[rcv_state];
        func.statefunc (rmsg,r,&rcv_state);
    }

    WSACleanup ();

    if (my_wmm)
    {
        destroyHandle (&my_wmm->thr_flag_mutex);
        destroyHandle (&my_wmm->thr_flag_cond);
        destroyHandle (&my_wmm->thr_stop_mutex);
        destroyHandle (&my_wmm->thr_stop_cond);
    }

	printf("<<MAINEXIT\n");
}




