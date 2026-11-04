#include <bbxml.h>
#include <bbjson.h>

int fndscrid(int id);

static BBXMLEL *xmlel = NULL;
/* common functions for babicon and babicmd */
int infcon(char *host){
  int infsock;

  /* Connect to babild */
  if(!(infsock = mktcpsend(host, INFCOMPORT))){
    bbjson_charobj("error", "babicon: Can't connet to babinfo. sleep 3sec");
    bbjson_end();
    sleep(3);
    //exit(0);
    return 0;
  }
  
  return infsock;
}

int ebcon(char *host){
  int ebsock;

  /* Connect to babild */
  if(!(ebsock = mktcpsend(host, EBCOMPORT))){
    bbjson_charobj("error", "babicon: Can't connet to babild. sleep 3sec");
    bbjson_end();
    sleep(3);
    //exit(0);
    return 0;
  }
  
  return ebsock;
}

int chkcom(char *com, char **comlist){
  int ret = -1, i;

  i = 0;
  while(comlist[i]){
    if(!strcmp(com, comlist[i])){
      ret = i/3;
      break;
    }
    i += 3;
  }

  return ret;
}

int fndscrid(int id){
  int ret, i;

  ret = -1;
  for(i=0;i<scranan;i++){
    if(scrana[i].scrid == id){
      ret = i;
    }
  }

  return ret;
}



int exec_psh(char *arg){
  FILE *pfp;
  char buff[1024];
  char js[128] = {0};

  memset(buff, 0, sizeof(buff));

  if(strlen(arg) <= 0) return 0;

  if((pfp = popen(arg, "r")) == NULL){
    sprintf("Can't exec %s", arg);
    bbjson_charobj("error", js);
    return 0;
  }else{
    fread(buff, 1, sizeof(buff)-1, pfp);
    if(pclose(pfp) < 0){
      sprintf(js, "Unknown error : %s", arg);
      bbjson_charobj("error", js);
      return 0;
    }
    //printf("%s", buff);
    if(buff[0] == 'O' && buff[1] == 'K'){
      // OK
      return 1;
    }else{
      // Error
      return 0;
    }
  }
}

int exec_nsh(char *arg){
  char buff[1024];

  memset(buff, 0, sizeof(buff));
  sprintf(buff, "%s &", arg);
  system(buff);

  return 1;
}


int pstart(char *arg){
  if(strlen(arg) <= 0){
    bbjson_charobj("error", "pstart COMMAND");
    return 0;
  }

  if(!exec_nsh(arg)){
    return 0;
  }else{
    start();
  }

  return 0;
}

int pnssta(char *arg){
  if(strlen(arg) <= 0){
    bbjson_charobj("error", "pnssta COMMAND");
    return 0;
  }

  if(!exec_nsh(arg)){
    return 0;
  }else{
    nssta();
  }

  return 0;
}

int pstop(char *arg){
  if(strlen(arg) <= 0){
    bbjson_charobj("error", "pstop COMMAND");
    return 0;
  }

  if(!exec_psh(arg)){
    return 0;
  }else{
    stop();
  }

  return 0;
}

