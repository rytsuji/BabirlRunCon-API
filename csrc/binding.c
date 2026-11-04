/* Small C ABI around the original C command handlers. */
#include <stdarg.h>
#include <setjmp.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <arpa/inet.h>
#include <time.h>
static void emit_start_error(const char *detail);
#include "client.c"

static int checked_es_config(char *arg);
static void emit_start_error(const char *detail){
  const unsigned char *p=(const unsigned char *)detail;
  /* Stream the bounded wire message, avoiding both the CLI stack buffer and
   * the upstream JSON string formatter's 1024-byte truncation. */
  bbjson_charobj("error",NULL);
  printf("\"Can't start, because ");
  for(;*p;p++){
    if(*p=='"' || *p=='\\')printf("\\%c",*p);
    else if(*p<0x20)printf("\\u%04x",(unsigned int)*p);
    else printf("%c",*p);
  }
  printf("\"");
}
static int checked_es_connect(char *arg);
static int checked_stop(char *arg);
static int command_deadline_seconds = 30;
static int cancel_wait = 0;
static int long_operation = 0;
__attribute__((visibility("default"))) void babi_cancel_wait(void){
  __atomic_store_n(&cancel_wait,1,__ATOMIC_SEQ_CST);
}
static char stop_diagnostic[sizeof(buff)];
static char es_context[512];
static char transport_diagnostic[sizeof(buff)];

static pthread_mutex_t api_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *capture;
static jmp_buf escape;
static const char *failure;
static int live_socket = -1;
static unsigned short target_ebport = EBCOMPORT;
static int es_base = ESCOMPORT;
unsigned short babi_eb_port(void){ return target_ebport; }
static struct timespec began;
static void check_deadline(void){
  struct timespec now;
  if(__atomic_load_n(&cancel_wait,__ATOMIC_SEQ_CST)) babi_abort("wait cancelled for manual restart");
  clock_gettime(CLOCK_MONOTONIC, &now);
  if(now.tv_sec - began.tv_sec >= command_deadline_seconds)
    babi_abort(*stop_diagnostic ? stop_diagnostic : "command deadline exceeded");
}

int babi_printf(const char *fmt, ...){
  int n;
  va_list ap;
  va_start(ap, fmt);
  n = vfprintf(capture ? capture : stderr, fmt, ap);
  va_end(ap);
  return n;
}
void babi_abort(const char *message){
  if(live_socket >= 0){ close(live_socket); live_socket = -1; }
  if(*stop_diagnostic) failure=stop_diagnostic;
  else if(*es_context){
    snprintf(transport_diagnostic,sizeof(transport_diagnostic),"%s: %s",es_context,message);
    failure=transport_diagnostic;
  }else failure=message;
  longjmp(escape, 1);
}

/* Return 0 on a complete response, 1 on transport failure, -1 on bad input,
 * -2 on allocation failure. *out is owned by caller: babi_free(*out).
 * Command handlers often return 0 even on success; inspect the JSON too. */
