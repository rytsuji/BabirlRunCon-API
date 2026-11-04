/* babier: bi-cofig.h
 * last modified : 14/07/07 12:27:36 
 * Hidetada Baba (RIKEN)
 * baba@rarfaxp.riken.jp
 * 
 * Configuration file
 *
 */

#include "userdefine.h"


/* RTLinux installed */
/* If using RTLinux with babirtdrv, define follows */
//#define RTLINUXDRV
#define RTDRIVERNAME "babirtdrv"

/* Shared memory */
#ifdef RTLINUXDRV
#define MEMADDR 500                 // Start Address(MB) for SHM
#define RTSHMEM (MEMADDR*0x100000)   
#define MAPSIZE 0x100000             // Mapping size 0x100000 = 1MB
#endif


/* Configuration parameers */
/* Event Fragment */
#define EBDEFSIZE  16384     ///< Default Event Block Size
#define MAXEF      256       ///< Maximum number of Event fragment
#define MAXEVTMEM  0x80000   ///< Maximum number of event memory = 512kevt

#define EB_BUFF_SIZE    81920      ///< Buffer size of babirl tcp commands = 80kB

#ifdef HUGEBUFF
#define EB_EFBLOCK_SIZE 0x800000   ///< Usual max size of block data = 8MB
#else
#ifdef LARGEBUFF
#define EB_EFBLOCK_SIZE 0x200000   ///< Usual max size of block data = 2MB 
#else
#define EB_EFBLOCK_SIZE 0x20000   ///< Usual max size of block data = 256kB
#endif 
#endif

#define EB_EFBLOCK_BUFFSIZE EB_EFBLOCK_SIZE * 2  ///< Usual size of block data buffer
//#define EB_EFBLOCK_MAXSIZE 0x400000 ///< Real max size of block data = 8MB
#define EB_EFBLOCK_MAXSIZE 0x80000 ///< Temp max size of block data = 1MB
#define EB_UDP_BUFFSIZE 64*1024  ///< Buffer for UDP

#define BABIRL_COM_SIZE    1024*10 ///< Buffer size for babirl tcp commands 

#ifdef HUGEBUFF
#define EB_EBBUFF_SIZE     EB_EFBLOCK_BUFFSIZE ///< Buffer size for ebbuf = 512kshort = 8MB
#else
#ifdef LARGEBUFF
#define EB_EBBUFF_SIZE     EB_EFBLOCK_BUFFSIZE ///< Buffer size for ebbuf = 512kshort = 4MB
#else
#define EB_EBBUFF_SIZE     0x80000 ///< Buffer size for ebbuf = 512kshort = 1MB
#endif
#endif

#define MON_MAX_BUFF  1024*100  ///< Maximum buffer size for babimo

#define EB_MESS_SIZE 1024 ///< Maximum babild message size

#define WORDSIZE      2
#define EF_SHM_SIZE   EB_EFBLOCK_BUFFSIZE * 2 + 12
#define EF_SHM_ADSIZE EB_EFBLOCK_BUFFSIZE + 2
#define EF_SHM_DATA1  0
#define EF_SHM_DATA2  EB_EFBLOCK_BUFFSIZE
#define EF_SHM_DATA3  EF_SHM_SIZE
#define EF_SHM_FLAG1  EB_EFBLOCK_BUFFSIZE * 2
#define EF_SHM_FLAG2  EB_EFBLOCK_BUFFSIZE * 2 + 2
#define EF_SHM_FLAG3  EF_SHM_SIZE + EF_SHM_ADSIZE - 2
#define EF_SHM_EVTN   EB_EFBLOCK_BUFFSIZE * 2 + 4
#define EF_SHM_SSF    EB_EFBLOCK_BUFFSIZE * 2 + 8
#define EF_SHM_RUN    EB_EFBLOCK_BUFFSIZE * 2 + 10
#define EF_SHM_FREE   0x00
#define EF_SHM_READY1 0x01
#define EF_SHM_READY2 0x02

