/* babicon/babicmd.c
 * last modified : 19/02/01 10:07:52 
 *
 * Hidetada Baba (RIKEN)
 * baba@ribf.riken.jp
 *
 * DAQ Command line Manager/Configurator
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <bi-config.h>
#include <bi-common.h>
#include <ridf.h>
#include <bbjson.h>
#include "babicon.h"
#include "comfuncsjson.c"

int getscrdata(int);

int chkex(int ex, char *mes){
  //printf("ex %d\n", ex);
  
  if(ex){
    return 1;
  }else{
    //printf("%s %d is not exist\n", mes, ex);
    return 0;
  }
}

int chknex(int ex, char *mes){
  if(!ex){
    return 1;
  }else{
    //printf("%s %d is already exist\n", mes, ex);
    return 0;
  }
}

int escon(char *arg){
  int essock, efid;
  char js[256] = {0};

  efid = -1;
  essock = 0;
  if(arg == NULL){
    //printf("no EFSID\n");
    bbjson_charobj("error", "no EFSID");
    return 0;
  }
  
  efid = strtol(arg, NULL, 0);
  if(efid < 0 || efid > MAXEF){

    sprintf(js, "0 < EFSID < %d", MAXEF);
    bbjson_charobj("error", js);
    return 0;
  }
  if(!daqinfo.eflist[efid].ex){
    bbjson_charobj("error", "Invalid EFSID");
    return 0;
  }
  
  if(!(essock = mktcpsend(daqinfo.eflist[efid].host, ESCOMPORT+efid))){
    sprintf(js, "Can't connet to babies id=%d", efid);
    bbjson_charobj("error", js);
    return 0;
  }

  return essock;
}

int mocon(char *host){
  int mosock;

  /* Connect to babild */
  if(!(mosock = mktcpsend(host, BABIMOPORT))){
    bbjson_charobj("error", "Can't connet to babimo");
    return 0;
  }
  
  return mosock;
}



void quit(void){
  exit(0);
}

int mokill(char *arg){
  int sock, com, len;
  char buff[1024];

  memset(buff ,0, sizeof(buff));

  if(!strlen(arg)){
    //printf("kill babild/babinfo/babies ...\n");
    return 0;
  }

  strncpy(buff, arg, sizeof(buff)-1);
  com = MON_KILLPID;

  sock = mocon(ebhostname);

  len = sizeof(com) + strlen(buff);
  send(sock,(char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, sizeof(com), 0);
  send(sock, buff, strlen(buff), 0);

  close(sock);

  return 0;
}

int getevtnumber(char *arg){
  int sock;
  unsigned int ebn;

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_EVTN, (char *)&ebn);
  close(sock);

  /*
  if(ebn > 0){
    printf("Event built number = %d", ebn);
  }else{
    printf("No event build\n");
  }
  */
  bbjson_intobj("eventbuiltnumber", ebn);

  return 0;
}

int getebrate(char *arg){
  int sock, dt;
  double ts, rs;
  time_t now;
  char js[256] = {0};

  get_runinfo();

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_TOTSIZE, (char *)&totsize);
  time(&now);
  close(sock);

  ts = (double)totsize/1024./1024.*2.;

  if(runinfo.runstat){
    dt = (int)now - runinfo.starttime;
  }else{
    dt = runinfo.stoptime - runinfo.starttime;
  }

  if(totsize > 0){
    rs = ts / (double)dt;
    sprintf(js, "%7.2f (MB)\n", ts);
    bbjson_charobj("eventbuiltsize", js);
    sprintf(js, "%7.2f (MB/s)\n", rs);
    bbjson_charobj("eventbuiltrate", js);
  }else{
    //printf("No event build\n");
    bbjson_charobj("eventbuiltsize", "none");
    bbjson_charobj("eventbuiltrate", "none");
  }

  return 0;
}

int get_runinfo(void){
  int sock;

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_RUNINFO, (char *)&runinfo);
  close(sock);

  return 0;
}

void show_scrlist(void){
  int i, j, tn, ix;
  int st[MAXSCRANA];

  for(i=0;i<scranan;i++){
    tn = scrana[i].scrid;
    ix = 0;
    for(j=0;j<scranan;j++){
      if(i!=j && tn > scrana[j].scrid){
	ix++;
      }
    }
    st[ix] = i;
  }

  //printf("   ID SCRN : NAME\n");
  //jprintf(char *ef, char *tag, int cf, char *ch, int d, char *af)
  bbjson_begin_array("scrlist");
  for(i=0;i<scranan;i++){
    bbjson_begin_obj(0);
    bbjson_intobj("scrid", scrana[st[i]].scrid);
    bbjson_intobj("scrn", scrana[st[i]].scrn);
    bbjson_charobj("idname", scrana[st[i]].idname);
    bbjson_intobj("classid", scrana[st[i]].classid);
    bbjson_end_obj();
  }
  bbjson_end_array();

}


