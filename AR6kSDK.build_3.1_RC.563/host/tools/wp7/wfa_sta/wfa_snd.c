#include "os_wince.h"
#include "mpx.h"
#include "wfa_sta.h"


#define MAX_STOPS 10

extern unsigned int txmsg[512];             // tx msg buffer
extern unsigned int rmsg[512];              // rx msg buffer
extern int sta_id;                          // the test the STA was commanded to perform
//extern int cookie;                        // the test the STA was commanded to perform
extern tgWMM_t wmm_thr[WFA_THREADS_NUM];
extern int sd,rd;                           // socket descriptor
extern unsigned int fromlen;                // sizeof socket struct
extern int msgsize;
extern struct sockaddr_in from;
extern int  sockflags;
extern struct sockaddr dst;                 // sock declarations
extern int proc_recd;
extern int num_hello;
extern int sta_test;                        // the test the STA was commanded to perform
extern char traceflag;                      // enable debug packet tracing
extern StationProcStatetbl_t stationProcStatetbl[LAST_TEST+1][11];
extern int resetsnd;
extern int counter;

int sender (char psave, int sleep_period, int dsc)
{
    int r;
    printf ("\nsleeping for %d", sleep_period);

    usleep (sleep_period);
    set_pwr_mgmt ("STATION", psave);

    set_dscp (dsc);

    create_apts_msg (APTS_DEFAULT, txmsg,sta_id);

    pthread_mutex_lock (&wmm_thr[0].thr_flag_mutex);

    r = sendto (sd, (char *)&txmsg[0], msgsize, sockflags, &dst, sizeof(dst));

    pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);

    if (traceflag)
    {
        mpx("STA msg",txmsg,64);
    }

    return r;
}

int WfaStaSndVO (char psave, int sleep_period, int *state)
{
    int r;
    static int en = 1;
    printf ("\r\nEnterring WfastasndVO %d", en++);

    if ((r = sender (psave, sleep_period, TOS_VO)) >= 0)
    {
        (*state)++;
    }

    return 0;
}

int WfaStaSndVOCyclic (char psave, int sleep_period, int *state)
{
    int r = 0,i;
    static int en = 1;

    for (i = 0; i < 3000; i++)
    {
        printf ("\r\nEnterring WfastasndVO %d",en++);
        sender (psave,sleep_period,TOS_VO);
        printf ("\r\n Sent #%d\n",++r);
    }
    (*state)++;
    return 0;
}
int WfaStaSnd2VO (char psave, int sleep_period, int *state)
{
    int r;
    static int en = 1;
    printf ("\r\nEnterring Wfastasnd2VO %d", en++);

    if ((r = sender (psave, sleep_period, TOS_VO)) >= 0)
    {
        pthread_mutex_lock (&wmm_thr[0].thr_flag_mutex);

        printf ("\nlock met");
        r = sendto (sd, (char *)&txmsg[0], msgsize, sockflags, &dst, sizeof(dst));
        pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);

		if (traceflag)
		{
			mpx ("STA msg\n", txmsg, 64);
		}

        (*state)++;
    }
    return 0;
}
int WfaStaSndVI (char psave, int sleep_period, int *state)
{
    int r;

    printf ("\r\nEnterring WfastasndVI");

    if ((r = sender(psave,sleep_period,TOS_VI))>=0)
    {
        (*state)++;
    }
    return 0;
}

int WfaStaSndBE(char psave,int sleep_period,int *state)
{
    int r;

    printf ("\r\nEnterring WfastasndBE");
    if ((r = sender (psave, sleep_period, TOS_BE)) >= 0)
    {
        (*state)++;
    }
    return 0;
}

int WfaStaSndBK(char psave,int sleep_period,int *state)
{
    int r;

    printf ("\r\nEnterring WfastasndBK");

    if ((r = sender (psave, sleep_period, TOS_BK)) >= 0)
    {
        (*state)++;
    }
    else
    {
        printf ("\n Error");
    }
    return 0;
}

