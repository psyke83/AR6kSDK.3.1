#include "os_wince.h"
#include "mpx.h"
#include "wfa_sta.h"

extern unsigned char dscp, ldscp;       // new/last dscp output values
extern tgWMM_t wmm_thr[WFA_THREADS_NUM];
extern int All_over;

int receiver (unsigned int *rmsg, int length, int tos, unsigned int type)
{
    int r = 1;
    int new_dscp = rmsg[1];
    int dsc_size = sizeof(dscp);

    if ((new_dscp != tos) || (rmsg[10] != type))
    {
        printf ("\r\n dscp recd is %d msg type is %d\n",new_dscp,rmsg[10]);
        r = -6;
    }
    return r;
}

int WfaRcvVO (unsigned int *rmsg, int length,int *state)
{
    int r = 1;

    if ((r = receiver(rmsg,length,TOS_VO,APTS_DEFAULT)) >= 0)
    {
        (*state)++;
    }
    else
    {
        printf ("\nBAD REC in VO%d\n",r);
        WfaStaResetAll ();
        //  exit(-5);
    }

    return r;
}

int WfaRcvVOCyclic (unsigned int *rmsg, int length, int *state)
{
    int r = 1;
    tgWMM_t *my_wmm = &wmm_thr[0];

    if (rmsg[10] != APTS_STOP)
    {
        if ((r = receiver (rmsg, length, TOS_VO, APTS_DEFAULT)) >= 0)
        {
            ;
        }
        else
        {
            printf ("\nBAD REC in VO%d\n",r);
            //WfaStaResetAll();
            //  exit(-5);
        }
    }
    else
    {
        pthread_mutex_lock (&my_wmm->thr_stop_mutex);

        while (!my_wmm->stop_flag)
        {
            pthread_cond_wait (&my_wmm->thr_stop_cond, &my_wmm->thr_stop_mutex);
        }

        pthread_mutex_unlock (&my_wmm->thr_stop_mutex);
        my_wmm->stop_flag = 0;
        sleep (1);
        All_over = 1;
    }

    return r;
}

int WfaRcvVI (unsigned int *rmsg, int length, int *state)
{

    int r = 1;
    if ((r  = receiver(rmsg,length,TOS_VI,APTS_DEFAULT)) >= 0)
    {
        (*state)++;
    }
    else
    {
        printf ("\nBAD REC in VI%d\n",r);
        WfaStaResetAll();
    }

    return r;
}

int WfaRcvBE (unsigned int *rmsg, int length, int *state)
{

    int r = 1;
    if (( r = receiver (rmsg, length, TOS_BE, APTS_DEFAULT)) >= 0)
    {
        (*state)++;
    }
    else
    {
        printf ("\nBAD REC BE%d\n",r);
        WfaStaResetAll ();
    }
    return r;
}
int WfaRcvBK(unsigned int *rmsg,int length,int *state)
{
    int r = 1;

    if ((r = receiver (rmsg, length, TOS_BK, APTS_DEFAULT)) >= 0)
    {
        (*state)++;
    }
    else
    {
        printf ("\nBAD REC in BK %d\n",r);
        WfaStaResetAll ();
    }
    return r;
}

int WfaRcvStop (unsigned int *rmsg, int length, int *state)
{
    int r = 1;
    tgWMM_t *my_wmm = &wmm_thr[0];

    my_wmm->stop_flag = 0;
    printf ("\r\nEnterring Wfarcvstop\n");

    if (rmsg[10] != APTS_STOP)
    {
        WfaStaResetAll();
    }
    else
    {
        pthread_mutex_lock (&my_wmm->thr_stop_mutex);
        while (!my_wmm->stop_flag)
        {
            pthread_cond_wait (&my_wmm->thr_stop_cond, &my_wmm->thr_stop_mutex);
        }
        pthread_mutex_unlock (&my_wmm->thr_stop_mutex);
        my_wmm->stop_flag = 0;
        sleep (1);
        All_over = 1;
    }

    return r;
}
