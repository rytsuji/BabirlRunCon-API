/* Hdur: babicon/babicon.h
 * last modified : 15/02/18 10:38:13 
 * Hidetada Baba (RIKEN)
 * baba@ribf.riken.jp
 *
 * Header for DAQ Manager/Configurator
 *
 */

/* Global */
int ebsock;
static char buff[EB_BUFF_SIZE];
//char *com;
int row,col, nofef;
unsigned int evtn[MAXEF];
int evtnbuff[1024];
int evtnidx = 0;
unsigned long long int totsize;
char ebhostname[128];
struct stclinfo clinfo[MAXCLI];
struct sttcpclinfo tcpclinfo[MAXTCPCLI];
int auclinfo[MAXEF];
struct ridf_scr_livest scrlive;
struct ststatcom statcom;
struct stdbcon dbcon;
void babinfoxcom(char *com);

/* Prototype */
void quit(void);
int start();
int nssta();
int stop();
int pstart();
int pnssta();
int pstop();
int setrunnumber();
int setrunname();
int setebsize();
int setconfig();
int getconfig();
int sethdlist();
int seteflist();
int getesconfig();
int setesconfig();
int reloadesdrv();
int esquit();
int esconnect();
int esdisconnect();
int setclinfo();
int getclinfo();
int settcpclinfo();
int gettcpclinfo();
int setauclinfo();
int getauclinfo();
int help();
int exec_sh();
int exec_psh();
int exec_rsh();
int exec_nrsh();
int exec_nsh();
int mokill();
int synopsis();
int wth();
int shutdownall();
int setssminfo();
int getevtn();
int getscrlist();
int setscrname();
int setscrlive();
int getscrlive();
int getevtnumber();
int getesevtnumber();
int getebrate();
int delscr();
int setbabildes();
int get_runinfo();
int whoareyou();
int setstatcom();
int getstatcom();
int setdbcon();
int getdbcon();
int chkdbcon();
int dbgetexpid(char *arg);
int dbsetexpid(char *arg);
int dbgetscr(char *arg);
int dbsetscr(char *arg);
int dbgetdaqname(char *arg);
int dbsetdaqname(char *arg);
int setchkerhost();
int getchkerhost();
int settgig();
int gettgig();
int getefblkn();

void show_runinfo(void);
void show_daqinfo(void);
void show_statcom(void);
void show_dbcon(void);
void show_clinfo(void);
void show_tcpclinfo(void);


/** command structure */
typedef struct {
  char *name;           ///< Command name
  rl_icpfunc_t *func;   ///< Pointer of function
  char *doc;            ///< Document
} COMMAND;

