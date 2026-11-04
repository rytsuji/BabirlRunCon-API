/* Babirl: bi-common.h
 * last modified : 16/12/28 13:45:34 
 * Hidetada Baba (RIKEN)
 * baba@rarfaxp.riken.jp
 * 
 * Common header file
 *
 */

#include <bbcpri.h>
#include <libbabirl.h>

/* alias */
#define u64 unsigned long long int

/* macros */
#ifdef DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

#define LEFN(x) (x & 0xff) ///< real EFN to local EFN (0-255)

/* usefull values */
typedef int boolean;
typedef int bool;

#define TRUE  1
#define FALSE 0
#define STAT_RUN_IDLE     0
#define STAT_RUN_START    1
#define STAT_RUN_NSSTA    2
#define STAT_RUN_WAITSTOP 3
#define STAT_RUN_DRY      4

#define EB_EFLIST_OFF     0
#define EB_EFLIST_ON      1
#define EB_EFLIST_SCR     2
#define EB_EFLIST_PAS     3

#define WHOAREYOU        98

/* babild tcp commands*/
#define EB_GET_DAQINFO   1    // Get all daqinfo
#define EB_SET_DAQINFO   2    // Set all daqinfo
#define EB_GET_RUNINFO   3    // Get all runinfo
#define EB_SET_RUNINFO   4    // Set all runinfo
#define EB_GET_EFNUM     5    // Get Event fragment number
#define EB_GET_EFLIST    6    // Get Event fragment list
#define EB_SET_EFLIST    7    // Set Event fragment list
#define EB_SET_HEADER    8    // Set runinfo.header
#define EB_SET_ENDER     9    // Set runinfo.ender
/* #define EB_GET_MTON     11 */
/* #define EB_SET_MTON     12 */
#define EB_GET_MTLIST   13
#define EB_SET_MTLIST   14
/* #define EB_GET_HDON     15 */
/* #define EB_SET_HDON     16 */
#define EB_GET_HDLIST   17
#define EB_SET_HDLIST   18
#define EB_RUN_START    21
#define EB_RUN_NSSTA    22
#define EB_RUN_STOP     23
#define EB_RUN_CLOSE    24
#define EB_CHK_DIR      31    // Check Directory
#define EB_SET_RUNNUMBER   41    // Runnumber
#define EB_SET_RUNNAME     42    // Runname
#define EB_SET_EBSIZE   43    // Event build size
#define EB_GET_EVTN     44    // Get event built number
#define EB_SET_BABILDES 45    // Set babildes mode
#define EB_SET_CHKERHOST 46   // Check event receiver's host 0=off, 1=on
#define EB_GET_CHKERHOST 47   // 
#define EB_GET_SSMINFO  51
#define EB_SET_SSMINFO  52
#define EB_SET_STAT_COMMAND 53
#define EB_GET_STAT_COMMAND 54
#define EB_GET_TOTSIZE  61
#define EB_SET_TGIG     62
#define EB_GET_TGIG     63
#define EB_GET_EFBLKN   64
#define EB_QUIT         99    // Quit EB and ESs

/** babies tcp commands */
#define ES_SET_CONFIG    1
#define ES_GET_CONFIG    2
#define ES_RUN_START     3
#define ES_RUN_NSSTA     4
#define ES_RUN_STOP      5
#define ES_RELOAD_DRV    6
#define ES_GET_EVTN      8    // Get last event number
#define ES_GET_RUNSTAT   9    // Get Run status
#define ES_CON_EFR      11    // Connect EFS to EFR
#define ES_DIS_EFR      12    // Disconnect EFS to EFR
#define ES_RUN_DRY      13    // start dry run
#define ES_RUN_DRYSTOP  14    // terminate dry run / client
#define ES_QUIT         99
/** babies tcp arguments */
#define ES_EF_ON    0x00000000    // Join event build
#define ES_EF_OFF   0x80000000    // Not join event build

