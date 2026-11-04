/* lib/bi-tcp.c
 *
 * last modified : 13/11/02 15:56:59 
 *
 * babirl network library
 * not only TCP but also UDP
 *
 * Hidetada Baba (RIKEN)
 * baba@ribf.riken.jp
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <bi-config.h>
#include <bi-common.h>

/** Make TCP server socket (receiver), automatically bind and listen.
 *  Return socket number.
 *  @param port port number should be binded
 *  @return socket number
 */
int mktcpsock(unsigned short port){
  int sock = 0;
  int sockopt = 1;
  struct sockaddr_in saddr;

  memset((char *)&saddr,0,sizeof(saddr)); 
  if((sock = socket(PF_INET,SOCK_STREAM,0)) < 0){ 
    perror("bi-tcp.mktcpsock: Can't make socket.\n"); 
    return 0;
  }
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  saddr.sin_family = AF_INET; 
  saddr.sin_addr.s_addr = INADDR_ANY; 
  saddr.sin_port = htons(port); 
  if(bind(sock,(struct sockaddr *)&saddr,sizeof(saddr)) < 0){ 
    perror("bi-tcp.mktcpsock: Can't bind socket.\n"); 
    return 0;
  }
  if(listen(sock,100) < 0){
    perror("bi-tcp.mktcpsock: Can't listen socket.");
    return 0;
  }

  return sock;
}

/** Make TCP client socket (sender), automatically connect.
 *  Return socket number.
 *  @param host server hostname
 *  @param port port number should be connected
 *  @return socket number
 */
int mktcpsend(char *host, unsigned short port){
  int sock = 0;
  struct hostent *hp;
  struct sockaddr_in saddr;
  int sockopt = 1;

  if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0){
    perror("bi-tcp.mktcpsend: Can't make socket.\n");
    return 0;
  }

  memset((char *)&saddr,0,sizeof(saddr));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  if((hp = gethostbyname(host)) == NULL){
    fprintf(stderr, "bi-tcp.mktcpsend : No such host (%s)\n", host);
    return 0;
  }

  memcpy(&saddr.sin_addr,hp->h_addr,hp->h_length);
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);

  //DB(printf("host : %s  port : %d\n",host,port);)

  if(connect(sock,(struct sockaddr *)&saddr,sizeof(saddr)) < 0){
    perror("bi-tcp.mktcpsend: Error in tcp connect.\n");
    //close(sock);
    return 0;
  }

  return sock;
}

int mktcpsend_tout(char *host, unsigned short port, int tout){
#ifdef NOTOUT
  return mktcpsend(host, port);
#else
  int sock = 0, ret;
  struct hostent *hp;
  struct sockaddr_in saddr;
  fd_set set;
  struct timeval tv;
  int sockopt = 1;

  if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0){
    perror("bi-tcp.mktcpsend: Can't make socket.\n");
    return 0;
  }

  memset((char *)&saddr,0,sizeof(saddr));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  if((hp = gethostbyname(host)) == NULL){
    printf("bi-tcp.mktcpsend : No such host (%s)\n", host);
    return 0;
  }

  memcpy(&saddr.sin_addr,hp->h_addr,hp->h_length);
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0){
    DB(printf("error in fcntl to nonblock\n"));
    return 0;
  }
  //DB(printf("host : %s  port : %d\n",host,port);)

  ret = connect(sock,(struct sockaddr *)&saddr,sizeof(saddr));
  DB(printf("ret1 = %d\n", ret));
  if(ret < 0){
    tv.tv_sec = tout;
    tv.tv_usec = 0; 
    FD_ZERO(&set); FD_SET(sock, &set);
    ret = select(sock+1, NULL, &set, NULL, &tv);
    DB(printf("ret2 = %d\n", ret));
    if(ret <= 0){
      DB(printf("bi-tcp.mktcpsend: Error in tcp connect.\n"));
      //close(sock);
      return 0;
    }else{
      /* connect OK */
      ret = connect(sock,(struct sockaddr *)&saddr,sizeof(saddr));
      DB(printf("ret3 = %d\n", ret));
      if(ret < 0){
	//close(sock);
	return 0;
      }
    }
  }
  fcntl(sock, F_SETFL, 0);

  return sock;
#endif
}