COMMAND commands[]={
     {"exit",(rl_icpfunc_t *)quit,
      "Exit babicon"},
     {"help",(rl_icpfunc_t *)help,
      "Help"},
     {"start",(rl_icpfunc_t *)start,
      "Start DAQ with save mode"},
     {"nssta",(rl_icpfunc_t *)nssta,
      "Start DAQ with no save mode"},
     {"stop",(rl_icpfunc_t *)stop,
      "Stop DAQ"},
     {"pstart",(rl_icpfunc_t *)pstart,
      "Start DAQ with save mode, with pre command"},
     {"pnssta",(rl_icpfunc_t *)pnssta,
      "Start DAQ with no save mode, with pre command"},
     {"pstop",(rl_icpfunc_t *)pstop,
      "Stop DAQ, with pre command"},
     {"setrunnumber",(rl_icpfunc_t *)setrunnumber,
      "Set run number to Number"},
     {"setrunname", (rl_icpfunc_t *)setrunname,
      "Set run name"},
     {"setebsize", (rl_icpfunc_t *)setebsize,
      "Set event build size"},
     {"setbabildes", (rl_icpfunc_t *)setbabildes,
      "Set babildes mode"},
     {"setchkerhost", (rl_icpfunc_t *)setchkerhost,
      "Set checking event receiver's host"},
     {"getchkerhost", (rl_icpfunc_t *)getchkerhost,
      "Get checking event receiver's host"},
     {"settgig", (rl_icpfunc_t *)settgig,
      "Set trim mode"},
     {"gettgig", (rl_icpfunc_t *)gettgig,
      "Get trim mode"},
     {"getefblkn", (rl_icpfunc_t *)getefblkn,
      "Get EF block number"},
     //{"setconfig",(rl_icpfunc_t *)setconfig,
     // "Set DAQ configuration"},
     {"getconfig",(rl_icpfunc_t *)getconfig,
      "Get DAQ configuration"},
     {"sethdlist",(rl_icpfunc_t *)sethdlist,
      "Set HD list"},
     {"seteflist",(rl_icpfunc_t *)seteflist,
      "Set EF list"},
     {"getesconfig", (rl_icpfunc_t *)getesconfig,
      "Get EFS Configuration"},
     {"setesconfig", (rl_icpfunc_t *)setesconfig,
      "Set EFS Configuration"},
     {"reloadesdrv", (rl_icpfunc_t *)reloadesdrv,
      "Reload ES device driver (Linux mode only)"},
     {"setssminfo", (rl_icpfunc_t *)setssminfo,
      "Set SSM information"},
     {"setstatcom", (rl_icpfunc_t *)setstatcom,
      "Set Start/Stop status command"},
     {"getstatcom", (rl_icpfunc_t *)getstatcom,
      "Get Start/Stop status command"},
     {"setdbcon", (rl_icpfunc_t *)setdbcon,
      "Set DB connection"},
     {"getdbcon", (rl_icpfunc_t *)getdbcon,
      "Get DB connection"},
     {"chkdbcon", (rl_icpfunc_t *)chkdbcon,
      "Check DB connection"},
     {"esconnect", (rl_icpfunc_t *)esconnect,
      "Make connection EFS to EFR"},
     {"esdisconnect", (rl_icpfunc_t *)esdisconnect,
      "Disconnect EFS to EFR"},
     {"esquit", (rl_icpfunc_t *)esquit,
      "Quit ES"},
     {"wth", (rl_icpfunc_t *)wth, "Write header"},
     {"setclinfo", (rl_icpfunc_t *)setclinfo,
      "Set UDP analysis client list"},
     {"getclinfo", (rl_icpfunc_t *)getclinfo,
      "Get UDP analysis client list"},
     {"settcpclinfo", (rl_icpfunc_t *)settcpclinfo,
      "Add or Delete TCP data client"},
     {"gettcpclinfo", (rl_icpfunc_t *)gettcpclinfo,
      "Get TCP data client list"},
     {"setauclinfo", (rl_icpfunc_t *)setauclinfo,
      "Define UDP analysis client list for AUEB"},
     {"getauclinfo", (rl_icpfunc_t *)getauclinfo,
      "Get definition of UDP analysis client list for AUEB"},
     {"getscrlist", (rl_icpfunc_t *)getscrlist,
      "Get scaler list"},
     {"setscrname", (rl_icpfunc_t *)setscrname,
      "Set scaler name"},
     {"getscrlive", (rl_icpfunc_t *)getscrlive,
      "Get live time information"},
     {"setscrlive", (rl_icpfunc_t *)setscrlive,
      "Set live time information"},
     {"delscr", (rl_icpfunc_t *)delscr,
      "Delete scaler definition"},
     {"getevtnumber", (rl_icpfunc_t *)getevtnumber,
      "Get event built number"},
     {"getesevtnumber", (rl_icpfunc_t *)getesevtnumber,
      "Get es event number"},
     {"getebrate", (rl_icpfunc_t *)getebrate,
      "Get rate of event built size"},
     {"dbsetexpid", (rl_icpfunc_t *)dbsetexpid,
      "Set ExpId for Database"},
     {"dbgetexpid", (rl_icpfunc_t *)dbgetexpid,
      "Get ExpId for Database"},
     {"dbgetscr", (rl_icpfunc_t *)dbgetscr,
      "Get Scaler information from DB"},
     {"dbsetscr", (rl_icpfunc_t *)dbsetscr,
      "Set Scaler information to DB"},
     {"dbsetdaqname", (rl_icpfunc_t *)dbsetdaqname,
      "Set DAQ Name for Database"},
     {"dbgetdaqname", (rl_icpfunc_t *)dbgetdaqname,
      "Get DAQ Name for Database"},
     {"whoareyou", (rl_icpfunc_t *)whoareyou,
      "Get EF process name"},
     {"shutdown", (rl_icpfunc_t *)shutdownall,
      "Shutdown babild and babies"},
     {"sh",(rl_icpfunc_t *)exec_sh,
      "Execute shell command"},
     {"psh",(rl_icpfunc_t *)exec_psh,
      "Execute shell command (returns OK or Error)"},
     {"rsh",(rl_icpfunc_t *)exec_rsh,
      "Execute remote shell command with return"},
     {"nrsh",(rl_icpfunc_t *)exec_nrsh,
      "Execute remote shell command without any return"},
     {"kill",(rl_icpfunc_t *)mokill,
      "Kill babirl process via babimo"},
     {(char *)NULL,(rl_icpfunc_t *)NULL,(char *)NULL}
};


