#include "os_wince.h"
#include "mpx.h"
#include "wfa_sta.h"

extern struct sockaddr_in from;
extern struct sockaddr_in local;
extern int sd,rd;                           // socket descriptor
extern unsigned char  dscp,ldscp;           // new/last dscp output values
extern tgWMM_t wmm_thr[WFA_THREADS_NUM];
struct sockaddr_in target;
extern int port;
extern unsigned int txmsg[512];             // tx msg buffer
extern struct sockaddr dst, rcv;            // sock declarations
extern struct apts_msg apts_msgs[];
extern char traceflag;                      // enable debug packet tracing
extern char *targetname;                    // dst system name or ip address
extern struct itimerval waitval_codec;
extern struct itimerval waitval_state;
extern int codec_sec;
extern int codec_usec;
extern int state_sec;
extern int state_usec;
extern int cookie;

int     ntarg;
char    athrflag   = 0;     // is ATHR device
unsigned char bIsSetupSocket = 0;

char    companyProduct = 0;
char   *targetnames[NTARG];
void    timeout();

is_ipdotformat (char *s)
{
    int d;
    for (d = 0; *s; s++)
    {
        if (*s == '.')
            d++;
    }
    return (d == 3);
}

void pbaddr (char *s, unsigned long int in)
{
    unsigned char a, b, c, d;

    a = (unsigned char)in&0xff;
    b = (unsigned char)(in>>8)&0xff;
    c = (unsigned char)(in>>16)&0xff;
    d = (unsigned char)(in>>24)&0xff;

    printf ("%s %d.%d.%d.%d\n", s, a, b, c, d);
}

int strvec_sep (char *s, char *array[], int n, char *sep)
{
    char *p;
    static char buf[2048];
	char *next_token;
    int i;

	printf(">strvec_sep\n");

    SAFE_STRN_CPY (buf, s, sizeof (buf));

    p = strtok_s (buf, sep,&next_token);

    for (i = 0; p && i < n; )
    {
        array[i++] = p;
        if (i == n)
        {
            i--;
            break;
        }
        p = strtok_s (0, sep,&next_token);
    }

    array[i] = 0;

	printf("<strvec_sep\n");
    return(i);
}

int setup_addr (char *name, struct sockaddr *dst)
{
    struct  hostent *h;
    char   *array[5];
    int     d, r;

    unsigned long in =0;
    unsigned char b;

    if (is_ipdotformat (name))
    {
		printf("IP DOT FORMAT\n");
        // check for dot format addr
        strvec_sep (name, array, 5, ".");
        for (d = 0; d < 4; d++)
        {
            b = atoi (array[d]);
            in |= (b << (d * 8));
			printf("%d\n",b);
		}

        target.sin_addr.s_addr = in;
    }
    else
    {
        h = gethostbyname (name);                // try name lookup
        if (h)
        {
            memcpy ((caddr_t)&target.sin_addr.s_addr, h->h_addr, h->h_length);
        }
        else
        {
            fprintf(stderr, "name lookup failed for (%s)\n", name);
            exit(-1);
        }
    }

    pbaddr("dst:", target.sin_addr.s_addr);
    target.sin_family   = AF_INET;
    target.sin_port     = htons( port);

    memcpy((caddr_t)dst, (caddr_t)&target, sizeof(target));


    from.sin_family         = AF_INET;
    from.sin_port           = htons(port);
    from.sin_addr.s_addr    = htonl(INADDR_ANY);
    local.sin_addr.s_addr   = htonl(INADDR_LOOPBACK);

    r = bind (sd, (struct sockaddr *)&from, sizeof(from));
    if ( r < 0)
    {
        perror("bind call failed");
        exit(-1);
    }

    return (r);
}