void show_hdlist(void){
  int i;
  float avail;
  char js[272] = {0};

  bbjson_begin_array("hdlist");
  for(i=0; i<MAXHD; i++){
    if(daqinfo.hdlist[i].ex){
      bbjson_begin_obj(0);
      bbjson_intobj("hdn", i);
      bbjson_charobj("path", daqinfo.hdlist[i].path);
      bbjson_charobj("onoff", ofstr[daqinfo.hdlist[i].of]);
      avail = (float)(daqinfo.hdlist[i].free)/1024./1024./1024.;

      if(avail < 10.0){
	sprintf(js, "%3.1fGB", avail);
      }else if(avail < 1000.){
	avail = avail;
	sprintf(js, "%3.0fGB", avail);
      }else if(avail < 10000.){
	avail = avail / 1024.;
	sprintf(js, "%3.1fTB", avail);
      }else{
	avail = avail / 1024.;
	sprintf(js, "%3.0fTB", avail);
      }
      bbjson_charobj("free", js);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();
  gettgig();
}


int show_scrlive(void){
  bbjson_begin_obj("scrlive");
  bbjson_intobj("gatedid", scrlive.gatedid);
  bbjson_intobj("gatedch", scrlive.gatedch);
  bbjson_intobj("ungatedid", scrlive.ungatedid);
  bbjson_intobj("ungatedch", scrlive.ungatedch);
  bbjson_end_obj();

  return 1;
}

int setscrlive(char *arg){
  char this[] = "setscrlive";
  int argc;
  char *argv[10];
  int sock, ret;
  int com, tid, tch;
  char js[272] = {0};
  enum comlist {GATED, UNGATED};

  argc = striparg(arg, argv);

  if(argc != 3){
    synopsis(this);
    return 0;
  }


  ret = 0;
  
  if((com = chkcom(argv[0], comsetscrlive)) < 0){
    sprintf(js, "Unknown command : %s ", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_SCRLIVE, (char *)&scrlive);
  close(sock);
  
  tid = strtol(argv[1], NULL, 0);
  tch = strtol(argv[2], NULL, 0);

  switch(com){
  case GATED:
    scrlive.gatedid = tid;
    scrlive.gatedch = tch;
    break;
  case UNGATED:
    scrlive.ungatedid = tid;
    scrlive.ungatedch = tch;
    break;
  }

  sock = infcon(ebhostname);
  ret = eb_set(sock, INF_SET_SCRLIVE, (char *)&scrlive,
	 sizeof(scrlive));
  close(sock);
  if(!ret){
    bbjson_charobj("error", "setscrlive, faled");
  }

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_SCRLIVE, (char *)&scrlive);
  close(sock);

  show_scrlive();

  return 1;
}


int getscrlive(char *arg){
  int sock;

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_SCRLIVE, (char *)&scrlive);
  close(sock);

  show_scrlive();

  return 1;
}

void show_statcom(void){
  bbjson_begin_obj("statcom");
  bbjson_charobj("onoff", ofstr[statcom.of]);
  bbjson_charobj("start", statcom.start);
  bbjson_charobj("stop", statcom.stop);
  bbjson_end_obj();
}

int setstatcom(char *arg){
  int sock, argc;
  char *argv[10];
  char this[] = "setstatcom";
  enum comlist {ON, OFF, START, STOP};
  char ch1[80], js[272]={0};
  int ret;
  int com;

  argc = striparg(arg, argv);
  if(argc != 1 && argc != 2){
    synopsis(this);
    return 0;
  }

  ret = 0;

  if((com = chkcom(argv[0], comstatcom)) < 0){
    sprintf(js, "Unknown command : %s", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }
  memset(ch1, 0, sizeof(ch1));
  switch(com){
  case START:
  case STOP:
    strncpy(ch1, argv[2], sizeof(ch1)-1);
    break;
  }

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_STAT_COMMAND, (char *)&statcom);
  close(sock);


  switch(com){
  case ON:
    statcom.of = 1;
    break;
  case OFF:
    statcom.of = 0;
    break;
  case START:
    strncpy(statcom.start, ch1, sizeof(statcom.start));
    break;
  case STOP:
    strncpy(statcom.stop, ch1, sizeof(statcom.stop));
    break;
  }

  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_STAT_COMMAND, (char *)&statcom, sizeof(statcom));
  close(sock);
  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
  }

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_STAT_COMMAND, (char *)&statcom);
  close(sock);

  show_statcom();

  return 1;
}

int getstatcom(char *arg){
  int sock;

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_STAT_COMMAND, (char *)&statcom);
  close(sock);
  show_statcom();

  return 1;
}

