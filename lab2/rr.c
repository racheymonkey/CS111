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

struct process_list ready_queue;
TAILQ_INIT(&ready_queue);

u32 current_time = 0, completed_processes = 0;
u32 process_index = 0;
u32 remaining_burst_time[size]; // Track remaining burst times for each process.
for (u32 i = 0; i < size; i++) {
    remaining_burst_time[i] = data[i].burst_time; // Initialize with burst times.
}

bool first_response[size]; // Track if first response time has been recorded for each process.
memset(first_response, 0, sizeof(first_response));

while (completed_processes < size) {
    // Add processes to the ready queue as they arrive
    while (process_index < size && data[process_index].arrival_time <= current_time) {
        TAILQ_INSERT_TAIL(&ready_queue, &data[process_index], pointers);
        process_index++;
    }

    if (!TAILQ_EMPTY(&ready_queue)) {
        struct process *proc = TAILQ_FIRST(&ready_queue);

        // Calculate waiting time if this is the first time the process is getting CPU after its arrival
        if (!first_response[proc->pid]) {
            total_waiting_time += current_time - proc->arrival_time;
            total_response_time += current_time - proc->arrival_time;
            first_response[proc->pid] = true;
        }

        // Execute the process for a quantum or its remaining burst time, whichever is smaller
        u32 execution_time = remaining_burst_time[proc->pid] < quantum_length ? remaining_burst_time[proc->pid] : quantum_length;
        remaining_burst_time[proc->pid] -= execution_time;
        current_time += execution_time;

        if (remaining_burst_time[proc->pid] == 0) {
            // Process completed
            TAILQ_REMOVE(&ready_queue, proc, pointers);
            completed_processes++;
        } else {
            // Not completed, move to the end of the queue
            TAILQ_REMOVE(&ready_queue, proc, pointers);
            TAILQ_INSERT_TAIL(&ready_queue, proc, pointers);
        }
    } else {
        // No process is ready; increment current time
        current_time++;
    }
}

/* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
