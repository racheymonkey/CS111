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

struct process {
  u32 pid;
  u32 arrival_time;
  u32 burst_time;
  bool has_been_executed;
  u32 first_execution_time;
  u32 total_waiting_time;
  u32 last_added_time; // Time when the process was last added to the queue.
  TAILQ_ENTRY(process) pointers;
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
  // Inside init_processes, after loading each process:
  (*process_data)[i].has_been_executed = false;
  (*process_data)[i].first_execution_time = 0; // Will be set upon first execution.
  (*process_data)[i].total_waiting_time = 0; // Initialize waiting time.
  
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
  
  u32 current_time = 0;
  u32 completed_processes = 0;
  // Assuming `remaining_burst_time` and other arrays are properly initialized.
  
  while (completed_processes < size) {
      // Add arriving processes to the ready queue...
      for (u32 i = 0; i < size; i++) {
          if (!data[i].has_been_executed && data[i].arrival_time <= current_time) {
              TAILQ_INSERT_TAIL(&ready_queue, &data[i], pointers);
              data[i].last_added_time = current_time; // Mark when it was added to the queue.
          }
      }
  
      if (!TAILQ_EMPTY(&ready_queue)) {
          struct process *proc = TAILQ_FIRST(&ready_queue);
  
          if (!proc->has_been_executed) {
              proc->first_execution_time = current_time;
              proc->has_been_executed = true;
          }
  
          // Calculate waiting time before execution.
          if (proc->has_been_executed) {
              proc->total_waiting_time += current_time - proc->last_added_time;
          }
  
          // Determine execution time (quantum or remaining burst, whichever is smaller)
          u32 execution_time = quantum_length < proc->burst_time ? quantum_length : proc->burst_time;
          proc->burst_time -= execution_time;
          current_time += execution_time; // Advance time by the execution time.
  
          if (proc->burst_time == 0) {
              // Process completed.
              TAILQ_REMOVE(&ready_queue, proc, pointers);
              completed_processes++;
          } else {
              // Process not completed, needs to be added back to the queue.
              proc->last_added_time = current_time; // Update last added time.
              TAILQ_REMOVE(&ready_queue, proc, pointers);
              TAILQ_INSERT_TAIL(&ready_queue, proc, pointers);
          }
      } else {
          // If no process is ready, increment the current time.
          current_time++;
      }
  }
  
  // Calculate the total waiting and response times.
  for (u32 i = 0; i < size; i++) {
      total_waiting_time += data[i].total_waiting_time;
      total_response_time += data[i].first_execution_time - data[i].arrival_time;
  }
  
  // Calculate average waiting and response times.
  float avg_waiting_time = (float)total_waiting_time / (float)size;
  float avg_response_time = (float)total_response_time / (float)size;

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