int gettcpclinfo(char *arg){
  int sock;

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_TCPCLIHOSTS, (char *)&tcpclinfo);
  close(sock);
  show_tcpclinfo();

  return 1;
}


// DB connection
void show_dbcon(void){
  bbjson_begin_obj("dbcon");
  bbjson_charobj("onoff", ofstr[dbcon.of]);
  bbjson_charobj("host", dbcon.host);
  bbjson_charobj("dbname", dbcon.dbname);
  bbjson_charobj("user", dbcon.user);
  bbjson_charobj("pass", "*******");
  bbjson_end_obj();
}

int setdbcon(char *arg){
  int sock, argc;
  char *argv[10];
  char this[] = "setdbcon";
  enum comlist {ON, OFF, SET};
  char ch1[80]={0}, ch2[80]={0};
  char ch3[80]={0}, ch4[80]={0};
  int ret;
  int com;
  char js[80] = {0};

  argc = striparg(arg, argv);
  if(argc != 1 && argc != 5){
    synopsis(this);
    return 0;
  }

  ret = 0;

  if((com = chkcom(argv[0], comdbcon)) < 0){
    sprintf(js, "Unknown command : %s", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_DBCON, (char *)&dbcon);
  close(sock);

  memset(ch1, 0, sizeof(ch1));
  memset(ch2, 0, sizeof(ch2));
  memset(ch3, 0, sizeof(ch3));
  memset(ch4, 0, sizeof(ch4));

  switch(com){
  case SET:
    if(argc == 1){
    }else{
      strncpy(ch1, argv[2], sizeof(ch1)-1);
      strncpy(ch2, argv[3], sizeof(ch2)-1);
      strncpy(ch3, argv[4], sizeof(ch3)-1);
      strncpy(ch4, argv[5], sizeof(ch4)-1);
    }
    break;
  }


  switch(com){
  case ON:
    dbcon.of = 1;
    break;
  case OFF:
    dbcon.of = 0;
    break;
  case SET:
    if(strlen(ch1) > 1){
      strncpy(dbcon.host, ch1, sizeof(dbcon.host));
    }
    if(strlen(ch2) > 1){
      strncpy(dbcon.dbname, ch2, sizeof(dbcon.dbname));
    }
    if(strlen(ch3) > 1){
      strncpy(dbcon.user, ch3, sizeof(dbcon.user));
    }
    if(strlen(ch4) > 1){
      strncpy(dbcon.passwd, ch4, sizeof(dbcon.passwd));
    }
    break;
  }

  sock = infcon(ebhostname);
  ret = eb_set(sock, INF_SET_DBCON, (char *)&dbcon, sizeof(dbcon));
  close(sock);

  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameters");
  }

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_DBCON, (char *)&dbcon);
  close(sock);

  show_dbcon();

  return 1;
}

int getchkerhost(char *arg){
  int chkerhost;
  int sock;

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_CHKERHOST, (char *)&chkerhost);
  close(sock);

  bbjson_charobj("chkerhost", ofstr[chkerhost]);

  return 1;
}

int getdbcon(char *arg){
  int sock;

  sock = infcon(ebhostname);
  eb_get(sock, INF_GET_DBCON, (char *)&dbcon);
  close(sock);

  show_dbcon();

  return 1;
}

int chkdbcon(char *arg){
  int sock, ret;

  sock = infcon(ebhostname);
  eb_get(sock, INF_CHK_DBCON, (char *)&ret);
  close(sock);

  if(ret){
    bbjson_charobj("chkdbcon", "ok");
  }else{
    bbjson_charobj("chkdbcon", "ng");
  }

  return 1;
}

