#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	// TODO: it's all yours
	int prev = STDIN_FILENO;
	int fds[2];

	// not enough args
	if (argc <= 1) {
		exit(EINVAL);
	}

	// failed to create pipe
	if (pipe(fds) != 0) {
		exit(EXIT_FAILURE);
	}

	// iterate through arguments
	for (int i = 1; i < argc; i++) {
		pid_t pid = fork();

		// fork failure
		if (pid == -1) { 
			exit(EXIT_FAILURE);
		} 

		// fork success, in child process
		else if (pid == 0) {
			// arg reads from standard input
			dup2(prev, STDIN_FILENO);

			// make last arg output to standard output
			if (i != argc - 1) {
				dup2(fds[1], STDOUT_FILENO);
			}
			execlp(argv[i], argv[i], NULL); //execute command
			return 1;
		} 
		
		// in parent process
		else {
			int st = 0;
			waitpid(pid, &st, 0);
			if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
				return WEXITSTATUS(st); //handle error
			}

			if (prev != STDIN_FILENO) {
				close(prev);
			}

			prev = fds[0]; // reset previous file descriptor
			close(fds[1]);
			pipe(fds); // create pipe for next argument if it exists
		}
	}

	// close file descriptors
	close(fds[0]);
	close(fds[1]);

	if (argc >= 2) {
		close(prev); // prevent leak
	} else {
		return 0;
	}

}