__attribute__((visibility("default"))) int babi_call_target_timed(const char *host, const char *command, const char *argument,
              int port, int es_port_base, int timeout_seconds, char **out){
  char *text = NULL, *arg;
  size_t length = 0;
  int i, result = 0;
  int (*handler)(char *) = NULL;
  if(!out) return -1;
  *out = NULL;
  if(es_port_base < 1 || es_port_base > 65535-MAXEF+1 || port < 1 || port > 65535) return -1;
  if(!host || !command || !argument || !*host || strlen(host) >= sizeof(ebhostname)
      || strlen(argument) > 79) return -1;
  /* Deliberately expose a small, reviewed subset. */
  if(!strcmp(command, "getconfig")) handler = getconfig;
  else if(!strcmp(command, "getevtnumber")) handler = getevtnumber;
  else if(!strcmp(command, "getesevtnumber")) handler = getesevtnumber;
  else if(!strcmp(command, "getesconfig")) handler = checked_es_config;
  else if(!strcmp(command, "esconnect")) handler = checked_es_connect;
  else if(!strcmp(command, "getebrate")) handler = getebrate;
  else if(!strcmp(command, "setrunnumber")) handler = setrunnumber;
  else if(!strcmp(command, "setrunname")) handler = setrunname;
  else if(!strcmp(command, "wth")) handler = wth;
  else if(!strcmp(command, "start")) handler = start;
  else if(!strcmp(command, "nssta")) handler = nssta;
  else if(!strcmp(command, "stop")) handler = checked_stop;
  else return -1;
  if((handler == setrunnumber || handler == setrunname) &&
      (!*argument || strpbrk(argument, " \t\r\n"))) return -1;
  if(handler == setrunnumber){
    char *end;
    long number;
    errno = 0; number = strtol(argument, &end, 0);
    if(errno || *end || number < 0 || number > 2147483647L) return -1;
  }
  if(handler == checked_es_config || handler == checked_es_connect){
    char *end;
    long id;
    if(!*argument) return -1;
    errno = 0; id = strtol(argument, &end, 10);
    if(errno || *end || id < 0 || id >= MAXEF) return -1;
  }
  arg = strdup(argument); /* striparg modifies its input. */
  if(!arg) return -2;
  pthread_mutex_lock(&api_lock);
  target_ebport = (unsigned short)port;
  es_base = es_port_base;
  capture = open_memstream(&text, &length);
  if(!capture){ pthread_mutex_unlock(&api_lock); free(arg); return -2; }
  clock_gettime(CLOCK_MONOTONIC, &began);
  command_deadline_seconds = timeout_seconds > 0 ? timeout_seconds : 300;
  long_operation = handler==checked_stop || handler==start || handler==nssta;
  __atomic_store_n(&cancel_wait,0,__ATOMIC_SEQ_CST);
  failure = NULL;
  memset(stop_diagnostic, 0, sizeof(stop_diagnostic));
  es_context[0]=0;
  live_socket = -1;
  strncpy(ebhostname, host, sizeof(ebhostname)-1);
  /* Do not call init_hcom(): it installs process-wide readline hooks. */
  for(i=0;i<MAXSCRANA;i++) scr[i] = NULL;
  bbjson_begin();
  if(setjmp(escape) == 0){
    /* Event reads do not require the complete Run/SSM/HD configuration. */
    if(handler == getevtnumber || handler == checked_stop){
      /* EB_GET_EVTN is sufficient. */
    }else if(handler == getesevtnumber){
      int fd = ebcon(ebhostname);
      eb_get(fd, EB_GET_EFLIST, (char *)daqinfo.eflist);
      close(fd);
    }else{
      /* Keep the original preparation for configuration and mutation calls. */
      get_runinfo();
      getconfig("noshow");
    }
    handler(arg);
  }else{
    result = 1;
    fclose(capture); free(text); text = NULL; length = 0;
    capture = open_memstream(&text, &length);
    if(!capture){
      pthread_mutex_unlock(&api_lock); free(arg); return -2;
    }
    bbjson_begin();
    bbjson_charobj("error", (char *)failure);
  }
  bbjson_end();
  if(fclose(capture) != 0){ free(text); text = NULL; result = -2; }
  capture = NULL;
  pthread_mutex_unlock(&api_lock);
  free(arg);
  *out = text;
  return result;
}

/* Stop must be sent before any state/config read, including during partial failure. */
static int checked_stop(char *arg){
  int fd, ret;
  char ender[80] = {0};
  fd = ebcon(ebhostname);
  eb_get(fd, EB_RUN_STOP, (char *)&ret);
  close(fd);
  /* Repeated Stop while already IDLE is benign; retain all other diagnostics. */
  if(!ret && !strcmp(stop_diagnostic+strspn(stop_diagnostic," "),"runstatus=IDLE"))
    stop_diagnostic[0]=0;
  /* A repeated Stop can return zero while the EB is already waiting for close. */
  do{
    get_runinfo();
    if(runinfo.runstat == STAT_RUN_IDLE || runinfo.runstat == STAT_RUN_WAITSTOP) break;
    check_deadline();
    usleep(10000);
  }while(1);
  if(runinfo.runstat == STAT_RUN_WAITSTOP){
    strncpy(ender, *arg ? arg : "no ender", sizeof(ender)-1);
    fd = ebcon(ebhostname);
    ret = eb_set(fd, EB_SET_ENDER, ender, sizeof(ender));
    close(fd);
    if(!ret) babi_abort("Stop ender rejected");
    fd = ebcon(ebhostname);
    eb_get(fd, EB_RUN_CLOSE, (char *)&ret);
    close(fd);
    if(!ret) babi_abort("Stop close rejected");
  }
  get_runinfo();
  /* runname belongs to getconfig, which was deliberately not read on this path. */
  if(*stop_diagnostic) babi_abort(stop_diagnostic);
  bbjson_begin_obj("runinfo");
  bbjson_intobj("runnumber", runinfo.runnumber);
  bbjson_charobj("runstatus", runinfo.runstat == STAT_RUN_IDLE ? "IDLE" : "UNKNOWN");
  bbjson_charobj("header", runinfo.header);
  bbjson_charobj("ender", runinfo.ender);
  bbjson_end_obj();
  return 1;
}
__attribute__((visibility("default"))) int babi_call_target(const char *host,const char *command,const char *argument,
              int port,int es_port_base,char **out){
  return babi_call_target_timed(host,command,argument,port,es_port_base,300,out);
}
__attribute__((visibility("default"))) int babi_call_at(const char *host,
              const char *command, const char *argument, int port, char **out){
  return babi_call_target(host, command, argument, port, ESCOMPORT, out);
}
/* Preserve the original C ABI and production default port. */
__attribute__((visibility("default"))) int babi_call(const char *host,
              const char *command, const char *argument, char **out){
  return babi_call_at(host, command, argument, EBCOMPORT, out);
}
__attribute__((visibility("default"))) void babi_free(void *p){ free(p); }

