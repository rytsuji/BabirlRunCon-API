/* Hdur: lib/bi-com.c
 * last modified : 06/03/14 16:07:07 
 * Hidetada Baba (CNS Tokyo University)
 * baba@cns.s.u-tokyo.ac.jp
 *
 * Terminal Controller library
 *  (Original: documentation of 'The GNU Readline Library')
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

typedef struct {
  char *name;
  rl_icpfunc_t *func;
  char *doc;
} COMMAND;

COMMAND commands[0];

char *stripwhite();
COMMAND *find_command();

int initialize_readline();
int done;

char *dupstr(char *s){
  char *r;
  r = malloc(strlen(s) + 1);
  strcpy (r,s);
  return (r);
}

/* initialize command */
int init_hcom(COMMAND com[]){
  commands[0] = com[0];
  return initialize_readline();
}
     
/* Execute a command line. */
int execute_line(char *line){
  register int i;
  COMMAND *command;
  char *word;
  
  i = 0;
  while(line[i] && whitespace (line[i])){
    i++;
  }
  word = line + i;
  
  while(line[i] && !whitespace (line[i])){
    i++;
  }
  
  if(line[i]) line[i++] = '\0';
  
  command = find_command(word);
  
  if(!command){
    fprintf(stderr, "%s: No such command\n", word);
    return -1;
  }
  
  while(whitespace(line[i])){
    i++;
  }
  
  word = line + i;
  
  return ((*(command->func))(word));
}

/* Execute a command line. */
int execute_line_noerr(char *line){
  register int i;
  COMMAND *command;
  char *word;
  
  i = 0;
  while(line[i] && whitespace (line[i])){
    i++;
  }
  word = line + i;
  
  while(line[i] && !whitespace (line[i])){
    i++;
  }
  
  if(line[i]) line[i++] = '\0';
  
  command = find_command(word);
  
  if(!command){
    return -1;
  }
  
  while(whitespace(line[i])){
    i++;
  }
  
  word = line + i;
  
  return ((*(command->func))(word));
}

COMMAND *find_command(char *name){
  register int i;
  
  for (i = 0;commands[i].name;i++){
    if(strcmp(name,commands[i].name) == 0){
      return &commands[i];
    }
  }
  
  return ((COMMAND *)NULL);
}

char *stripwhite(char *string){
  register char *s, *t;
  
  for(s=string;whitespace(*s);s++){}
  
  if(*s == 0){
    return s;
  }
  
  t = s + strlen(s) - 1;
  while(t > s && whitespace (*t)){
    t--;
  }
  *++t = '\0';
  
  return s;
}

int striparg(char *arg, char *argv[]){
  int argc = 0, i;
  char *word;

  //len = strlen(arg);

  while(argc < 10 && arg != NULL){
    arg = stripwhite(arg);
    i = 0;
    while(arg[i] && whitespace (arg[i])){
      i++;
    }
    word = arg + i;
    while(arg[i] && !whitespace (arg[i])){
      i++;
    }
    if(arg[i]) arg[i++] = '\0';
    
    if(i > 0 && arg != NULL){
      argv[argc++] = word;
    }else{
      break;
    }
    arg = arg + i;
  }

  return argc;
}

char *command_generator __P((const char *, int));
char **fileman_completion __P((const char *, int, int));

int initialize_readline (){
  rl_attempted_completion_function = fileman_completion;
  rl_completion_entry_function = command_generator;
  return 0;
}

char **fileman_completion(const char *text, int start, int end){
  char **matches;
  
  matches = (char **)NULL;
  
  if (start == 0){
    matches = rl_completion_matches(text, command_generator);
  }

  return matches;
}

char *command_generator(const char *text, int state){
  static int list_index, len;
  char *name;

  if(!state){
    list_index = 0;
    len = strlen (text);
  }
  
  while((name = commands[list_index].name)){
    list_index++;

    if(strncmp(name, text, len) == 0)
      return dupstr(name);
  }
  
  return ((char *)NULL);
}