void show_clinfo(void){
  int i;

  bbjson_begin_array("clinfo");
  for(i=0;i<MAXCLI;i++){
    if(clinfo[i].ex){
      //printf("%3d : %s (SHMID = %d)\n", i, clinfo[i].clihost, clinfo[i].ex-1);
      bbjson_begin_obj(0);
      bbjson_intobj("id", i);
      bbjson_charobj("host", clinfo[i].clihost);
      bbjson_intobj("shmid", clinfo[i].ex-1);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();
}


void show_tcpclinfo(void){
  int i;
  bbjson_begin_array("tcpclinfo");
  for(i=0;i<MAXTCPCLI;i++){
    if(tcpclinfo[i].sock){
      bbjson_begin_obj(0);
      bbjson_intobj("id", i);
      bbjson_charobj("host", tcpclinfo[i].host);
      bbjson_intobj("port", tcpclinfo[i].port);
      bbjson_end_obj();
    }
  }
  bbjson_end_array();
}


int dbsetexpid(char *arg){
  char this[] = "dbsetexpid";
  int argc, expid;
  char *argv[10];
  char com[1024];
  char js[272]={0};
  BBXMLEL *exp=NULL, *id=NULL, *er=NULL, *name=NULL, *babinfo=NULL;
  argc = striparg(arg, argv);
  if(argc != 1){
    synopsis(this);
    return 0;
  }

  expid = strtol(arg, NULL, 0);
  memset(com, 0, sizeof(com));
  sprintf(com, "<dbsetexp><id>%d</id></dbsetexp>\n", expid);
  babinfoxcom(com);

  DB(bbxml_printall(xmlel));
  
  if(!(babinfo = bbxml_node(xmlel, "babinfo"))){
    bbjson_charobj("error", "error xml, this is not babinfo xml");
  }

  if(!(exp = bbxml_node(babinfo, "exp"))){
    bbjson_charobj("error", "error in xml");
  }

  if((er = bbxml_node(exp, "error"))){
    sprintf(js, "Error : %s", er->text);
    bbjson_charobj("error", js);
  }
  
  if(!(id = bbxml_node(exp, "id"))){
    sprintf(js, "No ExpID");
    bbjson_charobj("error", js);
  }else{
    bbjson_charobj("expid", id->text);
    if((name = bbxml_node(exp, "name"))){
      bbjson_charobj("exname", name->text);
    }
  }
  

  return 1;
}

int dbgetexpid(char *arg){
  BBXMLEL *exp=NULL, *id=NULL, *er=NULL, *name=NULL, *babinfo=NULL;
  char com[1024];
  char js[272]={0};

  memset(com, 0, sizeof(com));
  sprintf(com, "<dbgetexp/>");
  babinfoxcom(com);

  if(!(babinfo = bbxml_node(xmlel, "babinfo"))){
    bbjson_charobj("error", "error xml, this is not babinfo xml");
  }

  if(!(exp = bbxml_node(babinfo, "exp"))){
    bbjson_charobj("error", "error in xml");
  }

  if((er = bbxml_node(exp, "error"))){
    sprintf(js, "Error : %s", er->text);
    bbjson_charobj("error", js);
  }
  
  if(!(id = bbxml_node(exp, "id"))){
    sprintf(js, "No ExpID");
    bbjson_charobj("error", js);
  }else{
    bbjson_charobj("expid", id->text);
    if((name = bbxml_node(exp, "name"))){
      bbjson_charobj("exname", name->text);
    }
  }
  
  return 1;
}

int dbgetscr(char *arg){
  char com[1024];
  memset(com, 0, sizeof(com));
  sprintf(com, "<dbgetscr/>");
  babinfoxcom(com);

  return 1;
}

int dbsetscr(char *arg){
  char com[1024];
  memset(com, 0, sizeof(com));
  sprintf(com, "<dbsetscr/>");
  babinfoxcom(com);

  return 1;
}

void babinfoxcom(char *com){
  char buff[EB_BUFF_SIZE];
  int idx = 0, len;
  int sock;

  if(xmlel) bbxml_free(xmlel);

  len = strlen(com);
  memset(buff, 0, sizeof(buff));
  idx = sprintf(buff, "<?xml version=\"1.0\"?>\n");
  idx += sprintf(buff+idx, "<babinfoxcom>\n");
  memcpy(buff+idx, com, len);
  idx += len;
  idx += sprintf(buff+idx, "\n</babinfoxcom>\n");

  DB(printf("%s\n", buff));

  sock = infcon(ebhostname);
  send(sock, (char *)&idx, sizeof(idx), 0);
  send(sock, buff, idx, 0);
  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, buff, len, MSG_WAITALL);
  close(sock);
  buff[len] = 0;
  
  xmlel = bbxml_parsebuff(buff, len);
}

int setchkerhost(char *arg){
  char str[80];
  char *argv[10];
  int chkerhost, argc, sock, ret;

  DB(printf("babicon: setsetchkerhost %s\n", arg));
  argc = striparg(arg, argv);
  if(argc == 1){
    strncpy(str, argv[0], sizeof(str)-1);
  }else{
    bbjson_charobj("error", "need specify on off");
  }

  if(strcmp(str, "on") == 0){
    chkerhost = 1;
  }else if(strcmp(str, "off") == 0){
    chkerhost = 0;
  }else{
    bbjson_charobj("error", "need specify on off");
    return 0;
  }
    
  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_CHKERHOST, (char *)&chkerhost, sizeof(chkerhost));
  close(sock);
  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameter of chkerhost");
    return 0;
  }

  getchkerhost(NULL);

  return 1;
}

