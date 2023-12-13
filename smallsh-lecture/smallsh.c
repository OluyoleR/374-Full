#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>


#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
char* new_arr[MAX_WORDS] = {0};
size_t wordsplit(char const *line);
char * expand(char const *word);

int is_builtin(char * cmd);
int status_lfg,bgpid ;
int main(int argc, char *argv[])
{
  FILE *input = stdin;
  char *input_fn = "(stdin)";
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  char *line = NULL;
  size_t n = 0;
  for (;;) {
prompt:;
    /* TODO: Manage background processes */

    /* TODO: prompt */
    if (input == stdin) {
      char * prompt = getenv("PS1");
      prompt ? fprintf(stderr,"%s",prompt):fprintf(stderr,"%s","#");
    }
    errno = 0 ; 
    ssize_t line_len = getline(&line, &n, input);
    
    if (line_len < 0){
      if (errno > 0 )
          err(1,"%s","Error reading input ");    
      else{
        break; 
      }
    }
    
    size_t nwords = wordsplit(line);
    char * infile = NULL;
    char * outfile = NULL ;
    if (nwords == 0) goto prompt;//continue ; // move on to next available command 
     
    for (size_t i = 0; i < nwords; ++i) {
      //fprintf(stderr, "Word %zu: %s\n", i, words[i]);
      char *exp_word = expand(words[i]);
      //free(words[i]);
      words[i] = exp_word;
      //fprintf(stderr, "Expanded Word %zu: %s\n", i, words[i]);
    }
    for (size_t i = 0; i < nwords; ++i) {
        if (strcmp("<",words[i]) == 0 )
            infile = words[i+1];
        if (strcmp(">",words[i]) == 0 )
            outfile = words[i+1];
     }
    

    words[nwords] = NULL;
    if (nwords == 1 && is_builtin(words[0]))
    { // cd or exit used w/no argument 
        if (strcmp(words[0],"cd")==0){
            chdir(getenv("HOME"));
        }
        if (strcmp(words[0],"exit")==0){
            exit(status_lfg);
        }
    }
    else if (nwords >= 2 && is_builtin(words[0]))
    { // cd or exit used with an argument   
        if (nwords > 2)
           err(1,"%s","Too many arguments for chosen command");

        if (strcmp(words[0],"cd")==0){
            int newdir = chdir(words[1]);
            if (newdir < 0 ){
               err(1,"%s","could not find dir");
            }
        }
        if (strcmp(words[0],"exit")==0){
            exit(atoi(words[1]));
        }
    }
    else
    { // fork 
        pid_t newpid = -5; // default to detect if something went wrong
        newpid = fork();
        switch(newpid){

           case -1 :
              err(1,"%s","Unable to fork");


          case 0 :{
              
              if(infile){ 
              char * new_args[512] = {0};
              int x = 0 ;
              char * pathname = NULL;
              for(size_t i= 0 ; i < 512 ; ++i){
                  if (strcmp(words[i],"<") == 0){
                      pathname = words[i+1];
                  } else{
                      new_args[x] = words[i];
                      x += 1 ;
                  }
              }
              if (pathname){
              new_args[x+1]= NULL;

              int open_file = open(pathname,O_RDONLY);
              if(open_file  < 0 ) 
                err(1,"%s","redirection file unavailable");
              int dup_file = dup2( open_file, STDIN_FILENO );
              if (dup_file < 0 ) err(1,"%s","redirect failed");
              close(open_file);
              execvp(new_args[0],new_args);
              }
            }
           if (execvp(words[0],words) <0) exit(1); 
             exit(0); break;
           }

           default:{
            int child_status = -10 ;
            waitpid(newpid,&child_status,0);
            if(WIFEXITED(child_status) )
              status_lfg = WEXITSTATUS(child_status);
            if(WIFSIGNALED(child_status))
              status_lfg= WTERMSIG(child_status) + 128;
            break;
          }
                
        }

   }
  
   
  }// else for fork
  
 for (size_t i = 0; i < 512; ++i) 
      free(words[i]);
    

}// main 

char *words[MAX_WORDS] = {0};

int is_builtin(char * cmd){
    return (strcmp(cmd,"exit")==0 || strcmp(cmd,"cd")==0);
}

  

/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char const **start, char const **end)
{
  static char const *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = 0;
  *end = 0;
  for (char const *s = word; *s && !ret; ++s) {
    s = strchr(s, '$');
    if (!s) break;
    switch (s[1]) {
    case '$':
    case '!':
    case '?':
      ret = s[1];
      *start = s;
      *end = s + 2;
      break;
    case '{':;
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = s[1];
        *start = s;
        *end = e + 1;
      }
      break;
    }
  }
  prev = *end;
  return ret;
}
/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char const *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);


  while (c) {
    if (c == '!') build_str("<BGPID>", NULL);
    else if (c == '$'){ 
      char pid_buffer[512]={0};
      pid_t pid = getpid ();
      sprintf(pid_buffer,"%d",pid);
      build_str(pid_buffer, NULL);
    }
    else if (c == '?') {
      char status_buffer[512]={0};
      sprintf(status_buffer,"%d",status_lfg);
      build_str(status_buffer, NULL);
      }
    else if (c == '{') {
    char env_buf[512]={0}; 
    strncpy(env_buf,start+2,end-start-3); 
    char *env_var = getenv(env_buf);
    env_var ? build_str(env_var,NULL): build_str(NULL,NULL);
  }
           
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
   return build_str(start, NULL);
}

