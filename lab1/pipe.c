#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int prev = STDIN_FILENO;
    int fds[2];

    // not enough args
    if (argc <= 1) {
        exit(EINVAL);
    }

    // Track error status
    int errorOccurred = 0;

    for (int i = 1; i < argc; i++) {
        if (i < argc - 1) { // Create pipe except for the last command
            if (pipe(fds) != 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();

        // fork failure
        if (pid == -1) { 
            perror("fork");
            exit(EXIT_FAILURE);
        } 

        // fork success, in child process
        else if (pid == 0) {
            // arg reads from standard input
            dup2(prev, STDIN_FILENO);
            if (prev != STDIN_FILENO) {
                close(prev); // Close previous read end not needed anymore
            }

            // make last arg output to standard output, except for the last command
            if (i != argc - 1) {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[0]); // Close read end not needed in child
                close(fds[1]); // Close write end after duplicating
            }

            execlp(argv[i], argv[i], NULL); //execute command
            perror("execlp"); // If execlp returns, it's an error
            exit(errno);
        } 
        
        // in parent process
        else {
            if (prev != STDIN_FILENO) {
                close(prev); // Close previous read end not needed anymore
            }
            if (i < argc - 1) {
                close(fds[1]); // Close write end not needed in parent
                prev = fds[0]; // Use read end for the next command
            }

            int st = 0;
            waitpid(pid, &st, 0);
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                errorOccurred = 1; // Mark an error occurred
            }
        }
    }

    if (argc >= 2 && prev != STDIN_FILENO) {
        close(prev); // Close last read end to prevent leak
    }

    if (errorOccurred) {
        fprintf(stderr, "An error occurred in executing one of the commands.\n");
        exit(1); // Exit with error if any command failed
    }

    return 0;
}
