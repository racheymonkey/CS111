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

    for (int i = 1; i < argc; i++) {
        if (i < argc - 1) { // Only create a pipe if there are more commands to execute
            if (pipe(fds) != 0) {
                perror("pipe failed");
                exit(EXIT_FAILURE); // Exit if pipe creation fails
            }
        }

        pid_t pid = fork();

        // fork failure
        if (pid == -1) { 
            perror("fork failed");
            exit(EXIT_FAILURE); // Exit if fork fails
        } 

        // fork success, in child process
        else if (pid == 0) {
            // Redirect input from previous pipe, if not first command
            dup2(prev, STDIN_FILENO);
            if (prev != STDIN_FILENO) {
                close(prev);
            }

            // For all but the last command, redirect output to the pipe
            if (i != argc - 1) {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[0]);
            }

            // Close all fds in child to ensure clean execution environment
            if (i < argc - 1) {
                close(fds[1]);
            }

            execlp(argv[i], argv[i], NULL); // Execute command
            perror(argv[i]); // Only reached if execlp fails
            exit(EXIT_FAILURE); // Exit with failure if command does not execute
        } 

        // In parent process
        else {
            if (prev != STDIN_FILENO) {
                close(prev); // Close previous read end
            }
            if (i < argc - 1) {
                close(fds[1]); // Close write end not needed in parent
                prev = fds[0]; // Next command reads from here
            }

            int st = 0;
            waitpid(pid, &st, 0); // Wait for child to complete
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                fprintf(stderr, "Command failed with error.\n");
                exit(WEXITSTATUS(st)); // Exit immediately with the child's exit status
            }
        }
    }

    if (argc >= 2 && prev != STDIN_FILENO) {
        close(prev); // Prevent leak
    }

    return 0; // Success
}
