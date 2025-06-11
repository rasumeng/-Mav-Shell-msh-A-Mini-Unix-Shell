// The MIT License (MIT)
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h> // So I can make or write to the file that the output is getting redirected to

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 128    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10    // Mav shell Supports 10 arguments

#define MAX_HISTORY 50          // Maximum number of history of commands

// Store command history
char *history[MAX_HISTORY];
int history_count = 0;

 


// Using a function to add commands to the history array
void append_history(char *command) {
  // skip empyty commands
  if (strlen(command) == 0){
    return;
  }

  if (history_count < MAX_HISTORY) {
    history[history_count++] = strdup(command);
  } else {
    // if the history is full, remove oldest command
    free(history[0]);
    for (int i = 1; i < MAX_HISTORY; i++){
      history[i-1] = history[i];
    }
    history[MAX_HISTORY - 1] = strdup(command);
  }

}

// display history
void display_history() {
  for (int i = 0; i < history_count; i++) {
    printf("[%d] %s", i + 1, history[i]);
  }
}

// use one of the history commadns
char* get_history_command(int history_number) {
  if (history_number > 0 && history_number <= history_count) {
    return history[history_number - 1];
  } else {
    return NULL;
  }
}

// More of a brute force approach but basically just tossed in everything important from while loop
void* rerun_command(char *command_string){
  char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup(command_string);                

    //char *head_ptr = working_string;

    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    if (strcmp(token[0], "cd") == 0){

      if (token[1] == NULL) {
        fprintf(stderr, "cd: expected argument to\"cd\"\n");
      } else {
        
        if (chdir(token[1]) != 0){

          perror("cd failed");
        }
      }
    }  

    if (strcmp(token[0], "history") == 0){
      display_history();
    }


    int redirect_found = 0;
    char *file = NULL;
    

    for (int i = 0; i < token_count; i++){
      
      if (token[i] != NULL && strcmp(token[i], ">") == 0){

        redirect_found = 1;

        if (token[i+1] != NULL) {

          file = token[i + 1];
          token[i]= NULL;
          
        } else {
          fprintf(stderr, "No file specified to get redirected");
          continue;
        }
        break;
      }
    }

    //Pipe handling
    int pipe_found = 0;
    int pipe_index = -1;
    
    for(int i = 0; i < token_count; i++){
      if(token[i] != NULL && strcmp(token[i], "|") == 0){
        pipe_found = 1;
        pipe_index = i;
        token[i] = NULL;

        break;
      } 
    }

    //create the pipe
    int pipefd[2];
    if(pipe_found){
      if (pipe(pipefd) == -1){
        perror("pipe failed");
        exit(0);
      }    
    }
    
    // make the fork
    pid_t pid1, pid2;
    
    pid1 = fork ();

    // forst child
    if (pid1 == 0) {

      if (redirect_found){

        int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        

        if (fd < 0) {
          perror("failed to open");
          exit(0);
        }


        if (dup2(fd, STDOUT_FILENO) < 0) {
          perror("dup2 failed");
          exit(0);
        }
        close(fd);
      } 
      
      //Pipe Handling
      if(pipe_found){
        
        dup2(pipefd[1], STDOUT_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);
      }

      execvp(token[0], token);
      perror("execvp faild");
      exit(0);
    }

    // Parent logic
    else if(pid1 > 0) { 

      // Second child, if pipe is found
      if (pipe_found){

        pid2 = fork();

        if (pid2 == 0) {
          

          dup2(pipefd[0], STDIN_FILENO);
          close(pipefd[1]);
          close(pipefd[0]);

          execvp(token[pipe_index + 1], &token[pipe_index + 1]);
          perror("execvp failed");
          exit(0);
        }
      }

      if (pipe_found) {
        close(pipefd[0]);
        close(pipefd[1]);
      }
      int status;
      waitpid(pid1, &status,0);
      

      if (pipe_found) {
        waitpid(pid2, &status, 0);
      }

    }
    return 0;
  }

