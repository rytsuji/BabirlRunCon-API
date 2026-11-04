/** bb xml libraly */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <expat.h>
#include <bbxml.h>

//#define DEBUG
/* macros */
#ifdef DEBUG
#define DB(x) x
#else
#define DB(x)
#endif

static BBXMLEL *local_dom = NULL;
static char stag[BBXML_TAGMAX];
static char stext[BBXML_TEXTMAX];

BBXMLEL *bbxml_createElement(char *tag, char *text){
  BBXMLEL *this;

  this = (BBXMLEL *)calloc(1, sizeof(BBXMLEL));

  strncpy(this->tag, tag, BBXML_TAGMAX-1);
  strncpy(this->text, text, BBXML_TEXTMAX-1);

  DB(printf("el tag:%s, text:%s\n", this->tag, this->text));

  return this;
}

BBXMLEL *bbxml_appendChild(BBXMLEL *parent, BBXMLEL *child){
  BBXMLEL *p;

  if(strlen(child->tag)){
    child->parentNode = parent;
    if(!parent->cont){
      DB(printf("append cont (tag=%s)\n", child->tag));
      parent->cont = child;
    }else{
      p = parent->cont;
      while(p->next){
	p = p->next;
      }
      p->next = child;
      //printf("%s->next = %p\n", p->tag, child);
    }
    return child;
  }else if(strlen(child->text)){
    strcpy(parent->text, child->text);
    free(child);
    return parent;
  }

  return 0;
}


BBXMLEL *bbxml_DomDocument(void){

  return (BBXMLEL *)calloc(1, sizeof(BBXMLEL));
}

void bbxml_printall(BBXMLEL *first){
  char sp[BBXML_TAGMAX];
  BBXMLEL *p = first->cont;
  int idx = 0, spf = 0;

  printf("<?xml version=\"%s\"?>\n", XMLVERSION);
  memset(sp, 0, sizeof(sp));

  while(p){
    spf = 0;
    if(!strlen(p->text) && !p->cont && strlen(p->tag)){
      printf("%s<%s/>\n", sp, p->tag);
      spf = 1;
    }else{
      //printf("len=%d, tag=%s, text=%s\n", strlen(p->tag), p->tag, p->text);
      if(strlen(p->tag)){
	printf("%s<%s>%s", sp, p->tag, p->text);
      }
    }

    if(p->cont){
      //printf("there is pcont p=%p\n", p->cont);
      p = p->cont;
      idx += 2;
      memset(sp, ' ', idx);
      printf("\n");
    }else if(p->next){
      if(!spf) printf("</%s>\n", p->tag);
      p = p->next;
    }else{
      if(!spf) printf("</%s>\n", p->tag);
      //printf("go to parent p=%p\n", p->parentNode);
      idx -= 2;
      if(idx < 0) idx = 0;
      memset(sp, 0, sizeof(sp));
      if(idx > 0) memset(sp, ' ', idx);
      if(strlen(p->parentNode->tag)){
	printf("%s</%s>\n", sp, p->parentNode->tag);
      }
      while(!p->parentNode->next){
	idx -= 2;
	if(idx < 0) idx = 0;
	memset(sp, 0, sizeof(sp));
	if(idx > 0) memset(sp, ' ', idx);
	p = p->parentNode;
	if(!p->parentNode) break;
	if(strlen(p->parentNode->tag)){
	  printf("%s</%s>\n", sp, p->parentNode->tag);
	}
      }
      if(p->parentNode){
	p = p->parentNode->next;
      }else{
	p = NULL;
      }
      //printf("go to parent->next p=%p\n", p);
    }
  }

}


