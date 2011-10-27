/*
* APTS messages/tests
*/

#define B_D             1
#define B_H             2
#define B_B             3
#define B_M             4
#define M_D             5
#define B_Z             6
#define M_Y             7
#define L_1             8
#define A_Y             9       // active mode version of M_Y
#define B_W             10      //
#define A_J             11      // Active test of sending 4 down
#define M_V             12
#define M_U             13
#define A_U             14
#define M_L             15
#define B_K             16
#define M_B             17
#define M_K             18
#define M_W             19
#define LAST_TEST       M_W
#define APTS_DEFAULT    20      // message codes
#define APTS_HELLO      21
#define APTS_BCST       22
#define APTS_CONFIRM    23
#define APTS_STOP       24
#define APTS_CK_BE      25
#define APTS_CK_BK      26
#define APTS_CK_VI      27
#define APTS_CK_VO      28
#define APTS_RESET      29
#define APTS_RESET_RESP 30
#define APTS_RESET_STOP 31
#define APTS_LAST       99

#define WFA_THREADS_NUM 1
#define MAXRETRY        3
#define MAXHELLO        20
#define NTARG           32       // number of target names
#define EQ(a,b)         (strcmp(a,b) == 0)

typedef struct _tg_wmm
{
    int thr_flag;     /* this is used to indicate stream id */
    int stop_flag;    /* this is used to indicate stream id */
    pthread_t thr;
    HANDLE thr_id;
    pthread_cond_t thr_flag_cond;
    pthread_cond_t thr_stop_cond;
    pthread_mutex_t thr_flag_mutex;
    pthread_mutex_t thr_stop_mutex;

}tgWMM_t;

/*
* internal table
*/
struct apts_msg {           //
    char *name;             // name of test
    int cmd;                // msg num
};

/*
* wme
*/
#define CE_TOS_VO7     DSCPAudio           // 111 0  0000 (7)  AC_VO tos/dscp values
#define CE_TOS_VO      DSCPAudio           // 110 0  0000 (6)  AC_VO tos/dscp values

#define CE_TOS_VI      DSCPVideo           // 101 0  0000 (5)  AC_VI
#define CE_TOS_VI4     DSCPVideo           // 100 0  0000 (4)  AC_VI

#define CE_TOS_BE      DSCPBestEffort      // 000 0  0000 (0)  AC_BE
#define CE_TOS_EE      DSCPBestEffort      // 011 0  0000 (3)  AC_BE

#define CE_TOS_BK      DSCPBackground      // 001 0  0000 (1)  AC_BK


#define	TOS_VO7		0xE0				// 111 0  0000 (7)  AC_VO tos/dscp values
#define	TOS_VO	    0xC0				// 110 0  0000 (6)  AC_VO tos/dscp values

#define	TOS_VI	    0xA0                // 101 0  0000 (5)  AC_VI
#define	TOS_VI4		0x80                // 100 0  0000 (4)  AC_VI

#define	TOS_BE	    0x00                // 000 0  0000 (0)  AC_BE
#define	TOS_EE	    0x60                // 011 0  0000 (3)  AC_BE

#define	TOS_BK	    0x20                // 001 0  0000 (1)  AC_BK
/*
* power management
*/
#define P_ON    1
#define P_OFF   0

#if WFA_DEBUG
#define PRINTF(s,args...) printf(s,## args)
#else
#define PRINTF printf;
#endif

typedef int (*StationStateFunctionPtr)( char, int,int *); //PS,sleep period,dscp

typedef struct station_state_table
{
    StationStateFunctionPtr statefunc;
    char                    pw_offon;
    int                     sleep_period;
} StationProcStatetbl_t;

typedef struct _tg_thr_data
{
    int tid;
    StationProcStatetbl_t  *state;
    int state_num;
} tgThrData_t;

typedef int (*stationRecvStateFunctionPtr)(unsigned int *, int,int * ); //Recieved message buffer, length,state

typedef struct console_rcv_state_table
{
    stationRecvStateFunctionPtr statefunc;
} StationRecvProcStatetbl_t;

int WfaStaSndHello(char,int,int *state);
int WfaStaRcvProc(char,int,int *state);
int WfaStaSndConfirm(char,int,int *state);
int WfaStaSndVO(char,int,int *state);
int WfaStaSndVOCyclic(char,int,int *state);
int WfaStaSnd2VO(char,int,int *state);
int WfaStaWaitStop(char,int,int *state);
int WfaStaSndVI(char,int,int *state);
int WfaStaSndBE(char,int,int *state);
int WfaStaSndBK(char,int,int *state);
int WfaStaSndVIE(char,int,int *state);
int WfaStaSndBEE(char,int,int *state);
int WfaStaSnd2VOE(char,int,int *state);
void create_apts_msg(int msg, unsigned int txbuf[],int id);
int WfaRcvStop(unsigned int *,int ,int *);
int WfaRcvVO(unsigned int *,int ,int *);
int WfaRcvVOCyclic(unsigned int *,int ,int *);
int WfaRcvVI(unsigned int *,int ,int *);
int WfaRcvBE(unsigned int *,int ,int *);
int WfaRcvBK(unsigned int *,int ,int *);
int set_dscp (int new_dscp);
void setup_socket (char *name);
void set_pwr_mgmt (char *msg, int mode);
void parse_cmdline (int argc, char argv[16][32]);
void setup_timers ();
int setup_addr (char *name, struct sockaddr *dst);
int strvec_sep (char *s, char *array[], int n, char *sep);
void timeout ();