int show_scrdata(int id){
  int tdx, i;
  char js[256] = {0};

  if((tdx = fndscrid(id)) < 0){
    sprintf(js, "No such scaler id = %d", id);
    bbjson_charobj("error", js);
    return 0;
  }

  bbjson_begin_obj("scrdata");
  bbjson_intobj("scrid", scrana[tdx].scrid);
  bbjson_intobj("scrn", scrana[tdx].scrn);
  bbjson_charobj("scrname", scrana[tdx].idname);
  bbjson_begin_array("data");
  for(i=0;i<scrana[tdx].scrn;i++){
    bbjson_intobj("ch", i);
    bbjson_charobj("name", scr[tdx][i].name);
    bbjson_uintobj("current", scr[tdx][i].cur);
    bbjson_lluintobj("total", scr[tdx][i].tot);
  }
  bbjson_end_array();
  bbjson_end_obj();

  return 1;
}

void show_efrc(struct stefrc efrc){
  bbjson_begin_obj("efrc");
  bbjson_intobj("efn", efrc.efid);
  bbjson_intobj("erport", efrc.erport);
  bbjson_charobj("erhost", efrc.erhost);
  bbjson_intobj("hd1", efrc.hd1);
  bbjson_charobj("hd1dir", efrc.hd1dir);
  bbjson_intobj("hd2", efrc.hd2);
  bbjson_charobj("hd2dir", efrc.hd2dir);
  bbjson_intobj("mt", efrc.mt);
  bbjson_charobj("rtdrv", efrc.mtdir);
  bbjson_intobj("connect", efrc.connect);
  bbjson_end_obj();
}

void show_eflist(void){
  int i;

  bbjson_begin_array("eflist");
  for(i=0; i<MAXEF; i++){
    if(daqinfo.eflist[i].ex){
      bbjson_begin_obj(0);
      bbjson_intobj("efn", i);
      bbjson_charobj("host", daqinfo.eflist[i].host);
      bbjson_charobj("name", daqinfo.eflist[i].name);
      bbjson_charobj("onoff", ofstr[daqinfo.eflist[i].of]);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();

}

void show_mtlist(void){
  int i;

  bbjson_begin_array("mtlist");
  for(i=0; i<MAXMT; i++){
    if(daqinfo.mtlist[i].ex){
      bbjson_begin_obj(0);
      bbjson_intobj("id", i);
      bbjson_charobj("path", daqinfo.mtlist[i].path);
      bbjson_charobj("onoff", ofstr[daqinfo.mtlist[i].of]);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();
}

void show_runinfo(void){
  time_t ttime;
  char chst[80];
  struct tm *strtime;

  memset(chst, 0, sizeof(chst));

  bbjson_begin_obj("runinfo");
  bbjson_charobj("runname", daqinfo.runname);
  bbjson_intobj("runnumber", runinfo.runnumber);
  bbjson_charobj("runstatus", runstatstr[runinfo.runstat]);
  ttime = runinfo.starttime;
  strtime = localtime(&ttime);
  strftime(chst, sizeof(chst), "%d-%b-%y %X", strtime);
  bbjson_charobj("startdate", chst);
  if(!runinfo.runstat){
    ttime = runinfo.stoptime;
    strtime = localtime(&ttime);
    strftime(chst, sizeof(chst), "%d-%b-%y %X", strtime);
    bbjson_charobj("stopdate", chst);
  }
  bbjson_charobj("header", runinfo.header);
  if(!runinfo.runstat){
    bbjson_charobj("ender", runinfo.ender);
  }
  bbjson_end_obj();
  
}

void show_ebinfo(void){
  bbjson_begin_obj("ebinfo");
  bbjson_intobj("efn", daqinfo.efn);
  bbjson_intobj("ebsize", daqinfo.ebsize);
  bbjson_intobj("ebsizekb", daqinfo.ebsize*WORDSIZE/1024);
  bbjson_charobj("babildes", ofstr[daqinfo.babildes]);
  bbjson_end_obj();
}

void show_ssminfo(void){
  bbjson_begin_obj("ssminfo");
  if(ssminfo.ex){
    bbjson_charobj("host", ssminfo.host);
    bbjson_charobj("onoff", ofstr[ssminfo.of]);
    bbjson_charobj("start", ssminfo.start);
    bbjson_charobj("stop", ssminfo.stop);
  }else{
    bbjson_charobj("info", "no ssm information");
  }
  bbjson_end_obj();
}

void show_daqinfo(void){
  show_ssminfo();
  show_eflist();
  show_mtlist();
  show_hdlist();
  show_ebinfo();
  show_runinfo();
}

int start(char *arg){
  int sock, ret;
  char js[272] = {0};

  getconfig("noshow\0");

  sock = ebcon(ebhostname);
  eb_get(sock, EB_RUN_START, buff);
  close(sock);


  memcpy((char *)&ret, buff, sizeof(ret));
  if(!ret){
    if(daqinfo.babildes){
      bbjson_charobj("error", "Can't start, babild is babildes mode");
    }else{
      sprintf(js, "Can't start, because %s", buff+sizeof(ret));
      bbjson_charobj("error", js);
    }
  }else{
    get_runinfo();
    show_runinfo();
  }

  return 0;
}

int nssta(char *arg){
  int sock, ret;
  char js[272] = {0};

  getconfig("noshow\0");

  sock = ebcon(ebhostname);
  eb_get(sock, EB_RUN_NSSTA, buff);
  close(sock);

  memcpy((char *)&ret, buff, sizeof(ret));
  if(!ret){
    if(daqinfo.babildes){
      bbjson_charobj("error", "Can't start, babild is babildes mode");
    }else{
      sprintf(js, "Can't start, because %s", buff+sizeof(ret));
      bbjson_charobj("error", js);
    }
  }else{
    get_runinfo();
    show_runinfo();
  }

  return 0;
}

int stop(char *arg){
  int sock, ret;
  char tender[80]={0};

  getconfig("noshow\0");

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_RUNINFO, (char *)&runinfo);
  close(sock);

  sock = ebcon(ebhostname);
  eb_get(sock, EB_RUN_STOP, (char *)&ret);
  close(sock);
  if(!ret){
    bbjson_charobj("error", "Can't stop, now stopping or some error or babildes mode");
    return 0;
  }

  if(runinfo.runstat == STAT_RUN_START){
    memset(tender, 0, sizeof(tender));
    if(!strlen(arg)){
      sprintf(tender, "no ender");
    }else{
      strncpy(tender, arg, sizeof(tender)-1);
    }

  }

  usleep(300000);
  get_runinfo();

  while(runinfo.runstat != STAT_RUN_IDLE
	&& runinfo.runstat != STAT_RUN_WAITSTOP){
#ifdef DEBUG
    usleep(100000);
#else
    usleep(10000);
#endif
    get_runinfo();
  }
  if(runinfo.runstat == STAT_RUN_WAITSTOP){
    sock = ebcon(ebhostname);
    ret = eb_set(sock, EB_SET_ENDER, tender, sizeof(tender));
    close(sock);

    sock = ebcon(ebhostname);
    ret = eb_get(sock, EB_RUN_CLOSE, (char *)&ret);
    close(sock);
  }
  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_RUNINFO, (char *)&runinfo);
  close(sock);
  show_runinfo();

  getevtn(NULL);

  return 0;
}

int setrunnumber(char *arg){
  char *argv[10];
  int newn, argc, sock, ret;

  DB(printf("babicon: setnumber %s\n", arg));

  newn = -1;
  argc = striparg(arg, argv);

  if(argc == 1){
    newn = strtol(argv[0], NULL, 0);
  }else{
    newn = -1;
  }

  if(newn < 0){
    bbjson_charobj("error", "runnumber >= 0");
    return 0;
  }

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_RUNNUMBER, (char *)&newn, sizeof(newn));
  close(sock);

  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
    return 0;
  }

  getconfig(NULL);

  return 1;
}