int bbxml_sprintall(BBXMLEL *first, char *ret){
  char sp[BBXML_TAGMAX];
  BBXMLEL *p = first->cont;
  int idx = 0, spf = 0, ridx = 0;

  ridx += sprintf(ret+ridx, "<?xml version=\"%s\"?>\n", XMLVERSION);
  memset(sp, 0, sizeof(sp));

  while(p){
    spf = 0;
    if(!strlen(p->text) && !p->cont && strlen(p->tag)){
      ridx += sprintf(ret+ridx, "%s<%s/>\n", sp, p->tag);
      spf = 1;
    }else{
      //ridx += sprintf(ret+ridx, "len=%d, tag=%s, text=%s\n", strlen(p->tag), p->tag, p->text);
      if(strlen(p->tag)){
	ridx += sprintf(ret+ridx, "%s<%s>%s", sp, p->tag, p->text);
      }
    }

    if(p->cont){
      //ridx += sprintf(ret+ridx, "there is pcont p=%p\n", p->cont);
      p = p->cont;
      idx += 2;
      memset(sp, ' ', idx);
      ridx += sprintf(ret+ridx, "\n");
    }else if(p->next){
      if(!spf) ridx += sprintf(ret+ridx, "</%s>\n", p->tag);
      p = p->next;
    }else{
      if(!spf) ridx += sprintf(ret+ridx, "</%s>\n", p->tag);
      //ridx += sprintf(ret+ridx, "go to parent p=%p\n", p->parentNode);
      idx -= 2;
      if(idx < 0) idx = 0;
      memset(sp, 0, sizeof(sp));
      if(idx > 0) memset(sp, ' ', idx);
      if(strlen(p->parentNode->tag)){
	ridx += sprintf(ret+ridx, "%s</%s>\n", sp, p->parentNode->tag);
      }
      while(!p->parentNode->next){
	idx -= 2;
	if(idx < 0) idx = 0;
	memset(sp, 0, sizeof(sp));
	if(idx > 0) memset(sp, ' ', idx);
	p = p->parentNode;
	if(!p->parentNode) break;
	if(strlen(p->parentNode->tag)){
	  ridx += sprintf(ret+ridx, "%s</%s>\n", sp, p->parentNode->tag);
	}
      }
      if(p->parentNode){
	p = p->parentNode->next;
      }else{
	p = NULL;
      }
      //ridx += sprintf(ret+ridx, "go to parent->next p=%p\n", p);
    }
  }

  return ridx;
}


void bbxml_free(BBXMLEL *first){
  BBXMLEL *p = first->cont, *f, *g;

  while(p){
    if(p->cont){
      p = p->cont;
    }else if(p->next){
      f = p;
      p = p->next;
      free(f);
    }else{
      f = p;
      g = p->parentNode;
      p = p->parentNode->next;
      if(g != first) free(g);
      free(f);
    }
  }
  free(first);
  first = NULL;
}


BBXMLEL *bbxml_node(BBXMLEL *o, char *tag){
  BBXMLEL *p;

  if(!o) return 0;

  p = o->cont;
  while(p){
    if(!strcmp(p->tag, tag)){
      return p;
    }

    p = p->next;
  }

  return 0;
}

BBXMLEL *bbxml_next(BBXMLEL *o, char *tag){
  BBXMLEL *p;

  if(!o) return 0;

  p = o->next;
  while(p){
    if(!strcmp(p->tag, tag)){
      return p;
    }

    p = p->next;
  }

  return 0;
}


void bbxml_elstart(void *data, const XML_Char *name, const XML_Char *atts[]){
  memset(stag, 0, sizeof(stag));
  memset(stext, 0, sizeof(stext));
  strncpy(stag, name, sizeof(stag)-1);
  local_dom = bbxml_appendChild(local_dom, bbxml_createElement(stag, ""));
}

void bbxml_elend(void *data, const XML_Char *name){
  local_dom = local_dom->parentNode;
}

void bbxml_elch(void *data, const XML_Char *name, int len){
  strncpy(stext, name, len);
  stext[len] = 0;
  if(stext[0] != '\n' && stext[0] != ' '){
    bbxml_appendChild(local_dom, bbxml_createElement("", stext));
  }
}

/** Search Tag and return its Text.
 *  @param o Pointer of parent XML element
 *  @param tag Tag name
 *  @return Text of Tag
 */
char *bbxml_getTextByTagName(BBXMLEL *o, char *tag){
  BBXMLEL *p;

  if(!o) return 0;

  p = o->cont;
  while(p){
    //printf("%s %s\n", p->tag, p->text);

    if(!strcmp(p->tag, tag)){
      return p->text;
    }

    if(p->cont){
      p = p->cont;
    }else if(!p->next){
      p = p->parentNode->next;
    }else{
      p = p->next;
    }
  }

  return NULL;
}


BBXMLEL *bbxml_parsebuff(char *buff, int size){
  XML_Parser parser;
  int idx = 0, len, flag = 0;

  parser = XML_ParserCreate(NULL);
  XML_SetElementHandler(parser, bbxml_elstart, bbxml_elend);
  XML_SetCharacterDataHandler(parser, bbxml_elch);

  local_dom = bbxml_DomDocument();
  DB(printf("bbxml_parsebuff size=%d\n", size));
  while(idx < size){
    len = 1024;
    if(idx + len > size){
      len = size - idx;
    }
    DB(printf("len=%d idx=%d size=%d\n", len, idx, size));

    XML_Parse(parser, buff+idx, len, flag);

    idx += len;
    if(!flag && idx >= size) break;
  }
  
  XML_ParserFree(parser);

  return local_dom;
}