/* Command for eflist */
char *comseteflist[] = {"add", " HOSTNAME NICKNAME", "Add a definition of EF",
			"del", "", "Delete a defitnion of EF",
			"host","", "Change hostname",
			"name","", "Change nickname",
			"on", "","EF data participate in event build",
			"off", "", "Unregister EF data from event build",
			"scr", "", "Regist scr type EF data in event build",
			"pas", "", "Regist pas type EF data in event build",
			NULL};

char *comsethdlist[] = {"on", "", "HDD on",
			"off","",  "HDD off",
			"del","",  "Delete a definition of HDD",
			"path", " PATH", "Add a definition of HDD",
			NULL};
char *comsetclinfo[] = {"add", " HOSTNAME", "Add a definition of UDP client",
			"del", "", "Delete a definition of UDP client",
			"id", "ID", "Set shared memory ID",
			NULL};
char *comssminfo[]   = {"on", "", "SSM on",
			"off", "", "SSM off",
			"host", " [HOSTNAME]", "Change hostname",
			"start", "" ,"Change start command",
			"stop", "", "Change stop command", 
			NULL};
char *comsetauclinfo[]  = {"add", " CliID", "Add a relation ship between EFN and UDP client ID",
			"del", "", "Delete a definition",
			NULL};
char *comsettcpclinfo[]  = {"add", "HOSTNAME PORT", "Add TCP client",
			    "del", "HOSTNAME PORT", "Delete TCP client",
			NULL};
char *comsetscrlive[] = {"gated", "ID CH", "Scaler for gated trigger",
			 "ungated", "ID CH", "Scaler for ungated trigger",
			 NULL};
char *comstatcom[]   = {"on", "", "Run status on",
			"off", "", "Run status off",
			"start", "" ,"Change start command",
			"stop", "", "Change stop command", 
			NULL};
char *comdbcon[]   = {"on", "", "DB connection on",
			"off", "", "DB connection off",
			"set", "" ,"Change parameters for DB connection",
			NULL};

char *comsetesconfig[] = {"host", "HOSTNAME", "define the EB server",
			  "rtdrv", "PATH", "define the driver to be used",
			  NULL};

/** Structure for manual */
typedef struct {
  char *name;         ///< command name
  char *synopsis;     ///< synopsis of command
  char *description;  ///< description of command
  char **command;     ///< description of local commands
} MANUAL;