int dbgetdaqname(char *arg){
  //char this[] = "dbgetdaqname";
  BBXMLEL *exp=NULL, *id=NULL, *er=NULL, *babinfo=NULL;
  char com[1024];
  char js[272]={0};

  memset(com, 0, sizeof(com));
  sprintf(com, "<dbgetdaqname/>");
  babinfoxcom(com);

  if(!(babinfo = bbxml_node(xmlel, "babinfo"))){
    bbjson_charobj("error", "error xml, this is not babinfo xml");
  }

  if(!(exp = bbxml_node(babinfo, "daq"))){
    bbjson_charobj("error", "error in xml");
  }

  if((er = bbxml_node(exp, "error"))){
    sprintf(js, "Error : %s", er->text);
    bbjson_charobj("error", js);
  }
  
  if(!(id = bbxml_node(exp, "name"))){
    bbjson_charobj("daqname", "No Name");
  }else{
    bbjson_charobj("daqname", id->text);
  }
  return 1;
}

int dbsetdaqname(char *arg){
  //char this[] = "dbsetdaqname";
  int argc;
  char *argv[10];
  char str[64];
  char com[1024];
  char js[272]={0};
  BBXMLEL *exp=NULL, *babinfo=NULL, *er=NULL;

  argc = striparg(arg, argv);
  if(argc == 1){
    strncpy(str, argv[0], sizeof(str)-1);
  }else{
    bbjson_charobj("error", "dbsetdaqname need specify daqname");
    return 0;
  }

  memset(com, 0, sizeof(com));
  sprintf(com, "<dbsetdaqname><name>%s</name><server>%s</server></dbsetdaqname>", str, ebhostname);
  babinfoxcom(com);

  //bbxml_printall(xmlel);

  if(!(babinfo = bbxml_node(xmlel, "babinfo"))){
    bbjson_charobj("error", "error xml, this is not babinfo xml");
    return 0;
  }

  if(!(exp = bbxml_node(babinfo, "daq"))){
    bbjson_charobj("error", "error in xml");
    return 0;
  }

  if((er = bbxml_node(exp, "error"))){
    sprintf(js, "Error : %s", er->text);
    bbjson_charobj("error", js);
    return 0;
  }
  
  dbgetdaqname(NULL);

  return 1;
}