int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  struct sigaction act;
 
  //Zero out the sigaction struct
  memset (&act, '\0', sizeof(act));

  //had to add to block SIGINT and SIGTSTP
  sigaddset(&act.sa_mask, SIGINT);
  sigaddset(&act.sa_mask, SIGTSTP);

  // Needed to actually block ctrl C
  act.sa_handler = SIG_IGN; 

  //Install the handler and check the return value.
  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror ("sigaction: SIGINT");
    return 1;
  }
  if (sigaction(SIGTSTP, &act, NULL) < 0) {
    perror("sigaction: SIGTSTP");
    return 1;
  } 
  
  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    
    //int token_index  = 0;

    
    if (token[0] == NULL){
      continue;
    }
    
    if (strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0) {
      exit (0);
    } 
    

    // read cd command and act accordingly
    if (strcmp(token[0], "cd") == 0){
      // if no directory is given, print an error message
      if (token[1] == NULL) {
        fprintf(stderr, "cd: expected argument to\"cd\"\n");
      } else {
        
        if (chdir(token[1]) != 0){

          perror("cd failed");
        }
      }
      continue;
    }

    // add command to history after every input
    if (strlen(command_string) > 0) {
      append_history(command_string);
    }

    //display history, doesnt works ;)
    if (strcmp(token[0], "history") == 0){
      display_history();
      continue;
    }

    // check for !*\ and use commad chosen
    if (token[0][0] == '!' && strlen(token[0]) > 1){

      int history_number = atoi(&token[0][1]);

      // see if it is a valid number
      if (history_number > 0 && history_number <= history_count) {
        char *history_command = history[history_number -1];

        printf("Re-running command: %s", history_command);

        rerun_command(history_command);

        free(history_command);
      

        continue;
      }
    }

    //Figuring out redirection
    // I figured it out
    int redirect_found = 0; // this is a true/false variable
    char *file = NULL;
    
    //need for loop to cyle through each input in token[] to find ">"
    for (int i = 0; i < token_count; i++){
      
      if (token[i] != NULL && strcmp(token[i], ">") == 0){
        // if foudn set redirect to true(1) and set the filename to the token after token[i]
        redirect_found = 1; // set true

        // Check to see if there was a file specified
        if (token[i+1] != NULL) {
          // save file name and ignore the >
          file = token[i + 1];
          token[i]= NULL;
        } else {
          fprintf(stderr, "No file specified to get redirected");
          continue;
        }
        break;
      }
    }
    
    
    //Pipe handling
    int pipe_found = 0; //false
    int pipe_index = -1;
    
    // for loop to find | similar to finding >
    for(int i = 0; i < token_count; i++){
      if(token[i] != NULL && strcmp(token[i], "|") == 0){
        pipe_found = 1; // set true
        pipe_index = i; // where | was found
        token[i] = NULL;
        break;
      } 
    }

    //create the pipe
    int pipefd[2];
    if(pipe_found){
      if (pipe(pipefd) == -1){
        perror("pipe failed");
        exit(0);
      }    
    }
    
    // make the fork
    pid_t pid1, pid2;
    
    pid1 = fork (); //Child one, will handle basic stuff and redirecting and piping

    // forst child
    if (pid1 == 0) {

      if (redirect_found){
        // Open file for writing only and write output into file
        int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        
        //if file failed to open
        if (fd < 0) {
          perror("failed to open");
          exit(0);
        }

        // failed to copy output into the file
        if (dup2(fd, STDOUT_FILENO) < 0) {
          perror("dup2 failed");
          exit(0);
        }
        close(fd);
      } 
      
      //Pipe Handling
      if(pipe_found){
        
        dup2(pipefd[1], STDOUT_FILENO); // push output down the pipe
        close(pipefd[0]);
        close(pipefd[1]);
      }

      execvp(token[0], token);
      perror("execvp faild");
      exit(0);
    }

    // Parent logic
    else if(pid1 > 0) { 
      // Second child, if pipe is found
      if (pipe_found){
        // make second child (reading)
        pid2 = fork();

        if (pid2 == 0) {
          
          
          dup2(pipefd[0], STDIN_FILENO);// take the ouput from the end of the pipe
          close(pipefd[1]);
          close(pipefd[0]);

          execvp(token[pipe_index + 1], &token[pipe_index + 1]);
          perror("execvp failed");
          exit(0);
        }
      }

      if (pipe_found) {
        close(pipefd[0]);
        close(pipefd[1]);
      }
      int status;
      waitpid(pid1, &status,0);
      

      if (pipe_found) {
        // so it doesn't wait for a nonexistent pipe
        waitpid(pid2, &status, 0);
      }

    }
    



    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

  return 0;
  // e1234ca2-76f3-90d6-0703ac120004

}
