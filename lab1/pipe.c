#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int prev = STDIN_FILENO; // previous file descriptor for reading
    int fds[2];

    // check for at least one command argument
    if (argc <= 1) {
        exit(EINVAL); // exit if no command arguments
    }

    for (int i = 1; i < argc; i++) {
        // create pipe if more commands to execute
        if (i < argc - 1) { 
            if (pipe(fds) != 0) {
                exit(1); // exit if pipe creation fails
            }
        }

        pid_t pid = fork(); // create a new process

        // check for fork failure
        if (pid == -1) { 
            exit(1); // exit if fork fails
        } 
        // child process
        else if (pid == 0) {
            // redirect stdin from previous pipe if not first command
            dup2(prev, STDIN_FILENO);
            // close previous read end if not stdin
            if (prev != STDIN_FILENO) {
                close(prev);
            }

            // redirect stdout to the pipe if not last command
            if (i != argc - 1) {
                dup2(fds[1], STDOUT_FILENO);
                // close read end of the pipe in child
                close(fds[0]);
            }

            // close write end of pipe if not last command
            if (i < argc - 1) {
                close(fds[1]);
            }

            // execute command
            execlp(argv[i], argv[i], NULL); 
            // exit with failure if execlp fails
            exit(1); 
        } 
        // parent process
        else {
            // close previous read end if not stdin
            if (prev != STDIN_FILENO) {
                close(prev); 
            }
            // setup for next command if more commands to execute
            if (i < argc - 1) {
                close(fds[1]); // close write end of pipe in parent
                prev = fds[0]; // prepare next read end for subsequent command
            }

            int st = 0;
            // wait for child process to complete
            waitpid(pid, &st, 0); 
            // exit if child did not exit cleanly
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                exit(1); 
            }
        }
    }

    // close last used read end if any
    if (argc >= 2 && prev != STDIN_FILENO) {
        close(prev); 
    }

    return 0; // success
}
