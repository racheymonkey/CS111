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
  u32 remaining_time;
  bool responded;
  bool in_queue;
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
  u32 current_time = 0; // initialize current simulation time to 0
  bool all_done = false; // check if all processes are done
  u32 time_slice = 0; // time slice for quantum

  // loop to initialize each process's remaining time, response flag, queue status
  for (u32 i = 0; i < size; ++i) {
    data[i].remaining_time = data[i].burst_time; // set remaining time of current process to burst time
    data[i].responded = false;
    data[i].in_queue = false;
  }

  struct process *current_proc = NULL;
  struct process *next_proc = NULL;

  // loop until all processes are complete
  while (!all_done) {
bool new_processes_added = false; // Flag to track if new processes have been added to the queue

    // Loop to enqueue processes that have just arrived
    for (u32 i = 0; i < size; ++i) {
        // If current process's arrival time is now and it's not in the queue
        if (data[i].arrival_time == current_time && !data[i].in_queue) {
            // Check if there's already a process running
            if (current_proc != NULL) {
                // If the new arrival has the same arrival time as the current process,
                // prioritize the new arrival over the current process
                if (current_proc->arrival_time == current_time) {
                    TAILQ_INSERT_BEFORE(current_proc, &data[i], pointers); // Insert before current process
                } else {
                    TAILQ_INSERT_TAIL(&list, &data[i], pointers); // Insert to end of list
                }
            } else {
                TAILQ_INSERT_TAIL(&list, &data[i], pointers); // Insert to end of list
            }
            data[i].in_queue = true;
            new_processes_added = true; // Set flag to true indicating new process added to the queue
        }
    }

    // If no new processes were added and there are more processes to arrive
    if (!new_processes_added && !all_done) {
        // Find the earliest arrival time among the remaining processes
        u32 next_arrival_time = UINT32_MAX;
        for (u32 i = 0; i < size; ++i) {
            if (data[i].remaining_time > 0 && data[i].arrival_time < next_arrival_time) {
                next_arrival_time = data[i].arrival_time;
            }
        }
        // If next_arrival_time is still UINT32_MAX, it means all processes are done
        if (next_arrival_time == UINT32_MAX) {
            break; // Exit the loop if all processes are done
        }
        // Advance simulation time to the arrival time of the next process
        current_time = next_arrival_time;
    }

    
    // if current time slice is over or the current process is done
    if (time_slice == 0 || (current_proc != NULL && current_proc->remaining_time == 0)) {
      // if there is a current process and it still has time left
      if (current_proc != NULL && current_proc->remaining_time > 0) {
        TAILQ_INSERT_TAIL(&list, current_proc, pointers); // reinsert current process to end of queue
        current_proc->in_queue = true;
      }

      // if the list is not empty, run next process
      if (!TAILQ_EMPTY(&list)) {
        next_proc = TAILQ_FIRST(&list);
        TAILQ_REMOVE(&list, next_proc, pointers); // remove next process from list
        next_proc->in_queue = false;
        
        // if the next process has not been responded to yet
        if (!next_proc->responded) {
          next_proc->responded = true;

          // add the response time for the next process to the total response time
          total_response_time += current_time - next_proc->arrival_time;
        }

        time_slice = quantum_length; // reset time slice to quantum length for next process
        current_proc = next_proc;
      }
    }

    // if there is a current process to run
    if (current_proc != NULL) {
      current_proc->remaining_time--; // decrement remaining time of current process
      time_slice--;
      current_time++;

      // increment waiting time for all processes in the queue
      struct process *tmp;
      TAILQ_FOREACH(tmp, &list, pointers) {
        if (tmp != current_proc) {
          total_waiting_time++;
        }
      }
    } else {
      // if no current process, just increment the simulation time
      current_time++;
    }

    all_done = true;

    // check if there are still processes with remaining time
    for (u32 i = 0; i < size; ++i) {
      // if any process still has remaining time
      if (data[i].remaining_time > 0) {
        // not all processes are done
        all_done = false;
        break;
      }
    }
  }
  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
