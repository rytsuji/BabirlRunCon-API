#include <stdio.h>
#include <string.h>
#include <bbjson.h>

static int cidx=0, lidx=0;
static char safeout[1024];

char *bbjson_safechar(char *in){
  int len, i, max, idx=0;
  
  len = strlen(in);
  if(len > sizeof(safeout) - 2){
    len = sizeof(safeout) - 2;
  }
  max = sizeof(safeout) - 2;

  for(i=0;i<len;i++){
    switch(in[i]){
    case '"':
      safeout[idx] = '\\';
      idx ++;
      safeout[idx] = in[i];
      idx ++;
      break;
    case '\\':
      safeout[idx] = '\\';
      idx ++;
      safeout[idx] = in[i];
      idx ++;
      break;
    case '/':
      safeout[idx] = '\\';
      idx ++;
      safeout[idx] = in[i];
      idx ++;
      break;
    case '\n':
      safeout[idx] = '\\';
      idx ++;
      safeout[idx] = 'n';
      idx ++;
      break;
    case '\r':
      safeout[idx] = '\\';
      idx ++;
      safeout[idx] = 'r';
      idx ++;
      break;
    case '\t':
      safeout[idx] = ' ';
      idx ++;
      break;
    case '\b':
      safeout[idx] = ' ';
      idx ++;
      break;
    case '\f':
      safeout[idx] = ' ';
      idx ++;
      break;
    default:
      safeout[idx] = in[i];
      idx ++;
      break;
    }
    
    if(idx >= max){
      break;
    }
  }
  safeout[idx] = 0;

  return safeout;
}

void putidx(void){
  int i;
  for(i=0;i<cidx;i++){
    printf("  ");
  }
}

void bbjson_begin(void){
  cidx = 0;
  lidx = -1;
  bbjson_begin_obj(NULL);
}

void bbjson_begin_obj(char *key){
  if(cidx <= lidx){
    printf(",\n");
  }else if(lidx != -1){
    printf("\n");
  }
  lidx = cidx;

  putidx();
  if(!key){
    printf("{");
  }else{
    printf("\"%s\": {", key);
  }
  cidx ++;
}

void bbjson_charobj_single(char *key, char *val){
  if(cidx <= lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  if(val){
    printf("\"%s\"", bbjson_safechar(val));
  }
  
}

void bbjson_charobj(char *key, char *val){
  if(cidx == lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  if(val){
    printf("\"%s\"", bbjson_safechar(val));
  }
  
}

void bbjson_intobj_single(char *key, int val){
  if(cidx <= lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  printf("%d", val);
}

void bbjson_intobj(char *key, int val){
  if(cidx == lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  printf("%d", val);
}

void bbjson_uintobj(char *key, unsigned int val){
  if(cidx == lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  printf("%u", val);
}

void bbjson_lluintobj(char *key, unsigned long long int val){
  if(cidx == lidx){
    printf(",\n");
  }else{
    printf("\n");
    lidx = cidx;
  }
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  printf("%Lu", val);
}


void bbjson_end_obj(void){
  printf("\n");
  cidx --;
  putidx();
  printf("}");
}

void bbjson_begin_array(char *key){
  if(cidx <= lidx){
    printf(",\n");
  }else{
    printf("\n");
  }
  lidx = cidx;
  putidx();
  if(key){
    printf("\"%s\": ", key);
  }
  printf("[");
  lidx--;
}
void bbjson_end_array(void){
  if(lidx < cidx){
    lidx = cidx;
    printf("\n");
  }
  putidx();
  printf("]");
}

void bbjson_end(void){
  bbjson_end_obj();
  cidx = 0 ;
  lidx = 0;
  printf("\n");
}
