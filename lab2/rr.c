#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 remaining_time; // For tracking remaining burst time
  bool responded;     // For tracking if the process has been responded to
  bool in_queue;  // New field to track if the process is already in the queue
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  u32 current_time = 0;
  bool all_done = false;

/* Inside main function, after initializing the processes */
for (u32 i = 0; i < size; ++i) {
  data[i].remaining_time = data[i].burst_time;
  data[i].responded = false;
  data[i].in_queue = false;  // Initialize the new in_queue flag
}

struct process *current_proc = NULL; // Pointer to the currently executing process

while (!all_done) {
  bool idle = true;
  
  // Enqueue processes that have arrived
  for (u32 i = 0; i < size; ++i) {
    if (data[i].arrival_time <= current_time && !data[i].in_queue && data[i].remaining_time > 0) {
      TAILQ_INSERT_TAIL(&list, &data[i], pointers);
      data[i].in_queue = true; // Mark the process as in the queue
      idle = false;
    }
  }

  // If there's no current process, or the quantum for the current process has expired, select the next process
  if (current_proc == NULL || quantum_length == 0) {
    if (!TAILQ_EMPTY(&list)) {
      if (current_proc != NULL) {
        // Only re-queue if there's remaining time
        if (current_proc->remaining_time > 0) {
          TAILQ_INSERT_TAIL(&list, current_proc, pointers);
          current_proc->in_queue = true;
        }
        current_proc = NULL; // Reset current_proc after re-queuing
      }
      current_proc = TAILQ_FIRST(&list); // Select the next process from the queue
      TAILQ_REMOVE(&list, current_proc, pointers); // Remove from queue
      current_proc->in_queue = false; // Mark as not in the queue

      if (!current_proc->responded) {
        current_proc->responded = true;
        total_response_time += current_time - current_proc->arrival_time;
      }
    }
  }

  // Execute the current process
  if (current_proc != NULL) {
    idle = false;
    u32 exec_time = (current_proc->remaining_time < quantum_length) ? current_proc->remaining_time : quantum_length;
    current_proc->remaining_time -= exec_time;
    current_time += exec_time; // Move the system clock forward by exec_time
    quantum_length -= exec_time; // Decrease the quantum_length by exec_time

    // Update waiting time for other processes in the queue
    struct process *tmp;
    TAILQ_FOREACH(tmp, &list, pointers) {
      total_waiting_time += exec_time;
    }

    // If the process has finished or quantum expired, reset current_proc to NULL
    if (current_proc->remaining_time == 0 || quantum_length == 0) {
      current_proc = NULL;
    }
  }

  // Check if all processes are done
  all_done = true;
  for (u32 i = 0; i < size; ++i) {
    if (data[i].remaining_time > 0) {
      all_done = false;
      break;
    }
  }

  // Simulate time passing if idle
  if (idle) {
    current_time++;
    quantum_length = next_int_from_c_str(argv[2]); // Reset quantum for the next cycle
  }
}
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