MANUAL manual[]={
  {"exit", "",
   "Exit babicon", NULL},
  {"help", "[command]",
   "show help", NULL},
  {"start", "",
   "DAQ start with save mode", NULL},
  {"nssta", "",
   "DAQ start with no save mode", NULL},
  {"stop", "",
   "DAQ stop", NULL},
  {"pstart", "PATH",
   "DAQ start with save mode with pre command", NULL},
  {"pnssta", "PATH",
   "DAQ start with no save mode with pre command", NULL},
  {"pstop", "PATH",
   "DAQ stop with pre command", NULL},
  {"setrunnumber", "[RUNNUMBER]",
   "Change run number to RUNNUMBER + 1", NULL},
  {"setrunname", "[RUNNAME]",
   "Change run name to RUNNAME", NULL},
  {"setebsize", "[EBSIZE]",
   "Change event build size to EBSIZE", NULL},
  {"setbabildes", "on/off",
   "Change babildes mode", NULL},
  {"setchkerhost", "on/off",
   "Change check of event receiver's host", NULL},
  {"getchkerhost", NULL,
   "Show check of event receiver's host", NULL},
  {"settgig", "on/off",
   "Set trim mode, limit rawdata file size 1--10GB (separate multiple files, 0 is off)", NULL},
  {"gettgig", NULL,
   "Get trim mode", NULL},
  {"getefblkn", NULL,
   "Get EF block numbers", NULL},
  //{"setconfig", "[OPTION] [VALUE]",
  // "Set DAQ configurations", NULL  },
  {"getconfig", "",
   "Get DAQ configurations", NULL},
  {"sethdlist", "HDNUMBER COMMAND [VALUE]",
   "HDD on/off, if PATH is set, change HDD path", comsethdlist},
  {"seteflist", "EFN COMMAND [VALUE]",
   "Update EF list, NAME is a nickname of EF", comseteflist},
  {"getesconfig", "EFN",
   "Get babies configuration", NULL},
  {"setesconfig", "EFN COMMAND VALUE",
   "Set babies configuration", comsetesconfig},
  {"reloadesdrv", "EFN",
   "Reload ES device driver (Linux mode only)", NULL},
  {"setssminfo", "COMMAND [VALUE]",
   "Set babissm configuration", comssminfo},
  {"setstatcom", "COMMAND [VALUE]",
   "Set Run status commands", comstatcom},
  {"getstatcom", "",
   "Get Run status commands", NULL},
  {"setdbcon", "COMMAND [HOST DBNAME USER PASSWD]",
   "Set DB connection parameters", comdbcon},
  {"getdbcon", "",
   "Get DB connection parameters", NULL},
  {"chkdbcon", "",
   "Check DB connection", NULL},
  {"esconnect", "EFN",
   "Connect babies to babild (for test purpose only)", NULL},
  {"esdisconnect", "EFN",
   "Disconnect babies to babild (for test purpose only)", NULL},
  {"esquit", "EFN",
   "Quit babies", NULL},
  {"wth", "[HEADER]", "Write header", NULL},
  {"setclinfo", "CLIN COMMAND [VALUE]",
   "Set UDP client list", comsetclinfo},
  {"getclinfo", "",
   "Get UDP client list", NULL},
  {"gettcpclinfo", "",
   "Get TCP data client list", NULL},
  {"settcpclinfo", "",
   "Add or Delete TCP data client", comsettcpclinfo},
  {"setauclinfo", "EFN COMMAND [VALUE]",
   "Define UDP client list for AUEB", comsetauclinfo},
  {"getauclinfo", "",
   "Get definition of UDP client list for AUEB", NULL},
  {"getscrlist", "[SCRID]",
   "Get all scaler list or detailes of SCRID scaler", NULL},
  {"setscrname", "SCRID",
   "Set scaler name", NULL},
  {"getscrlive", "", "Get live time information", NULL},
  {"setscrlive", "COM ID CH", "Set live time information", comsetscrlive},
  {"delscr", "SCRID",
   "Delete scaler definition (not implimented yet)", NULL},
  {"getevtnumber", "", "Get event built number", NULL},
  {"getesevtnumber", "", "Get es event number", NULL},
  {"getebrate", "", "Get rate of event built size", NULL},
  {"dbsetexpid", "EXPID", "Set ExpId for Database", NULL},
  {"dbgetexpid", "", "Get ExpId for Database", NULL},
  {"dbgetscr", "", "Get Scaler infomation from Database", NULL},
  {"dbsetscr", "", "Set Scaler infomation to Database", NULL},
  {"dbsetdaqname", "DAQNAME", "Set DAQ Name for Database", NULL},
  {"dbgetdaqname", "DAQNAME", "Get DAQ Name for Database", NULL},
  {"whoareyou", "EFN", "Get EF process name", NULL},
  {"shutdown", "", "Shutdown babild and babies", NULL},
  {"sh", "COMMAND",
   "Execute shell command", NULL},
  {"psh", "COMMAND",
   "Execute shell command (returns OK or Error)", NULL},
  {"rsh", "COMMAND",
   "Execute remote shell command with return", NULL},
  {"nrsh", "COMMAND",
   "Execute remote shell command without any return", NULL},
  {"kill", "PROCESSNAME",
   "Kill babirl process such as babild, babinfo, babies", NULL},
  {(char *)NULL, (char *)NULL, (char *)NULL, NULL}
};

/* Globals */
struct stdaqinfo daqinfo;
struct struninfo runinfo;
struct stefrc    efrc;
struct stssminfo ssminfo;

int scranan;
struct ridf_scr_anast scrana[MAXSCRANA];
struct ridf_scr_contst *scr[MAXSCRANA];

char esname[64];
char ebname[64];

/* Strings */
char *runstatstr[4] =  {"IDLE\0", "START\0", "NSSTA\0", "WAITSTOP\0"};
char *ofstr[4] = {"off\0", "on\0", "scr\0", "pas\0"};
char tab[] = "     ";
