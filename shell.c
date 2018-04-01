/* 
 
 Name: Aditya Taday

C Program for interpreting basic unix commands specifically IO Redirection, Pipes and Background Processing
Recognizes single level of redirection and process operators '<', '>', '|', '&' 

compile using : gcc -o new-shell new-shell.c 

*/

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUFSIZE 64     // Size of a single token
#define TOKEN_DELIM " \t\r\n\a"  // Token Delimiters

/* The breakline function takes a command line as input and splits it into tokens 
 * delimited by the standard delimiters */
char **BreakLine(char *string)
{
    int bufsize = BUFSIZE, k = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "memory allocation failure\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(string, TOKEN_DELIM);  // Get the first token
    while (token != NULL) {
        tokens[k] = token;
        k++;

        if (k >= bufsize) {     // If number of tokens are more then reallocate
            bufsize += BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "memory allocation failure\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIM);
    }
    tokens[k] = NULL;  // indicate token end
    return tokens;
}

/* The CopyArgs function is used to selectively copy an array of strings into another array. 
 * Parameter argc specifies copy limit */
static char** CopyArgs(int argc, char **args)                                                                                                                                                         
{                                                                                                                                                                                                                                                                                                                                                                                       
    char **copy_of_args = malloc(sizeof(char *) * (argc+1));
    int k;
    for (k = 0; k < argc; k++) {
        copy_of_args[k] = strdup(args[k]);
    }
    copy_of_args[k] = 0;
    return copy_of_args;
}

/* The StartProcess function initiates a foreground or background process using fork() and
 * execvp() function calls. parameter k > 0 indicates a background process */
void StartProcess(char **args, int k)
{
    pid_t pid, wpid;
    int status;

    if(k > 0)
        args[k] = NULL; // Set the location of '&' to NULL in order to pass it to execvp()
    pid = fork();
    if(pid == 0) // fork success. child initiated          
    {
        execvp(args[0], args);
        perror("Program execution failed");
        if(k == 0)
            exit(1);
    }
    if(k == 0) {
        // If not a background process, wait till it finishes execution.
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } 
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    else {
        // For a background process, do not wait indefinitely, but return to prompt
        fprintf(stderr, "Starting background process...");
        wpid = waitpid(-1, &status, WNOHANG);
    }
}

/* PipeRedirect function is used to initiate two child processes. The output of one
 * process is used to fuel the input of the other child process. */ 
void PipeRedirect(char **args, int k)
{
    int fpipe[2];
    char **copy_args;

    copy_args = CopyArgs(k,args);
    if(pipe(fpipe) == -1) {  // Create a pipe with one input pointer and one output pointer
        perror("pipe redirection failed");
        return;
    }

    if(fork() == 0)   // child 1        
    {
        dup2(fpipe[1], STDOUT_FILENO);   // Redirect STDOUT to output part of pipe     
        close(fpipe[0]);      //close pipe read
        close(fpipe[1]);      //close write pipe

        execvp(copy_args[0], copy_args);    // pass the truncated command line as argument
        perror("First program execution failed");
        exit(1);
    }

    if(fork() == 0)   // child 2
    {
        dup2(fpipe[0], STDIN_FILENO);   // Redirect STDIN to Input part of pipe         
        close(fpipe[1]);       //closing pipe write
        close(fpipe[0]);       //close read pipe 

        execvp(args[k+1], args+k+1);    // pass the second part of command line as argument
        perror("Second program execution failed");
        exit(1);
    }

    close(fpipe[0]);
    close(fpipe[1]);
    wait(0);   // Wait for child 1 to finish
    wait(0);   // Wait for child 2
}

/* IORedirect function redirects Standard Input or Standard Output depending on value of
 * parameter ioMode */
void IORedirect(char **args, int k, int ioMode)
{
    pid_t pid, wpid;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd, status = 0;
    
    pid = fork();
    if (pid == 0) {
        // Child process
        if(ioMode == 0)   // Input mode
            fd = open(args[k+1], O_RDONLY, mode);
        else              // Output mode
            fd = open(args[k+1], O_WRONLY|O_CREAT|O_TRUNC, mode);
        if(fd < 0)
            fprintf(stderr, "File error");
        else {
        
            dup2(fd, ioMode);   // Redirect input or output according to ioMode
            close(fd);          // Close the corresponding pointer so child process can use it
            args[k] = NULL;
            args[k+1] = NULL;

            if (execvp(args[0], args) == -1) {
                perror("SHELL");
            }
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0) { // Error forking process
        perror("SHELL");
    } 
    else {
        // Parent process. Wait till it finishes execution
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } 
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }        
}

void main() {
    char *cmdtext = NULL, cwd[1024], *pwd;
    char **args, *options[4] = {"<", ">", "|", "&"};
    int k = 0, option, found;
    
    pwd = NULL;

    // Get the home directory of the user
    pwd = getenv("HOME");

    printf("***** Unix/Linux Command interpreter *****\ntype 'exit' to terminate the interpreter\n");
    do {
        ssize_t bsize = 0;
        found = 0;
    
        if(getcwd(cwd, sizeof(cwd)) != NULL)   // Get current working directory and add to prompt
            printf("\n[%s]#", cwd);
        getline(&cmdtext, &bsize, stdin);
        args = BreakLine(cmdtext);             // Split command line into tokens

        if(args[0] == NULL) {
            free(cmdtext);
            free(args);
            continue;
        }
        if(strcmp(args[0],"exit") == 0)        // Handle the exit command. Break from command loop
            break;
        else if(strcmp(args[0],"cd") == 0) {   // Handle cd command.
            if(args[1] == NULL) {              // If no argument specified in cd change to home
                if(pwd[0] != 0) {              // directory. 
                    chdir(pwd);
                }
            }
            else {
                chdir(args[1]);
            }
        }
        else {
            k = 1;
            while(args[k] != NULL) {           // Check for any of the redirect or process operators <,<,|,&
                for(option = 0; option < 4; option++) {
                    if(strcmp(args[k],options[option]) == 0) 
                        break;
                }
                if(option < 4) {
                    found = 1;
                    if(option < 3 && args[k+1] == NULL) {   // For IORedirect and Pipe, argument is necessary
                        fprintf(stderr, "SHELL: parameter missing\n"); 
                        break;
                    }
                    if(option < 2)                          // Redirect IO, option=0 for Input, option=1 for Output
                        IORedirect(args, k, option);
                    else if(option == 2)
                        PipeRedirect(args, k);              // Pipe Redirection
                    else if(option == 3)
                        StartProcess(args, k);              // Start a background process
                    break;
                }

                k++;
            }
            if(found == 0)
                StartProcess(args, 0);                      // Start a foreground process or command
        }
        free(cmdtext);
        free(args);
    } 
    while (1);
}