int setebsize(char *arg){
  char *argv[10];
  int newn, argc, sock, ret;
  char js[128]={0};

  DB(printf("babicon: setebsize %s\n", arg));

  newn = -1;
  argc = striparg(arg, argv);

  if(argc == 1){
    newn = strtol(argv[0], NULL, 0);
  }else{
    newn = -1;
  }

  if(newn < 0 || newn > EB_EFBLOCK_MAXSIZE){
    
    sprintf(js, "Please set 0 < ebsize < %d (%d kB)", EB_EFBLOCK_MAXSIZE,
	   EB_EFBLOCK_MAXSIZE*2/1024);
    bbjson_charobj("error", js);
    return 0;
  }

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_EBSIZE, (char *)&newn, sizeof(newn));
  close(sock);

  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
    return 0;
  }

  getconfig(NULL);

  return 1;
}

int setbabildes(char *arg){
  char str[80];
  char *argv[10];
  int babildes, argc, sock, ret;

  DB(printf("babicon: setbabildes %s\n", arg));

  argc = striparg(arg, argv);

  if(argc == 1){
    strcpy(str, argv[0]);
  }else{
    sprintf(str, "error");
  }

  if(strcmp(str, "on") == 0){
    babildes = 1;
  }else if(strcmp(str, "off") == 0){
    babildes = 0;
  }else{
    bbjson_charobj("error", "setbabildes on/off");
    return 0;
  }
    

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_BABILDES, (char *)&babildes, sizeof(babildes));
  close(sock);

  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
    return 0;
  }

  getconfig(NULL);

  return 1;
}

int setrunname(char *arg){
  char *argv[10];
  char nname[80];
  int argc, sock, ret;

  DB(printf("babicon: setnumber %s\n", arg));

  memset(nname, 0, sizeof(nname));
  argc = striparg(arg, argv);

  if(argc == 1){
    strncpy(nname, argv[0], sizeof(nname)-1);
  }else{
    bbjson_charobj("error", "need to specify runname");
    return 0;
  }

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_RUNNAME, nname, sizeof(nname));
  close(sock);

  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
    return 0;
  }

  getconfig(NULL);

  return 1;
}

int setconfig(char *arg){
  bbjson_charobj("error", "This command is not implemented yet.");
  return 0;
}