/* Checked transport for the exposed EB command subset. Fixed 5 s per I/O;
 * IPv4 numeric addresses avoid potentially blocking resolver calls. */
int mktcpsend(char *host, unsigned short port){
  int fd, err = 0, flags;
  socklen_t n = sizeof(err);
  struct sockaddr_in addr;
  struct addrinfo hints, *resolved = NULL;
  struct pollfd p;
  struct timeval tv = {5, 0};
  if(long_operation)tv.tv_sec=1;
  live_socket = -1;
  check_deadline();
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET; addr.sin_port = htons(port);
  if(inet_pton(AF_INET, host, &addr.sin_addr) != 1){
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, NULL, &hints, &resolved) != 0 || !resolved)
      babi_abort("cannot resolve EF IPv4 hostname");
    addr.sin_addr = ((struct sockaddr_in *)resolved->ai_addr)->sin_addr;
    freeaddrinfo(resolved);
  }
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0) babi_abort("socket failed");
  live_socket = fd;
#ifdef SO_NOSIGPIPE
  err = 1; setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &err, sizeof(err));
#endif
  flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    babi_abort("fcntl failed");
  if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    if(errno != EINPROGRESS) babi_abort("connect failed");
    p.fd = fd; p.events = POLLOUT;
    if(poll(&p, 1, 5000) <= 0 ||
       getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &n) < 0 || err)
      babi_abort("connect failed or timed out");
  }
  if(fcntl(fd, F_SETFL, flags) < 0 ||
     setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
     setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
    babi_abort("socket options failed");
  return fd;
}
static void transfer(int fd, void *data, size_t size, int writing){
  char *p = data;
  ssize_t n;
  int sf = 0;
#ifdef MSG_NOSIGNAL
  sf = MSG_NOSIGNAL;
#endif
  while(size){
    check_deadline();
    n = writing ? send(fd, p, size, sf) : recv(fd, p, size, 0);
    if(n < 0 && errno == EINTR) continue;
    if(n < 0 && long_operation && (errno==EAGAIN || errno==EWOULDBLOCK))continue;
    if(n <= 0) babi_abort("socket closed or I/O timed out");
    p += n; size -= n;
  }
}
int eb_get(int fd, int command, char *dest){
  int len = sizeof(command), expected = 0, variable = 0;
  switch(command){
  case EB_GET_RUNINFO: expected = sizeof(runinfo); break;
  case EB_GET_DAQINFO: expected = sizeof(daqinfo); break;
  case EB_GET_EFLIST: expected = sizeof(daqinfo.eflist); break;
  case EB_GET_SSMINFO: expected = sizeof(ssminfo); break;
  case EB_GET_TGIG: expected = sizeof(int); break;
  case EB_GET_EVTN: expected = sizeof(unsigned int); break;
  case EB_GET_TOTSIZE: expected = sizeof(totsize); break;
  case EB_RUN_CLOSE: expected = sizeof(int); break;
  case EB_RUN_STOP: expected = sizeof(buff)-1; variable = 2; break;
  case EB_RUN_START: case EB_RUN_NSSTA:
    expected = sizeof(buff)-1; variable = 1; break;
  default: babi_abort("unsupported EB read");
  }
  transfer(fd, &len, sizeof(len), 1);
  transfer(fd, &command, sizeof(command), 1);
  transfer(fd, &len, sizeof(len), 0);
  if((!variable && len != expected) ||
     (variable && (len < sizeof(int) || len > expected)))
    babi_abort("unexpected reply size: check babirl ABI/version");
  if(variable == 2){
    /* Stop has an integer plus an optional diagnostic. The caller only has
     * space for the integer, so receive the bounded tail separately. */
    transfer(fd, dest, sizeof(int), 0);
    if(len > sizeof(int)) transfer(fd, stop_diagnostic, len-sizeof(int), 0);
    return len;
  }
  memset(dest, 0, variable ? sizeof(buff) : expected);
  transfer(fd, dest, len, 0);
  return len;
}
int eb_set(int fd, int command, char *src, int size){
  int len = sizeof(command) + size, ret;
  transfer(fd, &len, sizeof(len), 1);
  transfer(fd, &command, sizeof(command), 1);
  transfer(fd, src, size, 1);
  transfer(fd, &len, sizeof(len), 0);
  if(len != sizeof(ret)) babi_abort("invalid EB set reply size");
  transfer(fd, &ret, sizeof(ret), 0);
  return ret;
}