int setclinfo(char *arg){
  char this[] = "setclinfo";
  int argc;
  char *argv[10];
  int cln, fl;
  char ch1[80];
  int sock, ret;
  char js[272] = {0};
  int com, upf, id;
  enum comlist {ADD, DEL, ID};

  argc = striparg(arg, argv);

  if(argc != 2 && argc != 3){
    synopsis(this);
    return 0;
  }


  fl = 1;
  cln = 0;
  ret = 0;
  upf = 1;
  id = 0;

  cln = strtol(argv[0], NULL, 0);
  if(cln < 0 || cln >= MAXCLI){
    sprintf(js, "0 < CLIN < %d", MAXCLI);
    bbjson_charobj("error", js);
    return 0;
  }

  if((com = chkcom(argv[1], comsetclinfo)) < 0){
    sprintf(js, "Unknown command : %s", argv[1]);
    bbjson_charobj("error", js);
    synopsis(this);
    return 0;
  }

  switch(com){
  case ADD:
    if(argc != 3){
      fl = 0;
    }else{
      memset(ch1, 0, sizeof(ch1));
      strncpy(ch1, argv[2], sizeof(ch1)-1);
    }
    break;
  case ID:
    if(argc != 3){
      fl = 0;
    }else{
      id = strtol(argv[2], NULL, 0);
      if(id < 0 || id >= MAXANPT){
	sprintf(js, "Error : 0 <= ID < %d", MAXANPT);
	bbjson_charobj("error", js);
	fl = 0;
      }
    }
    break;
  }
    
  if(fl){
    sock = infcon(ebhostname);
    eb_get(sock, INF_GET_CLIHOSTS, (char *)clinfo);
    close(sock);
    
    switch(com){
    case ADD:
      clinfo[cln].ex = 1;
      strncpy(clinfo[cln].clihost, ch1,
		sizeof(clinfo[cln].clihost));
      break;
    case DEL:
      clinfo[cln].ex = 0;
      memset(clinfo[cln].clihost, 0, sizeof(clinfo[cln].clihost));
      break;
    case ID:
      clinfo[cln].ex = 1 + id;
      break;
    }

    if(upf){
      sock = infcon(ebhostname);
      ret = eb_set(sock, INF_SET_CLIHOST, (char *)clinfo,
		   sizeof(clinfo));
      close(sock);
      if(!ret){
	sprintf(js, "Run is now running, can't change parameters");
      }

      sock = infcon(ebhostname);
      eb_get(sock, INF_GET_CLIHOSTS, (char *)clinfo);
      close(sock);
    }

    show_clinfo();
  }else{
    synopsis(this);
  }


  return 1;
}


int settcpclinfo(char *arg){
  char this[] = "settcpclinfo";
  int argc;
  char *argv[10];
  //int cln, fl;
  //char ch1[80];
  //int sock, ret;
  //int com, upf, id;
  //enum comlist {ADD, DEL};

  argc = striparg(arg, argv);

  if(argc != 3){
    synopsis(this);
    return 0;
  }

  //printf("not implemented yet\n");

  /*
  fl = 1;
  cln = 0;
  ret = 0;
  upf = 1;
  id = 0;

  cln = strtol(argv[0], NULL, 0);
  if(cln < 0 || cln >= MAXCLI){
    printf("0 < CLIN < %d\n", MAXCLI);
    return 0;
  }

  if((com = chkcom(argv[1], comsetclinfo)) < 0){
    printf("Unknown command : %s \n", argv[1]);
    synopsis(this);
    return 0;
  }

  switch(com){
  case ADD:
    if(argc != 3){
      fl = 0;
    }else{
      memset(ch1, 0, sizeof(ch1));
      strncpy(ch1, argv[2], sizeof(ch1));
    }
    break;
  case ID:
    if(argc != 3){
      fl = 0;
    }else{
      id = strtol(argv[2], NULL, 0);
      if(id < 0 || id >= MAXANPT){
	printf("Error : 0 <= ID < %d\n\n", MAXANPT);
	fl = 0;
      }
    }
    break;
  }
    
  if(fl){
    sock = infcon(ebhostname);
    eb_get(sock, INF_GET_CLIHOSTS, (char *)clinfo);
    close(sock);
    
    switch(com){
    case ADD:
      clinfo[cln].ex = 1;
      strncpy(clinfo[cln].clihost, ch1,
		sizeof(clinfo[cln].clihost));
      break;
    case DEL:
      clinfo[cln].ex = 0;
      memset(clinfo[cln].clihost, 0, sizeof(clinfo[cln].clihost));
      break;
    case ID:
      clinfo[cln].ex = 1 + id;
      break;
    }

    if(upf){
      sock = infcon(ebhostname);
      ret = eb_set(sock, INF_SET_CLIHOST, (char *)clinfo,
		   sizeof(clinfo));
      close(sock);
      if(!ret){
	printf("Run is now running, can't change parameters\n");
      }

      sock = infcon(ebhostname);
      eb_get(sock, INF_GET_CLIHOSTS, (char *)clinfo);
      close(sock);
    }

    show_clinfo();
  }else{
    synopsis(this);
  }
  */

  return 1;
}