// no block slave
int mktcpsend_noblk(char *host, unsigned short port){
  int sock = 0, ret;
  struct hostent *hp;
  struct sockaddr_in saddr;
  int sockopt = 1;

  if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0){
    perror("bi-tcp.mktcpsend: Can't make socket.\n");
    return 0;
  }

  memset((char *)&saddr,0,sizeof(saddr));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  if((hp = gethostbyname(host)) == NULL){
    //printf("bi-tcp.mktcpsend : No such host (%s)\n", host);
    return 0;
  }

  memcpy(&saddr.sin_addr,hp->h_addr,hp->h_length);
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0){
    //DB(printf("error in fcntl to nonblock\n"));
    return 0;
  }
  //DB(printf("host : %s  port : %d\n",host,port);)

  ret = connect(sock,(struct sockaddr *)&saddr,sizeof(saddr));
  if(ret < 0){
    return 0;
  }
  return sock;
}


/** Make UDP server socket, automatically binded.
 *  Return socket number.
 *  @param port port number should be connected
 *  @param pointer for saddr sockaddr_in
 *  @return socket number
 */
int mkudpsock(int port, struct sockaddr_in *saddr){
  int sock = 0;
  int sockopt = 1;

  if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
    perror("mkudpsock: can't make udp socket\n");
    return 0;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  saddr->sin_family = AF_INET;
  saddr->sin_addr.s_addr = INADDR_ANY;
  saddr->sin_port = htons(port);
  if(bind(sock, (struct sockaddr *)saddr, sizeof(struct sockaddr_in)) < 0){
// for checkin error message. KY
//    int errbind = errno;
//    char *str = strerror(errbind);
//    perror(*str);
//
    perror("mkudpsock: can't bind socket.\n");
    return 0;
  }

  return sock;
}

/** Make UDP client socket.
 *  Return socket number.
 *  @param port port number should be connected
 *  @param pointer for caddr sockaddr_in
 *  @param hostname of server
 *  @return socket number
 */
