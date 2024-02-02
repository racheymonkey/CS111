#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        // The lab specifies to exit with errno EINVAL if no commands are given.
        exit(EINVAL);
    }

        // Skip pipe creation and directly execute the command if only one command is provided
    if (argc == 2) {
        pid_t pid = fork();
        if (pid == -1) {
            exit(errno);
        }

        if (pid == 0) { // Child process
            execlp(argv[1], argv[1], (char *)NULL);
            exit(errno); // In case execlp fails
        } else { // Parent process
            wait(NULL); // Wait for the child process to finish
            return 0;
        }
    }

    int pipes[argc - 1][2]; // One less than argc since argc includes the program name
    pid_t pid;
    int status;

    // Create pipes for communication between processes
    for (int i = 0; i < argc - 2; i++) {
        if (pipe(pipes[i]) == -1) {
            exit(errno); // Exit with the errno set by pipe failure
        }
    }

    for (int i = 1; i < argc; i++) {
        pid = fork();
        if (pid == -1) {
            exit(errno); // Exit with the errno set by fork failure
        }

        if (pid == 0) { // Child process
            if (i != 1) { // If not the first command, get input from previous pipe
                dup2(pipes[i - 2][0], STDIN_FILENO);
                close(pipes[i - 2][0]);
                close(pipes[i - 2][1]);
            }

            if (i != argc - 1) { // If not the last command, output to next pipe
                dup2(pipes[i - 1][1], STDOUT_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            execlp(argv[i], argv[i], (char *)NULL);
            exit(errno); // Exit with the errno set by execlp failure
        } else { // Parent process
            if (i != 1) {
                close(pipes[i - 2][0]);
            }
            if (i != argc - 1) {
                close(pipes[i - 1][1]);
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
}