int WfaStaWaitStop (char psave,int sleep_period,int *state)
{
    int r;
    int myid = 0;
    static int num_stops = 0;
    tgWMM_t *my_wmm = &wmm_thr[0];
    num_stops++;

    if (num_stops > MAX_STOPS)
    {
        printf("\r\n Too many stops...quitting");
        exit(0);
    }

    printf ("\n Entering Sendwait");

    usleep (sleep_period);
    set_pwr_mgmt ("STATION", psave);

    set_dscp (TOS_BE);
    create_apts_msg (APTS_STOP, txmsg, sta_id);

    pthread_mutex_lock (&wmm_thr[0].thr_stop_mutex);

    r = sendto (sd, (char *)&txmsg[0], msgsize, sockflags, &dst, sizeof(dst));

	if (traceflag)
	{
		mpx ("STA msg", txmsg, 64);
	}

    pthread_mutex_unlock (&wmm_thr[0].thr_stop_mutex);
    wmm_thr[myid].stop_flag = 1;

    pthread_mutex_lock (&wmm_thr[myid].thr_stop_mutex);

    pthread_cond_signal (&wmm_thr[myid].thr_stop_cond);
    pthread_mutex_unlock (&wmm_thr[myid].thr_stop_mutex);

    return 0;
}

unsigned long* wfa_wmm_thread (void *thr_param)
{
    int r;
    int myid = ((tgThrData_t *)thr_param)->tid;
    tgThrData_t *tdata =(tgThrData_t *)thr_param;
    tgWMM_t *my_wmm = &wmm_thr[myid];

    StationProcStatetbl_t  curr_state;
    fromlen = sizeof (from);

    setup_timers ();
    set_pwr_mgmt ("STATION", P_OFF);

    timeout ();

    while (1)
    {
        while (!proc_recd)
        {
            r = recvfrom (sd, (char *)&rmsg[0], sizeof(rmsg), sockflags, (struct sockaddr *)&from, &fromlen);

            if (r < 0)
            {
                continue;                                 // Waiting for HELLO
            }

            sta_test = rmsg[10];

            if (!((sta_test >= B_D) && (sta_test <= LAST_TEST)))
            {
                continue;
            }

            num_hello = 0;

            signal (SIGALRM, SIG_IGN);                   // Turn off TX timer

            if (traceflag)
            {
                mpx ("STA recv\n", rmsg, 64);
            }
            sta_id = rmsg[9];
            create_apts_msg (APTS_CONFIRM, txmsg,rmsg[9]);

            r = sendto (sd, (char *)&txmsg[0], msgsize, sockflags, &dst, sizeof(dst));

            if (traceflag)
            {
                mpx ("STA send\n", txmsg, 64);
            }

            tdata->state_num = 0;
            tdata->state     = stationProcStatetbl[sta_test];
            proc_recd        = 1;
            wmm_thr[myid].thr_flag = 1;
            pthread_mutex_lock (&wmm_thr[myid].thr_flag_mutex);

            pthread_cond_signal (&wmm_thr[myid].thr_flag_cond);
            pthread_mutex_unlock (&wmm_thr[myid].thr_flag_mutex);
        }

        pthread_mutex_lock (&wmm_thr[myid].thr_flag_mutex);

        if (resetsnd)
        {
            tdata->state_num = 0;

            resetsnd = 0;
            proc_recd = 0;
            setup_timers ();

            timeout ();

            pthread_mutex_unlock (&wmm_thr[myid].thr_flag_mutex);
            continue;
        }

        curr_state = tdata->state[tdata->state_num];
        curr_state.statefunc (curr_state.pw_offon,curr_state.sleep_period,&(tdata->state_num));
        pthread_mutex_unlock (&wmm_thr[myid].thr_flag_mutex);
        printf ("\n\n count %d\n\n", ++counter);
    }
}