int mkudpsend(int port, struct sockaddr_in *caddr, char *hostname){
  int sock = 0;
  int sockopt = 1;
  struct hostent *hp;

  if((sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
    perror("mkudpsend: Can't make udp socket.\n");
    return 0;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
       &sockopt, sizeof(sockopt));

  if((hp = gethostbyname(hostname)) == NULL){
    return 0;
  }else{
    memcpy((char *)&caddr->sin_addr, (char *)hp->h_addr, hp->h_length);
    caddr->sin_family = AF_INET;
    caddr->sin_port = htons(port);
  }

  return sock;
}

/** Make UDP multisender socket.
 *  Return socket number.
 *  @return socket number
 */
int mkmultisend(void){
  int sock = 0;
  int sockopt = 1;

  if((sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
    perror("mkudpsend: Can't make udp socket.\n");
    return 0;
  }
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
	     &sockopt, sizeof(sockopt));

  return sock;
}

/** Register client address for multisender.
 *  Return 1 = suceed, 0 = failed.
 *  @param port Port number of server
 *  @param caddr sockaddr_in
 *  @param host hostname of server
 *  @return 1 = sucdeed, 0 = failed
 */
int registmultisend(int port, struct sockaddr_in *caddr, char *hostname){
  struct hostent *hp;

  if((hp = gethostbyname(hostname)) == NULL){
    return 0;
  }else{
    memcpy((char *)&caddr->sin_addr, (char *)hp->h_addr, hp->h_length);
    caddr->sin_family = AF_INET;
    caddr->sin_port = htons(port);
  }

  return 1;
}


/** Get parameters from babild or babinfo.
 *  @param sock Socket number
 *  @param com  Command
 *  @param dest Buffer address
 *  @return Size of obtained buffer (char size)
 */
int eb_get(int sock, int com, char *dest){
  int len;

  len = sizeof(com);
  send(sock, (char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, len, 0);

  recv(sock, (char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, dest, len, MSG_WAITALL);

  //printf("bi-tcp: com=%d, len=%d\n", com, len);

  return len;
}

int eb_set(int sock, int com, char *src, int size){
  int len, ret;

  len = sizeof(com) + size;
  send(sock,(char *)&len, sizeof(len), 0);
  send(sock, (char *)&com, sizeof(com), 0);
  send(sock, src, size, 0);

  recv(sock,(char *)&len, sizeof(len), MSG_WAITALL);
  recv(sock, (char *)&ret, len, MSG_WAITALL);

  return ret;
}

int eb_set_eflist(int sock, char *eflist, int efsize){
  int val, len;
  char buff[EB_BUFF_SIZE];

  len = 0;
  val = EB_SET_EFLIST;
  memcpy(buff, (char *)&val, sizeof(val));
  len += sizeof(val);
  memcpy(buff+len, eflist, efsize);
  len += efsize;
  
  send(sock,(char *)&len, sizeof(int), 0);
  send(sock, buff, len, 0);
  recv(sock,(char *)&len, sizeof(int), MSG_WAITALL);
  recv(sock, buff,len, MSG_WAITALL);

  return 0;
}

/** Retrun this hostname and ipaddress
 *  @param host   Char pointer Hostname
 *  @param domain Char pointer Domainname
 *  @param ipaddr Char pointer IP Address
 *  @return 0=only local host , 1=host name
 */
int getmyaddress(char *host, char *domain, char *ipaddr){
  char lhost[256];
  char tmp[256];
  struct hostent *hs;
  struct in_addr **in;

  if(gethostname(lhost, sizeof(lhost))){
    strcpy(host, "localhost");
    strcpy(domain, "localhost.localdomain");
    strcpy(ipaddr, "127.0.0.1");
    return 0;
  }

  strcpy(host, lhost);
  hs = gethostbyname(host);

  if(getdomainname(tmp, sizeof(tmp))){
    domain[0] = 0;
  }else{
    //sprintf(domain, "%s.%s", host, domain);
    strcat(domain, ".");
    strcat(domain, host);
  }

  if(hs){
    in = (struct in_addr **)hs->h_addr_list;
    sprintf(ipaddr, "%s", inet_ntoa(*in[0]));
  }else{
    strcpy(ipaddr, "127.0.0.1");
  }

  return 1;
}

/** Retrun default interface 
 *  @param ifname Char pointer interface name
 *  @return 0 = error, 1 = vaild
 */
int getdefinterface(char *ifname){
  FILE *f;
  char line[100], *p, *c;
  int ret = 0;

  if(!(f = fopen("/proc/net/route", "r"))){
    return 0;
  }

  while(fgets(line, 100, f)){
    p = strtok(line, "\t");
    c = strtok(NULL, "\t");
    if(p != NULL && c != NULL){
      if(strcmp(c, "00000000") == 0){
	snprintf(ifname, IFNAMSIZ-1, "%s", p);
	ret = 1;
	break;
      }
    }
  }

  fclose(f);

  return ret;
}

/** Return mac address
 *  @param mac Char pointer mac address to be stored
 *  @return 0 = error, 1 = valid
 */
int getmacbysock(int sock, char *mac){
  char ifname[IFNAMSIZ] = {0};
  struct ifreq ifr;
  
  if(!getdefinterface(ifname)){
    return 0;
  }

  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  ioctl(sock, SIOCGIFHWADDR, &ifr);

  memcpy(mac, (char *)ifr.ifr_hwaddr.sa_data, 6);

  return 1;
}

/** Return ip address
 *  @param IP Char pointer mac address to be stored
 *  @return 0 = error, 1 = valid
 */
int getipbysock(int sock, char *ip){
  char ifname[IFNAMSIZ] = {0};
  struct ifreq ifr;
  struct sockaddr_in addr;
  
  if(!getdefinterface(ifname)){
    return 0;
  }

  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  ioctl(sock, SIOCGIFADDR, &ifr);

  memcpy(&addr, &ifr.ifr_addr, sizeof(addr));
  memcpy(ip, &addr.sin_addr, 4);

  return 1;
}

/** Make UDP broadcast socket.
 *  Return socket number.
 *  @param port port number should be connected
 *  @param pointer for caddr sockaddr_in
 *  @param hostname of server
 *  @return socket number
 */
int mkbroadsend(int port, struct sockaddr_in *caddr){
  int sock = 0;
  int sockopt = 1;
  if((sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
    perror("mkbroadsend: Can't make udp socket.\n");
    return 0;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, 
       &sockopt, sizeof(sockopt));

  caddr->sin_family = AF_INET;
  caddr->sin_port = htons(port);
  caddr->sin_addr.s_addr = inet_addr("255.255.255.255");
  //caddr->sin_addr.s_addr = inet_addr("172.27.224.255");
  //if(bind(sock, (struct sockaddr *)caddr, sizeof(struct sockaddr_in)) < 0){
  //printf("Cannot bind UDP sock\n");
  //}

  return sock;
}

void getipfromsockaddr(struct sockaddr_in caddr, unsigned char *ip){
  unsigned int v;
  v = caddr.sin_addr.s_addr;
  ip[0] = v & 0xff;
  ip[1] = (v >> 8) & 0xff;
  ip[2] = (v >> 16) & 0xff;
  ip[3] = (v >> 24) & 0xff;
}