int sethdlist(char *arg){
  int argc;
  char *argv[10];
  char this[] = "sethdlist";
  int hdn, of, fl;
  char hdp[80]={0}, js[272]={0};
  int sock, ret, ofc, pathc, dlc, er;

  fl = 1;
  of = 0;
  hdn = 0;
  ret = 0;
  ofc = 0;
  pathc = 0;
  dlc = 0;
  er = 1;

  DB(printf("babicon: sethdlist %s\n", arg));
  argc = striparg(arg, argv);

  if(argc == 2 || argc == 3){
    hdn = strtol(argv[0], NULL, 0);
    if(hdn < 0 || hdn >= MAXHD){
      sprintf(js, "0 < HDN < %d", MAXHD);
      bbjson_charobj("error", js);
      fl = 0;
    }
    
    if(!strcmp(argv[1], "on")){
      of = 1;
      ofc = 1;
    }else if(!strcmp(argv[1], "off")){
      of = 0;
      ofc = 1;
    }else if(!strcmp(argv[1], "del")){
      dlc = 1;
    }else if(!strcmp(argv[1], "path")){
      if(argc != 3){
	fl = 0;
      }else{
	memset(hdp, 0, sizeof(hdp));
	strncpy(hdp, argv[2], sizeof(hdp)-1);
	sock = ebcon(ebhostname);
	ret = eb_set(sock, EB_CHK_DIR, hdp, strlen(hdp)+1);
	close(sock);
	if(!ret){
	  sprintf(js, "Can't open dir %s", hdp);
	  bbjson_charobj("error", js);
	  fl = 0;
	}else{
	  pathc = 1;
	}
      }
    }else{
      bbjson_charobj("error", "COMMAND = on/off/del/path");
      fl = 0;
    }
  }else{
    fl = 0;
  }

  if(!fl){
    synopsis(this);
    return 0;
  }

  if(fl){
    sock = ebcon(ebhostname);
    eb_get(sock, EB_GET_HDLIST, (char *)daqinfo.hdlist);
    close(sock);

    if(ofc){
      if(daqinfo.hdlist[hdn].ex){
	daqinfo.hdlist[hdn].of = of;
      }else{
	sprintf(js, "Don't exist such HDN %d", hdn);
	bbjson_charobj("error", js);
	er = 0;
      }
    }else if(pathc){
      daqinfo.hdlist[hdn].ex = 1;
      strncpy(daqinfo.hdlist[hdn].path, hdp,
	      sizeof(daqinfo.hdlist[hdn].path));
    }else if(dlc){
      daqinfo.hdlist[hdn].ex = 0;
      memset(daqinfo.hdlist[hdn].path, 0,
	     sizeof(daqinfo.hdlist[hdn].path));
    }
    if(er){
      sock = ebcon(ebhostname);
      ret = eb_set(sock, EB_SET_HDLIST, (char *)daqinfo.hdlist,
		   sizeof(daqinfo.hdlist));
      close(sock);
      if(!ret){
	bbjson_charobj("error", "Run is now running, can't change parameters");
      }
    }

    sock = ebcon(ebhostname);
    eb_get(sock, EB_GET_HDLIST, (char *)daqinfo.hdlist);
    close(sock);

    show_hdlist();
  }

  return 1;
}

int seteflist(char *arg){
  char this[] = "seteflist";
  int argc;
  char *argv[10];
  int efn, fl;
  char ch1[80], ch2[80], js[272]={0};
  int sock, ret;
  int com, upf;
  enum comlist {ADD, DEL, HOST, NAME, ON, OFF, SCR, PAS};

  //DB(printf("babicon: seteflist %s\n", arg));
  argc = striparg(arg, argv);

  if(argc != 2 && argc != 3 && argc != 4){
    synopsis(this);
    return 0;
  }


  fl = 1;
  efn = 0;
  ret = 0;
  upf = 0;

  efn = strtol(argv[0], NULL, 0);
  if(efn < 0 || efn >= MAXEF){
    sprintf(js, "0 < EFN < %d", MAXEF);
    bbjson_charobj("error", js);
    return 0;
  }

  if((com = chkcom(argv[1], comseteflist)) < 0){
    sprintf(js, "Unknown command : %s", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }

  switch(com){
  case ADD:
    if(argc != 4){
      fl = 0;
    }else{
      memset(ch1, 0, sizeof(ch1));
      memset(ch2, 0, sizeof(ch2));
      strncpy(ch1, argv[2], sizeof(ch1)-1);
      strncpy(ch2, argv[3], sizeof(ch2)-1);
    }
    break;
  case HOST:
  case NAME:
    if(argc != 3){
      fl = 0;
    }else{
      memset(ch1, 0, sizeof(ch1));
      strncpy(ch1, argv[2], sizeof(ch1)-1);
    }
    break;
  }
    
  if(fl){
    sock = ebcon(ebhostname);
    eb_get(sock, EB_GET_EFLIST, (char *)daqinfo.eflist);
    close(sock);

    switch(com){
    case ADD:
      if((upf = chknex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].ex = 1;
	daqinfo.eflist[efn].of = EB_EFLIST_ON;
	strncpy(daqinfo.eflist[efn].host, ch1,
		sizeof(daqinfo.eflist[efn].host));
	strncpy(daqinfo.eflist[efn].name, ch2,
		sizeof(daqinfo.eflist[efn].name));
      }
      break;
    case DEL:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].ex = 0;
	daqinfo.eflist[efn].of = EB_EFLIST_OFF;
	memset(daqinfo.eflist[efn].host, 0, sizeof(daqinfo.eflist[efn].host));
	memset(daqinfo.eflist[efn].name, 0, sizeof(daqinfo.eflist[efn].name));
      }
      break;
    case HOST:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	strncpy(daqinfo.eflist[efn].host, ch1,
		sizeof(daqinfo.eflist[efn].host));
      }
      break;
    case NAME:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	strncpy(daqinfo.eflist[efn].name, ch1,
		sizeof(daqinfo.eflist[efn].name));
      }
      break;
    case ON:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].of = EB_EFLIST_ON;
      }
      break;
    case OFF:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].of = EB_EFLIST_OFF;
      }
      break;
    case SCR:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].of = EB_EFLIST_SCR;
      }
      break;
    case PAS:
      if((upf = chkex(daqinfo.eflist[efn].ex, "EFN"))){
	daqinfo.eflist[efn].of = EB_EFLIST_PAS;
      }
      break;
    }

    if(upf){
      sock = ebcon(ebhostname);
      ret = eb_set(sock, EB_SET_EFLIST, (char *)daqinfo.eflist,
		   sizeof(daqinfo.eflist));
      close(sock);
      if(!ret){
	bbjson_charobj("error", "Run is now running, can't change parameters");
      }

      sock = ebcon(ebhostname);
      eb_get(sock, EB_GET_EFLIST, (char *)daqinfo.eflist);
      close(sock);
    }

    show_eflist();
  }else{
    synopsis(this);
  }

  return 1;
}

