#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);

/* HW2 directory commands */
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* HW2 not built-in commands */
int cmd_bash(struct tokens* tokens);

/* HW2 path resolution */
char* path_resolution(const char*);
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 200
#endif

/* HW2 redirection */
int find_redirect(struct tokens* tokens, const char* redirection);
int cmd_redirect(struct tokens* tokens);

/* HW2 pipes */
int* find_pipes(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "get current working directory"},
    {cmd_cd, "cd", "change current working directory"},
};

/* Prints the current working directory to standard output */
int cmd_pwd(unused struct tokens* tokens)
{
  const int MAX_CWD = 100;
  char buf[MAX_CWD];
  char *s = getcwd(buf, MAX_CWD);
  if(s==NULL){
    printf("get current working directory failed.\n");
    return 0;
  }
  printf("%s\n", s);
  return 1;

}

/* changes the current working directory */
int cmd_cd(struct tokens* tokens)
{
  char *s = tokens_get_token(tokens, 1);
  if(s == NULL){
    printf("missing arguments: cd PATH_NAME\n");
    return 0;
  }
  int err = chdir(s);
  if(err == -1){
    printf("change working directory failed\n");
    return 0;
  }
  return 1;
}

/* find redirection symbols */
/* return -1 on error */
/* return a non-negative number on success */
/* which represents the redirection symbol location */
int find_redirect(struct tokens* tokens, const char* redirection)
{
  int redirect_loc = -1;
  int tokens_len = tokens_get_length(tokens);
  if(strlen(redirection) > 1){
    fprintf(stderr, "Only supports '<' or '>' \n");
    return -1;
  }
  for(int k = 0 ; k < tokens_len; k++){
    if(strncmp(tokens_get_token(tokens,k), redirection, 1) == 0){
      if(redirect_loc < 0){
        redirect_loc = k;
      }
      else{
        fprintf(stderr, "More than one redirection symbol\n");
        return -1;
      }
    }
  }
  return redirect_loc;
}

/* parse tokens when redirect is present */
char **redirect_parse(struct tokens* tokens)
{
  size_t tokens_len = tokens_get_length(tokens);
  int index = 0;  
  char **argv = (char**)malloc(sizeof(char*)*(tokens_len + 1));
  
  for(int k=0;k<tokens_len;k++){
    char* cur_token = tokens_get_token(tokens, k);
    if(strncmp(cur_token, ">", 1) == 0|| strncmp(cur_token, "<", 1) ==0){
      k++;
    }
    else{
      argv[index++] = tokens_get_token(tokens, k);
    }
  }
  argv[index] = NULL;
  return argv;
}

/* bash execution with redirection symbols */
int redirect(struct tokens* tokens)
{
  int output_loc = find_redirect(tokens, ">");
  int input_loc = find_redirect(tokens, "<");
  char* out_file;
  char* in_file;
  int fd_out, fd_in;

  if(output_loc > 0){
    out_file = tokens_get_token(tokens, output_loc+1);
    if(!out_file){
      fprintf(stderr, "Syntax error for >\n");
      return -1;
    }
    fd_out = open(out_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd_out < 0){
      fprintf(stderr, "Output stream error\n");
      return -1;
    }
    dup2(fd_out, STDOUT_FILENO);
  }

  if(input_loc > 0){
    in_file = tokens_get_token(tokens, input_loc+1);
    if(!in_file){
      fprintf(stderr, "Syntax error for <\n");
      return -1;
    }
    fd_in = open(in_file,O_RDONLY);
    if(fd_in < 0){
      fprintf(stderr, "Output stream error\n");
      return -1;
    }
    dup2(fd_in, STDIN_FILENO);
  }

  char** argv = redirect_parse(tokens);
  char* full_cmd_path = path_resolution(argv[0]);
  if(execv(full_cmd_path, argv) == -1){
    fprintf(stderr, "%s execution failed in redirection\n", full_cmd_path);
    exit(1);
  }

  return 0;
}

/* divide pipes */ 
char** pipes_divide(struct tokens* tokens)
{
  size_t tokens_len = tokens_get_length(tokens);
  char **argv = (char**)malloc(sizeof(char*)*(tokens_len + 1));
  for(int k = 0;k<tokens_len;k++){
    if(strncmp(tokens_get_token(tokens, k), "|", 1) == 0){
      argv[k] = NULL;
    }
    else{
      argv[k] = tokens_get_token(tokens,k);
    }
  }
  argv[tokens_len] = NULL;
  return argv;
}

/* locate pipes */
int *find_pipes(struct tokens* tokens)
{
  size_t tokens_len = tokens_get_length(tokens);
  int* pipe_locs = (int*)malloc(sizeof(int)*(tokens_len + 1));
  int index = 0;
  for(int k = 0;k<tokens_len;k++){
    if(strncmp(tokens_get_token(tokens, k), "|", 1) == 0){
     pipe_locs[index++] = k; 
    }
  }
  return pipe_locs;
}

/* regular_parse, no redirection or) pipes */
char **regular_parse(struct tokens* tokens)
{
  size_t tokens_len = tokens_get_length(tokens);
  char **argv = (char**)malloc(sizeof(char*)*(tokens_len + 1));
  for(int k=0;k<tokens_len;k++){
    argv[k] = tokens_get_token(tokens, k);
  }
  argv[tokens_len] = NULL;
  return argv;
}

/* not built-in commands execution */
int cmd_bash(struct tokens* tokens)
{
  char **argv = regular_parse(tokens);
  char* cmd = argv[0];
  
  /* fork and execv */
  pid_t pid = fork();
  if(pid<0){
    fprintf(stderr, "Fork failed\n");
    return -1;
  }
  else if(pid == 0){
    char* full_cmd_path = path_resolution(cmd);
    if(full_cmd_path == NULL){
      fprintf(stderr, "path resolution failed\n");
      exit(1);
    }
    if(find_redirect(tokens, ">")>=0 || find_redirect(tokens, "<")>=0){
      if(redirect(tokens)==-1){
        fprintf(stderr, "redirection failed\n");
        exit(1);
      }
      exit(0);
    }
    else{
      if(execv(full_cmd_path, argv) == -1){
      fprintf(stderr, "%s execution failed\n", cmd);
      exit(1);
      }
      exit(0);
    }
  }
  else{
    wait(NULL);
  }
  return 1;
}

/* path resolution */
char* path_resolution(const char* path)
{
  if(*path == '/'){
    return path;
  }
  char *envvar = "PATH";
  char *s_path = getenv(envvar);
  if(s_path == NULL){
    fprintf(stderr, "Not found the %s environment variable\n", envvar);
    exit(1);
  }
  char* full_path = (char*)malloc(MAX_PATH_LEN);
  size_t path_len = strlen(s_path);
  char *s_path_copy = (char*)malloc(path_len+1);
  strncpy(s_path_copy, s_path, path_len+1);

  char* cur_path = strtok(s_path_copy,":");
  while(cur_path!=NULL){
    strncpy(full_path, cur_path, strlen(cur_path)+1);
    strncat(full_path, "/", 1);
    strncat(full_path, path, strlen(path));
    if(access(full_path, F_OK) == 0){
      return full_path;
    }
    else{
      cur_path = strtok(NULL, ":");
    }
  }
  return NULL;
}
/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      // fprintf(stdout, "This shell doesn't know how to run programs.\n");
      fun_desc_t func = {
        .fun = cmd_bash,
        .cmd = tokens_get_token(tokens, 0),
        .doc = "bash commands",
      };
      func.fun(tokens);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