#define BABIAN_SHM_BUFF_SIZE 0x80000

/* Maximum values */
#define MAXHD      10      ///< Maximum number of write hd path
#define HDMAXSIZE  2000000000U
#define MAXMT      1
#define MTMAXSIZE  4000000000U
#define MAXCLI     16      ///< Maximum client number for online analysis
#define MAXSCRANA  32      ///< Maximum number of sclaer ananlysis
#define MAXMTPT    20      ///< Maximum number of check of mount point
#define MAXANPT    6       ///< Maximum number of babian receive port (should be less than 9)
#define MAXTCPCLI  3       ///< Maximum number of babinfo's tcp client

/* nomally, not change following */
/* Keys */
#define EFSHMKEY    523400   ///< First number of Shared memory
#define EFSEMKEY    523700   ///< First number of Semaphore key
//#define FSEMKEY     523600   // Semaphore for FIFO

#define ANSHMKEY    561000   ///< Shared memory key for babian
#define ANSEMKEY    561001   ///< Semaphore key for babian shm

/* Ports */
/* 17502 - 17507  for babian receive port */
#define ANRCVPORT   17502   ///< babian receive port
#define EBCOMPORT   17511
#define SLRCVPORT   17512   ///< Slow data receive port (UDP)
#define TCPSLRCVPORT   17513   ///< Slow data receive port (TCP)
#define INFEAPORT   17515   ///< babinfo EA receive port
#define INFCOMPORT  17516   ///< babinfo communication port
#define SSMCOMPORT  17517   ///< babissm communication port
#define BABIMOPORT  17518   ///< babimo communication port
/** 17550       : for eb data */
/** 17601       : for receiver */
#define ERRCVPORT   17601
#define ANYREVPORT 17622  // for anystore but
/** 17651-18000 : for babies communication port */
#define ESCOMPORT   17651
/** 18000-18255 : for babies proxy communication port */
#define ESCOMPORTPROXY   18000


/* babinfo setting*/
#define MAXBABINFO  256     ///< Maximum number of thread for babinfo

/* Files */
#define EBFIFO      "/tmp/ebfifo"          ///< Path of EBFIFO
#define RCVPID      "/tmp/bi-rcvpid"       ///< 
#ifndef PIDDIR
#define PIDDIR      "/var/run"             ///< PID Directory
#endif
#ifndef BABIRLDIR
#define BABIRLDIR   "/usr/babirl/"         ///< Install directory
#endif
#define EFLIST      "init/eflist.rc"       ///< Path of EF list file
#define HDLIST      "init/hdlist.rc"       ///< Path of HD list file
#define MTLIST      "init/mtlist.rc"       ///< Path of MT list file
#define BABIESRC    "init/babies.rc"       ///< Path of rc file for babies
#define BABILDRC    "init/babild.rc"       ///< Path of rc file for babild
#define BABILDRCXML "init/babildrc.xml"    ///< Path of rc file for babild
#define BABIAURC    "init/babiau.rc"       ///< Path of rc file for babiau
#define CLIHOSTSRC  "init/clihosts.rc"     ///< Path of clihost list file
#define TCPCLIHOSTSRC "init/tcpclihosts.rc"   ///< Path of TCP clihost list file
#define AUCLINFORC  "init/auclinfo.rc"     ///< Path of clihost list file
#define SSMINFO     "init/ssminfo.rc"      ///< Path of ssm information
#define SCRLIVERC   "init/scrlive.rc"      ///< Path of scr live information
#define STATCOMRC   "init/statcom.rc"      ///< Path of statcom
#define DBCONRC     "init/dbcon.rc"        ///< Path of DB connection

#define BABINFORCXML "init/babinfo.xml"   ///< Path of rc-xml for babinfo

#define PROMPT      "BABICON>"             ///< Prompt of babicon

/* mtab file */
#define MTABFAILE   "/etc/mtab"