int synopsis(char *arg){
  int i, m;

  m = 0;
  i = 0;

  while(manual[i].name){
    if(strcmp(manual[i].name, arg) == 0){
      m = i;
      break;
    }
    i++;
  }

  bbjson_begin_obj("manual");
  bbjson_charobj("name", manual[m].name);
  bbjson_charobj("synopsis", manual[m].synopsis);

  bbjson_begin_array("command");
  if(manual[m].command){
    i=0;
    while(manual[m].command[i]){
      bbjson_begin_obj(0);
      bbjson_charobj("item", manual[m].command[i]);
      bbjson_charobj("parameter", manual[m].command[i+1]);
      bbjson_end_obj();
      i += 3;
    }
  }
  bbjson_end_array();
  bbjson_end_obj();

  return 0;
}

int help(char *arg){
  int i, f, argc, m;
  char *argv[10];

  i = 0;
  f = -1;
  m = -1;
  argc = striparg(arg, argv);
  if(argc > 0){
    while(commands[i].name){
      if(strcmp(commands[i].name, argv[0]) == 0){
	f = i;
	break;
      }
      i++;
    }
    i =0;
    while(manual[i].name){
      if(strcmp(manual[i].name, argv[0]) == 0){
	m = i;
	break;
      }
      i++;
    }
  }

  bbjson_begin_obj("help");

  if(f < 0){
    i = 0;
    bbjson_begin_array("commandlist");
    while(1){
      if(commands[i].name == NULL) break;
      bbjson_begin_obj(0);
      bbjson_charobj("name", commands[i].name);
      bbjson_charobj("doc", commands[i].doc);
      bbjson_end_obj();
      i++;
    }
    bbjson_end_array();
  }else{
    if(m > -1){
      bbjson_charobj("name", commands[m].name);
      bbjson_charobj("doc", commands[m].doc);
      bbjson_begin_obj("manual");
      bbjson_charobj("name", manual[m].name);
      bbjson_charobj("synopsis", manual[m].synopsis);
      bbjson_charobj("description", manual[m].description);
      if(manual[m].command){
	bbjson_begin_array("command");
	i=0;
	while(manual[m].command[i]){
	  bbjson_begin_obj(0);
	  bbjson_charobj("item", manual[m].command[i]);
	  bbjson_charobj("parameter", manual[m].command[i+1]);
	  bbjson_charobj("description", manual[m].command[i+2]);
	  i+=3;
	}
	bbjson_end_array();
      }
      bbjson_end_obj();
    }else{
      bbjson_charobj("name", commands[f].name);
      bbjson_charobj("doc", commands[f].doc);
    }
  }
  
  bbjson_end_obj();

  return 0;
}

int exec_sh(char *arg){
  system(arg);
  return 0;
}

int exec_nrsh(char *arg){
  int sock, com, len;
  char buff[1024];

  memset(buff ,0, sizeof(buff));
  strncpy(buff, arg, sizeof(buff)-1);
  com = MON_EXEC;

  sock = mocon(ebhostname);

  len = sizeof(com) + strlen(buff);
  send(sock,(char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, sizeof(com), 0);
  send(sock, buff, strlen(buff), 0);

  close(sock);
  

  return 0;
}


int exec_rsh(char *arg){
  int sock, com, len;
  char buff[BABIRL_COM_SIZE];

  DB(printf("babicon: rsh %s\n", arg));

  memset(buff ,0, sizeof(buff));
  strncpy(buff, arg, sizeof(buff)-1);
  com = MON_PEXEC;

  sock = mocon(ebhostname);

  len = sizeof(com) + strlen(buff);
  send(sock,(char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, sizeof(com), 0);
  send(sock, buff, strlen(buff), 0);

  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, buff, len, MSG_WAITALL);

  close(sock);

  bbjson_begin_obj("rsh");
  bbjson_charobj("result", buff);
  bbjson_end_obj();

  return 0;
}

int getconfig(char *arg){
  int sock;

  DB(printf("babicon: getconfig %s\n", arg));

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_SSMINFO, (char *)&ssminfo);
  close(sock);
  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_DAQINFO, (char *)&daqinfo);
  close(sock);
  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_RUNINFO, (char *)&runinfo);
  close(sock);

  if(arg){
    if(!strcmp("noshow", arg)){
      /* noop */
    }else{
      show_daqinfo();
    }
  }else{
    show_daqinfo();
  }

  return 0;
}

