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
  /* Inside the while loop */
if (current_proc != NULL) {
  idle = false;
  u32 exec_time = (current_proc->remaining_time < quantum_length) ? current_proc->remaining_time : quantum_length;
  current_proc->remaining_time -= exec_time;
  current_time += exec_time;

  // Correctly update waiting time for all processes in the queue
  struct process *tmp = TAILQ_FIRST(&list); // Start from the head of the list
  while (tmp != NULL) {
    if (tmp != current_proc) {
      total_waiting_time += exec_time; // Only increment waiting time for processes not currently executing
    }
    tmp = TAILQ_NEXT(tmp, pointers); // Move to the next process in the list
  }

  // If the process has finished, don't put it back in the queue
  if (current_proc->remaining_time > 0) {
    TAILQ_INSERT_TAIL(&list, current_proc, pointers); // Re-insert the process at the end of the list if not done
    current_proc->in_queue = true;
  } else {
    // If finished, don't set current_proc to NULL immediately, wait until next cycle
    // This allows the scheduler to check for new arrivals before picking the next process
  }
}
  }

  if (current_proc != NULL) {
    idle = false;
    u32 exec_time = (current_proc->remaining_time < quantum_length) ? current_proc->remaining_time : quantum_length;
    current_proc->remaining_time -= exec_time;
    current_time += exec_time;

    // Update waiting time for all processes in queue
    struct process *tmp;
    TAILQ_FOREACH(tmp, &list, pointers) {
      total_waiting_time += exec_time;
    }

    if (current_proc->remaining_time > 0) {
      TAILQ_INSERT_TAIL(&list, current_proc, pointers);
      current_proc->in_queue = true;
    } else {
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
  }
}

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
