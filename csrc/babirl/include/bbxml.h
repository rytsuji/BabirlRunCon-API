/** Header file for xml */

#define BBXML_TAGMAX  64
#define BBXML_TEXTMAX 256

typedef struct bbxmlst BBXMLEL;
typedef struct bbxmlcomst BBXMLCOM;

struct bbxmlst{
  char tag[BBXML_TAGMAX];
  char text[BBXML_TEXTMAX];
  //struct bbattrst *attr;
  struct bbxmlst *cont;
  struct bbxmlst *next;
  struct bbxmlst *parentNode;
};

struct bbattrst{
  char attr[BBXML_TAGMAX];
  char text[BBXML_TEXTMAX];
  struct bbattrst *next;
};

struct bbxmlcomst{
  char *name;
  int (*func)(BBXMLEL *, BBXMLEL *);
};

#define XMLVERSION  "1.0"

/* prototype */
BBXMLEL *bbxml_createElement(char *, char *);
BBXMLEL *bbxml_appendChild(BBXMLEL *, BBXMLEL *);
BBXMLEL *bbxml_DomDocument(void);
void bbxml_printall(BBXMLEL *);
int bbxml_sprintall(BBXMLEL *, char *ret);
int bbxml_strcat(BBXMLEL *, char *);
void bbxml_free(BBXMLEL *);
BBXMLEL *bbxml_node(BBXMLEL *, char *);
BBXMLEL *bbxml_next(BBXMLEL *, char *);
BBXMLEL *bbxml_parsebuff(char *, int);
char *bbxml_getTextByTagName(BBXMLEL *, char *);