int setssminfo(char *arg){
  char this[] = "setssminfo";
  int argc;
  char *argv[10];
  char ch1[80];
  int sock, ret;
  int com;
  enum comlist {ON, OFF, HOST, START, STOP};

  argc = striparg(arg, argv);

  if(argc != 1 && argc != 2){
    synopsis(this);
    return 0;
  }


  ret = 0;
  
  if((com = chkcom(argv[0], comssminfo)) < 0){
    bbjson_charobj("error", "setssminfo Unknown command");
    synopsis(this);
    return 0;
  }

  memset(ch1, 0, sizeof(ch1));
  switch(com){
  case HOST:
    if(argc != 2){
      bbjson_charobj("error", "no hostname");
    }else{
      strncpy(ch1, argv[1], sizeof(ch1)-1);
    }
    break;
  case START:
  case STOP:
    if(argc != 2){
      bbjson_charobj("error", "no start/stop path");
    }else{
      strncpy(ch1, argv[1], sizeof(ch1)-1);
    }
    break;
  }

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_SSMINFO, (char *)&ssminfo);
  close(sock);
  
  switch(com){
  case ON:
    ssminfo.of = 1;
    break;
  case OFF:
    ssminfo.of = 0;
    break;
  case HOST:
    strncpy(ssminfo.host, ch1, sizeof(ssminfo.host));
    break;
  case START:
    strncpy(ssminfo.start, ch1, sizeof(ssminfo.start));
    break;
  case STOP:
    strncpy(ssminfo.stop, ch1, sizeof(ssminfo.stop));
    break;
  }

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_SSMINFO, (char *)&ssminfo,
	 sizeof(ssminfo));
  close(sock);
  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
  }

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_SSMINFO, (char *)&ssminfo);
  close(sock);

  show_ssminfo();

  return 1;
}


int getclinfo(char *arg){
  int sock;

  if(!(sock = infcon(ebhostname))) return 0;

  eb_get(sock, INF_GET_CLIHOSTS, (char *)clinfo);
  close(sock);

  show_clinfo();
  
  return 1;
}


