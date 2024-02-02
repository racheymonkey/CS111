#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command1 command2 ... commandN\n", argv[0]);
        exit(EINVAL);
    }

    int pipes[argc - 1][2]; // One less than argc since argc includes the program name
    pid_t pid;

    // Create pipes
    for (int i = 0; i < argc - 2; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(errno);
        }
    }

    for (int i = 1; i < argc; i++) {
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(errno);
        }

        if (pid == 0) { // Child process
            if (i != 1) { // If not the first command, get input from previous pipe
                if (dup2(pipes[i - 2][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(errno);
                }
            }

            if (i != argc - 1) { // If not the last command, output to next pipe
                if (dup2(pipes[i - 1][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(errno);
                }
            }

            // Close all pipe fds in the child process
            for (int j = 0; j < argc - 2; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execlp(argv[i], argv[i], (char *)NULL);
            perror("execlp");
            exit(errno);
        } else { // Parent process
            if (i != 1) {
                close(pipes[i - 2][0]); // Close read end of the previous pipe
            }
            if (i != argc - 1) {
                close(pipes[i - 1][1]); // Close write end of the current pipe
            }
        }
    }

    // Parent waits for all child processes to finish
    for (int i = 1; i < argc; i++) {
        pid = waitpid(-1, &status, 0);
        if (pid == -1) {
            exit(errno); // Exit with the errno set by waitpid failure
        }

        // If the child process exited with an error, reflect that in the parent's exit status.
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            exit(WEXITSTATUS(status));
        }
    }

    // If all child processes exited successfully, return 0.
    return 0;

    return 0;
}