#define MEM_OVER_EVTMEM           1
#define MEM_NMALLOC_EVTMEM_FST    2
#define MEM_NMALLOC_EVTDATA_FST   3
#define MEM_NMALLOC_EVTDATA_ADD   5
#define MEM_NMALLOC_EVTMEM_NEV    6
#define MEM_NMALLOC_EVTDATA_NEV   7
#define MEM_NMALLOC_EVTN_JUMP     8

#define EARECV_DATA      0
#define EARECV_DAQINFO   1
#define EARECV_RUNINFO   2

/** babinfo commands */
#define INF_GET_DAQINFO     1
#define INF_GET_RUNINFO     2
#define INF_GET_RAWDATA    10
#define INF_GET_BLOCKNUM   11
#define INF_QUIT           99
#define INF_GET_CLIHOSTS  101
#define INF_SET_CLIHOST   102
#define AU_GET_CLINFO     103
#define AU_SET_CLINFO     104
#define INF_GET_TCPCLIHOSTS  105
#define INF_MK_TCPCLIENT  106
#define INF_GET_COMLIST   201
#define INF_GET_SCRLIST   301
#define INF_SET_SCRNAME   302
#define INF_GET_SCRDATA   303
#define INF_GET_SCRLIVE   304
#define INF_SET_SCRLIVE   305
#define INF_DEL_SCR       311
#define INF_SET_SCRFROMDB 321
#define INF_GET_STATLIST  401
#define INF_SET_DBCON     402
#define INF_GET_DBCON     403
#define INF_CHK_DBCON     404
#define INF_UPDATE_DBRUNSTOP 405

/** babissm commands */
#define SSM_SET_SSMINFO   1
#define SSM_START         2
#define SSM_STOP          3

/** babimo commands */
#define MON_GETHDST        11
#define MON_CHKPID         12
#define MON_KILLPID        13
#define MON_EXEC           14
#define MON_PEXEC          15
#define MON_BABIEXEC       16
#define MON_COPYFILE       17
#define MON_CHKBABIES      18
#define MON_GETHDST_STR   101
#define MON_CHKPID_STR    102


/* babils ascii commands */
#define ASC_EB_SET_DAQINFO    0x0001
#define ASC_EB_SET_EFLIST     0x0002
#define ASC_EB_SET_MTLIST     0x0004
#define ASC_EB_SET_HDLIST     0x0008
#define ASC_EB_SET_SSMINFO    0x0010
#define ASC_EB_RUN_START      0x0020
#define ASC_EB_RUN_NSSTA      0x0040
#define ASC_EB_RUN_STOP       0x0080
#define ASC_EB_RUN_CLOSE      0x0100
#define ASC_EB_SET_RUNNUMBER  0x0200
#define ASC_EB_SET_RUNNAME    0x0400
#define ASC_EB_SET_EBSIZE     0x0800
#define ASC_EB_SET_BABILDES   0x1000
#define ASC_EB_SET_EFN        0x2000
#define ASC_EB_QUIT           0x8000

/* statcom */
#define STATCOM_START_XML     11
#define STATCOM_STOP_XML      12


//#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
//#else
#ifndef __APPLE__
union semun {
  int val;                        /* value for SETVAL */
  struct semid_ds *buf;           /* buffer for IPC_STAT & IPC_SET */
  unsigned short *array;          /* array for GETALL & SETALL */
  struct seminfo *__buf;          /* buffer for IPC_INFO */
};
#endif

struct ebshm{
  int shmid;
  int semid;
  int efid;
  int malloc;
  union semun sem;
  char *data;
};

struct ebrshm{
  int efid;
  int ringid;
  char *data;
};

/** Event memory for event building
 */
struct evtmem{
  //! Pointer for next event memory (next event)
  struct evtmem *next;
  //! Pointer array for event data buffer, malloc each event and each EF
  char *evtdata[MAXEF];
 //! Size of evtdata
  int size[MAXEF];
  //! Event number of this event memory
  unsigned int evtn;
};

/** Non-event memory for event building (scaler, status, ...)
 */
struct nevtmem{
  struct nevtmem *next;   ///< Pointer for next nevtmem
  //! Pointer array for nevt data buffer, malloc each nevtdata
  char *nevtdata;
  //! Size of nevtdata
  int size;
};

