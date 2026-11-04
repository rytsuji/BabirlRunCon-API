/* prototype */
/* bi-tcp */
int mktcpsock(unsigned short);
int mktcpsend(char *, unsigned short);
int mktcpsend_tout(char*, unsigned short, int);
int mktcpsend_noblk(char*, unsigned short);
int mkudpsock();
int mkudpsend();
int mkmultisend();
int registmultisend();
int getmyaddress(char *host, char *domain, char *ipaddr);
int getdefinterface(char *ifname);
int getmacbysock(int sock, char *mac);
int getipbysock(int sock, char *ip);
void getipfromsockaddr();
int mkbroadsend();

// to avoid warnings
//int mkbroadsend(int port, struct sockaddr_in *caddr);
//void getipfromsockaddr(struct sockaddr_in caddr, unsigned char *ip);

/* bi-shm */
void sem_p(); // void sem_p(int semid, struct sembuf *semb)
void sem_v(); // void sem_v(int semid, struct sembuf *semb)
int initshm();      // int initshm(int key, int size, char *addr)
int initsem();      // int initsem(int key, union semun *semunion)
int delshm();
int delsem();

int get_terminfo(int *, int *);
int execute_line();
int execute_line_noerr();
char *stripwhite(char *string);
int tab2space(char *buff);
int eb_get_efnum(int sock, int *n);
int eb_set_eflist(int sock, char *eflist, int efsize);
int eb_get_eflist(int sock, char *eflist);
int eb_get(int sock, int com, char *dest);
int eb_set(int sock, int com, char *src, int size);
int init_hcom();
char *now_time(void);

/* bi-mem */
struct stbrstat{
  int n;
  int max;
  unsigned int blkn;
  struct stbrbuff *first;
  struct stbrbuff *last;
};

struct stbrbuff{
  char *data;
  int size;
  unsigned int blkn;
  struct stbrbuff *next;
};

struct evtmem *newevt(char *, int, int, unsigned int, int*);
int showevtlist();
int chkefdata();
int cleanfirst();
int chkevtsize();
int mkef();
int mkefts();
int cleanall();
struct nevtmem *newnevt(char *, int);
int ncleanfirst();
int ncleanall();
int storenevt(char *);
int getmemn(void);
unsigned int getebn(void);
unsigned int getfirstevtn(void);
unsigned int getlastevtn(void);
struct tsmem *newts(unsigned int, long long int, int);
struct tsmem *updatets(unsigned int, long long int, int);
int tscleanfirst();
int tscleanall();
long long int chkts(unsigned int, int);
int addrbuff(struct stbrstat *st, char *data, int sz);
int getrbuff(struct stbrstat *st, char *data, int *sz, unsigned int blkn);

int striparg();


/* bi-file */
int isdir(char *path);
int isfile(char *path);
int lfprintf(FILE *, const char *, ...);
int gethdst();
int openoutfile();
int addfdbyfile(char *, int, int, int);
int getmaxfd(void);
void cleanfd();
struct fdinfost *getfdfirst(void);
struct fdinfost *getisfd();
int getifd();
int setfdset();
int closefd();
char *filenamefromflt(int, char *);
void splitrunname(char *, char *, int *, int);

/* bi-pid */
int mkpid(char *name);
int rmpid(char *name);
int chkpid(char *name);
int killpid(char *name);
int pidof(char *name, int *pids, int *n);
int chkpidof(char *name);
int killpidof(char *name);
int dirtoname(char *path, char *name);


/* bi-sim */
double rnd(void);
int ne(double, double, double);
void init_nt(void);
double nt(double p);


/* bi-log */
void initlog(void);
char *getlog(void);
int getlogsize(void);
int storelog(char *s);