int set_dscp (int new_dscp)
{
    int r;
    int dscpVal = 0;

    if (!bIsSetupSocket)
    {
        txmsg[1] = new_dscp;
    }

    if (new_dscp == ldscp)
    {
        return 0;
    }

    pthread_mutex_lock (&wmm_thr[0].thr_flag_mutex);

    switch (new_dscp)
    {
		case TOS_VO7:
		    dscpVal = CE_TOS_VO7;
			break;
		case TOS_VO:
			dscpVal = CE_TOS_VO;
			break;
		case TOS_VI:
			dscpVal = CE_TOS_VI;
			break;
		case TOS_VI4:
			dscpVal = CE_TOS_VI4;
			break;

		case TOS_BE:
			dscpVal = CE_TOS_BE;
			break;
		case TOS_EE:
			dscpVal = CE_TOS_EE;
			break;

		case TOS_BK:
			dscpVal = CE_TOS_BK;
			break;

		default:
			dscpVal = CE_TOS_BE;
	}

    if ((r = setsockopt (sd, IPPROTO_IP, IP_DSCP_TRAFFIC_TYPE, (char *)&dscpVal, sizeof(dscpVal))) == SOCKET_ERROR)
    {
        perror("can't set dscp/tos field");
        exit(-1);
    }

    if (!bIsSetupSocket)
    {
        dscp = ldscp = new_dscp;
    }

    pthread_mutex_unlock (&wmm_thr[0].thr_flag_mutex);

    return (new_dscp);
}
void setup_socket (char *name)
{
    int local_dscp = 0;
    int stype = SOCK_DGRAM;
    int sproto = 0;
    int r;
    int iMode = 0;  //

    local_dscp = TOS_BE;
	printf("entry\n");
    if ((sd = socket (AF_INET, stype, sproto)) < 0)
    {
        perror ("socket");
		printf("socket error\n");
        exit (-1);
    }

    bIsSetupSocket = 1;
	printf("set_dscp\n");
    set_dscp (local_dscp);
    bIsSetupSocket = 0;

    iMode = 1; // Enable non-blocking mode

    ioctlsocket (sd, FIONBIO, &iMode);
	printf("setup_addr\n");
    if ((r = setup_addr (name, &dst)) < 0)
    {
        fprintf (stderr, "can't map address (%s)\n", name);
        exit (-1);
    }
	printf("exit\n");
}

void set_pwr_mgmt (char *msg, int mode)
{
    static int last_mode = -1;

    //if (last_mode == mode)
    {
        return;
    }

    if (athrflag)
    {
        // ATHR specific
        if (mode == P_OFF)
        {
			if ( system(_T("-i AR6K_SD1 -p maxperf")) < 0)
			{
				printf("%s: AR6K powersave botch\n", msg);
				return;
            }
            else
            {
            	printf("%s: AR6K power save OFF\n", msg);
            }
		}

        if (mode == P_ON)
        {
			if ( system(_T("-i AR6K_SD1 -p rec")) < 0)
			{
            	printf("%s: AR6K powersave botch\n", msg);
            	return;
            }
            else
            {
            	printf("%s: AR6K power save on\n", msg);
            }
		}
    }
    else
    {
        printf("%s: Unknown device for power save change\n", msg);
    }

    last_mode = mode;
}

void create_apts_msg (int msg, unsigned int txbuf[],int id)
{
    struct apts_msg *t;

    t = &apts_msgs[msg];
    txbuf[ 0] = cookie;
    txbuf[ 1] = dscp;
    txbuf[ 2] = 0;
    txbuf[ 3] = 0;
    txbuf[ 4] = 0;
    txbuf[ 5] = 0;

    //txbuf[ 6] = t->param0;
    //txbuf[ 7] = t->param1;
    //txbuf[ 8] = t->param2;

    txbuf[ 9]  = id;
    txbuf[ 10] = t->cmd;

    SAFE_STR_CPY ((char *)&txbuf[11], t->name);

    if (traceflag)
    {
        printf ("create_apts_msg (%s) %d\n", t->name, t->cmd);
    }
}

void parse_cmdline (int argc, char argv[16][32])
{
    int i;


    for (i = 1; i < argc; i++)
    {
        // parse command line options
        if (argv[i][0] != '-')
        {
            if (EQ(argv[i], "?") || EQ(argv[i], "help") || EQ(argv[i], "-h"))
            {
                goto helpme;
            }

            targetname = argv[i];           // gather non-option args here
            if (ntarg < NTARG)
            {
                targetnames[ntarg++] = targetname;
            }
            continue;
        }
        if (EQ(argv[i], "-t") || EQ(argv[i], "-trace"))
        {
            traceflag = 1;
            continue;
        }
        if (EQ(argv[i], "-athr") || EQ(argv[i], "-ATHR"))
        {
			printf("athr arg\n");
            athrflag = 1;
            continue;
        }

       if (EQ(argv[i], "-product"))
       {
            i++;
            companyProduct = atoi(argv[i]);
            continue;
        }

        if (EQ(argv[i], "-h"))
        {
helpme:
            printf("-codec\n-bcstIP\n-period\n-hist\n-wmeload\n-broadcast\n-count\n-qdata\n-qdis\n-tspec\n");
            exit(0);
        }
    }
}

void setup_timers ()
{
    waitval_codec.it_value.tv_sec  = codec_sec;      // sec codec sample period
    waitval_codec.it_value.tv_usec = codec_usec;
    waitval_state.it_value.tv_sec  = state_sec;      // sec state sample period
    waitval_state.it_value.tv_usec = state_usec;
    signal (SIGALRM, &timeout);                      // enable alarm signal
}
