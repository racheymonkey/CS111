#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h> // Include this for INT_MAX

/* A process table entry.  */
struct process {
  long pid;
  long arrival_time;
  long burst_time;

  TAILQ_ENTRY (process) pointers;

  /* Additional fields here */
  bool running;
  long waiting_time;
  long start_exec_time;
  long remaining_time;
  long response_time;
  long end_time;
  /* End of "Additional fields here" */
};

TAILQ_HEAD (process_list, process);

/* Skip past initial nondigits in *DATA, then scan an unsigned decimal
   integer and return its value.  Do not scan past DATA_END.  Return
   the integer’s value.  Report an error and exit if no integer is
   found, or if the integer overflows.  */
static long next_int (char const **data, char const *data_end) {
  long current = 0;
  bool int_start = false;
  char const *d;

  for (d = *data; d < data_end; d++) {
      char c = *d;
      if ('0' <= c && c <= '9') {
        int_start = true;
	// Replace the ckd_mul and ckd_add calls with these checks
	if (current > LONG_MAX / 10 || (current * 10) > LONG_MAX - (c - '0')) {
	    fprintf(stderr, "integer overflow\n");
	    exit(1);
	} else {
	    current = current * 10 + (c - '0');
	}
  }

  if (!int_start) {
    fprintf (stderr, "missing integer\n");
    exit (1);
  }

  *data = d;
  return current;
}

/* Return the first unsigned decimal integer scanned from DATA.
   Report an error and exit if no integer is found, or if it overflows.  */
static long next_int_from_c_str (char const *data) {
  return next_int (&data, strchr (data, 0));
}

/* A vector of processes of length NPROCESSES; the vector consists of
   PROCESS[0], ..., PROCESS[NPROCESSES - 1].  */
struct process_set {
  long nprocesses;
  struct process *process;
};

/* Return a vector of processes scanned from the file named FILENAME.
   Report an error and exit on failure.  */
static struct process_set init_processes (char const *filename) {
  int fd = open (filename, O_RDONLY);
  if (fd < 0) {
    perror ("open");
    exit (1);
  }

  struct stat st;
  if (fstat (fd, &st) < 0) {
    perror ("stat");
    exit (1);
  }

  size_t size;
  if (ckd_add (&size, st.st_size, 0)) {
    fprintf (stderr, "%s: file size out of range\n", filename);
    exit (1);
  }

  char *data_start = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED) {
    perror ("mmap");
    exit (1);
  }

  char const *data_end = data_start + size;
  char const *data = data_start;

  long nprocesses = next_int (&data, data_end);
  if (nprocesses <= 0) {
    fprintf (stderr, "no processes\n");
    exit (1);
  }

  struct process *process = calloc (sizeof *process, nprocesses);
  if (!process) {
    perror ("calloc");
    exit (1);
  }

  for (long i = 0; i < nprocesses; i++) {
    process[i].pid = next_int (&data, data_end);
    process[i].arrival_time = next_int (&data, data_end);
    process[i].burst_time = next_int (&data, data_end);
    if (process[i].burst_time == 0) {
	    fprintf (stderr, "process %ld has zero burst time\n", process[i].pid);
	    exit (1);
	  }
  }

  if (munmap (data_start, size) < 0) {
    perror ("munmap");
    exit (1);
  }

  if (close (fd) < 0) {
    perror ("close");
    exit (1);
  }

  return (struct process_set) {nprocesses, process};
}

struct IndexedLong {
    long value;
    long index;
};

// Comparison function for qsort
int compareIndexedLongs(const void *a, const void *b) {
    const struct IndexedLong *indexedA = (const struct IndexedLong *)a;
    const struct IndexedLong *indexedB = (const struct IndexedLong *)b;

    if (indexedA->value < indexedB->value) 
      return -1;
    if (indexedA->value > indexedB->value) 
      return 1;
    return indexedA->index - indexedB->index;  // Ensure stability by comparing original indices
}

int main (int argc, char *argv[]) {
  long completed_processes = 0;

if (argc != 3) {
    fprintf (stderr, "%s: usage: %s file quantum\n", argv[0], argv[0]);
    return 1;
  }

  struct process_set ps = init_processes (argv[1]);
  long quantum_length = (strcmp (argv[2], "median") == 0 ? -1 : next_int_from_c_str (argv[2]));
  if (quantum_length == 0) {
    fprintf (stderr, "%s: zero quantum length\n", argv[0]);
    return 1;
  }

  struct process_list list;
  TAILQ_INIT (&list);

  long total_wait_time = 0;
  long total_response_time = 0;

 /* Your code here */
long current_time = 0;  // Current time in the simulation
struct process *current_process = NULL;  // Pointer to the currently running process

// Initialize each process's additional fields
for (long i = 0; i < ps.nprocesses; i++) {
    ps.process[i].remaining_time = ps.process[i].burst_time;
    ps.process[i].response_time = -1; // -1 indicates response time not yet calculated
    ps.process[i].running = false;
    ps.process[i].waiting_time = 0;
    ps.process[i].start_exec_time = -1;
    ps.process[i].end_time = -1;
}

// Main simulation loop
while (completed_processes < ps.nprocesses) {
    // Check for new arrivals and add them to the queue
    for (long i = 0; i < ps.nprocesses; i++) {
        if (!ps.process[i].running && ps.process[i].arrival_time <= current_time) {
            ps.process[i].running = true; // Mark as added to the queue
            TAILQ_INSERT_TAIL(&list, &ps.process[i], pointers); // Add to ready queue
        }
    }

    // If no process is currently running, get the next one from the queue
    if (!current_process || current_process->remaining_time <= 0) {
        if (current_process) {
            // Process just finished, calculate its end time
            current_process->end_time = current_time;
            total_response_time += current_process->start_exec_time - current_process->arrival_time;
            total_wait_time += current_process->waiting_time;
            completed_processes++;
        }
        
        if (!TAILQ_EMPTY(&list)) {
            // Get the next process from the head of the queue
            current_process = TAILQ_FIRST(&list);
            TAILQ_REMOVE(&list, current_process, pointers); // Remove from the queue

            // If response time not set, set it now
            if (current_process->response_time == -1) {
                current_process->response_time = current_time - current_process->arrival_time;
            }
            current_process->start_exec_time = current_time;
        } else {
            // No process is ready to run
            current_process = NULL;
        }
    }

    // Execute the current process
    if (current_process) {
        current_process->remaining_time--;
        if (current_process->remaining_time <= 0) {
            // Process finished execution
            current_process = NULL;
        }
    }

    // Update waiting time for all other processes in the queue
    struct process *p;
    TAILQ_FOREACH(p, &list, pointers) {
        p->waiting_time++;
    }

    // Move forward in time
    current_time++;

    // Re-queue the current process if it's still running and its quantum expired
    if (current_process && current_time - current_process->start_exec_time >= quantum_length) {
        TAILQ_INSERT_TAIL(&list, current_process, pointers); // Re-queue to the end
        current_process = NULL; // Clear the current process as it's back in the queue
    }
}

/* End of "Your code here" */

// After simulation, calculate average wait time and average response time
double average_wait_time = (double)total_wait_time / ps.nprocesses;
double average_response_time = (double)total_response_time / ps.nprocesses;

printf("Average wait time: %.2f\n", average_wait_time);
printf("Average response time: %.2f\n", average_response_time);

/* Ensure to free the memory allocated for the process set */
free(ps.process);
  return 0;
}