int setauclinfo(char *arg){
  char this[] = "setauclinfo";
  int argc;
  char *argv[10];
  int cln, fl;
  int sock, ret;
  int com, efn;
  char js[272] = {0};
  enum comlist {ADD, DEL};


  argc = striparg(arg, argv);

  if(argc != 2 && argc !=3){
    synopsis(this);
    return 0;
  }


  fl = 1;
  cln = 0;
  efn = 0;

  ret = 0;
  //upf = 1;
  //id = 0;

  efn = strtol(argv[0], NULL, 0);
  if(efn < 0 || efn >= MAXEF){
    sprintf(js, "0 < EFN < %d", MAXEF);
    bbjson_charobj("error", js);
    return 0;
  }


  if((com = chkcom(argv[1], comsetclinfo)) < 0){
    sprintf(js, "Unknown command : %s", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }

  sock = infcon(ebhostname);
  eb_get(sock, AU_GET_CLINFO, (char *)auclinfo);
  close(sock);

  switch(com){
  case ADD:
    if(argc != 3){
      fl = 0;
    }else{
      cln = strtol(argv[2], NULL, 0);
      if(cln < 0 || cln >= MAXCLI){
	sprintf(js, "0 < CLIN < %d", MAXCLI);
	bbjson_charobj("error", js);
	fl = 0;
      }
    }
    break;
  case DEL:
    auclinfo[efn] = -1;
    break;
  }
    
  if(fl){
    sock = infcon(ebhostname);
    ret = eb_set(sock, AU_SET_CLINFO, (char *)auclinfo,
		   sizeof(clinfo));
    close(sock);
    if(!ret){
      bbjson_charobj("error", "Run is now running, can't change parameters");
    }

    getauclinfo();
  }else{
    synopsis(this);
  }
  

  return 1;
}

int getauclinfo(char *arg){
  //char this[] = "getauclinfo";
  int sock, i;

  sock = infcon(ebhostname);
  eb_get(sock, AU_GET_CLINFO, (char *)auclinfo);
  close(sock);

  bbjson_begin_array("auclinfo");
  for(i=0;i<MAXEF;i++){
    if(auclinfo[i] != -1){
      bbjson_begin_obj(0);
      bbjson_intobj("id", i);
      bbjson_intobj("auclinfo", auclinfo[i]);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();

  return 1;
}




int getscrdata(int id){
  int len, com, sock;
  int idx, tid, i;

  if(!(sock = infcon(ebhostname))) return 0;
  eb_get(sock, INF_GET_SCRLIST, (char *)buff);
  memcpy((char *)&scranan, buff, sizeof(scranan));
  memcpy((char *)scrana, buff+sizeof(scranan), sizeof(scrana));
  close(sock);

  if(!(sock = infcon(ebhostname))) return 0;
  com = INF_GET_SCRDATA;
  len = sizeof(com) + sizeof(id);
  memcpy(buff, (char *)&com, sizeof(com));
  memcpy(buff+sizeof(com), (char *)&id, sizeof(id));

  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, buff, len, 0);

  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, buff, len, MSG_WAITALL);


  memcpy((char *)&tid, buff, sizeof(tid));
  if(tid < 0){
    return 0;
  }

  idx = sizeof(int);
  memcpy((char *)&scrana[tid].classid, buff+idx, sizeof(int));
  idx += sizeof(int);
  memcpy((char *)&scrana[tid].scrid, buff+idx, sizeof(int));
  idx += sizeof(int);
  memcpy((char *)&scrana[tid].scrn, buff+idx, sizeof(int));
  idx += sizeof(int);
  memcpy(scrana[tid].idname, buff+idx, sizeof(scrana[tid].idname));
  idx += sizeof(scrana[tid].idname);

  if(!scr[tid]){
    scr[tid] = (struct ridf_scr_contst *)
      malloc(sizeof(struct ridf_scr_contst)*scrana[tid].scrn);
  }

  for(i=0;i<scrana[tid].scrn;i++){
    memcpy((char *)&scr[tid][i], buff+idx, sizeof(scr[tid][i]));
    idx += sizeof(scr[tid][id]);
  }

  return 1;
}

int getscrlist(char *arg){
  int sock, id;

  if(arg){
    if(strlen(arg)){
      id = strtol(arg, NULL, 0);
      if(id < 0){
	bbjson_charobj("error", "getscrlist 0 =< scrid");
	return 0;
      }
      if(getscrdata(id)){
	show_scrdata(id);
	return 1;
      }
    }
  }

  if(!(sock = infcon(ebhostname))) return 0;
  eb_get(sock, INF_GET_SCRLIST, (char *)buff);
  memcpy((char *)&scranan, buff, sizeof(scranan));
  memcpy((char *)scrana, buff+sizeof(scranan), sizeof(scrana));
  close(sock);

  show_scrlist();

  return 1;
}

int setscrname(char *arg){

  bbjson_charobj("error", "setscrname is not supported by babicmdjson");
  return 0;
}

int getesconfig(char *arg){
  int sock, len, com;

  DB(printf("babicon: getesconfig %s\n", arg));

  if(!(sock = escon(arg))) return 0;
  
  com = ES_GET_CONFIG;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);
  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, (char *)&efrc, len, MSG_WAITALL);
  close(sock);

  show_efrc(efrc);
  
  return 1;
}

int esquit(char *arg){
  int sock, len, com;
  DB(printf("babicon: esquit %s\n", arg));

  if(!(sock = escon(arg))) return 0;
  
  com = ES_QUIT;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);
  close(sock);

  return 1;
}

int setesconfig(char *arg){
  int sock, len, com;
  char *rarg[80];
  int ar, fl;
  char this[] = "setesconfig";

  fl = 1;
  com = 0;
  DB(printf("babicon: setesconfig %s\n", arg));
  ar = striparg(arg, rarg);
  if(ar != 3) fl = 0;

  if(!getesconfig(rarg[0])){
    return 0;
  }

  if(ar == 3){
    if(!strcmp(rarg[1], "hd1")){
      if(!strcmp(rarg[2], "on")){
	efrc.hd1 = 1;
      }else if(!strcmp(rarg[2], "off")){
	efrc.hd1 = 0;
      }else{
	fl = 0;
      }
    }else if(!strcmp(rarg[1], "hd2")){
      if(!strcmp(rarg[2], "on")){
	efrc.hd2 = 1;
      }else if(!strcmp(rarg[2], "off")){
	efrc.hd2 = 0;
      }else{
	fl = 0;
      }
    }else if(!strcmp(rarg[1], "mt")){
      if(!strcmp(rarg[2], "on")){
	efrc.mt = 1;
      }else if(!strcmp(rarg[2], "off")){
	efrc.mt = 0;
      }else{
	fl = 0;
      }
    }else if(!strcmp(rarg[1], "hd1dir")){
      strncpy(efrc.hd1dir, rarg[2], sizeof(efrc.hd1dir)-1);
    }else if(!strcmp(rarg[1], "hd2dir")){
      strncpy(efrc.hd2dir, rarg[2], sizeof(efrc.hd2dir)-1);
    }else if(!strcmp(rarg[1], "rtdrv")){
      strncpy(efrc.mtdir, rarg[2], sizeof(efrc.mtdir)-1);
    }else if(!strcmp(rarg[1], "host")){
      strncpy(efrc.erhost, rarg[2], sizeof(efrc.erhost)-1);
    }
  }

  if(fl){
    if(!(sock = escon(rarg[0]))) return 0;
  
    com = ES_SET_CONFIG;
    len = sizeof(com) + sizeof(efrc);
    send(sock, (char *)&len, sizeof(len), 0);
    send(sock, (char *)&com, sizeof(com), 0);
    send(sock, (char *)&efrc, sizeof(efrc), 0);
    recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
    recv(sock, (char *)&efrc, len, MSG_WAITALL);
    close(sock);

    getesconfig(rarg[0]);
    show_efrc(efrc);
    
  }else{
    bbjson_charobj("error", "invalid command for setesconfig");
    synopsis(this);
    return 0;
  }


  return 1;
}

