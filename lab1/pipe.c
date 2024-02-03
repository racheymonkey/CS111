#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// argc is the number of arguments, and argv stores the actual arguments
// argv[0] is the command we are running, so argv[0] = ./pipe
int main(int argc, char *argv[])
{
	int prev = STDIN_FILENO;
	int pipefd[2];
	pipe(pipefd);

	if (pipe(pipefd) != 0)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	// iterate through all given arguments (argc)
	for (int i = 1; i < argc; i++)
	{
		int p = fork();
		
		if (p == -1) //fails forking b/c system lacks resources
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

		// child
		if (p == 0)
		{
			dup2(prev, STDIN_FILENO);
			if (i != argc - 1)
			{
				dup2(pipefd[1], STDOUT_FILENO);
			}

			execlp(argv[i], argv[i], NULL);
			return 1;
		}
		else //parent
		{
			// need to wait for child process to finish
			int st = 0;
			waitpid(p, &st, 0);
			st = WEXITSTATUS(st);

			// error
			if (st != 0)
				return st;
			
			if (prev != STDIN_FILENO)
				close(prev);
			prev = pipefd[0];

			close(pipefd[1]);
			pipe(pipefd); // new pipe
		}
	}
	close(pipefd[0]);
	close(pipefd[1]);

	if (argc >= 2)
		close(prev);
	else
		return 0;
}
