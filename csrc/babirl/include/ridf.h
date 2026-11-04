/* ridf.h
 * 
 * last modified : 12/02/21 19:21:29 
 *
 * Header file for RIDF
 * RIDF = RIKEN RIBF Data Format
 *
 * Hidetada Baba
 * baba@ribf.riken.jp
 *
 */


/*
 Header Rule

   2bit      2bit     6bit       22bit
| Reserve | Layer | Class ID | Block Size |
|                  EFN                    |
                  32bit
*/


/* Macros */
#define RIDF_MKHD1(a,b,c) ((a&0x3)<<28|(b&0x3f)<<22|(c&0xffffff))
#define RIBF_MKHD2(d)      (d & 0xffffffff)
#define RIDF_LY(x) ((x & 0x30000000) >> 28) // Layer
#define RIDF_CI(x) ((x & 0x0fc00000) >> 22) // Class ID
#define RIDF_SZ(x)  (x & 0x003fffff)        // Block Size
#define RIDF_EF(x)  (x & 0xffffffff)        // Event Fragment Number


/* Bits */
#define RIDF_LY0               0
#define RIDF_LY1               1
#define RIDF_LY2               2
#define RIDF_LY3               3


/* Class ID */
#define RIDF_EF_BLOCK          0
#define RIDF_EA_BLOCK          1
#define RIDF_EAEF_BLOCK        2
#define RIDF_EAF_BLOCK         2
#define RIDF_EVENT             3
#define RIDF_SEGMENT           4
#define RIDF_COMMENT           5
#define RIDF_EVENT_TS          6
#define RIDF_BLOCK_NUMBER      8
#define RIDF_END_BLOCK         9
#define RIDF_SCALER            11
#define RIDF_NCSCALER          11
#define RIDF_CSCALER           12
#define RIDF_NCSCALER32        13
#define RIDF_TIMESTAMP         16
#define RIDF_STATUS            21

/* Comment ID */
#define RIDF_COMMENT_TEXT        0
#define RIDF_COMMENT_RUNINFO_ASC 1
#define RIDF_COMMENT_EFINFO_ASC  2
#define RIDF_COMMENT_DAQINFO_BIN 3
#define RIDF_COMMENT_RUNINFO_BIN 4

/* Comment ID */
#define RIDF_STATUS_START_XML    11
#define RIDF_STATUS_STOP_XML     12
#define RIDF_STATUS_DAQRUNINFO   13

/* Size */
#define RIDF_COMMENT_RUNINFO_ASC_SIZE   1024

/** RIDF readable header */
struct ridf_rhdst{
  int layer;
  int classid;
  int blksize;
  int efn;
};

/** RIDF header common structure */
struct ridf_hdst{
  int hd1; ///< First 2 words for layer, classid, blksize
  int hd2; ///< Second 2 words for efn
};

/** RIDF end of block structure */
struct ridf_hdst_eob{
  struct ridf_hdst chd;  ///< Common header
  unsigned int size; ///< Size of this block
};

/** RIDF end of block structure */
struct ridf_hdst_blkn{
  struct ridf_hdst chd;  ///< Common header
  unsigned int blkn; ///< Size of this block
};

/** RIDF Event header structure */
struct ridf_hdst_evt{
  struct ridf_hdst chd;  ///< Common header
  unsigned int evtn; ///< Event number
};

/** RIDF Event header with Time Stamp structure */
struct ridf_hdst_evtts{
  struct ridf_hdst chd;  ///< Common header
  unsigned int evtn; ///< Event number
  int tsl; ///< Lower bit of Time Stamp
  int tsu; ///< Upper bit of Time Stamp
};

/** RIDF Segment header structure */
struct ridf_hdst_seg{
  struct ridf_hdst chd;  ///< Common header
  unsigned int segid;  ///< Segment ID
};

/** RIDF Segment header structure
 *  alias = RIDFHDSCR
 */
struct ridf_hdst_scr{
  struct ridf_hdst chd;  ///< Common header
  int date;             ///< Date (seconds from year 1970)
  unsigned int scrid;  ///< Scaler ID
};

struct ridf_hdst_com{
  struct ridf_hdst chd;  ///< Common header
  int date;             ///< Date (seconds from year 1970)
  unsigned int comid;  ///< Comment ID
};


/** RIDF comment header structure */
struct ridf_comment_runinfost{
  char runname[100];
  char runnumber[100];
  char starttime[20];
  char stoptime[20];
  char date [20];
  char rev[40];
  char header[100];
  char ender[100];
};


/** RIDF scalar analysys structure */
struct ridf_scr_anast{
  int classid; // 12=non clear, 13=clear scaler
  int scrid;
  int ratech;
  int rate;
  int update;
  int scrn;
  char idname[80];
};

/** RIDF scaler container structure */
struct ridf_scr_contst{
  unsigned int cur;
  unsigned int dummy;
  unsigned long long int tot;
  char name[80];
};


/** RIDF scaler live time infomation */
struct ridf_scr_livest{
  int gatedid;
  int gatedch;
  int ungatedid;
  int ungatedch;
};

/* Typedefs */
typedef struct ridf_rhdst RIDFRHD;
typedef struct ridf_hdst RIDFHD;
typedef struct ridf_hdst_eob RIDFHDEOB;
typedef struct ridf_hdst_blkn RIDFHDBLKN;
typedef struct ridf_hdst_evt RIDFHDEVT;
typedef struct ridf_hdst_evtts RIDFHDEVTTS;
typedef struct ridf_hdst_seg RIDFHDSEG;
typedef struct ridf_hdst_scr RIDFHDSCR;
typedef struct ridf_hdst_com RIDFHDCOM;


/* Prototypes */
int ridf_ly(struct ridf_hdst);
int ridf_ci(struct ridf_hdst);
int ridf_sz(struct ridf_hdst);
int ridf_ef(struct ridf_hdst);
struct ridf_rhdst ridf_dechd(struct ridf_hdst);
struct ridf_hdst ridf_mkhd(int, int, int, int);
struct ridf_hdst_eob ridf_mkhd_eob(int, int, int, int, int);
struct ridf_hdst_blkn ridf_mkhd_blkn(int, int, int, int, int);
struct ridf_hdst_evt ridf_mkhd_evt(int, int, int, int, int);
struct ridf_hdst_evtts ridf_mkhd_evtts(int, int, int, int, int, long long int);
struct ridf_hdst_seg ridf_mkhd_seg(int, int, int, int, int);
struct ridf_hdst_scr ridf_mkhd_scr(int, int, int, int, int, int);
struct ridf_hdst_com ridf_mkhd_com(int, int, int, int, int, int);