int reloadesdrv(char *arg){
  int sock, len, com;
  char *rarg[80];
  //int ar;

  striparg(arg, rarg);
  com = 0;
  DB(printf("babicon: setesconfig %s\n", arg));

  if(!(sock = escon(rarg[0]))) return 0;

  com = ES_RELOAD_DRV;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, sizeof(com), 0);
  close(sock);

  getesconfig(rarg[0]);
  show_efrc(efrc);

  return 1;
}

void show_evtn(char *arg){
  int i;

  bbjson_begin_array("lastevtn");
  for(i=0; i<MAXEF; i++){
    if(daqinfo.eflist[i].ex && daqinfo.eflist[i].of == EB_EFLIST_ON){
      bbjson_begin_obj(0);
      bbjson_intobj("efn", i);
      bbjson_charobj("name", daqinfo.eflist[i].name);
      bbjson_intobj("evtn", evtn[i]);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();
}

int getevtn(char *arg){
  int sock, len, com, i;
  char chefn[80];

  DB(printf("babicon: getevtn %s\n", arg));

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_DAQINFO, (char *)&daqinfo);
  close(sock);

  for(i=0;i<MAXEF;i++){
    if(daqinfo.eflist[i].ex && daqinfo.eflist[i].of == EB_EFLIST_ON){
      memset(chefn, 0, sizeof(chefn));
      sprintf(chefn, "%d", i);
      if(!(sock = escon(chefn))) return 0;
      com = ES_GET_EVTN;
      len = sizeof(com);
      send(sock, (char *)&len, sizeof(len), 0);
      send(sock, (char *)&com, len, 0);
      recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
      recv(sock, (char *)&evtn[i], len, MSG_WAITALL);
      close(sock);
    }
  }

  show_evtn(NULL);
  
  return 1;
}

int getesevtnumber(char *arg){
  getevtn(NULL);

  return 1;
}

int whoareyou(char *arg){
  int sock, len, com;

  DB(printf("babicon: whoareyou %s\n", arg));

  if(!(sock = escon(arg))) return 0;
  
  com = WHOAREYOU;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);
  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, (char *)&esname, len, MSG_WAITALL);
  close(sock);

  bbjson_begin_obj("whoareyou");
  bbjson_charobj("efn", arg);
  bbjson_charobj("esname", esname);
  bbjson_end_obj();

  return 1;
}

int wth(char *arg){
  int sock;

  DB(printf("babicon: wth %s\n", arg));

  get_runinfo();
  if(!runinfo.runstat){
    if(!strlen(arg)){
      bbjson_charobj("error", "no header");
    }else{
      strncpy(runinfo.header, arg, sizeof(runinfo.header)-1);
    }
  }else{
    bbjson_charobj("error", "Now running...");
    return 0;
  }

  sock = ebcon(ebhostname);
  eb_set(sock, EB_SET_HEADER, runinfo.header,
	       sizeof(runinfo.header));
  close(sock);


  return 1;
}

int shutdownall(char *arg){
  int sock, ret;

  DB(printf("babicon: shutdown %s\n", arg));


  sock = infcon(ebhostname);
  eb_get(sock, INF_QUIT, (char *)&ret);
  close(sock);
  
  sock = ebcon(ebhostname);
  eb_get(sock, EB_QUIT, (char *)&ret);
  close(sock);
  

  return 0;
}

int esconnect(char *arg){
  int sock, len, com;

  DB(printf("babicon: esconnect %s\n", arg));
  if(!(sock = escon(arg))) return 0;
  com = ES_CON_EFR;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);
  close(sock);

  return 1;
}

int esdisconnect(char *arg){
  int sock, len, com;

  DB(printf("babicon: esdisconnect %s\n", arg));

  if(!(sock = escon(arg))) return 0;
  com = ES_DIS_EFR;
  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);
  close(sock);

  return 1;
}

int main(int argc,char *argv[]){
  char line[1024], js[256]={0};
  int i, ret;
  char *com;

  /* Signal SIGINT -> quit() */
  signal(SIGINT,(void *)quit);

  /* Get environmnet */
  if(argc >= 3){
    strncpy(ebhostname, argv[1], sizeof(ebhostname)-1);
  }else{
    exit(0);
  }

  memset(line, 0, sizeof(line));
  strcat(line, argv[2]);
  for(i=3; i<argc; i++){
    strcat(line, " ");
    strcat(line, argv[i]);
  }

  /* Initialize commands */
  init_hcom(commands);
  
  for(i=0;i<MAXSCRANA;i++){
    scr[i] = NULL;
  }
  
  bbjson_begin();

  get_runinfo();
  getconfig("noshow\0");


  com = stripwhite(line);
  if(*com){
    ret = execute_line_noerr(com, commands);
    if(ret == -1){
      snprintf(js, sizeof(js), "command not fount (%s)", com);
      bbjson_charobj("error", js);
    }
  }
  bbjson_end();

  quit();

  return 0;
}