/* ES may return multiple words (e.g. a nested babild event list).
 * Keep all words, including signed structural markers, and expose the first
 * word as the unsigned count. Never receive variable data into one integer. */
int babi_get_es_events(void){
  int i, j, fd, command, len;
  int words[1024];
  char efhost[sizeof(daqinfo.eflist[0].host)+1];
  bbjson_begin_array("lastevtn");
  for(i=0; i<MAXEF; i++){
    if(!daqinfo.eflist[i].ex || daqinfo.eflist[i].of != EB_EFLIST_ON) continue;
    memcpy(efhost, daqinfo.eflist[i].host, sizeof(efhost)-1);
    efhost[sizeof(efhost)-1] = 0;
    /* Use the configured ES base, independently of the EB port override. */
    snprintf(es_context,sizeof(es_context),"Can't communicate with ES host=%s efn=%d",efhost,i);
    fd = mktcpsend(efhost, (unsigned short)(es_base+i));
    command = ES_GET_EVTN; len = sizeof(command);
    transfer(fd, &len, sizeof(len), 1);
    transfer(fd, &command, sizeof(command), 1);
    transfer(fd, &len, sizeof(len), 0);
    if(len < sizeof(int) || len > sizeof(words) || len % sizeof(int))
      babi_abort("invalid ES event reply size");
    transfer(fd, words, len, 0);
    close(fd); live_socket = -1;
    bbjson_begin_obj(NULL);
    bbjson_intobj("efn", i);
    bbjson_charobj("host", efhost);
    bbjson_charobj("name", daqinfo.eflist[i].name);
    bbjson_uintobj("evtn", (unsigned int)words[0]);
    bbjson_begin_array("raw_words");
    for(j=0; j<len/(int)sizeof(int); j++) bbjson_intobj(NULL, words[j]);
    bbjson_end_array();
    bbjson_end_obj();
  }
  bbjson_end_array();
  return 1;
}

/* Checked ES configuration reads. Never use the CLI's unchecked recv(). */
static int es_socket(char *arg){
  int id = atoi(arg);
  char host[sizeof(daqinfo.eflist[0].host)+1];
  if(id < 0 || id >= MAXEF || !daqinfo.eflist[id].ex)
    babi_abort("ES is not registered");
  memcpy(host, daqinfo.eflist[id].host, sizeof(host)-1);
  host[sizeof(host)-1] = 0;
  snprintf(es_context,sizeof(es_context),"Can't communicate with ES host=%s efn=%d",host,id);
  return mktcpsend(host, (unsigned short)(es_base+id));
}
static int checked_es_config(char *arg){
  int fd = es_socket(arg), command = ES_GET_CONFIG, len = sizeof(command);
  struct stefrc config;
  transfer(fd, &len, sizeof(len), 1);
  transfer(fd, &command, sizeof(command), 1);
  transfer(fd, &len, sizeof(len), 0);
  if(len != sizeof(config)) babi_abort("invalid ES configuration reply size");
  transfer(fd, &config, sizeof(config), 0);
  close(fd); live_socket = -1;
  config.erhost[sizeof(config.erhost)-1] = 0;
  config.hd1dir[sizeof(config.hd1dir)-1] = 0;
  config.hd2dir[sizeof(config.hd2dir)-1] = 0;
  config.mtdir[sizeof(config.mtdir)-1] = 0;
  show_efrc(config);
  return 1;
}
static int checked_es_connect(char *arg){
  int fd, command = ES_CON_EFR, len = sizeof(command), id = atoi(arg);
  if(runinfo.runstat != STAT_RUN_IDLE)
    babi_abort("ES connect requires verified IDLE; request was not sent");
  if(daqinfo.eflist[id].of < EB_EFLIST_ON || daqinfo.eflist[id].of > EB_EFLIST_PAS)
    babi_abort("ES is disabled; connect request was not sent");
  fd = es_socket(arg);
  transfer(fd, &len, sizeof(len), 1);
  transfer(fd, &command, sizeof(command), 1);
  close(fd); live_socket = -1;
  bbjson_intobj("efn", id);
  bbjson_intobj("connect_requested", 1);
  return 1;
}

/* Validate Python's native struct sizes against the headers used to build us. */
__attribute__((visibility("default"))) int babi_webutil_layout(int kind){
  switch(kind){
    case 1: return sizeof(struct steflist);
    case 2: return sizeof(struct sthdlist);
    case 3: return sizeof(struct stssminfo);
    case 4: return sizeof(struct stefrc);
    case 5: return MAXEF;
    case 6: return MAXHD;
    default: return -1;
  }
}
