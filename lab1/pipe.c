#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        // exit with EINVAL if no commands are given
        exit(EINVAL);
    }

    // array of pipes to connect processes
    int pipes[argc - 1][2]; // the number of pipes is one less than argc
    
    // store process IDs
    pid_t pid;
    
    // store child process status
    int status;
    
    // creating pipes for communication among processes
    for (int i = 0; i < argc - 2; i++) {
        if (pipe(pipes[i]) == -1) {
            exit(errno);
        }
    }

    for (int i = 1; i < argc; i++) {
        // creating a child process 
        pid = fork();
        if (pid == -1) {
            exit(errno);
        }

        // child process
        if (pid == 0) { 
            if (i != 1) { // if not the first command, get input from previous command
                dup2(pipes[i - 2][0], STDIN_FILENO);
                // close read and write end of previous pipe
                close(pipes[i - 2][0]);
                close(pipes[i - 2][1]);
            }

            if (i != argc - 1) { // if not last command, output to next command
                dup2(pipes[i - 1][1], STDOUT_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            execlp(argv[i], argv[i], (char *)NULL);
            exit(errno); 
        } else { // parent process
            if (i != 1) {
                // close read end of previous pipe
                close(pipes[i - 2][0]);
            }
            if (i != argc - 1) {
                // close write end of previous pipe
                close(pipes[i - 1][1]);
            }
        }
    }

    // parent waits for all child processes to finish
    for (int i = 1; i < argc; i++) {
        // wait for child processes to exit
        pid = waitpid(-1, &status, 0);
        if (pid == -1) {
            // if waitpid fails, exit
            exit(errno);
        }

        // if child process exited with error, so does the parent
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            exit(WEXITSTATUS(status));
        }
    }

    // if all child process exits properly, return 0
    return 0;
}