int delscr(char *arg){
  int id, tdx = -1;
  int sock, len;
  char js[272] = {0};

  if(!strlen(arg)){
    sprintf(js, "delscr SCRID");
    bbjson_charobj("error", js);
    return 0;
  }

  id = strtol(arg, NULL, 0);
  if(id < 0){
    sprintf(js, "0 < scrid");
    bbjson_charobj("error", js);
    return 0;
  }

  getscrlist(NULL);

  if((tdx = fndscrid(id)) < 0){
    sprintf(js, "Not exist such scaler=%d", id);
    bbjson_charobj("error", js);
    return 0;
  }

  if(!(sock = infcon(ebhostname))) return 0;
  len = sizeof(id);
  eb_set(sock, INF_DEL_SCR, (char *)&id, len);
  close(sock);
  
  return 1;
}

int gettgig(char *arg){
  int tgig=0;
  int sock;

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_TGIG, (char *)&tgig);
  close(sock);

  if(tgig < 0){
    tgig = 0;
  }
  bbjson_intobj_single("tgigmode", tgig);

  return 1;
}


int settgig(char *arg){
  char str[80];
  char *argv[10];
  int tgig, argc, sock, ret;

  DB(printf("babicon: setsettgig %s\n", arg));
  argc = striparg(arg, argv);
  if(argc == 1){
    strncpy(str, argv[0], sizeof(str)-1);
  }else{
    bbjson_charobj("error", "need specify 0--10 (0 is off)");
  }

  tgig = strtol(str, NULL, 0);

  if(tgig < 0 || tgig > 10){
    bbjson_charobj("error", "tgig 0--10 (0 is off)");
    return 0;
  }
    
  sock = ebcon(ebhostname);
  ret = eb_set(sock, EB_SET_TGIG, (char *)&tgig, sizeof(tgig));
  close(sock);
  if(!ret){
    bbjson_charobj("error", "Run is now running, can't change parameter of tgig");
    return 0;
  }

  gettgig(NULL);

  return 1;
}

int show_efblkn(unsigned int blkn[MAXEF]){
  int i;

  bbjson_begin_obj(0);
  for(i=0;i<MAXEF;i++){
    if(blkn[i]){
      bbjson_intobj("efn", i);
      bbjson_intobj("blkn", blkn[i]);
    }
  }
  bbjson_end_obj();
  return 1;
}

int getefblkn(char *arg){
  int sock;
  unsigned int blkn[MAXEF];

  sock = ebcon(ebhostname);
  eb_get(sock, EB_GET_EFBLKN, (char *)blkn);
  close(sock);

  show_efblkn(blkn);

  return 1;
}