/** Time stamp memory for CC event building 
 */
struct tsmem{
  struct tsmem *next;  ///< Pointer for next tsmem
  long long int ts;    ///< Time stamp
  unsigned int evtn;   ///< event number
  int efn;    ///< efn
};

/*! Structure for EF list */
struct steflist{
  int  ex;   ///< 0=nan, 1=exist
  int  of;    ///< 0=off, 1=on, 2=scr
  char name[80];   ///< Name of EF
  char host[80];   ///< Host name or IP address of EF
};

/*! Structre for HD list */
struct sthdlist{
  //!  0=nan, 1=exist
  int  ex;
  //!  0=off, 1=on
  int  of;
  //!  Path where into saved of this HD
  char path[80];
  //!  Free size of this HD
  u64  free;
  //!  Full size of this HD
  u64  full;
  //!  Maximam size of file size
  u64  maxsize;
};

/*! Structure for MT List */
struct stmtlist{
  int  ex; /// 0=nan, 1=exist
  int  of; /// 0=off, 1=on
  char path[80]; 
  u64  maxsize;
};

/*! Structure for DAQ information */
struct stdaqinfo{
  char runname[80]; ///< Run name (File name)
  int runnumber;    ///< Run number
  int ebsize;       ///< Event build size
  int efn;          ///< Event fragment number of this
  int babildes;     ///< Flag for babildes mode (1=babildes, 0=standalone)
  struct steflist eflist[MAXEF];  ///< Event fragment list
  struct sthdlist hdlist[MAXHD];  ///< HDD list
  struct stmtlist mtlist[MAXMT];  ///< MT list
};

/*! Structure for Run information */
struct struninfo{
  int runnumber;   ///< Run number (same as stdaqinfo.runnumber)
  int runstat;     ///< Run status (read only)
  int starttime;   ///< Run start time (read only)
  int stoptime;    ///< Run stop time (read only)
  char header[80]; ///< Run header comment
  char ender[80];  ///< Run enver comment
};



/*! Structure for event fragment resource */
struct stefrc{
  int efid;          ///< EFID
  char runname[80];  ///< Run name (RIDF file name)
  int runnumber;     ///< Run number
  short erport;      ///< ER port
  short comport;     ///< ES communication port
  char erhost[80];   ///< ER hostname
  int hd1;           ///< Flag for HDD1
  char hd1dir[80];   ///< Path of HDD1
  int hd2;           ///< Flag for HDD2
  char hd2dir[80];   ///< Path of HDD2
  int mt;            ///< Flag for MT
  char mtdir[80];    ///< Path of MT
  int connect;       ///< Connectivity of ER (0=disconnected., 1=connected)
};

/*! Structure for UDP client hosts */
struct stclinfo{
  int ex;            ///< 0=none, 1>=exist, indicates SHMID
  char clihost[80];  ///< Hostname 
};

/*! Structure for TCP client hosts */
struct sttcpclinfo{
  int sock;          ///< 0=none, 1>=fd of socket
  int port;          ///< Port number
  char host[80];     ///< Hostname
};


/* Structure for babissm */
struct stssminfo{
  int ex;
  int of;
  char host[80];
  char start[80];
  char stop[80];
};

/* Structure for start stop status command */
struct ststatcom{
  int of;
  int id;
  char start[80];
  char stop[80];
};


struct hdstatst{
  int ex;
  char dev[80];
  char path[80];
  int tot;    // total size in KB
  int used;   // used
  int free;   // free
};

struct stbsegid{
  unsigned module   : 8;
  unsigned detector : 6;
  unsigned focal    : 6;
  unsigned device   : 6;
  unsigned revision : 6;
};


/* Structure for babits */
struct fdinfost{
  int id;
  int fn;
  int ifd;
  int ofd;
  int runnumber;
  char name[256];
  struct fdinfost *next;
  struct fdinfost *prev;
};


/* Structure for db connection */
struct stdbcon{
  int  of;          // onoff
  int  rev;         // reserve
  char host[80];    
  char dbname[80];
  char user[80];
  char passwd[80];
};

/* prototype */
extern int pselect();


